#ifndef __FORWARDFS_COMMON_H__
#define __FORWARDFS_COMMON_H__


#define FFS_REGISTER            0
#define FFS_OPEN                1
#define FFS_READ                2
#define FFS_WRITE               3


#define OKAY "ok"
#define ERROR "error"
 
 struct payload {
         uint8_t length;
         void *data;
 };
 
#endif