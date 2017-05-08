/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
/* Xi (Ma) Chen
 * Fri 22 Nov 2013 04:21:49 EST */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pci/pci.h>
#include <pci/virtual_device.h>

#define PCI_HOST_BUS_INVALID 0xFFFF
#define PCI_INVALID_READ_VALUE 0xFFFFFFFF
#define PCI_MAX_VDEVICES 64

/* A virtual device slot */
typedef struct libpci_virtual_device {
    uint16_t host_bus;
    uint8_t host_dev;
    uint8_t host_fun;
} libpci_passthrough_vdevice_t;

struct libpci_virtual_pci;
typedef struct libpci_virtual_pci libpci_virtual_pci_t;

/* A virtual PCI space. */
struct libpci_virtual_pci {
    libpci_passthrough_vdevice_t allowed_devices[PCI_MAX_VDEVICES];
    uint32_t num_allowed_devices;
    bool override_allow_all_devices;

    libpci_vdevice_t virtual_devices[PCI_MAX_VDEVICES];
    uint32_t num_virtual_devices;
    uint32_t current_addr;

    bool (*device_allow) (libpci_virtual_pci_t* self, libpci_device_t *device);
    bool (*device_allow_id) (libpci_virtual_pci_t* self, uint16_t vendor_id, uint16_t device_id);
    bool (*device_disallow) (libpci_virtual_pci_t* self, const libpci_device_t *device);
    bool (*device_check) (libpci_virtual_pci_t* self, uint8_t bus, uint8_t dev, uint8_t fun);

    libpci_vdevice_t* (*vdevice_assign) (libpci_virtual_pci_t* self);
    void (*vdevice_resign) (libpci_virtual_pci_t* self, libpci_vdevice_t* vdev);
    libpci_vdevice_t* (*vdevice_check) (libpci_virtual_pci_t* self,
            uint8_t bus, uint8_t dev, uint8_t fun);

    int (*ioread) (libpci_virtual_pci_t* self, uint32_t port_no, uint32_t* val, uint32_t size);
    int (*iowrite) (libpci_virtual_pci_t* self, uint32_t port_no, uint32_t val, uint32_t size);
};

void libpci_virtual_pci_init(libpci_virtual_pci_t* vp);
