
#include <stdint.h>
#include <errno.h>

#include "kapi.h"

#ifdef __ASSEMBLY__
#define _AC(X,Y)        X
#define _AT(T,X)        X
#else
#define __AC(X,Y)       (X##Y)
#define _AC(X,Y)        __AC(X,Y)
#define _AT(T,X)        ((T)(X))
#endif

#define _UL(x)          (_AC(x, UL))
#define _ULL(x)         (_AC(x, ULL))

#define _BITUL(x)       (_UL(1) << (x))
#define _BITULL(x)      (_ULL(1) << (x))

#define UL(x)           (_UL(x))
#define ULL(x)          (_ULL(x))
#define BIT(nr)                     (UL(1) << (nr))

#define SZ_1                            0x00000001
#define SZ_2                            0x00000002
#define SZ_4                            0x00000004
#define SZ_8                            0x00000008
#define SZ_16                           0x00000010
#define SZ_32                           0x00000020
#define SZ_64                           0x00000040
#define SZ_128                          0x00000080
#define SZ_256                          0x00000100
#define SZ_512                          0x00000200

#define SZ_1K                           0x00000400
#define SZ_2K                           0x00000800
#define SZ_4K                           0x00001000
#define SZ_8K                           0x00002000
#define SZ_16K                          0x00004000
#define SZ_32K                          0x00008000
#define SZ_64K                          0x00010000
#define SZ_128K                         0x00020000
#define SZ_256K                         0x00040000
#define SZ_512K                         0x00080000

#define SZ_1M                           0x00100000
#define SZ_2M                           0x00200000
#define SZ_4M                           0x00400000
#define SZ_8M                           0x00800000
#define SZ_16M                          0x01000000
#define SZ_32M                          0x02000000
#define SZ_64M                          0x04000000
#define SZ_128M                         0x08000000
#define SZ_256M                         0x10000000
#define SZ_512M                         0x20000000

#define SZ_1G                           0x40000000
#define SZ_2G                           0x80000000

#define SZ_4G                           _AC(0x100000000, ULL)
#define SZ_8G                           _AC(0x200000000, ULL)
#define SZ_16G                          _AC(0x400000000, ULL)
#define SZ_32G                          _AC(0x800000000, ULL)

#define SZ_1T                           _AC(0x10000000000, ULL)
#define SZ_64T                          _AC(0x400000000000, ULL)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#ifndef __always_inline
#define __always_inline
#endif

#define bool _Bool
#define false 0
#define true 1
