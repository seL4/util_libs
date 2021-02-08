/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * Configuration table structures: MP and ACPI
 *
 * NOTE: Some tables have a variable data field size.
 *       In this case, the data field is NOT included
 *       in the structure such that:
 *         sizeof(structure) + data_length
 *       can be used to create tables
 */

#pragma once

#include <stdint.h>
#include <stdlib.h>

#include <platsupport/plat/acpi/regions.h>
#include "regions.h"

#define BIOS_PADDR_START 0x0e0000
#define BIOS_PADDR_END   0x100000

#define ACPI_PTR(x) (char*)x

#define HAS_GENERIC_HEADER(tbl)  ( \
    ACPI_TABLE_TEST(table, RSDT) || \
    ACPI_TABLE_TEST(table, XSDT) || \
    ACPI_TABLE_TEST(table, FADT) || \
    ACPI_TABLE_TEST(table, SPMI) || \
    ACPI_TABLE_TEST(table, HPET) || \
    ACPI_TABLE_TEST(table, BOOT) || \
    ACPI_TABLE_TEST(table, SPCR) || \
    ACPI_TABLE_TEST(table, DMAR) || \
    ACPI_TABLE_TEST(table, SSDT) || \
    ACPI_TABLE_TEST(table, ASF ) || \
    ACPI_TABLE_TEST(table, HEST) || \
    ACPI_TABLE_TEST(table, ERST) || \
    ACPI_TABLE_TEST(table, BERT) || \
    ACPI_TABLE_TEST(table, EINJ) || \
    ACPI_TABLE_TEST(table, ASPT) || \
    ACPI_TABLE_TEST(table, MADT) || \
    ACPI_TABLE_TEST(table, MCFG) || \
    ACPI_TABLE_TEST(table, DSDT) )

/***************
 *** Helpers ***
 ***************/

/*
 * sum bytes at the given location
 */
uint8_t
acpi_calc_checksum(const char* start, int length);

/*
 * return the length of any table
 * returns -1 if the table is not recognised
 */
size_t
acpi_table_length(const void* tbl);

/*
 * Copies the tables from src to dst. Linkage is done on the fly.
 *
 * src must be a list of table regions that should be copied
 * dst must be a list of free regions for storing tables.
 *
 * ACPI_AVAILABLE_PTR type is used as a region to store root table
 * pointers. If ACPI_AVAILABLE is used as a first preference for
 * storing secondary tables. If there is not sufficient room, these
 * tables will be stored in the ACPI_AVAILABLE_PTR regions.
 *
 * On return, dst will reflect the location of duplicated tables.
 * The fields of duplicated tables will be modified to reflect
 * address and table changes. In particular, the RSDT and XSDT tables
 * are generated to reflect the appropriate tables listed in src.
 *
 * This function returns zero on success and non-zero on failure
 */
int
acpi_copy_tables(const RegionList_t* src, RegionList_t* dst);

