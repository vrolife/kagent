#include <elf.h>
#include <link.h>

#include <stdexcept>
#include <vector>
#include <string>
#include <array>

#include <boost/log/trivial.hpp>

#include "kdeploy.h"
#include "disasm.h"
#include "utils.h"

#if defined(__aarch64__)
#define CAPSTONE_INIT_OPTS CS_ARCH_ARM64, CS_MODE_ARM
#elif defined(__x86_64__)
#define CAPSTONE_INIT_OPTS CS_ARCH_X86, CS_MODE_64
#else
#error "Unwupported arch"
#endif

bool arm64_decode_movn_movk(cs_insn* insn, size_t count, uint64_t* output)
{
    if (count == 0) {
        return false;
    }

    if (insn[0].id != ARM64_INS_MOVN) {
        return false;
    }

    uint64_t val = 0;

    val = (~static_cast<uintptr_t>(insn[0].detail->arm64.operands[1].imm));
    val <<= insn[0].detail->arm64.operands[1].shift.value;

    for (size_t i = 1; i < count and insn[i].id == ARM64_INS_MOVK; ++i) {
        auto imm = insn[i].detail->arm64.operands[1].imm;
        imm <<= insn[i].detail->arm64.operands[1].shift.value;
        val |= imm;
    }

    *output = val;
    return true;
}

std::tuple<uintptr_t, uintptr_t> get_module_layout(uint8_t* sys_delete_module)
{
    csh handle {};
    cs_insn* insn { nullptr };

    if (cs_open(CAPSTONE_INIT_OPTS, &handle) != CS_ERR_OK) {
        throw std::runtime_error { "failed to initialize capstone engine" };
    }

    auto release_capstone_handle = ScopeTail([&]() {
        cs_close(&handle);
    });

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    constexpr size_t max_insn = 256;

    auto count = cs_disasm(handle,
        sys_delete_module, 0x1000,
        0x1000, max_insn, &insn);
    if (count == 0) {
        throw std::runtime_error { "failed to disassemble sys_delete_module" };
    }
    /*
        arm64
        0xffffff93023620c0:     ldr             x8, [x23, #0x150]
        0xffffff93023620c4:     cbz             x8, #0xffffff93023620e0
        0xffffff93023620c8:     ldr             x8, [x23, #0x2f8]
        0xffffff93023620cc:     cbnz            x8, #0xffffff93023620e0

        x86_64
        0xffffffff81197523:  cmp             qword ptr [rbx + 0x138], 0
        0xffffffff8119752b:  je              0xffffffff8119753b
        0xffffffff8119752d:  cmp             qword ptr [rbx + 0x338], 0
        0xffffffff81197535:  je              0xffffffff8119768b
    */
    std::vector<int32_t> disps {};
    disps.reserve(max_insn);

    std::basic_string<unsigned int> opcodes {};
    opcodes.reserve(max_insn);

    for (size_t i = 0; i < count; ++i) {
        auto& instruction = insn[i];

        opcodes.push_back(instruction.id);

        // printf("%zu 0x%" PRIx64 ":\t%s\t\t%s\n", i, insn[i].address, insn[i].mnemonic, insn[i].op_str);

#if defined(__aarch64__)
        if (ARM64_INS_LDR == instruction.id
            and instruction.detail->arm64.op_count == 2
            and instruction.detail->arm64.operands[1].type == ARM64_OP_MEM) {
            disps.push_back(instruction.detail->arm64.operands[1].mem.disp);
            continue;
        }
        if (ARM64_INS_CMP == instruction.id
            and instruction.detail->arm64.op_count == 2
            and instruction.detail->arm64.operands[1].type == ARM64_OP_IMM) {
            disps.push_back(instruction.detail->arm64.operands[1].imm);
            continue;
        }
#elif defined(__x86_64__)
        if (X86_INS_CMP == instruction.id
            and instruction.detail->x86.op_count == 2
            and instruction.detail->x86.operands[0].type == X86_OP_MEM) {
            disps.push_back(instruction.detail->x86.operands[0].mem.disp);
            continue;
        }
#else
#error "Unwupported arch"
#endif

        disps.push_back(0);
    }
    cs_free(insn, count);

#if defined(__aarch64__)
    std::array<unsigned int, 4> acces_mod_init_exit { ARM64_INS_LDR, ARM64_INS_CBZ, ARM64_INS_LDR, ARM64_INS_CBNZ };
#elif defined(__x86_64__)
    std::array<unsigned int, 4> acces_mod_init_exit { X86_INS_CMP, X86_INS_JE, X86_INS_CMP, X86_INS_JE };
#else
#error "Unwupported arch"
#endif
    auto pos = opcodes.find(std::basic_string_view<unsigned int> { acces_mod_init_exit.data(), acces_mod_init_exit.size() });
    if (pos == decltype(opcodes)::npos) {
        throw std::runtime_error { "Instructions not found" };
    }

    auto init_offset = disps.at(pos);
    auto exit_offset = disps.at(pos + 2);

#if defined(__aarch64__)
    {
        std::array<unsigned int, 2> access_mod_state { ARM64_INS_LDR, ARM64_INS_CMP };
        size_t pos = 0;
        while (true) {
            pos = opcodes.find(std::basic_string_view<unsigned int> { access_mod_state.data(), access_mod_state.size() }, pos);
            if (pos == decltype(opcodes)::npos) {
                break;
            }
            if (disps.at(pos) == -8 and disps.at(pos + 1) == 3) {
                init_offset += 8;
                exit_offset += 8;
            }
            pos += 2;
        }
    }
#endif

    return { init_offset, exit_offset };
}

size_t get_kernel_symbol_size(uint8_t* sym_module_get_kallsym)
{
    csh handle {};
    cs_insn* insn { nullptr };

    if (cs_open(CAPSTONE_INIT_OPTS, &handle) != CS_ERR_OK) {
        throw std::runtime_error { "failed to initialize capstone engine" };
    }

    auto release_capstone_handle = ScopeTail([&]() {
        cs_close(&handle);
    });

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    constexpr size_t max_insn = 256;

    auto count = cs_disasm(handle, sym_module_get_kallsym, 0x1000,
        0, max_insn, &insn);
    if (count == 0) {
        throw std::runtime_error { "failed to disassemble sym_module_get_kallsym" };
    }

    /*
        arm64
        mov     x3, xx

        x86_64
        mov     ecx, 18h
    */
    size_t kernel_symbol_size = 0;

    for (size_t i = 0; i < count; ++i) {
        auto& instruction = insn[i];

        // printf("%zu 0x%" PRIx64 ":\t%s\t\t%s\n", i, insn[i].address, insn[i].mnemonic, insn[i].op_str);

#if defined(__aarch64__)
        if (ARM64_INS_MOV == instruction.id
            and instruction.detail->arm64.op_count == 2
            and instruction.detail->arm64.operands[0].type == ARM64_OP_REG
            and instruction.detail->arm64.operands[0].reg == ARM64_REG_X3
            and instruction.detail->arm64.operands[1].type == ARM64_OP_IMM) {
            kernel_symbol_size = instruction.detail->arm64.operands[1].imm;
            break;
        }
#elif defined(__x86_64__)
        if (X86_INS_MOV == instruction.id
            and instruction.detail->x86.op_count == 2
            and instruction.detail->x86.operands[0].type == X86_OP_REG
            and instruction.detail->x86.operands[0].reg == X86_REG_ECX
            and instruction.detail->x86.operands[1].type == X86_OP_IMM) {
            kernel_symbol_size = instruction.detail->x86.operands[1].imm;
            break;
        }
#else
#error "Unwupported arch"
#endif
    }
    cs_free(insn, count);

    return kernel_symbol_size;
}

void relocate_arm64_kernel(KernelInformation& ki, uint8_t* __relocate_kernel)
{
    csh handle {};
    cs_insn* insn { nullptr };

    if (cs_open(CS_ARCH_ARM64, CS_MODE_ARM, &handle) != CS_ERR_OK) {
        throw std::runtime_error { "failed to initialize capstone engine" };
    }

    auto release_capstone_handle = ScopeTail([&]() {
        cs_close(&handle);
    });

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    constexpr size_t max_insn = 10;

    struct Aarch64KernelHeader {
        void* b;
        uint64_t offset;
    };

    auto count = cs_disasm(handle, __relocate_kernel, 0x1000,
        reinterpret_cast<uint64_t>(__relocate_kernel), max_insn, &insn);
    if (count == 0) {
        throw std::runtime_error { "failed to disassemble __relocate_kernel" };
    }

    /*
        old kernel
        ldr w9, =rela_offset # _head + (rela - _head) , _head = 0x8000 = hdr->offset
        ldr w10, =rela_size
        movn            x11, #0x7f, lsl #32
        movk            x11, #0x800, lsl #16
        movk            x11, #0

        6.x
        adr x9, relr_start
        adr x10, relr_end
    */

    if (ARM64_INS_ADR == insn[0].id) {
        throw std::runtime_error { "TODO __relocate_kernel with ADR" };

    } else if (ARM64_INS_LDR == insn[0].id) {
        auto rela_offset = *reinterpret_cast<uint32_t*>(insn[0].detail->arm64.operands[1].imm) - ki.load_offset; // relative to _head
        auto rela_size = *reinterpret_cast<uint32_t*>(insn[1].detail->arm64.operands[1].imm);
        uint64_t default_base = 0;

        arm64_decode_movn_movk(&insn[2], 3, &default_base);

        auto kaslr = (reinterpret_cast<uintptr_t>(ki.buffer.data()) - ki.load_offset) - default_base;

        BOOST_LOG_TRIVIAL(debug) << "kernel load offset " << (void*)(uintptr_t)ki.load_offset;
        BOOST_LOG_TRIVIAL(debug) << "kernel rela_offset " << (void*)(uintptr_t)rela_offset;
        BOOST_LOG_TRIVIAL(debug) << "kernel rela_size " << (void*)(uintptr_t)rela_size;
        BOOST_LOG_TRIVIAL(debug) << "kernel default_base " << (void*)(uintptr_t)default_base;

        auto* iter = ki.ptr<ElfW(Rela)*>(rela_offset);
        auto* end = ki.ptr<ElfW(Rela)*>(rela_offset + rela_size);

        ki.ARCH_RELOCATES_KCRCTAB = true;
        ki.kaslr = reinterpret_cast<uintptr_t>(kaslr);
        ki.default_base = default_base;

        for (; iter < end; ++iter) {
            auto* loc = reinterpret_cast<uint64_t*>(kaslr + iter->r_offset);
            auto val = static_cast<uint64_t>(kaslr + iter->r_addend);
            switch (iter->r_info) {
            case R_AARCH64_RELATIVE:
                memcpy(loc, &val, sizeof(val));
                break;
            default:
                BOOST_LOG_TRIVIAL(warning) << "Unsupported relocation type " << iter->r_info;
                break;
            }
        }

    } else {
        throw std::runtime_error { "unsupported __relocate_kernel" };
    }

    // for (size_t i = 0; i < count; ++i) {
    //     printf("%zu 0x%" PRIx64 ":\t%s\t\t%s\n", i, insn[i].address, insn[i].mnemonic, insn[i].op_str);
    // }

    cs_free(insn, count);
}
