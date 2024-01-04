#include "kapi.h"
#include "kagent/common.h"

#include <stdio.h>

int TEXT_INIT module_init() {
    pr_info("hello init\n");
    return 0;
}

void TEXT_EXIT module_exit() {
    pr_info("hello exit\n");
}
