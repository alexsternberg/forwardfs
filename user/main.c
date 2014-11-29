#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define NETLINK_NITRO 18
#define MAX_PAYLOAD 2048
int main()
{
    struct sockaddr_nl s_nladdr, d_nladdr;
    struct msghdr msg ;
    struct nlmsghdr *nlh=NULL ;
    struct iovec iov;
    int fd=socket(AF_NETLINK ,SOCK_RAW , NETLINK_USERSOCK );

    if (fd < 0) {
      perror("socket()");
      return -10;
    }
    printf("binding socket\n");
    /* source address */
    memset(&s_nladdr, 0 ,sizeof(s_nladdr));
    s_nladdr.nl_family= AF_NETLINK ;
    s_nladdr.nl_pad=0;
    s_nladdr.nl_pid = getpid();
    if(bind(fd, (struct sockaddr*)&s_nladdr, sizeof(s_nladdr)) < 0 ){
      close(fd);
      printf("error binding");
      return -10;
    }

    printf("configuring destination\n");
    /* destination address */
    memset(&d_nladdr, 0 ,sizeof(d_nladdr));
    d_nladdr.nl_family= AF_NETLINK ;
    d_nladdr.nl_pad=0;
    d_nladdr.nl_pid = 0; /* destined to kernel */

    printf("setting headers\n");
    /* Fill the netlink message header */
    nlh = (struct nlmsghdr *)malloc(100);
    memset(nlh , 0 , 100);
    strcpy(NLMSG_DATA(nlh), " Mr. Kernel, Are you ready ?" );
    nlh->nlmsg_len = 100;
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 1;
    nlh->nlmsg_type = 18;

    /*iov structure */

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;

    printf("sending message %s\n", (char *)iov.iov_base);
    /* msg */
    memset(&msg,0,sizeof(msg));
    msg.msg_name = (void *) &d_nladdr ;
    msg.msg_namelen=sizeof(d_nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    
    printf("Client %i sent %i bytes\n", getpid(), sendmsg(fd, &msg, 0));
    
    int len;
    struct msghdr message;
    char buf[4096];
    struct sockaddr_nl sa;
    struct iovec recv_iov = { buf, sizeof(buf) };
    struct nlmsghdr *nh;
    
    message.msg_name = &sa;
    message.msg_namelen = sizeof(sa);
    message.msg_iov = &recv_iov;
    message.msg_iovlen = 1;
    message.msg_control = NULL;
    message.msg_controllen = 0;
    message.msg_flags = 0;
    
    len = recvmsg(fd, &message, 0);
    nh = (struct nlmsghdr *) buf;
    
    printf("trying to recieve\n");
    printf("got message? %s\n",len>0?"true":"false");
    printf("got message: %s\n", NLMSG_DATA(nh));
    close(fd);
    return (EXIT_SUCCESS);
}


