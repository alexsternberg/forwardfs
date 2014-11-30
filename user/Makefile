KERNEL=3.11.10-301.fc20.i686
CC=gcc
CFLAGS=-Wall -I/usr/src/kernel/$(KERNEL)/ -I/usr/src/kernel/$(KERNEL)/include -I/usr/src/kernel/$(KERNEL)/arch/x86 -I/usr/src/kernel/$(KERNEL)/arch/x86/include -I/usr/include/libnl3 -lnl-3	

debug:clean
	$(CC) $(CFLAGS) -g -o forwardfs main.c
stable:clean
	$(CC) $(CFLAGS) -o forwardfs main.c
clean:
	rm -vfr *~ forwardfs
