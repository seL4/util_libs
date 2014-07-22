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

/* Firmware ACPI Control Structure "FACS" */
typedef struct acpi_facs {
    char          sig[4];
    uint32_t      length;
    uint32_t      hardware_sig;
    uint32_t       firmware_walking_vector;
    uint32_t      global_lock;
    uint32_t      flags;
    uint64_t       x_firmware_walking_vector;
    uint8_t       version;
    uint8_t       res1[3];
    uint32_t      ospm_flags;
    uint8_t       res2[24];
} acpi_facs_t;

#pragma pack(pop)

