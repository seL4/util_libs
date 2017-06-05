/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#include <binaries/efi/efi.h>

efi_boot_services_t *get_efi_boot_services(void)
{
    return ((efi_boot_services_t *)(__efi_system_table->boottime));
}

efi_simple_text_output_protocol_t *get_efi_con_out(void)
{
    return ((efi_simple_text_output_protocol_t *)(__efi_system_table->con_out));
}
