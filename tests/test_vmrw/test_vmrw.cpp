#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include <iostream>
#include <filesystem>

#include <boost/program_options.hpp>

#include "client.h"

namespace po = boost::program_options;
// namespace fs = std::filesystem;

int main(int argc, char *argv[])
{
    srand(time(NULL));

    int target = rand();

    po::options_description desc{"Options"};
    desc.add_options()("help", "show help message");
    desc.add_options()("vmrw,f", po::value<std::string>()->default_value("/sys/kernel/debug/vmrw"), "vmrw interface file");
    desc.add_options()("pid,p", po::value<pid_t>()->default_value(getpid()), "pid, getpid() as default");
    desc.add_options()("address,a", po::value<std::string>(), "address, internal random int as default");
    desc.add_options()("size,s", po::value<size_t>(), "size, sizeof(int) as default");
    desc.add_options()("width", po::value<size_t>()->default_value(16), "hexdump width");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    std::string vmrw_iface = vm["vmrw"].as<std::string>();

    int fd = ::open(vmrw_iface.c_str(), O_RDWR);
    if (fd == -1) {
        std::cerr << "Failed to open " << vmrw_iface << std::endl;
        return -1;
    }

    std::string buffer{};
    void* address{&target};
    size_t size{sizeof(target)};

    if (vm.count("address")) {
        auto addr = vm["address"].as<std::string>();
        if (addr.size() > 2 and addr.at(0) == '0' and (addr.at(1) == 'x' or addr.at(1) == 'X')) {
            address = reinterpret_cast<void*>(std::stoul(addr.c_str() + 2, nullptr, 16));
        } else {
            address = reinterpret_cast<void*>(std::stoul(addr, nullptr, 10));
        }
    }

    if (vm.count("size")) {
        size = vm["size"].as<size_t>();
    }

    buffer.resize(size);

    std::cout << "address: " << address << std::endl;
    std::cout << "size   : " << size << std::endl;

    size_t result = vmrw_read(fd, vm["pid"].as<pid_t>(), address, buffer.data(), buffer.size());
    if (result < 0) {
        std::cerr << "Failed to read remote memory. " << strerror(errno) << std::endl;
        return -1;
    }

    if (result != buffer.size()) {
        std::cerr << "Expect " << buffer.size() << " but got " << result << std::endl;
    }
    buffer.resize(result);

    if (vm["pid"].as<pid_t>() == getpid()) {
        std::cout << "pass " << (int)(memcmp(buffer.data(), &target, sizeof(target)) == 0) << std::endl;
    } else {
        size_t width = vm["width"].as<size_t>();
        size_t offset{0};

        while (offset < buffer.size()) {
            auto n = std::min(width, buffer.size() - offset);
            auto slice = std::string_view{buffer.data() + offset, n};

            std::cout << reinterpret_cast<void*>(reinterpret_cast<char*>(address) + offset) << ": ";

            int idx = 0;
            for (auto ch : slice) {
                if (idx and (idx % 4) == 0) {
                    std::cout << " ";
                }
                std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)ch << " ";
                idx ++;
            }
            std::cout << "| ";
            for (auto ch : slice) {
                if (ch >= 32 and ch <= 126) {
                    std::cout << ch;
                } else {
                    std::cout << ".";
                }
            }
            std::cout << std::endl;
            offset += n;
        }
    }
    return 0;
}
