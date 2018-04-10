#pragma once

#include <elfloader_common.h>

typedef void (*init_riscv_kernel_t)(paddr_t ui_p_reg_start,
                              paddr_t ui_p_reg_end, int32_t pv_offset, vaddr_t v_entry, 
                              unsigned long hard_id, unsigned long dtb_output);

extern uint64_t _lpae_boot_pgd[];
extern uint64_t _lpae_boot_pmd[];
extern uint32_t _boot_pd[];
