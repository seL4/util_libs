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

/* Server Platform Management Interface "SPMI" */
typedef struct acpi_spmi {
    acpi_header_t header;
    /* res1 must be 0x01 to be compatible
       with previous versions */
    uint8_t       res1[1];
    uint8_t       iface_type;
    uint16_t      specification_revision;
    uint8_t       interrupt_type;
    uint8_t       gpe;
    uint8_t       res2[1];
    uint8_t       pci_device_flag;
    uint32_t      global_system_interrupt;
    acpi_GAS_t    iface_reg_base;
    uint8_t       pci_segment_group;
    uint8_t       pci_bus;
    uint8_t       pci_device;
    uint8_t       pci_function;
} acpi_spmi_t;

#pragma pack(pop)
