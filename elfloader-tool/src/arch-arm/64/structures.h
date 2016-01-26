/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */
 
#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_

#define ARM_1GB_BLOCK_BITS      30
#define ARM_2MB_BLOCK_BITS      21

#define PGDE_SIZE_BITS          3
#define PGD_BITS                9
#define PGD_SIZE_BITS           (PGD_BITS + PGDE_SIZE_BITS)

#define PUDE_SIZE_BITS          3
#define PUD_BITS                9
#define PUD_SIZE_BITS           (PUD_BITS + PUDE_SIZE_BITS)

#define PMDE_SIZE_BITS          3
#define PMD_BITS                9
#define PMD_SIZE_BITS           (PMD_BITS + PMDE_SIZE_BITS)

#define GET_PGD_INDEX(x)        (((x) >> (ARM_2MB_BLOCK_BITS + PMD_BITS + PUD_BITS)) & MASK(PGD_BITS))
#define GET_PUD_INDEX(x)        (((x) >> (ARM_2MB_BLOCK_BITS + PMD_BITS)) & MASK(PUD_BITS))

extern uint64_t _boot_pgd_up[BIT(PGD_BITS)];
extern uint64_t _boot_pud_up[BIT(PUD_BITS)];
extern uint64_t _boot_pmd_up[BIT(PMD_BITS)];

extern uint64_t _boot_pgd_down[BIT(PGD_BITS)];
extern uint64_t _boot_pud_down[BIT(PUD_BITS)];

#endif /* _STRUCTURES_H_ */
