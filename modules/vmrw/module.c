#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "kagent/kapi.h"
#include "kagent/common.h"

#include "client.h"

#define ENABLE_DEBUG_LOG 0

#if ENABLE_DEBUG_LOG
#define DEBUG_LOG(...) pr_info(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

typedef uintptr_t pt_entry_t;

struct RuntimeInformation rti RUNTIME_INFORMATION = {
    .mm_pgd_required = 1
};

struct dentry * vmrw_file = NULL;

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
#define __offset_in_page(addr) ((addr) & 0xFFF)
#define __page_base(addr) ((addr) & ~0xFFFUL)

void* resolve_page(pt_entry_t*mm_pgd, uintptr_t addr)
{
    DEBUG_LOG("+  mm->pgd = %p\n", (unsigned long)mm_pgd);

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

static
ssize_t fop_read(struct file* file, char* req_buffer, size_t size, loff_t* offset)
{
    struct Request req;

    if (size != sizeof(struct Request)) {
        return -EINVAL;
    }

    copy_from_user(&req, req_buffer, size);

    if (req.version != VMRW_VERSION) {
        return -EINVAL;
    }

    struct pid* pid = find_get_pid(req.pid);

    if (pid == NULL) {
        return -EINVAL;
    }

    struct task_struct* task = get_pid_task(pid, PIDTYPE_PID);
    if (task == NULL) {
        return -EINVAL;
    }

    struct mm_struct* mm = get_task_mm(task);
    if (mm == NULL) {
        return -EINVAL;
    }
    
    pt_entry_t* mm_pgd = *(pt_entry_t**)((char*)mm + rti.mm_pgd_offset);

#ifdef __aarch64__
#if ENABLE_DEBUG_LOG
    typedef void (*show_pte_t)(unsigned long addr);
    void* kallsyms_lookup_name(const char* name);
    show_pte_t show_pte = (show_pte_t)kallsyms_lookup_name("show_pte");
    show_pte((uintptr_t)req.remote);
#endif
#endif

    size_t remain = req.size;
    uint64_t src = (uintptr_t)req.remote;
    char* dst = (char*)req.local;

    while(remain) {
        void* page = resolve_page(mm_pgd, src);
        if (page == NULL) {
            DEBUG_LOG("+  invalid page %016llx\n", src);
            break;
        }

        void* page_ptr = ((char*)page + __offset_in_page(src));

        size_t page_sz = __MIN(0x1000 - __offset_in_page(src), remain);

        unsigned long r = copy_to_user(dst, page_ptr, page_sz);
        if (r != 0) {
            remain -= page_sz - r;
            break;
        }

        remain -= page_sz;
        src += page_sz;
        dst += page_sz;
    }

    req.remain = remain;
    copy_to_user(req_buffer, &req, sizeof(struct Request));

    mmput(mm);
    return sizeof(struct Request);
}

static
ssize_t fop_write(struct file* file, const char* ptr, size_t size, loff_t* offset)
{
    return -EPERM;
}

struct file_operations vmrw_fop = {
    .owner = &__this_module,
    .read = fop_read,
    .write = fop_write
};

int TEXT_INIT module_init() {
    pr_info("vmrw init\n");
    vmrw_file = debugfs_create_file(__this_module.name, 0600, NULL, NULL, &vmrw_fop);
    if (vmrw_file == NULL) {
        pr_error("failed to create vmrw debugfs file\n");
    }
    return 0;
}

void TEXT_EXIT module_exit() {
    pr_info("vmrw exit\n");
    debugfs_remove_recursive(vmrw_file);
    vmrw_file = NULL;
}
