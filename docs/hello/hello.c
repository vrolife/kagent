#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int __init mod_init(void) {
    printk(KERN_INFO "Hello, World!\n");
    return 0;
}

static void __exit mod_exit(void) {
    printk(KERN_INFO "Goodbye, World!\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("");
MODULE_DESCRIPTION("");
MODULE_VERSION("0.1");
