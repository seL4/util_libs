/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma pack(push,1)

/* Differentiated System Description Table "DSDT" */
typedef struct acpi_dsdt {
    acpi_header_t header;
    uint8_t       definition_block[];
} acpi_dsdt_t;

#pragma pack(pop)
