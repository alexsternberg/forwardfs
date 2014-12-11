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
#include <sys/stat.h>
#include "../common/forwardfs_common.h"

#define MAX_PAYLOAD 2048
#define REGISTER_MSG "REGISTER"
#define FILENAME_FMT "/home/alex.sternberg/nodes/%lu.node"

struct nl_sock * nl_sk;

int forward_open(char * msg){
        //return the size of the file
        char *err = NULL;
        unsigned long fid = strtoul(msg, &err, 16);
        if(err)
                printf("could not parse: %s\n", err);
        printf("opening file_id %lu\n", fid);
        char filename[1024];
        sprintf(filename, FILENAME_FMT, fid);
        int fd = 0;
        printf("opening file: %s\n", filename);
        if ( ( fd = open(filename, O_RDWR | O_CREAT , S_IRWXU | S_IRWXG | S_IRWXO )) < 0) printf("error opening file\n");

        off_t fsize;
        fsize = lseek(fd, 0, SEEK_END);
        
        printf("file size: %li\n", (long int)fsize);
        char *resp = malloc(4096);
        sprintf(resp, "%lx", fsize);
        printf("sending reply: %s\n", resp);
        close(fd);
        int bytes = 0;    
        if ( (bytes = nl_send_simple ( nl_sk, FFS_OPEN, 0, resp, strlen(resp)+1)) < 0 )
                printf("Error sending message: %s\n", nl_geterror(bytes));
        
        return bytes;
}


int forward_read(char * msg){
        int bytes = 0;
        char *rem = NULL;
        unsigned long fid = strtoul(msg, &rem, 16);
        if(rem < 0 || *rem != '\n')
                printf("could not parse %s\n", rem);
        
        unsigned long len = strtoul(++rem, &rem, 16);
        if(rem < 0 || *rem != '\n')
                printf("could not parse %s\n", rem);
        
        long long int pos = strtoll(++rem, &rem, 16);
        if(rem < 0 || *rem != '\0' )
                printf("could not parse %s\n", rem);
        
               
        printf("opening file_id %lu\n", fid);
        char filename[1024];
        sprintf(filename, FILENAME_FMT, fid);
        
        int fd = 0;
        printf("opening file: %s\n", filename);
        if ( ( fd = open(filename, O_RDWR)) < 0) 
                printf("error opening file\n");
        
        printf("reading %lu bytes offset at %llu\n", len, pos);
        int ret = 0;
        char *data = malloc(4096);
        if ( (ret = lseek(fd, pos, SEEK_SET)) < 0)
                printf("error seeking");
        printf("move to %i", ret);
        if ( (ret = read(fd, data, len)) < 0)
                printf("error reading file: %i", bytes);
        
        printf("read %i bytes\n", ret);
        close(fd);
        
//         *(data + ret) = '\0'; 
        
        printf("sending reply: %s\n", data);
        if ( (bytes = nl_send_simple ( nl_sk, FFS_READ, 0, data, strlen(data))) < 0 )
                printf("Error sending message: %s\n", nl_geterror(bytes));
        
        return bytes;
}

int forward_write(char * msg){
        int bytes = 0;
        char *rem = NULL;
        unsigned long fid = strtoul(msg, &rem, 16);
        if(rem < 0 || *rem != '\n')
                printf("could not parse %s\n", rem);
        
        unsigned long len = strtoul(++rem, &rem, 16);
        if(rem < 0 || *rem != '\n')
                printf("could not parse %s\n", rem);
        
        long long int pos = strtoll(++rem, &rem, 16);
        if(rem < 0 || *rem != '\n' )
                printf("could not parse %s\n", rem);
        
        printf("length: %lu, offset: %lli\n", len, pos);
        printf("opening file_id %lu\n", fid);
        char filename[1024];
        sprintf(filename, FILENAME_FMT, fid);
        
        int fd = 0;
        printf("opening file: %s\n", filename);
        if ( ( fd = open(filename, O_RDWR)) < 0)
                printf("error opening file: %i\n", bytes);
        if ( lseek(fd, pos, SEEK_SET) < 0)
                printf("error seeking");
        if ( ( bytes = write(fd, ++rem, len)) < 0)
                printf("error writing file: %i", bytes);
        
        off_t fsize;
        fsize = lseek(fd, 0, SEEK_END);
        
        printf("file size: %li\n", (long int)fsize);
        char *resp = malloc(4096);
        sprintf(resp, "%lx", fsize);
        printf("sending reply: %s\n", resp);
        close(fd);
        
        if ( (bytes = nl_send_simple ( nl_sk, FFS_WRITE, 0, resp, strlen(resp) + 1)) < 0 )
                printf("Error sending message: %s\n", nl_geterror(bytes));
        
        return bytes;
}

int main()
{
        nl_debug = 2; //set netlink debug level
        
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
        if ( (bytes = nl_send_simple ( nl_sk, FFS_REGISTER, 0, REGISTER_MSG, strlen(REGISTER_MSG))) <= 0 )
                printf("Error sending message: %s\n", nl_geterror(bytes));      
        printf("sent bytes: %i\n", bytes);
        
        printf("entering main loop\n");
        unsigned char *buf = NULL;
        struct nlmsghdr *nh;
        bytes = 0;
        while ( ( bytes = nl_recv(nl_sk, &nla, &buf, (struct ucred **)NULL) ) >= 0) {
                //main loop
                nh = (struct nlmsghdr *) buf;
                
                printf("Got type: %i\n", nh->nlmsg_type);
                printf("Got data: %s\n", NLMSG_DATA(nh));              

                switch(nh->nlmsg_type){
                        case FFS_OPEN:
                                forward_open(NLMSG_DATA(nh));
                                break;
                        case FFS_READ:
                                forward_read(NLMSG_DATA(nh));
                                break;
                        case FFS_WRITE:
                                forward_write(NLMSG_DATA(nh));
                                break;
                        case FFS_REGISTER:
                                if(strcmp(NLMSG_DATA( nh ), OKAY)){
                                        printf("recieved error: %s\n", NLMSG_DATA( nh ));
                                }
                                break;
                        case 2:
                                break;
                        default:
                                printf("unknown command: %s\n", NLMSG_DATA( nh ));
                }
                free(buf); buf = NULL;
        }
        printf("escaped main: %s\n", nl_geterror(bytes));
        nl_socket_free ( nl_sk );
        return ( EXIT_SUCCESS );
}

int send_okay(uint32_t op){
        int bytes = 0;
        if ( (bytes = nl_send_simple ( nl_sk, op, 0, OKAY, strlen(OKAY))) < 0 )
                printf("Error sending message: %s\n", nl_geterror(bytes));
        return bytes;
}
       