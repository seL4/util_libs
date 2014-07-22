/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
/* Xi (Ma) Chen
 * Fri 22 Nov 2013 04:09:58 EST */

#ifndef __LIB_PCI_SUPPORT_LIBRARY_H__
#define __LIB_PCI_SUPPORT_LIBRARY_H__

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include <pci/pci_config.h>
#include <platsupport/io.h>

#define PCI_CONF_PORT_ADDR     0x0CF8
#define PCI_CONF_PORT_DATA     0x0CFC
#define PCI_CONF_PORT_ADDR_END (PCI_CONF_PORT_ADDR + 4)
#define PCI_CONF_PORT_DATA_END (PCI_CONF_PORT_DATA + 4)
#define PCI_MAX_DEVICES 128

/* Structure containing information about a device. When a device is found during scanning,
 * one of these structs is populated from the information read off the device. */
typedef struct libpci_device {
    uint8_t bus;
    uint8_t dev;
    uint8_t fun;

    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t interrupt_pin;
    uint8_t interrupt_line;

    const char* vendor_name;
    const char* device_name;
    libpci_device_iocfg_t cfg;
} libpci_device_t;

uint32_t libpci_ioread(uint32_t port_no, uint32_t* val, uint32_t size);
uint32_t libpci_iowrite(uint32_t port_no, uint32_t val, uint32_t size);

/* The global list of devices that have been found in the last PCI scan. */
extern libpci_device_t libpci_device_list[PCI_MAX_DEVICES];
extern uint32_t libpci_num_devices;

/* Return the first device found matching given vendor and device ID. */
libpci_device_t* libpci_find_device(uint16_t vendor_id, uint16_t device_id);

/* Return an array of all devices found matching vendor and device ID. The out parameter should
 * be of at least size PCI_MAX_DEVICES to be safe. */
int libpci_find_device_all(uint16_t vendor_id, uint16_t device_id, libpci_device_t** out);

/* Return the first device matching the bus, dev, and fun of given device struct. */
libpci_device_t* libpci_find_device_matching(libpci_device_t *device);

/* Scan the entire PCI space, find every device and popular device structures. */
void libpci_scan(ps_io_port_ops_t port_ops);

/* Read base addr info from give device, and populate a base addr info structure. */
void libpci_read_ioconfig(libpci_device_iocfg_t *cfg, uint8_t bus, uint8_t dev, uint8_t fun);

#endif /* __LIB_PCI_SUPPORT_LIBRARY_H__ */

