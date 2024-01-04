#ifndef __kagent_private_h__
#define __kagent_private_h__

#include "common.h"

#define VERMAGIC_PLACEHOLDER "VERMAGICVERMAGICVERMAGICVERMAGICVERMAGICVERMAGICVERMAGICVERMAGICVERMAGICVERMAGICVERMAGICVERMAGICVERMAGICVERMAGICVERMAGICVERMAGIC"
#define RANDOM_NAME_PLACEHOLDER "RANDOMNAME"

struct PACKED SymbolVersion {
    unsigned long crc;
    char name[64 - sizeof(unsigned long)];
};

struct ALIGN_AS(64) KernelModule {
    void* state; // state
    void* init; // prev
    void* exit; // next
    char name[64 - sizeof(unsigned long)];
    char pad1[2048];
};

#endif
