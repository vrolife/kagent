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
#ifndef __kdeploy_h__
#define __kdeploy_h__

#include <utility>
#include <vector>
#include <string>
#include <unordered_map>

#include "kagent/common.h"

using namespace std::string_literals;

enum class KernelSymbolStructType {
    None,
    V1,
    V2,
    V3,
    V4,
};

struct KernelInformation {
    // boot partition/kernel-image buffer
    std::vector<char> buffer {};

    uintptr_t load_offset { 0 };
    uintptr_t load_size { 0 };

    uintptr_t default_base { 0 };

    std::unordered_map<std::string, uintptr_t> kallsyms;

    uintptr_t sym_text { 0 };
    uintptr_t sym_delete_modulem { 0 };
    uintptr_t sym_module_get_kallsym { 0 };
    uintptr_t sym_vermagic { 0 };

    int major { 0 };
    int minor { 0 };
    int revision { 0 };

    KernelSymbolStructType symbol_struct_type;

    // TODO detect it
    bool CONFIG_MODULE_REL_CRCS { false };

    bool ARCH_RELOCATES_KCRCTAB { false };
    uintptr_t kaslr { 0 };

    RuntimeInformation* runtime_info{nullptr};

    template <typename T = uint8_t*>
    T ptr_of_sym(uintptr_t addr)
    {
        return reinterpret_cast<T>(buffer.data() + (addr - sym_text));
    }

    size_t buffer_offset_of_ptr(void* ptr) {
        return static_cast<size_t>((char*)ptr - buffer.data());
    }

    template <typename T = uint8_t*>
    T ptr(uintptr_t offset)
    {
        return reinterpret_cast<T>(buffer.data() + offset);
    }

    template <typename T = uintptr_t>
    T offset(uintptr_t addr)
    {
        return reinterpret_cast<T>(addr - sym_text);
    }

    bool version_new_then(int major, int minor, int revision)
    {
        uintptr_t self = (this->major << 24) | (this->minor << 16) | this->revision;
        uintptr_t target = (major << 24) | (minor << 16) | revision;
        return self > target;
    }

    bool version_old_then(int major, int minor, int revision)
    {
        uintptr_t self = (this->major << 24) | (this->minor << 16) | this->revision;
        uintptr_t target = (major << 24) | (minor << 16) | revision;
        return self < target;
    }

    bool version_equals(int major, int minor, int revision)
    {
        uintptr_t self = (this->major << 24) | (this->minor << 16) | this->revision;
        uintptr_t target = (major << 24) | (minor << 16) | revision;
        return self == target;
    }

    uintptr_t get_symbol(const std::string& name) {
        auto iter = kallsyms.find(name);
        if (iter == kallsyms.end()) {
            throw std::invalid_argument("symbol not found: "s + name);
        }
        return iter->second;
    }

    uintptr_t find_symbol(const std::string& name, uintptr_t default_value=0) {
        auto iter = kallsyms.find(name);
        if (iter == kallsyms.end()) {
            return default_value;
        }
        return iter->second;
    }
};

struct SymbolTable {
    const char* name;
    uintptr_t symbol_start;
    uintptr_t symbol_stop;
    uintptr_t crc_start;
    uintptr_t crc_stop;
    void* symbol_start_ptr;
    void* symbol_stop_ptr;
    void* crc_start_ptr;
    void* crc_stop_ptr;
};

enum class KernelSymbolMethod {
    Offset,
    Abs
};

struct KernelSymbol1 {
    constexpr static KernelSymbolMethod method = KernelSymbolMethod::Abs;

    unsigned long value;
    const char* name;
};

// 4.19
struct KernelSymbol2 {
    constexpr static KernelSymbolMethod method = KernelSymbolMethod::Offset;

    int value_offset;
    int name_offset;
};

// 5.4
struct KernelSymbol3 {
    constexpr static KernelSymbolMethod method = KernelSymbolMethod::Offset;

    int value_offset;
    int name_offset;
    int namespace_offset;
};

struct KernelSymbol4 {
    constexpr static KernelSymbolMethod method = KernelSymbolMethod::Abs;

    unsigned long value;
    const char* name;
    const char* namespaze;
};

#endif
