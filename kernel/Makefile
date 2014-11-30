KBUILD_CFLAGS += -w
LIBS = -lnl

obj-m := ffs.o
ffs-objs := ./forwardfs.o ./forwardfs_netlink.o

all:
	make -C /lib/modules/$(shell uname -r)/build  M=$(PWD) modules

clean: 
	make -C /lib/modules/$(shell uname -r)/build  M=$(PWD) clean
