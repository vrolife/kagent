#ifndef __fingeradj_client_h__
#define __fingeradj_client_h__

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FINGERADJ_VERSION 2

struct Request {
    int version;
    int finger;
    int x;
    int y;
};

ssize_t fingeradj_read(int fd, int pid, void* remote, void* local, size_t size);

#ifdef __cplusplus
}
#endif

#endif
