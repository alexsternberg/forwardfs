forwardfs
=========
use CMake to build the kernel module, then `insmod ffs.ko` to load it into the kernel. The userspace code can be built with CMake, and running it will send the message " Mr. Kernel, Are you ready ?" and the kernel module will respond with " Aye!". THe exchange is logged to stdout for the userspace program and `dmesg` for the kernel module.
