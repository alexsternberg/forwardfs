#ifndef __FORWARDFS_H__
#define __FORWARDFS_H__

#include <net/sock.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <net/sock.h>
#include <net/netlink.h>

#define MY_MSG_TYPE 18

// extern int forward_snd_msg( void *msg, int msg_size, int pid ); //netlink message callback
extern void forward_nl_rcv_msg ( struct sk_buff *skb );


extern struct sock *nl_sk; //netlink socket
/**
 * Netlink config
 */
static struct netlink_kernel_cfg cfg = { //netlink config
        .input  = forward_nl_rcv_msg,
};

#endif