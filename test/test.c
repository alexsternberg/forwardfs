#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define PATH "/mnt/%s"

int main(){
        char filename[1024];
        sprintf(filename, PATH, "1234");
        int fd = open(filename , O_RDWR | O_CREAT , S_IRWXU | S_IRWXG | S_IRWXO);
        sleep(1);
        close(fd);

        fd = open(filename , O_RDWR);
        char data[4096] = "test";
        sleep(1);
        printf("writing %i bytes: %s\n", strlen(data), data);
        int bytes = write(fd, data, strlen(data)+1);
        sleep(1);
        printf("put %i bytes\n", bytes);

        close(fd);

        sleep(1);

        fd = open(filename , O_RDWR);

        char read_buf[4096];
        sleep(1);
        if( (bytes = read(fd, &read_buf, 100)) < 0)
                printf("got error: %i", errno);
        sleep(1);
        printf("got %i bytes\n", bytes);
        printf("got data: %s", read_buf);
        close(fd);

        return 0;
}
