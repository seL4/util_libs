/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include "../elfloader.h"
#include "../stdint.h"

#include "structures.h"

/* Page directory for Stage1 translation in PL1 (short-desc format)*/
uint32_t _boot_pd[BIT(PD_BITS)] ALIGN(BIT(PD_SIZE_BITS));
uint32_t _boot_pt[BIT(PT_BITS)] ALIGN(BIT(PT_SIZE_BITS));

/* Page global and middle directory for Stage1 in HYP mode (long-desc format) */
uint64_t _lpae_boot_pgd[BIT(HYP_PGD_BITS)] ALIGN(BIT(HYP_PGD_SIZE_BITS));
uint64_t _lpae_boot_pmd[BIT(HYP_PGD_BITS + HYP_PMD_BITS)] ALIGN(BIT(HYP_PMD_SIZE_BITS));
