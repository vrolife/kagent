#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "client.h"

ssize_t fingeradj_read(int fd, int pid, void* remote, void* local, size_t size)
{
    struct Request req;
    req.version = VMRW_VERSION;
    req.pid = pid;
    req.remote = remote;
    req.local = local;
    req.size = size;
    req.result = 0;

    if (read(fd, &req, sizeof(struct Request)) == -1) {
        return -1;
    }
    return req.result;
}
