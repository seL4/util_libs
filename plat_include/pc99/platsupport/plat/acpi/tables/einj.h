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

