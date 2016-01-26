/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */
 
#ifndef __STRUCTURES_32_H_
#define __STRUCTURES_32_H_

#define ARM_SECTION_BITS      20
#define ARM_1GB_BLOCK_BITS    30
#define ARM_2MB_BLOCK_BITS    21

#define PGDE_SIZE_BITS        2
#define PGD_BITS              12
#define PGD_SIZE_BITS         (PGD_BITS + PGDE_SIZE_BITS)

#define HYP_PGDE_SIZE_BITS    3
#define HYP_PGD_BITS          2
#define HYP_PGD_SIZE_BITS     (HYP_PGD_BITS + HYP_PGDE_SIZE_BITS)

#define HYP_PMDE_SIZE_BITS    3
#define HYP_PMD_BITS          9
#define HYP_PMD_SIZE_BITS     (HYP_PMD_BITS + HYP_PMDE_SIZE_BITS)

extern uint32_t _boot_pd[BIT(PGD_BITS)];

extern uint64_t _lpae_boot_pgd[BIT(HYP_PGD_BITS)];
extern uint64_t _lpae_boot_pmd[BIT(HYP_PGD_BITS + HYP_PMD_BITS)];

#endif /* __STRUCTURES_32_H_ */
