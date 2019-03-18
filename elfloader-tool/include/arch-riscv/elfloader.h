/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */
#pragma once

#include <elfloader_common.h>

typedef void (*init_riscv_kernel_t)(paddr_t ui_p_reg_start,
                              paddr_t ui_p_reg_end, int32_t pv_offset, vaddr_t v_entry);

