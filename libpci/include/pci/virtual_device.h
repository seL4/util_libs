/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
/* Virtual PCI devices - allows rebasing base addresses.

     Example usage:
        d = libpci_find_device(0x1022, 0x2000); assert(d);
        v = vpci->vdevice_assign(vpci); assert(v);
        v->enable(v, d->bus, d->dev, d->fun, d);
        v->rebase_ioaddr_realdevice(v, 0, 0x3000, d);
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pci/pci.h>
#include <pci/pci_config.h>

/* Passthrough any config space writes straight onto real device.
   Make sure vdevice->physical_device_passthrough is set. */
#define PCI_VDEVICE_MODE_PASSTHROUGH 0

/* Debug mode which asserts and while(1); when read/write is attempted. */
#define PCI_VDEVICE_MODE_FATAL_ERROR 1

/* Invoke a callback function on read/write to this config space byte. */
#define PCI_VDEVICE_MODE_CALLBACK 2

/* Only used by rebase_addr_* functions. Rebased addrs and callbacks cannot
   be used at same time */
#define PCI_VDEVICE_MODE_REBASED_ADDR 3

struct libpci_vdevice;
typedef struct libpci_vdevice libpci_vdevice_t;

typedef struct libpci_vdevice_mode {
    int mode; /* PCI_VDEVICE_MODE_* */
    uint8_t (*callback_ioread) (libpci_vdevice_t* vdevice, int offset);
    void (*callback_iowrite) (libpci_vdevice_t* vdevice, int offset, uint8_t val);
} libpci_vdevice_mode_t;

struct libpci_vdevice {
    bool enabled;
    bool allow_extended_pci_config_space;

    uint8_t location_bus;
    uint8_t location_dev;
    uint8_t location_fun;

    /* Per-byte device config space virtualisation mode. */
    libpci_vdevice_mode_t mode[PCI_CONFIG_HEADER_SIZE_BYTES];
    /* Which physical device to pass through. */
    libpci_device_t* physical_device_passthrough;

    uint32_t rebased_addr[6];
    uint32_t rebased_writemask[6];
    uint32_t rebased_type[6];

    void (*enable) (libpci_vdevice_t* self, uint8_t bus, uint8_t dev, uint8_t fun,
                    libpci_device_t* pdevice_passthrough /* may be NULL. */);

    void (*disable) (libpci_vdevice_t* self);

    bool (*match) (libpci_vdevice_t* self, uint8_t bus, uint8_t dev, uint8_t fun);

    void (*set_mode) (libpci_vdevice_t* self,
        int offset, /* 0 ... PCI_STD_HEADER_SIZEOF - 1 */
        libpci_vdevice_mode_t m /* The mode to set it to */
    );

    /* implicitly sets physical_device_passthrough to the given device. */
    void (*rebase_addr_realdevice) (libpci_vdevice_t* self,
        int base_addr_index, /* 0 ... 5 */
        uint32_t base_addr, /* correctly aligned new address. */
        libpci_device_t* dev /* contains physical device info */
    );

    /* implicitly sets physical_device_passthrough to the given device. */
    void (*rebase_ioaddr_realdevice) (libpci_vdevice_t* self,
        int base_addr_index, /* 0 ... 5 */
        uint32_t base_addr, /* correctly aligned new address. */
        libpci_device_t* dev /* contains physical device info */
    );

    void (*rebase_addr_virtdevice) (libpci_vdevice_t* self,
        int base_addr_index, /* 0 ... 5 */
        uint32_t base_addr, /* correctly aligned new address. */
        uint32_t size_mask, /* the size mask. last 3 bits must be 0. */
        bool prefetch, /* prefetchable? */
        bool LWord64 /* Is this address the LWORD of a 64-bit address? */
    );

    void (*rebase_ioaddr_virtdevice) (libpci_vdevice_t* self,
        int base_addr_index, /* 0 ... 5 */
        uint32_t base_addr, /* correctly aligned new address. */
        uint32_t size_mask /* the size mask. last 3 bits must be 0. */
    );

    uint32_t (*ioread) (libpci_vdevice_t* self,
                    int offset /* 0 ... PCI_STD_HEADER_SIZEOF - 1 */,
                    int size /* 1, 2 or 4 */);

    void (*iowrite) (libpci_vdevice_t* self,
                     int offset /* 0 ... PCI_STD_HEADER_SIZEOF - 1 */,
                     int size /* 1, 2 or 4 */,
                     uint32_t val);
};

void libpci_vdevice_init(libpci_vdevice_t* vd);
