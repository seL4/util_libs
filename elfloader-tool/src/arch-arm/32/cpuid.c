/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#include <types.h>

/* read MP ID register from CPUID */
uint32_t read_cpuid_mpidr(void)
{
    uint32_t val;
    asm volatile("mrc p15, 0, %0, c0, c0, 5" : "=r"(val) :: "cc");
    return val;
}

#define CPSR_MODE_MASK          0x1f
#define CPSR_MODE_HYPERVISOR    0x1a
word_t is_hyp_mode(void)
{
    uint32_t val;
    asm volatile("mrs %0, cpsr" : "=r"(val) :: "cc");
    return ((val & CPSR_MODE_MASK) == CPSR_MODE_HYPERVISOR);
}

/* read ID register from CPUID */
uint32_t read_cpuid_id(void)
{
    uint32_t val;
    asm volatile("mrc p15, 0, %0, c0, c0, 0" : "=r"(val) :: "cc");
    return val;
}
