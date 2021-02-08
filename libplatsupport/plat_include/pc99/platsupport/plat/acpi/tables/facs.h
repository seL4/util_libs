/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

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
