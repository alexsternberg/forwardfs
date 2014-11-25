#include <linux/kernel.h>
#include <linux/module.h>

#include <net/sock.h>
#include <net/netlink.h>

#define MY_MSG_TYPE 18

static struct sock *nl_sk;

DEFINE_MUTEX(forwardfs_mutex);

static int
forwardfs_rcv_msg(struct sk_buff *skb, struct nlmsghdr *nlh)
{
    char *data;
    switch(nlh->nlmsg_type){
	case MY_MSG_TYPE:
	    data = NLMSG_DATA(nlh);
	    printk("%s: %s\n", __func__, data);

	    int pid = nlh->nlmsg_pid;
	    printk("got client: %i\n", pid);
	    
	    char *msg = " Aye!";
	    int msg_size = strlen(msg);
	    struct sk_buff *skb_out;
	    
	    skb_out = nlmsg_new(msg_size, 0);
	    if(!skb_out)
	    {
		printk(KERN_ERR "Failed to allocate new skb\n");
		return -10;
	    } 
	    nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0); 
	    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
	    strncpy(nlmsg_data(nlh),msg,msg_size);
	    printk("sending message\n");
	    int res=nlmsg_unicast(nl_sk,skb_out,pid);
	    if(res<0)
		printk(KERN_INFO "Error while sending bak to user\n");	    
	    break;
    }
    return 0;
}

static void
forwardfs_nl_rcv_msg(struct sk_buff *skb)
{
    mutex_lock(&forwardfs_mutex);
    printk("got called!\n");
    netlink_rcv_skb(skb, &forwardfs_rcv_msg);
    mutex_unlock(&forwardfs_mutex);
}

static int
forwardfs_init(void)
{
    struct netlink_kernel_cfg cfg = {
	.input  = forwardfs_nl_rcv_msg,
    };
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USERSOCK, &cfg);
    if (!nl_sk) {
	printk(KERN_ERR "%s: receive handler registration failed\n", __func__);
	return -ENOMEM;
    }
    return 0;
}

static void
forwardfs_exit(void)
{
    if (nl_sk) {
        netlink_kernel_release(nl_sk);
    }
}

module_init(forwardfs_init);
module_exit(forwardfs_exit);
