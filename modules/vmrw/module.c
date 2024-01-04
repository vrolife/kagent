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
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "kapi.h"
#include "resolve_page.h"
#include "kagent/common.h"

#include "client.h"

#define ENABLE_DEBUG_LOG 0

#if ENABLE_DEBUG_LOG
#define DEBUG_LOG(...) pr_info(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

struct RuntimeInformation rti RUNTIME_INFORMATION = {
    .mm_pgd_required = 1
};

struct dentry * vmrw_file = NULL;

static
ssize_t fop_read(struct file* file, char* req_buffer, size_t size, loff_t* offset)
{
    struct Request req;

    if (size != sizeof(struct Request)) {
        return -EBADMSG;
    }

    copy_from_user(&req, req_buffer, size);

    if (req.version != VMRW_VERSION) {
        return -EBADMSG;
    }

    struct pid* pid = find_get_pid(req.pid);

    if (pid == NULL) {
        return -ENOENT;
    }

    struct task_struct* task = get_pid_task(pid, PIDTYPE_PID);
    if (task == NULL) {
        return -ENOENT;
    }

    struct mm_struct* mm = get_task_mm(task);
    if (mm == NULL) {
        return -ENOENT;
    }
    
    pt_entry_t* mm_pgd = *(pt_entry_t**)((char*)mm + rti.mm_pgd_offset);

#ifdef __aarch64__
// #if ENABLE_DEBUG_LOG
//     typedef void (*show_pte_t)(unsigned long addr);
//     void* kallsyms_lookup_name(const char* name);
//     show_pte_t show_pte = (show_pte_t)kallsyms_lookup_name("show_pte");
//     show_pte((uintptr_t)req.remote);
// #endif
#endif

    size_t remain = req.size;
    uint64_t src = (uintptr_t)req.remote;
    char* dst = (char*)req.local;

    DEBUG_LOG("=  read %016llx %lu to %016llx\n", req.remote, req.size, req.local);

    while(remain) {
        void* page = resolve_page(mm_pgd, src);
        if (page == NULL) {
            DEBUG_LOG("+  invalid page %016llx\n", src);
            break;
        }

        void* page_ptr = ((char*)page + __offset_in_page(src));

        size_t page_sz = __MIN(0x1000 - __offset_in_page(src), remain);

        DEBUG_LOG("!  copy %016llx %lu to %016llx\n", page_ptr, page_sz, dst);

        unsigned long r = copy_to_user(dst, page_ptr, page_sz);
        if (r != 0) {
            remain -= page_sz - r;
            break;
        }

        remain -= page_sz;
        src += page_sz;
        dst += page_sz;
    }

    req.result = req.size - remain;
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
    DEBUG_LOG("vmrw init\n");
    vmrw_file = debugfs_create_file(__this_module.name, 0600, NULL, NULL, &vmrw_fop);
    if (vmrw_file == NULL) {
        pr_error("failed to create vmrw debugfs file\n");
    }
    return 0;
}

void TEXT_EXIT module_exit() {
    DEBUG_LOG("vmrw exit\n");
    debugfs_remove_recursive(vmrw_file);
    vmrw_file = NULL;
}
