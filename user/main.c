#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <libnl3/netlink/socket.h>
#include <libnl3/netlink/netlink.h>
#include <libnl3/netlink/msg.h>
#include <libnl3/netlink/types.h>
#include <libnl3/netlink/handlers.h>
#include <libnl3/netlink/data.h>
#include "../common/forwardfs_common.h"

#define MAX_PAYLOAD 2048
#define REGISTER_MSG "REGISTER"
#define PATH                  
#define PAYLOAD(NH) (struct payload*)NLMSG(NH)

struct nl_sock * nl_sk;

int main()
{
//         nl_debug = 2; //set netlink debug level
        
        struct sockaddr_nl nla = {
                .nl_family = AF_NETLINK,
                .nl_pad = 0,
                .nl_pid = 0,
                .nl_groups = 0
        };
        
        
        printf("allocating handle\n");
        nl_sk = nl_socket_alloc ();
        nl_socket_disable_seq_check ( nl_sk );
        nl_connect(nl_sk, NETLINK_USERSOCK);
        int bytes = 0;
        if ( (bytes = nl_send_simple ( nl_sk, FFS_REGISTER, 0, REGISTER_MSG, strlen(REGISTER_MSG))) < 0 )
                printf("Error sending message: %s\n", nl_geterror(bytes));       
        
        printf("entering main loop\n");
        unsigned char *buf = NULL;
        struct nlmsghdr *nh;
        bytes = 0;
        while ( ( bytes = nl_recv(nl_sk, nla, &buf, (struct ucred **)NULL) ) >= 0) {
                //main loop
                nh = (struct nlmsghdr *) buf;
                struct payload *p = PAYLOAD(nh);
                printf("Got data: %s\n", NLMSG_DATA(nh));
                switch(nh->nlmsg_type){
                        case FFS_OPEN:
                                int fid = *(int*)p->data;
                                printf("opening %i", (int*)p->data);
                                //return the size of the file
                                int size = 0;
                                
                                if ( (bytes = nl_send_simple ( nl_sk, FFS_REGISTER, 0, size, sizeof(size))) < 0 )
                                        printf("Error sending message: %s\n", nl_geterror(bytes));
                                break;
                        case FFS_READ:
                                break;
                }
                free(buf); buf = NULL;
        }
        printf("escaped main: %s", nl_geterror(bytes));
        nl_socket_free ( nl_sk );
        return ( EXIT_SUCCESS );
}

int send_okay(enum forwardfs_operations op){
        int bytes = 0;
        if ( (bytes = nl_send_simple ( nl_sk, op, 0, OKAY, strlen(OKAY))) < 0 )
                printf("Error sending message: %s\n", nl_geterror(bytes));
        return bytes;
}
        