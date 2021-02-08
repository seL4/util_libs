/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma pack(push,1)

typedef struct acpi_erst_entry {
    uint8_t action;
    uint8_t instruction;
    uint8_t flags;
    uint8_t res;
    acpi_GAS_t register_region;
    uint64_t value;
    uint64_t mask;
} acpi_erst_entry_t;

typedef struct acpi_erst {
    acpi_header_t header;
    uint32_t      sheader_size;
    uint8_t       res[4];
    uint32_t      entry_count;
//    acpi_erst_entry_t entry[]
} acpi_erst_t;

#pragma pack(pop)

/* Retrieve the location of the first item in the list */
static inline acpi_erst_entry_t*
acpi_erst_first(acpi_erst_t* hdr)
{
    return (acpi_erst_entry_t*)(hdr + 1);
}

/* Retrieve the location of the next item in the list */
static inline acpi_erst_entry_t*
acpi_erst_next(acpi_erst_t* hdr, acpi_erst_entry_t* cur)
{
    char* next = (char*)(cur + 1);
    char* end = (char*)hdr + hdr->header.length;
    if (next < end) {
        return (acpi_erst_entry_t*)next;
    } else {
        return NULL;
    }
}
