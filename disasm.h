#ifndef __disasm_h__
#define __disasm_h__

#include <tuple>

#include "capstone/capstone.h"

#include "kdeploy.h"

bool arm64_decode_movn_movk(cs_insn* insn, size_t count, uint64_t* output);

std::tuple<uintptr_t, uintptr_t> get_module_layout(uint8_t* sys_delete_module);

size_t get_kernel_symbol_size(uint8_t* sym_module_get_kallsym);

void relocate_arm64_kernel(KernelInformation& ki, uint8_t* __relocate_kernel);

#endif
