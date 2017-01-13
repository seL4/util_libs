/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <abort.h>
#include <types.h>
#include <printf.h>

#define print_register(_r) do{                              \
        uint64_t val;                                       \
        asm volatile("mrs %0," #_r : "=r"(val));            \
        printf(#_r": %lx\n", val);                          \
    }while(0)

void invalid_vector_entry(void)
{
    printf("ELF-LOADER: Invalid exception received!\n");
    abort();
}

void el1_sync(void)
{
    printf("ELF-LOADER: Synchronous exception received:\n");
    print_register(esr_el1);
    print_register(elr_el1);
    print_register(spsr_el1);
    print_register(far_el1);
    abort();
}
