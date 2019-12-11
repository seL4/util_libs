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

int efi_guideq(efi_guid_t a, efi_guid_t b)
{
    for (unsigned int i = 0; i < sizeof(efi_guid_t); i++) {
        if (a.b[i] != b.b[i])
            return 0;
    }

    return 1;
}

efi_boot_services_t *get_efi_boot_services(void)
{
    return ((efi_boot_services_t *)(__efi_system_table->boottime));
}
