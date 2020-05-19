/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#pragma once
#include <autoconf.h>
#include <elfloader_common.h>

int riscv_fputc(int c, volatile void *ptr);

typedef void (*init_riscv_kernel_t)(paddr_t ui_p_reg_start,
                                    paddr_t ui_p_reg_end, int32_t pv_offset,
                                    vaddr_t v_entry,
                                    paddr_t dtb_addr_p,
                                    uint32_t dtb_size
#if CONFIG_MAX_NUM_NODES > 1
                                    ,
                                    uint64_t hart_id,
                                    uint64_t core_id
#endif
                                   );
