#include <linux/kernel.h>
#include <linux/module.h>

#include <net/sock.h>
#include <net/netlink.h>
#include "forwardfs.h"
#include "forwardfs_netlink.h"

#define MY_MSG_TYPE 18

// DEFINE_MUTEX ( forward_mutex );

struct sock *nl_sk; //defining location of socket
int pid;

/**
 * Netlink reciever
 */
int
forward_rcv_msg ( struct sk_buff *skb, struct nlmsghdr *nlh )
{
        char *data;
        switch ( nlh->nlmsg_type ) {
                case MY_MSG_TYPE:
                        data = NLMSG_DATA ( nlh );
                        printk ( "%s: %s\n", __func__, data );
                        
                        int pid = nlh->nlmsg_pid;
                        printk ( "got client: %i\n", pid );
                        
                        char *msg = " Aye!";
                        forward_snd_msg(msg, strlen(msg), pid);
                        
                        break;
        }
        return 0;
}

/**
 * Netlink recieve callback
 */
void
forward_nl_rcv_msg ( struct sk_buff *skb )
{
        netlink_rcv_skb ( skb, &forward_rcv_msg );
}

/**
 * Message echo convenience method
 */
static int
forward_echo_msg(struct nlmsghdr *nlh)
{ 
        char *msg = NLMSG_DATA ( nlh );
        forward_snd_msg(msg, strlen(msg), nlh->nlmsg_pid);
}

/**
 * Netlink Send Method
 */
int
forward_snd_msg ( void *msg, int msg_size, int pid )
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
        nlh=nlmsg_put ( skb_out,0,0,NLMSG_DONE,msg_size,0 );
        //and data
        memcpy ( nlmsg_data ( nlh ), msg, msg_size );
        
        if(!nl_sk){ //check socket
                printk( KERN_ERR "Lost socket\n");
                return -EXDEV;
        } else 
                printk ( KERN_INFO "sending message\n");
        
        int res=nlmsg_unicast ( nl_sk,skb_out,pid );
        if ( res<0 )
                printk ( KERN_INFO "Error while sending bak to user\n" );
        
        return res;
}

