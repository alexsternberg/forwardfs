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
struct payload payload;
int pid;
void *buf;
int buf_size;
struct sockaddr_nl client;
DEFINE_MUTEX (forward_nl_mutex);
DEFINE_MUTEX (forward_nl_recv_mutex);


/**
 * Netlink reciever
 */
int
forward_rcv_msg ( struct sk_buff *skb, struct nlmsghdr *nlh )
{
        switch ( nlh->nlmsg_type ) {
                /*
        case MY_MSG_TYPE:
                data = NLMSG_DATA ( nlh );
                printk ( "%s: %s\n", __func__, data );

                pid = nlh->nlmsg_pid;
                printk ( "got client: %i\n", pid );

                char *msg = " Aye!";
                forward_snd_msg ( msg, strlen ( msg ) );

                break;
                */
        case FFS_REGISTER:
                printk(KERN_INFO "registering client: %i", nlh->nlmsg_pid);
                pid = nlh->nlmsg_pid;
                client.nl_family = AF_NETLINK;
                client.nl_pid = nlh->nlmsg_pid;
                client.nl_pad = 0;
                
                payload.length = strlen(OKAY);
                payload.data = kmalloc(payload.length, 0);
                memcpy(payload.data, OKAY, payload.length);
                forward_send_simple (FFS_REGISTER, );
                break;
        case FFS_OPEN:
                payload.length = *(int*)NLMSG_DATA( nlh );
                payload.data = kmalloc(payload.length, 0);
                memcpy(payload.data, NLMSG_DATA( nlh ), payload.length);
                if (strcmp((char*)(((int*)NLMSG_DATA( nlh )) + 1), OKAY)){
                        printk("got unexpected data: %s\n", NLMSG_DATA( nlh ));
                        return -EBADF;
                }
                
                break;
        default:
                printk ( KERN_ERR "Invalid type: %i", nlh->nlmsg_type );
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
        printk(KERN_INFO "got message");
        netlink_rcv_skb ( skb, &forward_rcv_msg );
        mutex_unlock(&forward_nl_recv_mutex);
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
        skb_out = nlmsg_new ( msg_size, 0 );
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
forward_send_simple (uint32_t op, struct payload *p)
{
        //TODO: fill magic numbers
        //get socket buffer
        struct sk_buff *skb_out;
        skb_out = nlmsg_new ( p->length + 1, 0 );
        if ( !skb_out ) {
                printk ( KERN_ERR "Failed to allocate new skb\n" );
                return -EXDEV;
        }
        NETLINK_CB ( skb_out ).dst_group = 0; /* not in mcast group */

        //load buffer with netlink message
        struct nlmsghdr *nlh;
        nlh=nlmsg_put ( skb_out,0,0,op,sizeof(p),0 );
        //and data
        memcpy ( nlmsg_data ( nlh ), p, sizeof(p) );

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
