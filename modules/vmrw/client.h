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
#ifndef __vmrw_client_h__
#define __vmrw_client_h__

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VMRW_VERSION 2

struct Request {
    int version;
    int pid;
    void* remote;
    void* local;
    size_t size;
    ssize_t result;
};

ssize_t vmrw_read(int fd, int pid, void* remote, void* local, size_t size);

#ifdef __cplusplus
}
#endif

#endif
