/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
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
