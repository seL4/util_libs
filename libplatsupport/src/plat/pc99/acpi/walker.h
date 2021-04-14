/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>

#include <platsupport/plat/acpi/regions.h>
#include <platsupport/plat/acpi/acpi.h>

#include "regions.h"
/*
 * Find the address of "sig" between the given addresses.
 * sig_len provides the length of sig to allow a sig that
 * is not NULL terminated.
 * -- In general, use this to find the RSDT pointer
 */
void *
acpi_sig_search(acpi_t *acpi, const char *sig, int sig_len, void *start, void *end);

/*
 * walk the tables and report table locations and sizes
 * Returns -1 if unable to parse RSDP, 0 on success
 */
int
acpi_parse_tables(acpi_t *acpi);

/*
 * Parse the acpi table given its paddr.
 * Returns a dynamically allocated copy of the table
 * header. Returns NULL if unable to parse the table.
 */
acpi_header_t *
acpi_parse_table(acpi_t *acpi, void *table_paddr);
