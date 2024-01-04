#include <tuple>

#include "unicorn/unicorn.h"
#include "capstone/capstone.h"

#include <boost/log/trivial.hpp>

#include "kdeploy.h"

using namespace std::string_literals;

void dump_uc(cs_arch arch, cs_mode mode, uc_engine* uc)
{
    csh handle {};
    cs_insn* insn { nullptr };

    if (cs_open(arch, mode, &handle) != CS_ERR_OK) {
        throw std::runtime_error { "failed to initialize capstone engine" };
    }

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    {
        uint64_t reg{0};
        for (int i = UC_ARM64_REG_X0; i <= UC_ARM64_REG_X28; ++i) {
            uc_reg_read(uc, i, &reg);
            BOOST_LOG_TRIVIAL(debug) << (i - UC_ARM64_REG_X0) << "\t" << (void*)reg;
        }
        uc_reg_read(uc, UC_ARM64_REG_SP, &reg);
        BOOST_LOG_TRIVIAL(debug) << "sp" << "\t" << (void*)reg;
        uc_reg_read(uc, UC_ARM64_REG_PC, &reg);
        BOOST_LOG_TRIVIAL(debug) << "pc" << "\t" << (void*)reg;
        uc_reg_read(uc, UC_ARM64_REG_LR, &reg);
        BOOST_LOG_TRIVIAL(debug) << "lr" << "\t" << (void*)reg;
    }

    constexpr size_t max_insn = 24;

    uint64_t pc{0};
    uc_reg_read(uc, UC_ARM64_REG_PC, &pc);
    // pc -= 32;

    std::vector<uint8_t> buffer{};
    buffer.resize(max_insn * 8);
    uc_mem_read(uc, pc, buffer.data(), buffer.size());

    auto count = cs_disasm(handle, buffer.data(), buffer.size(), pc, max_insn, &insn);
    if (count == 0) {
        throw std::runtime_error { "failed to disassemble create_pgd_mapping" };
    }

    ssize_t pgd_offset = -1;

    for (size_t i = 0; i < count; ++i) {
        auto& instruction = insn[i];

        printf("%zu 0x%" PRIx64 ":\t%s\t\t%s\n", i, insn[i].address, insn[i].mnemonic, insn[i].op_str);
        fflush(stdout);
    }
    cs_free(insn, count);
    cs_close(&handle);
}

std::tuple<bool, unsigned long> find_symbol_crc_unicorn(KernelInformation& ki, const std::string& name)
{
    uc_engine *uc{nullptr};
    auto err = uc_open(UC_ARCH_ARM64, UC_MODE_ARM, &uc);

    if (err != UC_ERR_OK) {
        throw std::runtime_error("Unable to open unicorn engine "s + std::to_string(err));
    }

    auto head = ki.find_symbol("_head");
    auto end = ki.find_symbol("_end");
    uintptr_t kaslr = head - ki.load_offset - ki.default_base;

    err = uc_mem_map_ptr(uc, head, ki.buffer.capacity(), UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC, ki.buffer.data());
    if (err != UC_ERR_OK) {
        throw std::runtime_error("Unable to map kernel image "s + std::to_string(err));
    }

    uintptr_t stack_base = 0x200000;
    uintptr_t stack_size = 0x100000;
    uintptr_t stack_ptr = stack_base + stack_size - 0x1000;

    err = uc_mem_map(uc, stack_base, stack_size, UC_PROT_READ | UC_PROT_WRITE);
    if (err != UC_ERR_OK) {
        throw std::runtime_error("Unable to map stack memory "s + std::to_string(err));
    }

    {
        uintptr_t stop_addr = 1;

        uc_reg_write(uc, UC_ARM64_REG_SP, &stack_ptr);
        uc_reg_write(uc, UC_ARM64_REG_LR, &stop_addr);

        uc_reg_write(uc, UC_ARM64_REG_X23, &kaslr);

        err = uc_emu_start(uc, ki.get_symbol("__relocate_kernel"), stop_addr, 0, 0);
        if (err != UC_ERR_OK) {
            uintptr_t pc{0};
            uc_reg_read(uc, UC_ARM64_REG_PC, &pc);
            if (pc != 1) {
                BOOST_LOG_TRIVIAL(debug) << "head " << (void*)head;
                BOOST_LOG_TRIVIAL(debug) << "end  " << (void*)end;
                dump_uc(CS_ARCH_ARM64, CS_MODE_ARM, uc);
                throw std::runtime_error("uc_emu_start error __relocate_kernel "s + std::to_string(err));
            }
        }
    }

    {
        uintptr_t owner_ptr_ptr = stack_base;
        uintptr_t crc_ptr_ptr = stack_base + 8;
        uintptr_t name_ptr = stack_base + 16;
        uintptr_t stop_addr = 1;
        uintptr_t gpl_ok = 1;
        uintptr_t warn = 0;
        uintptr_t result = 0;

        uc_mem_write(uc, name_ptr, name.c_str(), name.size() + 1);

        uc_reg_write(uc, UC_ARM64_REG_SP, &stack_ptr);
        uc_reg_write(uc, UC_ARM64_REG_LR, &stop_addr);

        uc_reg_write(uc, UC_ARM64_REG_X0, &name_ptr);
        uc_reg_write(uc, UC_ARM64_REG_X1, &owner_ptr_ptr);
        uc_reg_write(uc, UC_ARM64_REG_X2, &crc_ptr_ptr);
        uc_reg_write(uc, UC_ARM64_REG_X3, &gpl_ok);
        uc_reg_write(uc, UC_ARM64_REG_X4, &warn);

        err = uc_emu_start(uc, ki.get_symbol("find_symbol"), stop_addr, 0, 0);
        if (err != UC_ERR_OK) {
            uintptr_t pc{0};
            uc_reg_read(uc, UC_ARM64_REG_PC, &pc);
            if (pc != 1) {
                BOOST_LOG_TRIVIAL(debug) << "head " << (void*)head;
                BOOST_LOG_TRIVIAL(debug) << "end  " << (void*)end;
                dump_uc(CS_ARCH_ARM64, CS_MODE_ARM, uc);
                throw std::runtime_error("uc_emu_start error find_symbol "s + std::to_string(err));
            }
        }

        uc_reg_read(uc, UC_ARM64_REG_X0, &result);
        if (result != 0) {
            uintptr_t kimage_vaddr{0};
            uc_mem_read(uc, ki.get_symbol("kimage_vaddr"), &kimage_vaddr, 8);
            auto reloc_start = kimage_vaddr - ki.default_base;
            // BOOST_LOG_TRIVIAL(debug) << "reloc_start  " << (void*)(kimage_vaddr - ki.default_base);
            
            // BOOST_LOG_TRIVIAL(debug) << "result  " << (void*)result;
            // uintptr_t name_ptr{0};
            // uc_mem_read(uc, result + 8, &name_ptr, 8);

            // std::string name_buf{};
            // name_buf.resize(32);
            // uc_mem_read(uc, name_ptr, name_buf.data(), 32);

            // BOOST_LOG_TRIVIAL(debug) << "s  " << name_buf;

            uintptr_t owner_ptr{0};
            uintptr_t crc_ptr{0};
            uintptr_t crc{0};

            uc_mem_read(uc, owner_ptr_ptr, &owner_ptr, 8);
            uc_mem_read(uc, crc_ptr_ptr, &crc_ptr, 8);
            uc_mem_read(uc, crc_ptr, &crc, 8);

            // BOOST_LOG_TRIVIAL(debug) << "reloc_start  " << (void*)reloc_start;
            // BOOST_LOG_TRIVIAL(debug) << "crc_ptr  " << (void*)crc_ptr;
            // BOOST_LOG_TRIVIAL(debug) << "crc  " << (void*)(crc);

            return { true, crc - reloc_start };
        }
    }

    return { false, 0 };
}
