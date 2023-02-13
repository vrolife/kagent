#include <sys/stat.h>

#include <array>
#include <random>
#include <filesystem>

#include <boost/log/trivial.hpp>

#include "utils.h"

std::string get_random_string(size_t n)
{
    std::string output{};
    output.resize(n);
    {
        // list(map(lambda n: chr(n + ord('A')), range(26)))
        std::array<char, 52> charset {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 
            'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 
            'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        };

        std::default_random_engine rng{std::random_device{}()};
        std::uniform_int_distribution<> dist(0, charset.size() - 1);

        for (auto& c : output) {
            c = dist(rng);
        }
    }
    return output;
}

std::string read_file(int fd, ssize_t size)
{
    std::string buffer{};

    if (size == -1) {
        struct stat status{};
        if (::fstat(fd, &status) == -1) {
            return {};
        }
        
        size = status.st_size;
    }

    buffer.resize(size);
    if (::read(fd, buffer.data(), buffer.size()) != size) {
        return {};
    }
    return buffer; 
}

std::string read_file(const std::string& filename, ssize_t size)
{
    UniqueFD fd{::open(filename.c_str(), O_RDONLY)};
    if (not fd) {
        BOOST_LOG_TRIVIAL(error) << "Failed to opend file " << filename << " " << strerror(errno);
        return {};
    }

    auto buffer = read_file(fd.get(), size);
    if (buffer.empty()) {
        BOOST_LOG_TRIVIAL(error) << "Failed to read file " << filename << " " << strerror(errno);
    }
    return buffer;
}

bool write_file(const std::string& filename, const std::string& content)
{
    auto fd = UniqueFD{::open(filename.c_str(), O_WRONLY)};
    if (not fd) {
        BOOST_LOG_TRIVIAL(error) << "Unable open " << filename << " " << strerror(errno);
        return false;
    }

    if (::write(fd.get(), content.data(), content.size()) != content.size()) {
        BOOST_LOG_TRIVIAL(error) << "Failed to write content to " << filename << " " << strerror(errno);
        return false;
    }
    return true;
}
