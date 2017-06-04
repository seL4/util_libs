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

#ifndef __ACPI_H__
#error This file should not be included directly
#endif

#pragma pack(push,1)
/* BOOT table struture "BOOT" */
typedef struct acpi_boot {
    acpi_header_t header;
    uint8_t       cmos_index;
    uint8_t       res[3]; /* 0's */
} acpi_boot_t;

#pragma pack(pop)
