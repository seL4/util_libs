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
#define __ACPI_H__

#include <stdint.h>
#include <stdlib.h>

#include <platsupport/io.h>
#include <platsupport/plat/acpi/regions.h>

/* pack structs (do not align fields) */
#pragma pack(push,1)

/*******************
 *** ACPI tables ***
 *******************/

/* Root System Descriptor Pointer "RSD PTR " */
typedef struct acpi_rsdp {
    char         signature[8];
    uint8_t      checksum; /* checksum of bytes 0-19 */
    char         oem_id[6];
    uint8_t      revision;
    uint32_t      rsdt_address;
    uint32_t     length;
    uint64_t      xsdt_address;
    uint8_t      extended_checksum; /* checksum of entire table */
    char         reserved[3];
} acpi_rsdp_t;

/* Generic System Descriptor Table Header */
typedef struct acpi_header {
    char         signature[4];
    uint32_t     length;
    uint8_t      revision;
    uint8_t      checksum;
    char         oem_id[6];
    char         oem_table_id[8];
    uint32_t     oem_revision;
    char         creater_id[4];
    uint32_t     creater_revision;
} acpi_header_t;


/* Generic Address Structure Format */
typedef struct acpi_gastruct {
    uint8_t      space_id;
    uint8_t      reg_bit_width;
    uint8_t      reg_but_offset;
    uint8_t      size;
    uint64_t      address;
} acpi_GAS_t;

#pragma pack(pop)

#include <platsupport/plat/acpi/tables/asf.h>
#include <platsupport/plat/acpi/tables/boot.h>
#include <platsupport/plat/acpi/tables/dmar.h>
#include <platsupport/plat/acpi/tables/dsdt.h>
#include <platsupport/plat/acpi/tables/ssdt.h>
#include <platsupport/plat/acpi/tables/facs.h>
#include <platsupport/plat/acpi/tables/fadt.h>
#include <platsupport/plat/acpi/tables/hpet.h>
#include <platsupport/plat/acpi/tables/rsdt.h>
#include <platsupport/plat/acpi/tables/xsdt.h>
#include <platsupport/plat/acpi/tables/spcr.h>
#include <platsupport/plat/acpi/tables/spmi.h>
#include <platsupport/plat/acpi/tables/hest.h>
#include <platsupport/plat/acpi/tables/erst.h>
#include <platsupport/plat/acpi/tables/bert.h>
#include <platsupport/plat/acpi/tables/einj.h>
#include <platsupport/plat/acpi/tables/mcfg.h>
#include <platsupport/plat/acpi/tables/madt.h>

/* acpi struct */
typedef struct acpi {
    void *regions;
    acpi_rsdp_t *rsdp;
    ps_io_mapper_t io_mapper;
} acpi_t;

/**
 * Initiliase the ACPI library.
 *
 * This will: allocate internal data and parse the acpi tables, which are
 * assumed to be in unmapped physical memory.
 * This function assumes that malloc will work.
 * Once completed, this function will only consume the virtual memory that it has malloced.
 * This function should only be called once.
 *
 * @param io_mapper Interface for mapping physical addresses. see io.h
 *
 * returns: an acpi handle to call other function with.
 */
acpi_t *acpi_init(ps_io_mapper_t io_mapper);

/*
 * Find a specific acpi table.
 *
 * @param acpi the handle returned from acpi_init.
 * @param region the type of region you want to find.
 * @return NULL if not found, a pointer to the acpi header in virtual memory on success.
 */
acpi_header_t *acpi_find_region(acpi_t *acpi, region_type_t region);

#endif /* __ACPI_H__ */
