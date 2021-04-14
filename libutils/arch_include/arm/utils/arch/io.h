/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define DMB()     asm volatile("dmb" ::: "memory")
#define DSB()     asm volatile("dsb" ::: "memory")
#define ISB()     asm volatile("isb" ::: "memory")
#define IO_READ_MEM_BARRIER()   DMB()
#define IO_WRITE_MEM_BARRIER()  DMB()

/* Helper for forcing a read and returning a value
 *
 * Forces a memory read access to the given address.
 */
static inline uintptr_t force_read_value(uintptr_t *address)
{
    uintptr_t value;
    asm volatile("mov %[val], %[addr]"
                 : [val]"=r"(value)
                 : [addr]"r"(*address)
                 /* no clobbers */
                );
    return value;
}
