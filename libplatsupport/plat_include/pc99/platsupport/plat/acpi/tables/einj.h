/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma pack(push,1)

/* Error INJection table */
typedef struct acpi_einj {
    acpi_header_t header;
    uint32_t      einj_header_size;
    uint8_t       injection_flags;
    uint8_t       res[3];
    uint32_t      entry_count;
    /* First of many injection instruction entries */
    /* Not implemented. See page 662 of ACPI book */
//    acpi_einj_entry_t entry;
} acpi_einj_t;

#pragma pack(pop)
