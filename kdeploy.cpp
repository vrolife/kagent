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
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <stdexcept>
#include <unistd.h>
#include <sys/cdefs.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <array>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>

#include "zlib.h"

#include "kdeploy.h"
#include "disasm.h"
#include "utils.h"

#include "kagent/private.h"

namespace po = boost::program_options;

template <typename S>
typename std::enable_if<S::method == KernelSymbolMethod::Offset, ssize_t>::
    type
    find_symbol(KernelInformation& ki, const std::string_view& symbol_name, S* begin, S* end)
{
    for (auto* iter = begin; iter != end; ++iter) {
        auto* name = reinterpret_cast<char*>(&iter->name_offset) + iter->name_offset;
        if (symbol_name == name) {
            return iter - begin;
        }
    }
    return -1;
}

template <typename S>
typename std::enable_if<S::method == KernelSymbolMethod::Abs, ssize_t>::
    type
    find_symbol(KernelInformation& ki, const std::string_view& symbol_name, S* begin, S* end)
{
    for (auto* iter = begin; iter < end; ++iter) {
        // BOOST_LOG_TRIVIAL(debug) << "symbol addr " << (void*)iter->name;
        
        if (symbol_name == iter->name) {
            // BOOST_LOG_TRIVIAL(debug) << "symbol " << symbol_name << " ptr: " << (void*)iter->value << " relative to buffer: " << (void*)ki.buffer_offset_of_ptr(iter);
            return iter - begin;
        }
    }
    return -1;
}

std::tuple<bool, unsigned long> find_symbol_crc(
    KernelInformation& ki,
    const std::string_view& symbol_name,
    std::vector<SymbolTable>& sym_tables)
{
    for (auto& table : sym_tables) {
        ssize_t index = -1;
        switch (ki.symbol_struct_type) {
        case KernelSymbolStructType::V1: {
            auto* begin = reinterpret_cast<KernelSymbol1*>(table.symbol_start_ptr);
            auto* end = reinterpret_cast<KernelSymbol1*>(table.symbol_stop_ptr);
            index = find_symbol(ki, symbol_name, begin, end);
            break;
        }
        case KernelSymbolStructType::V2: {
            auto* begin = reinterpret_cast<KernelSymbol2*>(table.symbol_start_ptr);
            auto* end = reinterpret_cast<KernelSymbol2*>(table.symbol_stop_ptr);
            index = find_symbol(ki, symbol_name, begin, end);
            break;
        }
        case KernelSymbolStructType::V3: {
            auto* begin = reinterpret_cast<KernelSymbol3*>(table.symbol_start_ptr);
            auto* end = reinterpret_cast<KernelSymbol3*>(table.symbol_stop_ptr);
            index = find_symbol(ki, symbol_name, begin, end);
            break;
        }
        case KernelSymbolStructType::V4: {
            auto* begin = reinterpret_cast<KernelSymbol4*>(table.symbol_start_ptr);
            auto* end = reinterpret_cast<KernelSymbol4*>(table.symbol_stop_ptr);
            index = find_symbol(ki, symbol_name, begin, end);
            break;
        }
        default:
            throw std::runtime_error("unsupported kernel symbol type");
        }
        if (index != -1) {
            auto* crc_ptr = reinterpret_cast<unsigned long*>(table.crc_start_ptr) + index;
            // BOOST_LOG_TRIVIAL(debug) << "found sym " << symbol_name << " index " << index << " crc " << (void*)*crc_ptr << " reloated-kcrctabl " << ki.ARCH_RELOCATES_KCRCTAB;
            if (ki.ARCH_RELOCATES_KCRCTAB) {
                return {
                    true,
                    // *crc_ptr - ki.kaslr

                    // __relocate_kernel did not touch the crc value
                    *crc_ptr - (ki.get_symbol("_text") - ki.load_offset - ki.default_base)
                };
            }
            return { true, *crc_ptr };
        }
    }
    return { false, 0u };
}

[[gnu::weak]] int main(int argc, const char* argv[])
{
    KernelInformation ki {};

    std::string module_file;
    std::string boot_partition;
    std::string kernel;
    std::string output_file;
    std::string symbol_map;

    try {
        po::options_description desc("Allowed options");

        desc.add_options()("help", "show help message");
        desc.add_options()("module,m", po::value<std::string>(), "module file");
        desc.add_options()("boot,b", po::value<std::string>(), "boot partition");
        desc.add_options()("kernel,k", po::value<std::string>(), "kernel image");
        desc.add_options()("symbol-map,s", po::value<std::string>(), "symbol map");
        desc.add_options()("output,o", po::value<std::string>()->default_value("out.ko"), "output file");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        if (vm.count("module")) {
            module_file = vm["module"].as<std::string>();
        }

        if (vm.count("boot")) {
            boot_partition = vm["boot"].as<std::string>();
        }

        if (vm.count("kernel")) {
            kernel = vm["kernel"].as<std::string>();
        }

        if (vm.count("symbol-map")) {
            symbol_map = vm["symbol-map"].as<std::string>();
        }

        output_file = vm["output"].as<std::string>();

    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "error: " << e.what();
        return 1;
    }

    std::vector<char> module_ko = read_file(module_file);

    // read module.ko
    if (module_ko.empty()) {
        BOOST_LOG_TRIVIAL(error) << "Empty file " << module_file;
        return -1;
    }

    BOOST_LOG_TRIVIAL(debug) << "module file size " << module_ko.size();

    // read kernel image
    if (not kernel.empty()) {
        ki.buffer = read_file(kernel);

    } else if (not boot_partition.empty()) {
        auto boot_fd = UniqueFD{::open(boot_partition.c_str(), O_RDONLY)};
        if (not boot_fd) {
            BOOST_LOG_TRIVIAL(error) << "Unable open " << boot_partition;
            return -1;
        }

        ki.buffer.resize(8192);

        // read boot
        auto sz = ::read(boot_fd.get(), ki.buffer.data(), ki.buffer.size());
        if (sz != ki.buffer.size()) {
            BOOST_LOG_TRIVIAL(error) << "boot partition too small";
            return -1;
        }

        // find zImage
        auto* ptr = memmem(ki.buffer.data(), ki.buffer.size(), "\x1F\x8B\x08\x00", 4);
        if (ptr == nullptr) {
            BOOST_LOG_TRIVIAL(error) << "zImage not found";
            return -1;
        }

        auto zImage_offset = reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(ki.buffer.data());
        ::lseek(boot_fd.get(), zImage_offset, SEEK_SET);

        // unzip
        {
            ki.buffer.clear();
            ki.buffer.resize(64 * 1024 * 1024);

            gzFile file = gzdopen(boot_fd.get(), "rb");
            int ret = gzread(file, ki.buffer.data(), ki.buffer.size());
            if (ret == -1) {
                BOOST_LOG_TRIVIAL(error) << "failed to decompress zImage";
                return -1;
            }
            gzclose(file);
            ki.buffer.resize(ki.buffer.size() - ret);
        }
    } else {
        BOOST_LOG_TRIVIAL(error) << "Do not known how to find kernel";
        return -1;
    }

    BOOST_LOG_TRIVIAL(debug) << "kernel buffer " << (void*)ki.buffer.data();
    {
        struct Aarch64KernelHeader {
            void* b;
            uint64_t offset;
            uint64_t size;
        };

        auto* hdr = reinterpret_cast<Aarch64KernelHeader*>(ki.buffer.data());

        ki.load_offset = hdr->offset;
        ki.load_size = hdr->size;
    }

    // get kallsyms

    // echo "1" > /proc/sys/kernel/kptr_restrict
    if (symbol_map.empty()) {
        if (not write_file("/proc/sys/kernel/kptr_restrict", "1")) {
            BOOST_LOG_TRIVIAL(error) << "Failed to write \"1\" to /proc/sys/kernel/kptr_restrict";
            return -1;
        }
        symbol_map = "/proc/kallsyms";
    }

    // read kernel symbol map
    {
        FILE* fp = fopen(symbol_map.c_str(), "rb");
        if (fp == nullptr) {
            BOOST_LOG_TRIVIAL(error) << "Unable open /proc/kallsyms";
            return -1;
        }

        char line[BUFSIZ];
        char name[BUFSIZ];

        while (fgets(line, sizeof(line), fp) != nullptr) {
            uintptr_t ptr { 0 };
            char type { 0 };

            if (sscanf(line, "%" SCNxPTR " %c %s", &ptr, &type, name) == 3) {
                ki.kallsyms.emplace(std::pair<std::string, uintptr_t> { name, ptr });
            }
        }

        fclose(fp);
    }

    BOOST_LOG_TRIVIAL(debug) << "symbol count " << ki.kallsyms.size();

    // find key symbol
    {
        ki.sym_text = ki.get_symbol("_text");

        ki.sym_delete_modulem = ki.find_symbol("sys_delete_module");
        if (ki.sym_delete_modulem == 0) {
            ki.sym_delete_modulem = ki.get_symbol("__do_sys_delete_module.constprop.0");
        }

        ki.sym_module_get_kallsym = ki.get_symbol("module_get_kallsym");
        ki.sym_vermagic = ki.get_symbol("vermagic");
    }

    // uintptr_t delete_module_offset = sym_delete_modulem - sym_text;
    // uintptr_t sym_vermagic_offset = sym_vermagic - sym_text;
    // uintptr_t sym_module_get_kallsym_offset = sym_module_get_kallsym - sym_text;

    BOOST_LOG_TRIVIAL(debug) << "head " << (void*)ki.sym_text;

    // BOOST_LOG_TRIVIAL(debug) << "delete_module_offset 0x" << std::hex << ki.offset(ki.sym_delete_modulem) << std::dec;
    // BOOST_LOG_TRIVIAL(debug) << "sym_vermagic_offset 0x" << std::hex << sym_vermagic_offset << std::dec;
    // BOOST_LOG_TRIVIAL(debug) << "sym_module_get_kallsym_offset 0x" << std::hex << sym_module_get_kallsym_offset << std::dec;

    // disassemble sys_delete_module
    // find offset of mod->init and mod->exit
    auto [module_init_offset, module_exit_offset] = get_module_layout(ki.ptr_of_sym(ki.sym_delete_modulem));

    BOOST_LOG_TRIVIAL(debug) << "module_init_offset 0x" << std::hex << module_init_offset << std::dec;
    BOOST_LOG_TRIVIAL(debug) << "module_exit_offset 0x" << std::hex << module_exit_offset << std::dec;

    // read vermagic and kernel version
    std::string vermagic { ki.ptr_of_sym<char*>(ki.sym_vermagic) };
    BOOST_LOG_TRIVIAL(info) << "vermagic: " << vermagic;

    if (sscanf(vermagic.c_str(), "%d.%d.%d", &ki.major, &ki.minor, &ki.revision) < 2) {
        BOOST_LOG_TRIVIAL(debug) << "Invalid vermagic";
        return -1;
    }
    BOOST_LOG_TRIVIAL(info) << "kernel version: " << ki.major << "." << ki.minor;

    // get kernel symbol structure

    size_t kernel_symbol_size = 0;

    if (ki.version_old_then(4, 19, 0)) {
        kernel_symbol_size = 2 * sizeof(void*);
    } else {
        kernel_symbol_size = get_kernel_symbol_size(ki.ptr_of_sym(ki.sym_module_get_kallsym));
    }

    if (kernel_symbol_size == 0) {
        throw std::runtime_error("kernel_symbol_size not found");
    }

    BOOST_LOG_TRIVIAL(debug) << "kernel_symbol_size " << kernel_symbol_size;

#ifdef __LP64__
    switch (kernel_symbol_size) {
    case 8:
        ki.symbol_struct_type = KernelSymbolStructType::V2;
        break;
    case 16:
        ki.symbol_struct_type = KernelSymbolStructType::V1;
        break;
    case 12:
        ki.symbol_struct_type = KernelSymbolStructType::V3;
        break;
    case 24:
        ki.symbol_struct_type = KernelSymbolStructType::V4;
        break;
    }
#else
#error "Unsupported arch"
#endif

    // relocate mod->{init, exit}
    {
        auto* header = reinterpret_cast<ElfW(Ehdr)*>(module_ko.data());
        auto* shdr = reinterpret_cast<ElfW(Shdr)*>(reinterpret_cast<char*>(header) + header->e_shoff);
        auto* shstrtab = reinterpret_cast<char*>(module_ko.data() + (shdr + header->e_shstrndx)->sh_offset);

        ElfW(Rela) * this_module_rela { nullptr };
        size_t rela_num { 0 };

        SymbolVersion* versions { nullptr };
        size_t vers_num { 0 };

        for (int i = 0; i < header->e_shnum; ++i) {
            if (std::string_view(shstrtab + shdr[i].sh_name) == ".rela.gnu.linkonce.this_module") {
                this_module_rela = reinterpret_cast<ElfW(Rela)*>(reinterpret_cast<char*>(header) + shdr[i].sh_offset);
                rela_num = shdr[i].sh_size / sizeof(ElfW(Rela));
            }
            if (std::string_view(shstrtab + shdr[i].sh_name) == "__versions") {
                versions = reinterpret_cast<SymbolVersion*>(reinterpret_cast<char*>(header) + shdr[i].sh_offset);
                vers_num = shdr[i].sh_size / sizeof(SymbolVersion);
            }
            if (std::string_view(shstrtab + shdr[i].sh_name) == ".kagent.runtime.information") {
                ki.runtime_info = reinterpret_cast<RuntimeInformation*>(reinterpret_cast<char*>(header) + shdr[i].sh_offset);
            }
        }

        if (this_module_rela == nullptr) {
            BOOST_LOG_TRIVIAL(debug) << ".rela.gnu.linkonce.this_module not found";
            return -1;
        }

        for (int i = 0; i < rela_num; ++i) {
            BOOST_LOG_TRIVIAL(debug) << "this_module rela " << i << " " << this_module_rela[i].r_offset;
            if (this_module_rela[i].r_offset == offsetof(KernelModule, init)) {
                this_module_rela[i].r_offset = module_init_offset;
            }
            if (this_module_rela[i].r_offset == offsetof(KernelModule, exit)) {
                this_module_rela[i].r_offset = module_exit_offset;
            }
        }

        // relocate kernel
        bool relocated { false };
        if (ki.symbol_struct_type == KernelSymbolStructType::V1
            or ki.symbol_struct_type == KernelSymbolStructType::V4) // TODO and KASLR
        {
            auto __relocate_kernel = ki.find_symbol("__relocate_kernel");
            if (__relocate_kernel != 0) {
                arm64_relocate_kernel(ki, ki.ptr_of_sym(__relocate_kernel));
                relocated = true;
            }
            if (not relocated) {
                BOOST_LOG_TRIVIAL(debug) << "Warning: Unsupported kernel relocation method";
            }
        }

        BOOST_LOG_TRIVIAL(debug) << "kernel buffer " << (void*)ki.buffer.data();

        // resolve symbol
        std::vector<SymbolTable> sym_tables {};

        {
            auto __start___ksymtab_gpl_future = ki.find_symbol("__start___ksymtab_gpl_future");
            auto __stop___ksymtab_gpl_future = ki.find_symbol("__stop___ksymtab_gpl_future");
            auto __start___kcrctab_gpl_future = ki.find_symbol("__start___kcrctab_gpl_future");
            auto __stop___kcrctab_gpl_future = ki.find_symbol("__stop___kcrctab_gpl_future");

            if (__start___kcrctab_gpl_future) {
                sym_tables.emplace_back(SymbolTable {
                    "ksymtab_gpl_future",
                    __start___ksymtab_gpl_future,
                    __stop___ksymtab_gpl_future,
                    __start___kcrctab_gpl_future,
                    __stop___kcrctab_gpl_future });
            }

            auto __start___ksymtab_gpl = ki.find_symbol("__start___ksymtab_gpl");
            auto __stop___ksymtab_gpl = ki.find_symbol("__stop___ksymtab_gpl");
            auto __start___kcrctab_gpl = ki.find_symbol("__start___kcrctab_gpl");
            auto __stop___kcrctab_gpl = ki.find_symbol("__stop___kcrctab_gpl");

            if (__start___kcrctab_gpl) {
                sym_tables.emplace_back(SymbolTable {
                    "ksymtab_gpl",
                    __start___ksymtab_gpl,
                    __stop___ksymtab_gpl,
                    __start___kcrctab_gpl,
                    __stop___kcrctab_gpl });
            }

            auto __start___ksymtab = ki.find_symbol("__start___ksymtab");
            auto __stop___ksymtab = ki.find_symbol("__stop___ksymtab");
            auto __start___kcrctab = ki.find_symbol("__start___kcrctab");
            auto __stop___kcrctab = ki.find_symbol("__stop___kcrctab");

            if (__start___kcrctab) {
                sym_tables.emplace_back(SymbolTable {
                    "ksymtab",
                    __start___ksymtab,
                    __stop___ksymtab,
                    __start___kcrctab,
                    __stop___kcrctab });
            }
        }

        for (auto& tbl : sym_tables) {
            BOOST_LOG_TRIVIAL(debug) << "symtable table "
                      << (void*)tbl.symbol_start << "-" << (void*)tbl.symbol_stop 
                      << " crc " << (void*)tbl.crc_start << "-" << (void*)tbl.crc_stop
                      << " sym " << ki.offset<void*>(tbl.symbol_start) << "-" << ki.offset<void*>(tbl.symbol_stop) 
                      << " crc " << ki.offset<void*>(tbl.crc_start) << "-" << ki.offset<void*>(tbl.crc_stop)
                      << " " << tbl.name
                     ;
            tbl.symbol_start_ptr = ki.ptr_of_sym(tbl.symbol_start);
            tbl.symbol_stop_ptr = ki.ptr_of_sym(tbl.symbol_stop);
            tbl.crc_start_ptr = ki.ptr_of_sym(tbl.crc_start);
            tbl.crc_stop_ptr = ki.ptr_of_sym(tbl.crc_stop);
        }

        for (auto& tbl : sym_tables) {
            if (ki.symbol_struct_type == KernelSymbolStructType::V1
                and ((tbl.symbol_stop - tbl.symbol_start) % sizeof(KernelSymbol1)) != 0) {
                BOOST_LOG_TRIVIAL(error) << "Wrong kernel symbol type 1";
                return -1;
            }
            if (ki.symbol_struct_type == KernelSymbolStructType::V2
                and ((tbl.symbol_stop - tbl.symbol_start) % sizeof(KernelSymbol2)) != 0) {
                BOOST_LOG_TRIVIAL(error) << "Wrong kernel symbol type 2";
                return -1;
            }
            if (ki.symbol_struct_type == KernelSymbolStructType::V3
                and ((tbl.symbol_stop - tbl.symbol_start) % sizeof(KernelSymbol3)) != 0) {
                BOOST_LOG_TRIVIAL(error) << "Wrong kernel symbol type 3";
                return -1;
            }
            if (ki.symbol_struct_type == KernelSymbolStructType::V4
                and ((tbl.symbol_stop - tbl.symbol_start) % sizeof(KernelSymbol4)) != 0) {
                BOOST_LOG_TRIVIAL(error) << "Wrong kernel symbol type 4";
                return -1;
            }
        }

        BOOST_LOG_TRIVIAL(debug) << "kernel symbol type " << (int)ki.symbol_struct_type;

        BOOST_LOG_TRIVIAL(debug) << "symbol count " << vers_num;

        for (int i = 0; i < vers_num; ++i) {
#if 0
            std::tuple<bool, unsigned long> find_symbol_crc_unicorn(KernelInformation& ki, const std::string& name);
            auto [found, crc] = find_symbol_crc_unicorn(ki, versions[i].name);
#else
            auto [found, crc] = find_symbol_crc(ki, versions[i].name, sym_tables);
#endif
            if (found) {
                versions[i].crc = crc;
                BOOST_LOG_TRIVIAL(info) << "crc " << (void*)(uintptr_t)crc << " " << versions[i].name;
            } else {
                BOOST_LOG_TRIVIAL(error) << "NOT FOUND " << versions[i].name;
            }
        }
    }

    // fill runtime information
    if (ki.runtime_info) {
        if (ki.runtime_info->mm_pgd_required) {
            auto create_pgd_mapping = ki.get_symbol("create_pgd_mapping");
            
#ifdef __aarch64__
            ki.runtime_info->mm_pgd_offset = arm64_get_mm_pgd_offset(ki.ptr_of_sym(create_pgd_mapping));
#else
            BOOST_LOG_TRIVIAL(debug) << "pgd offset for current arch not available";
            return -1;
#endif
        }
    }

    // modify module.ko
    //   vermagic
    //   name
    {
        std::string module_name = get_random_string(sizeof(RANDOM_NAME_PLACEHOLDER));

        auto* ptr = module_ko.data();
        size_t size = module_ko.size();
        while (true) {
            ptr = reinterpret_cast<char*>(memmem(ptr, size, VERMAGIC_PLACEHOLDER, sizeof(VERMAGIC_PLACEHOLDER)));
            if (ptr == nullptr) {
                break;
            }
            memset(ptr, 0, sizeof(VERMAGIC_PLACEHOLDER));
            memcpy(ptr, vermagic.data(), vermagic.size());
            ptr += sizeof(VERMAGIC_PLACEHOLDER);
        }

        ptr = module_ko.data();
        size = module_ko.size();
        while (true) {
            ptr = reinterpret_cast<char*>(memmem(ptr, size, RANDOM_NAME_PLACEHOLDER, sizeof(RANDOM_NAME_PLACEHOLDER)));
            if (ptr == nullptr) {
                break;
            }
            memset(ptr, 0, sizeof(RANDOM_NAME_PLACEHOLDER));
            memcpy(ptr, module_name.data(), module_name.size());
            ptr += sizeof(RANDOM_NAME_PLACEHOLDER);
        }
    }

    // write file
    {
        int out_fd = ::open(output_file.c_str(), O_WRONLY | O_CREAT, 0644);
        if (out_fd == -1) {
            BOOST_LOG_TRIVIAL(debug) << "Unable write file";
            return -1;
        }
        if (::write(out_fd, module_ko.data(), module_ko.size()) != module_ko.size()) {
            BOOST_LOG_TRIVIAL(debug) << "Disk out of space";
            ::close(out_fd);
            return -1;
        }
        ::close(out_fd);
    }

    BOOST_LOG_TRIVIAL(debug) << "!! done !!";
    return 0;
}
