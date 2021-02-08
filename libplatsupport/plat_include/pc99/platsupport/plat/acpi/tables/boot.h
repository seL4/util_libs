/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma pack(push,1)
/* BOOT table struture "BOOT" */
typedef struct acpi_boot {
    acpi_header_t header;
    uint8_t       cmos_index;
    uint8_t       res[3]; /* 0's */
} acpi_boot_t;

#pragma pack(pop)
