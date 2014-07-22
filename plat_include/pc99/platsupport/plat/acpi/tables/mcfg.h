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
/* These table not checked. */

typedef struct acpi_mcfg_desc {
    uint64_t  address;
    uint16_t segment;
    uint8_t  bus_end;
    uint8_t  bus_start;
    uint8_t  res[4];
} acpi_mcfg_desc_t;


typedef struct acpi_mcfg {
    acpi_header_t header;
    uint8_t       res[8];
    /* list of descriptors */
//     acpi_mcfg_descriptor_t descriptor
} acpi_mcfg_t;


#pragma pack(pop)


static inline acpi_mcfg_desc_t*
acpi_mcfg_desc_first(acpi_mcfg_t* hdr)
{
    return (acpi_mcfg_desc_t*)(hdr + 1);
}

static inline acpi_mcfg_desc_t*
acpi_mcfg_desc_next(acpi_mcfg_t* mcfg, acpi_mcfg_desc_t* cur)
{
    char* next = (char*)(cur + 1);
    char* end = (char*)mcfg + mcfg->header.length;
    if (next < end) {
        return (acpi_mcfg_desc_t*)next;
    } else {
        return NULL;
    }
}
