/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include "efi.h"

void *__application_handle = NULL;             // current efi application handler
efi_system_table_t *__efi_system_table = NULL; // current efi system table

void efi_early_init(uintptr_t application_handle, uintptr_t efi_system_table)
{
    __application_handle = (void *)application_handle;
    __efi_system_table = (efi_system_table_t *)efi_system_table;
}

/* Before starting the kernel we should notify the UEFI firmware about it
 * otherwise the internal watchdog may reboot us after 5 min.
 * 
 * This means boot time services are not available anymore. We should store 
 * system information e.g. current memory map and pass them to kernel.
 */
unsigned long efi_exit_boot_services(void)
{
    unsigned long status;
    efi_memory_desc_t *memory_map;
    unsigned long map_size;
    unsigned long desc_size, key;
    uint32_t desc_version;

    efi_boot_services_t *bts = get_efi_boot_services();

    /* 
     * As the number of existing memeory segments are unknown, 
     * we need to resort to a trial and error to guess that. 
     * We start from 32 and increase it by one until get a valid value.
     */
    map_size = sizeof(*memory_map) * 32;

again:
    status = bts->allocate_pool(EFI_LOADER_DATA, map_size, (void **)&memory_map);

    if (status != EFI_SUCCESS)
        return status;

    status = bts->get_memory_map(&map_size, memory_map, &key, &desc_size, &desc_version);
    if (status == EFI_BUFFER_TOO_SMALL) {
        bts->free_pool(memory_map);

        map_size += sizeof(*memory_map);
        goto again;
    }

    if (status != EFI_SUCCESS){
        bts->free_pool(memory_map);
        return status;
    }

    status = bts->exit_boot_services(__application_handle, key);
    return status;
}
