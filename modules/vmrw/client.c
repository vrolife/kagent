#include <unistd.h>
#include <fcntl.h>

#include "client.h"

size_t vmrw_read(int fd, int pid, void* remote, void* local, size_t size)
{
    struct Request req;
    req.pid = pid;
    req.remote = remote;
    req.local = local;
    req.size = size;
    req.remain = 0;

    if (read(fd, &req, sizeof(struct Request)) != sizeof(struct Request)) {
        return 0;
    }

    return req.remain;
}
