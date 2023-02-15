#ifndef __vmrw_client_h__
#define __vmrw_client_h__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VMRW_VERSION 1

struct Request {
    int version;
    int pid;
    void* remote;
    void* local;
    size_t size;
    size_t remain;
};

size_t vmrw_read(int fd, int pid, void* remote, void* local, size_t size);

#ifdef __cplusplus
}
#endif

#endif
