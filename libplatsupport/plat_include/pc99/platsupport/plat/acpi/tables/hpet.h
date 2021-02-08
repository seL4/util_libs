/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

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
