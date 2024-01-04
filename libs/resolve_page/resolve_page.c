#include <stdlib.h>

#include "resolve_page.h"

#if 0
#define DEBUG_LOG(...) pr_info(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

#ifdef __aarch64__

// TODO calc offset of memstart_addr at runtime
extern int64_t memstart_addr;

#define PHYS_OFFSET memstart_addr

// 48bit
#define PHYS_MASK 0x3FFFFFF000Ul

// 39bit
#define PAGE_OFFSET 0xffffffc000000000

#define __paddr_to_vaddr(pa) ((unsigned long)((pa) - PHYS_OFFSET) | PAGE_OFFSET)

#define __page_paddr(entry) (PHYS_MASK & entry)

#define __pgd_index(addr) (((addr) >> 30) & 0x1FF)
#define __pgd_offset(pgd, addr) ((pgd) + __pgd_index(addr))

#define __pmd_index(addr) (((addr) >> 21) & 0x1FF)
#define __pmd_offset(dir, addr) ((pt_entry_t*)__paddr_to_vaddr(__page_paddr(*(dir)) + __pmd_index(addr) * sizeof(pt_entry_t)))

#define __pte_index(addr) (((addr) >> 12) & 0x1FF)
#define __pte_offset(dir, addr) ((pt_entry_t*)__paddr_to_vaddr(__page_paddr(*(dir)) + __pte_index(addr) * sizeof(pt_entry_t)))

#define __page_addr(dir, addr) ((pt_entry_t*)__paddr_to_vaddr(__page_paddr(*(dir))))
#define __page_base(addr) ((addr) & ~0xFFFUL)

void* resolve_page(pt_entry_t*mm_pgd, uintptr_t addr)
{
    DEBUG_LOG("-  vma = %p\n", (unsigned long)addr);
    DEBUG_LOG("+  pgd = %p\n", (unsigned long)mm_pgd);

    pt_entry_t* pgd = __pgd_offset(mm_pgd, addr);
    DEBUG_LOG("+ *pgd = %016llx\n", *pgd);

    if ((*pgd & 2) != 0) {
        pt_entry_t* pmd = __pmd_offset(pgd, addr);

        DEBUG_LOG("+ *pmd = %016llx\n", *pmd);

        if ((*pmd & 2) != 0) {
            pt_entry_t* pte = __pte_offset(pmd, addr);
            DEBUG_LOG("+ *pte = %016llx\n", *pte);

            if ((*pte & 1)) {
                void* page = __page_addr(pte, addr);

                DEBUG_LOG("+ page = %016llx\n", page);

                return page;
            }
        }
    }
    return NULL;
}

#else
#error "Unsupported arch"
#endif
