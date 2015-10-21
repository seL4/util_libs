/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <platsupport/plat/acpi/regions.h>
#include <platsupport/plat/acpi/acpi.h>

#include "regions.h"
/*
 * Find the address of "sig" between the given addresses.
 * sig_len provides the length of sig to allow a sig that
 * is not NULL terminated.
 * -- In general, use this to find the RSDT pointer
 */
void*
acpi_sig_search(acpi_t* acpi, const char* sig, int sig_len, void* start, void* end);

// walk the tables and report table locations and sizes
void
acpi_parse_tables(acpi_t *acpi);
