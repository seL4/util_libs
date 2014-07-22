/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __ACPI_H__
#error This file should not be included directly
#endif

#pragma pack(push,1)

/* Fixed ACPI Descriptor Table "FACP" */
typedef struct acpi_fadt {
    acpi_header_t header;
    uint32_t       facs_address;
    uint32_t       dsdt_address;
    uint8_t       res1[1];
    uint8_t       pref_PM_profile;
    uint16_t      sci_int;
    uint32_t      smi_cmd;
    uint8_t       acpi_enable;
    uint8_t       acpi_disable;
    uint8_t       s4bios_req;
    uint8_t       pstate_cnt;
    uint32_t       pm1a_evt_blk;
    uint32_t       pm1b_evt_blk;
    uint32_t       pm1a_cnt_blk;
    uint32_t       pm1b_cnt_blk;
    uint32_t       pm2_cnt_blk;
    uint32_t       pm_tmr_blk;
    uint32_t       gpe0_blk;
    uint32_t       gpe1_blk;
    uint8_t       pm1_evt_len;
    uint8_t       pm1_cnt_len;
    uint8_t       pm2_cnt_len;
    uint8_t       pm_tmr_len;
    uint8_t       gpe0_blk_len;
    uint8_t       gpe1_blk_len;
    uint8_t       gpe1_base;
    uint8_t       cst_cnt;
    uint16_t      p_lvl2_lat;
    uint16_t      p_lvl3_lat;
    uint16_t      flush_size;
    uint16_t      flush_stride;
    uint8_t       duty_offset;
    uint8_t       duty_width;
    uint8_t       day_alrm;
    uint8_t       mon_alrm;
    uint8_t       century;
    uint16_t      iapc_boot_arch;
    uint8_t       res2[1];
    uint32_t      flags;
    acpi_GAS_t    reset_reg;
    uint8_t       reset_value;
    uint8_t       res3[3];
    uint64_t       x_facs_address;
    uint64_t       x_dsdt_address;
    acpi_GAS_t    x_pm1a_evt_blk;
    acpi_GAS_t    x_pm1b_evt_blk;
    acpi_GAS_t    x_pm1a_cnt_blk;
    acpi_GAS_t    x_pm1b_cnt_blk;
    acpi_GAS_t    x_pm2_cnt_blk;
    acpi_GAS_t    x_pm_tmr_blk;
    acpi_GAS_t    x_gpe0_blk;
    acpi_GAS_t    x_gpe1_blk;
    acpi_GAS_t    sleep_control_reg;
    acpi_GAS_t    sleep_status_reg;
    /* Physical table is 24 bytes shorter than this structure on dogfood? */
} acpi_fadt_t;


#pragma pack(pop)
