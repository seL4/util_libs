/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef __UNIMPLEMENTED_H__
#define __UNIMPLEMENTED_H__

/* this is a dumping ground */
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#ifndef RESOURCE
#define RESOURCE(mapper, id) ps_io_map(mapper,  (uintptr_t) id##_PADDR, id##_SIZE, 0, PS_MEM_NORMAL)
#define UNRESOURCE(mapper, id, addr) ps_io_unmap(mapper, addr, id##_SIZE)
#endif

#define __aligned(x) __attribute__((aligned(x)))
#define unlikely(x) __builtin_expect(!!(x), 0)

#define MAX_PKT_SIZE    1536

#define BITS_PER_LONG 32

void udelay(uint32_t us);

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base);


typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned int  uint;
typedef unsigned char uchar;

typedef u64 __u64;
typedef u32 __u32;
typedef u16 __u16;
typedef u8  __u8;

#include "debug.h"

#define __bitwise /*__attribute__((bitwise))*/
#define __force /* __attribute__((force)) */

typedef s64 __bitwise __le64;
typedef s32 __bitwise __le32;
typedef s16 __bitwise __le16;
typedef s8  __bitwise __le8;

typedef s64 __bitwise __be64;
typedef s32 __bitwise __be32;
typedef s16 __bitwise __be16;
typedef s8  __bitwise __be8;

#define gpio_init() 

#endif /* __UNIMPLEMENTED_H__ */
