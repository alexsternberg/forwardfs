#ifndef __FORWARDFS_NETLINK_H__
#define __FORWARDFS_NETLINK_H__

#include <net/sock.h>
#include <linux/kernel.h>
#include <linux/module.h>
// #include <netlink/netlink.h>
#include <uapi/linux/netlink.h>

#include <net/sock.h>
#include <net/netlink.h>
#include "../common/forwardfs_common.h"

extern int forward_snd_msg( void*, int);
extern int forward_send_simple(uint32_t, char*);
extern struct sockaddr_nl client;

extern struct mutex forward_nl_mutex;
extern struct mutex forward_nl_recv_mutex;

extern struct payload *msg_payload;
extern char *global_payload;
extern char *open_payload;
extern char *read_payload;
extern char *write_payload;
#define PAYLOAD_FREE(P) kfree(P); P = NULL
#define MAX_MSG_SIZE            4096

#endif