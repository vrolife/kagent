#ifndef __resolve_page_h__
#define __resolve_page_h__

#include <stdint.h>

typedef uintptr_t pt_entry_t;

#define __offset_in_page(addr) ((addr) & 0xFFF)

void* resolve_page(pt_entry_t*mm_pgd, uintptr_t addr);

#endif
