forwardfs
=========
use CMake to build the kernel module, then `insmod forwardfs.ko` to load it into the kernel. The userspace code can be built with `gcc main.c` and running it will send the message " Mr. Kernel, Are you ready ?" and the kernel module will respond with " Aye!"
