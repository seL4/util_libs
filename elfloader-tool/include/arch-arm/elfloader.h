/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#pragma once

#include <elfloader_common.h>

typedef void (*init_arm_kernel_t)(paddr_t ui_p_reg_start,
                                  paddr_t ui_p_reg_end,
                                  uintptr_t pv_offset,
                                  vaddr_t v_entry,
                                  paddr_t dtb, uint32_t dtb_size);


/* Enable the mmu. */
extern void arm_enable_mmu(void);
extern void arm_enable_hyp_mmu(void);


/* Setup boot VSpace. */
void init_boot_vspace(struct image_info *kernel_info);
void init_hyp_boot_vspace(struct image_info *kernel_info);

/* Assembly functions. */
extern void flush_dcache(void);
extern void cpu_idle(void);


void smp_boot(void);

/* Secure monitor call */
uint32_t smc(uint32_t, uint32_t, uint32_t, uint32_t);
