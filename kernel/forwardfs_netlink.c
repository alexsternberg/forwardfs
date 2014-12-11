#include <linux/kernel.h>
#include <linux/module.h>
#include <uapi/linux/netlink.h>
#include <net/genetlink.h>
#include <net/sock.h>
#include <net/netlink.h>
#include "forwardfs.h"
#include "forwardfs_netlink.h"
#include "../common/forwardfs_common.h"

struct sock *nl_sk; //defining location of socket
struct payload *msg_payload;
char *global_payload;
char *open_payload;
char *read_payload;
char *write_payload;
int pid;
void *buf;
int buf_size;
struct sockaddr_nl client;
DEFINE_MUTEX (forward_nl_mutex);
DEFINE_MUTEX (forward_nl_recv_mutex);

// #define FREE(P) kfree(P.data); P.data = NULL; P.length = 0;


/**
 * Netlink reciever
 */
int
forward_rcv_msg ( struct sk_buff *skb, struct nlmsghdr *nlh )
{
        printk("entering %s\n", __func__);
        printk("got type: %i\n",  nlh->nlmsg_type);
        switch ( nlh->nlmsg_type ) {
        case FFS_REGISTER:
                printk(KERN_INFO "registering client: %i\n", nlh->nlmsg_pid);
                pid = nlh->nlmsg_pid;
                client.nl_family = AF_NETLINK;
                client.nl_pid = nlh->nlmsg_pid;
                client.nl_pad = 0;
                forward_send_simple (FFS_REGISTER, OKAY);
                global_payload = kmalloc(4096, GFP_ATOMIC);
                strcpy( global_payload, NLMSG_DATA( nlh ));
                kfree(global_payload);
                break;
        case FFS_OPEN: 
                printk("open got %u bytes payload: %s\n", strlen(NLMSG_DATA( nlh )), (char*)NLMSG_DATA( nlh ));
                open_payload = kmalloc(4096, GFP_ATOMIC);
                strcpy( open_payload, NLMSG_DATA( nlh ));
                break;
        case FFS_READ:
                printk("read got %u bytes payload: %s\n", strlen(NLMSG_DATA( nlh )), (char*)NLMSG_DATA( nlh ));
                read_payload = kmalloc(4096, GFP_ATOMIC);
        if(!read_payload) printk("could not alloc");
                strcpy( read_payload, NLMSG_DATA( nlh ));
                printk("stored %i bytes", strlen(read_payload));
                break;
        case FFS_WRITE:
                printk("write got %u bytes payload: %s\n", strlen(NLMSG_DATA( nlh )), (char*)NLMSG_DATA( nlh ));
                write_payload = kmalloc(4096, GFP_ATOMIC);
                strcpy( write_payload, NLMSG_DATA( nlh ));
                break;
        default:
                printk ( KERN_ERR "Invalid type: %i\n", nlh->nlmsg_type );
        }
        mutex_unlock(&forward_nl_recv_mutex); //buffer full
        
        return 0;
}

/**
 * Netlink recieve callback
 */
void
forward_nl_rcv_msg ( struct sk_buff *skb )
{
        printk(KERN_INFO "got message\n");
        netlink_rcv_skb ( skb, forward_rcv_msg );
}

/**
 * Message echo convenience method
 */
static int
forward_echo_msg ( struct nlmsghdr *nlh )
{
        char *msg = NLMSG_DATA ( nlh );
        forward_snd_msg ( msg, strlen ( msg ) );
}

/**
 * Netlink Send Method
 */
int
forward_snd_msg ( void *msg, int msg_size )
{
        //TODO: fill magic numbers
        //get socket buffer
        struct sk_buff *skb_out;
        skb_out = nlmsg_new ( msg_size, GFP_ATOMIC );
        if ( !skb_out ) {
                printk ( KERN_ERR "Failed to allocate new skb\n" );
                return -EXDEV;
        }
        NETLINK_CB ( skb_out ).dst_group = 0; /* not in mcast group */

        //load buffer with netlink message
        struct nlmsghdr *nlh;
        nlh=nlmsg_put ( skb_out,0,0,2,msg_size,0 );
        //and data
        memcpy ( nlmsg_data ( nlh ), msg, msg_size );

        if ( !nl_sk ) { //check socket
                printk ( KERN_ERR "Lost socket\n" );
                return -EXDEV;
        } else
                printk ( KERN_INFO "sending message\n" );
        //send
        int res=nlmsg_unicast ( nl_sk,skb_out,pid );
        if ( res<0 )
                printk ( KERN_INFO "Error while sending bak to user\n" );

        return res;
}

/**
 * Netlink Send Method
 */
int
forward_send_simple (uint32_t op, char* payload)
{
        //TODO: fill magic numbers
        //get socket buffer
        struct sk_buff *skb_out;
        skb_out = nlmsg_new ( 4096, GFP_KERNEL );
        if ( !skb_out ) {
                printk ( KERN_ERR "Failed to allocate new skb\n" );
                return -EXDEV;
        }
        NETLINK_CB ( skb_out ).dst_group = 0; /* not in mcast group */

        //load buffer with netlink message
        struct nlmsghdr *nlh;
        nlh=nlmsg_put ( skb_out,0,0,op,strlen(payload)+1,0 );
        //and data
        memcpy ( nlmsg_data ( nlh ), payload, strlen(payload)+1 );

        if ( !nl_sk ) { //check socket
                printk ( KERN_ERR "Lost socket\n" );
                return -EXDEV;
        } else
                printk ( KERN_INFO "sending message\n" );       
        //send
        int res=nlmsg_unicast ( nl_sk,skb_out,pid );
        if ( res<0 )
                printk ( KERN_INFO "Error while sending bak to user\n" );

        return res;
}

struct sk_buff *forward_send_wait(void *msg, int msg_size){
        mutex_lock(&forward_nl_mutex); //get lock for socket
        forward_snd_msg(msg, strlen(msg));
        mutex_lock(&forward_nl_mutex); //block on recieve until arrival
        return 0;
}
