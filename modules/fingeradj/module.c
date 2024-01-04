#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "kapi.h"
#include "insn.h"
#include "kagent/common.h"

#include "client.h"

#define ENABLE_DEBUG_LOG 1

#if ENABLE_DEBUG_LOG
#define DEBUG_LOG(...) pr_info(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

struct dentry * fingeradj_file = NULL;

static
ssize_t fop_read(struct file* file, char* req_buffer, size_t size, loff_t* offset)
{
    struct Request req;

    if (size != sizeof(struct Request)) {
        return -EBADMSG;
    }

    copy_from_user(&req, req_buffer, size);

    if (req.version != FINGERADJ_VERSION) {
        return -EBADMSG;
    }
    
    return sizeof(struct Request);
}

static
ssize_t fop_write(struct file* file, const char* ptr, size_t size, loff_t* offset)
{
    return -EPERM;
}

struct file_operations fingeradj_fop = {
    .owner = &__this_module,
    .read = fop_read,
    .write = fop_write
};


extern void input_handle_event();

int TEXT_INIT module_init() {
    DEBUG_LOG("fingeradj init\n");

    DEBUG_LOG("input_handle_event %p\n", input_handle_event);

    fingeradj_file = debugfs_create_file(__this_module.name, 0600, NULL, NULL, &fingeradj_fop);
    if (fingeradj_file == NULL) {
        pr_error("failed to create fingeradj debugfs file\n");
    }

    return 0;
}

void TEXT_EXIT module_exit() {
    DEBUG_LOG("fingeradj exit\n");
    debugfs_remove_recursive(fingeradj_file);
    fingeradj_file = NULL;
}
