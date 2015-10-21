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

/* High precision event timer "HPET" */
typedef struct acpi_hpet {
    acpi_header_t header;
    uint32_t      event_timer_block_id;
    acpi_GAS_t    base_address;
    uint8_t       hpet_number;
    uint16_t      min_ticks;
    uint8_t       page_protection;
} acpi_hpet_t;

#pragma pack(pop)
