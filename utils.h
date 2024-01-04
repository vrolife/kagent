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
#ifndef __utils_h__
#define __utils_h__

#include <fcntl.h>
#include <unistd.h>

#include <string>

template <typename F>
auto ScopeTail(F&& func)
{
    class _ScopeTail {
        F _func;

    public:
        _ScopeTail(F&& func)
            : _func(std::move(func))
        {
        }
        ~_ScopeTail()
        {
            _func();
        }
    };
    return _ScopeTail(std::move(func));
};

struct UniqueFD {
    int fd;
public:
    UniqueFD(int fd=-1)
    : fd(fd)
    { }

    UniqueFD(const UniqueFD&) = delete;
    UniqueFD& operator =(const UniqueFD&) = delete;

    UniqueFD(UniqueFD&& other) noexcept {
        fd = other.fd;
        other.fd = -1;
    }

    UniqueFD& operator=(UniqueFD&& other) {
        close();
        fd = other.fd;
        other.fd = -1;
        return *this;
    }

    ~UniqueFD() {
        close();
    }

    void close() {
        if (fd != -1) {
            ::close(fd);
            fd = -1;
        }
    }

    explicit operator int() const {
        return fd;
    }

    explicit operator bool() const {
        return fd != -1;
    }

    int release() {
        int tmp = fd;
        fd = -1;
        return tmp;
    }

    int get() const {
        return fd;
    }
};

std::string get_random_string(size_t n);

std::vector<char> read_file(int fd, ssize_t size=-1);

std::vector<char> read_file(const std::string& filename, ssize_t size=-1);

bool write_file(const std::string& filename, const std::string& content);

#endif
