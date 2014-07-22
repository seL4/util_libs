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

/* Secondary System Descroption Table "SSDT" */
typedef struct acpi_ssdt {
    acpi_header_t header;
    uint8_t       definition_block[];
} acpi_ssdt_t;

#pragma pack(pop)
