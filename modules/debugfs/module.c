#include "kapi.h"
#include "kagent/common.h"

#include <stdio.h>
#include <string.h>

struct dentry * debugfs_dir = NULL;

ssize_t control_read(struct file* file, char* ptr, size_t size, loff_t* offset)
{
    char buf[128];

    if (*offset != 0) {
        return 0;
    }

    int n = snprintf(buf, 128, "hello world\n");
    copy_to_user(ptr, buf, __MAX(n, size));
    *offset = n;
    return n;
}

ssize_t control_write(struct file* file, const char* ptr, size_t size, loff_t* offset)
{
    char buf[128];
    memset(buf, 0, 128);
    copy_from_user(buf, ptr, __MIN(128, size));
    pr_info("%s", buf);
    return strlen(buf);
}

struct file_operations control_fop = {
    .owner = &__this_module,
    .read = control_read,
    .write = control_write
};

int TEXT_INIT module_init() {
    pr_info("debugfs init\n");
    debugfs_dir = debugfs_create_dir(__this_module.name, NULL);
    if (debugfs_dir == NULL) {
        pr_error("failed to create debugfs dir\n");
    }
    (void)debugfs_create_file("control", 0600, debugfs_dir, NULL, &control_fop);
    return 0;
}

void TEXT_EXIT module_exit() {
    pr_info("debugfs exit\n");
    debugfs_remove_recursive(debugfs_dir);
    debugfs_dir = NULL;
}
