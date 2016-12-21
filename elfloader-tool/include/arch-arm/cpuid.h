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

#include <types.h>

/* read ID register from CPUID */
uint32_t read_cpuid_id(void);

/* read MP ID register from CPUID */
uint32_t read_cpuid_mpidr(void);

/* check if CPU is in HYP/EL2 mode */
word_t is_hyp_mode(void);

/* Pretty print CPUID information */
void print_cpuid(void);

/* Returns the Cortex-Ax part number, or -1 */
int get_cortex_a_part(void);

#endif /* _CPUID_H_ */

