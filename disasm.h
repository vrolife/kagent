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
#ifndef __disasm_h__
#define __disasm_h__

#include <tuple>

#include "capstone/capstone.h"

#include "kdeploy.h"

std::tuple<uintptr_t, uintptr_t> get_module_layout(uint8_t* sys_delete_module);

size_t get_kernel_symbol_size(uint8_t* sym_module_get_kallsym);

bool arm64_decode_movn_movk(cs_insn* insn, size_t count, uint64_t* output);
void arm64_relocate_kernel(KernelInformation& ki, uint8_t* __relocate_kernel);
uintptr_t arm64_get_mm_pgd_offset(uint8_t* create_pgd_mapping);

#endif
