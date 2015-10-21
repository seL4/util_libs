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

/* Root System Descriptor Table "RSDT" */
typedef struct acpi_rsdt {
    acpi_header_t  header;
    /* access via inline functions */
//    uint32_t        entry[];
} acpi_rsdt_t;

#pragma pack(pop)


/* retrieve the number of entries in an rsdt table */
static inline int
acpi_rsdt_entry_count(acpi_rsdt_t* t)
{
    return (t->header.length - sizeof(*t)) / sizeof(uint32_t);
}

/* Retrieve the location of the first item in the list */
static inline uint32_t*
acpi_rsdt_first(acpi_rsdt_t* hdr)
{
    return (uint32_t*)(hdr + 1);
}

/* Retrieve the location of the next item in the list */
static inline uint32_t*
acpi_rsdt_next(acpi_rsdt_t* hdr, uint32_t* cur)
{
    char* next = (char*)(cur + 1);
    char* end = (char*)hdr + hdr->header.length;
    if (next < end) {
        return (uint32_t*)next;
    } else {
        return NULL;
    }
}
