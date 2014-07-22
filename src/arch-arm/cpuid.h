/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _CPUID_H_
#define _CPUID_H_

#define CPSR_MODE_MASK 0x1f
#define CPSR_MODE_SUPERVISOR 0x13
#define CPSR_MODE_HYPERVISOR 0x1a


/* read ID register from CPUID */
static inline uint32_t read_cpuid_id(void)
{
    uint32_t val;
    asm volatile("mrc p15, 0, %0, c0, c0, 0" : "=r" (val) :: "cc");
    return val;
}

/* read MP ID register from CPUID */
static inline uint32_t read_cpuid_mpidr(void)
{
    uint32_t val;
    asm volatile("mrc p15, 0, %0, c0, c0, 5" : "=r" (val) :: "cc");
    return val;
}

static inline uint32_t read_cpsr(void)
{
    uint32_t val;
    asm volatile("mrs %0, cpsr" : "=r" (val) :: "cc");
    return val;
}


#endif /* _CPUID_H_ */

