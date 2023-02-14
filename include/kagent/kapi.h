#ifndef __kagent_kapi_h__
#define __kagent_kapi_h__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <fcntl.h>

// log

#if defined(__x86_64__)
#define printk _printk
#endif

int printk(const char *fmt, ...);

#define pr_emerg(...) printk("\0010" __VA_ARGS__)
#define pr_alert(...) printk("\0011" __VA_ARGS__)
#define pr_cirt(...) printk("\0012" __VA_ARGS__)
#define pr_err(...) printk("\0013" __VA_ARGS__)
#define pr_error(...) printk("\0013" __VA_ARGS__)
#define pr_warn(...) printk("\0014" __VA_ARGS__)
#define pr_notice(...) printk("\0015" __VA_ARGS__)
#define pr_info(...) printk("\0016" __VA_ARGS__)
#define pr_debug(...) printk("\0017" __VA_ARGS__)

// module

struct module {
    void* state;
    void* prev;
    void* next;
    char name[64 - sizeof(unsigned long)];
    char pad[2048];
};

extern struct module __this_module;

// copy

#if defined(__aarch64__)
#define copy_from_user __arch_copy_from_user
#define copy_to_user __arch_copy_to_user
#define copy_in_user __arch_copy_in_user
#endif

unsigned long copy_from_user(void* dst, const void *src, unsigned long size);
unsigned long copy_to_user(void* dst, const void *src, unsigned long size);
unsigned long copy_in_user(void* dst, const void *src, unsigned long size);

// fs

struct file;

struct file_operations {
    struct module* owner;
    loff_t (*llseek)(struct file*, loff_t offset, int dir);
    ssize_t (*read)(struct file*, char* ptr, size_t size, loff_t* offset);
    ssize_t (*write)(struct file*, const char* ptr, size_t size, loff_t* offset);
    char pad[512];
};

// debugfs

struct dentry;

struct dentry *debugfs_create_dir(const char *name, struct dentry *parent);
void debugfs_remove_recursive(struct dentry *dentry);
struct dentry *debugfs_create_file(
    const char *name, unsigned short mode,
    struct dentry *parent, 
    void *data,
    const struct file_operations *fops);


// task/mm

struct task_struct;
struct mm_struct;
struct pid;

enum pid_type {
    PIDTYPE_PID
};

struct pid* find_get_pid(pid_t pid);
struct task_struct* get_pid_task(struct pid* pid, enum pid_type type);

struct mm_struct* get_task_mm(struct task_struct* task);
void mmput(struct mm_struct* mm);

#endif
