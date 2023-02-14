#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include <iostream>

#include "client.h"

int main(int argc, char *argv[])
{
    const char* vmrw = "/sys/kernel/debug/vmrw";

    if (argc == 2) {
        vmrw = argv[1];
    }

    int fd = ::open(vmrw, O_RDWR);
    if (fd == -1) {
        std::cerr << "Failed to open " << vmrw << std::endl;
        return -1;
    }

    srand(time(NULL));

    int target = rand();

    int buffer;

    size_t remain = vmrw_read(fd, getpid(), &target, &buffer, sizeof(buffer));
    if (remain) {
        std::cerr << "Failed to read vmrw, remain " << remain << std::endl;
        return -1;
    }

    std::cout << "pass " << (int)(buffer == target) << std::endl;

    return 0;
}
