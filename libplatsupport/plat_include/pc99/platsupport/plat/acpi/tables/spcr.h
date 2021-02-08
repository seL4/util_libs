/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma pack(push,1)

/* Serial Port Console Redirection "SPCR" */
typedef struct acpi_spcr {
    acpi_header_t header;
    uint8_t       iface_type;
    uint8_t       res1[3]; /* 0's */
    acpi_GAS_t    base_address;
    uint8_t       interrupt_type;
    uint8_t       irq;
    uint32_t      global_system_interrupt;
    uint8_t       baud_rate;
    uint8_t       parity;
    uint8_t       stop_bits;
    uint8_t       flow_control;
    uint8_t       term_type;
    uint8_t       res2[1]; /* 0 */
    uint16_t      pci_dev_id;
    uint16_t      pci_vendor_id;
    uint8_t       pci_bus;
    uint8_t       pci_func;
    uint32_t      pci_flags;
    uint8_t       pci_seg;
    uint8_t       res3[4]; /* 0 */
} acpi_spcr_t;

#pragma pack(pop)
