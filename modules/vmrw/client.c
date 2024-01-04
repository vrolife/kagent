/*
    Copyright (C) 2024 pom@vro.life

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "client.h"

ssize_t vmrw_read(int fd, int pid, void* remote, void* local, size_t size)
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
