#ifndef __FORWARDFS_COMMON_H__
#define __FORWARDFS_COMMON_H__


#define FFS_REGISTER            17
#define FFS_OPEN                18
#define FFS_READ                19
#define FFS_WRITE               20


#define OKAY "ok"
#define ERROR "error"

struct payload {
      uint32_t length;
      char data[0];  
};
#define PAYLOAD_ALLOC(P, D) P = malloc(sizeof(struct payload) + D)
#define PAYLOAD_LEN(P) *(uint32_t*)P
#define KPAYLOAD_ALLOC(P, D) P = kmalloc(sizeof(struct payload) + D, GFP_KERNEL)
 
#endif