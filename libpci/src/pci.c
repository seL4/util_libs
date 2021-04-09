/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pci/pci.h>
#include <pci/helper.h>
#include <pci/ioreg.h>
#include <utils/attribute.h>
#include <utils/zf_log.h>

libpci_device_t libpci_device_list[PCI_MAX_DEVICES];
uint32_t libpci_num_devices = 0;
static ps_io_port_ops_t global_port_ops;

uint32_t libpci_ioread(uint32_t port_no, uint32_t *val, uint32_t size)
{
    return (uint32_t)ps_io_port_in(&global_port_ops, port_no, (int)size, val);
}

uint32_t libpci_iowrite(uint32_t port_no, uint32_t val, uint32_t size)
{
    return (uint32_t)ps_io_port_out(&global_port_ops, port_no, (int)size, val);
}

libpci_device_t *libpci_find_device(uint16_t vendor_id, uint16_t device_id)
{
    for (uint32_t i = 0; i < libpci_num_devices; i++) {
        if (libpci_device_list[i].vendor_id == vendor_id &&
            libpci_device_list[i].device_id == device_id) {
            return &libpci_device_list[i];
        }
    }
    return NULL;
}

int libpci_find_device_all(uint16_t vendor_id, uint16_t device_id, libpci_device_t **out)
{
    assert(out);
    int n = 0;
    for (uint32_t i = 0; i < libpci_num_devices; i++) {
        if (libpci_device_list[i].vendor_id == vendor_id &&
            libpci_device_list[i].device_id == device_id) {
            out[n++] = &libpci_device_list[i];
        }
    }
    return n;
}

libpci_device_t *libpci_find_device_matching(libpci_device_t *device)
{
    for (uint32_t i = 0; i < libpci_num_devices; i++) {
        if (libpci_device_list[i].bus == device->bus &&
            libpci_device_list[i].dev == device->dev &&
            libpci_device_list[i].fun == device->fun &&
            libpci_device_list[i].vendor_id == device->vendor_id &&
            libpci_device_list[i].device_id == device->device_id) {
            return &libpci_device_list[i];
        }
    }
    return NULL;
}

libpci_device_t *libpci_find_device_bdf(uint8_t bus, uint8_t dev, uint8_t fun)
{
    for (uint32_t i = 0; i < libpci_num_devices; i++) {
        if (libpci_device_list[i].bus == bus &&
            libpci_device_list[i].dev == dev &&
            libpci_device_list[i].fun == fun) {
            return &libpci_device_list[i];
        }
    }
    return NULL;
}

static int libpci_add_fun(uint8_t bus, uint8_t dev, uint8_t fun)
{
    uint16_t vendor_id = libpci_read_reg16(bus, dev, fun, PCI_VENDOR_ID);

    if (vendor_id == PCI_VENDOR_ID_INVALID) {
        /* No device here. */
        return 0;
    }

    ZF_LOGD("PCI :: Device found at BUS %d DEV %d FUN %d:\n", (int)bus, (int)dev, (int)fun);
    ZF_LOGD("    vendorID = %s [0x%x]\n", libpci_vendorID_str(vendor_id), vendor_id);

    uint16_t device_id = libpci_read_reg16(bus, dev, fun, PCI_DEVICE_ID);
    ZF_LOGD("    deviceID = %s [0x%x]\n", libpci_deviceID_str(vendor_id, device_id), device_id);

    if (libpci_num_devices + 1 > PCI_MAX_DEVICES) {
        return 0;
    }

    libpci_device_list[libpci_num_devices].bus = bus;
    libpci_device_list[libpci_num_devices].dev = dev;
    libpci_device_list[libpci_num_devices].fun = fun;
    libpci_device_list[libpci_num_devices].vendor_id = vendor_id;
    libpci_device_list[libpci_num_devices].device_id = device_id;
    libpci_device_list[libpci_num_devices].vendor_name = libpci_vendorID_str(vendor_id);
    libpci_device_list[libpci_num_devices].device_name = libpci_deviceID_str(vendor_id, device_id);
    libpci_device_list[libpci_num_devices].interrupt_line = libpci_read_reg8(bus, dev, fun, PCI_INTERRUPT_LINE);
    libpci_device_list[libpci_num_devices].interrupt_pin = libpci_read_reg8(bus, dev, fun, PCI_INTERRUPT_PIN);
    libpci_device_list[libpci_num_devices].subsystem_id = libpci_read_reg16(bus, dev, fun, PCI_SUBSYSTEM_ID);
    libpci_read_ioconfig(&libpci_device_list[libpci_num_devices].cfg, bus, dev, fun);

#if (ZF_LOG_LEVEL == ZF_LOG_VERBOSE)
    libpci_device_iocfg_debug_print(&libpci_device_list[libpci_num_devices].cfg, false);
#endif

#ifdef CONFIG_PCI_DISPLAY_FOUND_DEVICES
    printf("PCI :: %.2x.%.2x.%.2x : %s %s (vid 0x%x did 0x%x) line%d pin%d\n", bus, dev, fun,
           libpci_vendorID_str(vendor_id), libpci_deviceID_str(vendor_id, device_id),
           vendor_id, device_id,
           libpci_read_reg8(bus, dev, fun, PCI_INTERRUPT_LINE),
           libpci_read_reg8(bus, dev, fun, PCI_INTERRUPT_PIN)
          );
    libpci_device_iocfg_debug_print(&libpci_device_list[libpci_num_devices].cfg, true);
#endif

    libpci_num_devices++;

    return 1;
}

static void lib_pci_scan_bus(int bus);

static void lib_pci_scan_fun(int bus, int dev, int fun)
{
    libpci_add_fun(bus, dev, fun);
    if (libpci_read_reg16(bus, dev, fun, PCI_CLASS_DEVICE) == 0x0604) {
        int new_bus = libpci_read_reg8(bus, dev, fun, PCI_SECONDARY_BUS);
        ZF_LOGD("%s found additional bus %d from %d %d %d\n", __FUNCTION__, new_bus, bus, dev, fun);
        lib_pci_scan_bus(new_bus);
    }
}

static void lib_pci_scan_dev(int bus, int dev)
{
    uint16_t vendor_id = libpci_read_reg16(bus, dev, 0, PCI_VENDOR_ID);
    if (vendor_id == PCI_VENDOR_ID_INVALID) {
        return;
    }
    ZF_LOGD("%s found pci device %d %d\n", __FUNCTION__, bus, dev);
    lib_pci_scan_fun(bus, dev, 0);
    if ((libpci_read_reg8(bus, dev, 0, PCI_HEADER_TYPE) & 0x80) != 0) {
        ZF_LOGD("%s found multi function device %d %d\n", __FUNCTION__, bus, dev);
        for (int function = 1; function < 8; function++) {
            if (libpci_read_reg16(bus, dev, function, PCI_VENDOR_ID) != PCI_VENDOR_ID_INVALID) {
                lib_pci_scan_fun(bus, dev, function);
            }
        }
    }
}

static void lib_pci_scan_bus(int bus)
{
    for (int dev = 0; dev < 32; dev++) {
        lib_pci_scan_dev(bus, dev);
    }
}

void libpci_scan(ps_io_port_ops_t port_ops)
{
    global_port_ops = port_ops;
    ZF_LOGD("PCI :: Scanning...\n");
    if ((libpci_read_reg8(0, 0, 0, PCI_HEADER_TYPE) & 0x80) == 0) {
        ZF_LOGD("Single bus detected\n");
        lib_pci_scan_bus(0);
    } else {
        for (int function = 0; function < 8; function++) {
            if (libpci_read_reg16(0, 0, function, PCI_VENDOR_ID) != PCI_VENDOR_ID_INVALID) {
                ZF_LOGD("%s detected bus %d\n", __FUNCTION__, function);
                lib_pci_scan_bus(function);
            }
        }
    }
}

void libpci_read_ioconfig(libpci_device_iocfg_t *cfg, uint8_t bus, uint8_t dev, uint8_t fun)
{
    assert(cfg);
    memset(cfg, 0, sizeof(libpci_device_iocfg_t));

    for (int i = 0; i < 6; i++) {
        // Read and save the base address assigned by the BIOS.
        uint32_t bios_base_addr = libpci_read_reg32(bus, dev, fun, PCI_BASE_ADDRESS_0 + (i * 4));
        cfg->base_addr_raw[i] = bios_base_addr;

        if (cfg->base_addr_64H[i]) {
            // Don't bother processing further if this is already part of a 64-bit address.
            cfg->base_addr[i] = cfg->base_addr_raw[i];
            cfg->base_addr_size_mask[i] = 0xFFFFFFFF;
            cfg->base_addr_size[i] = 0;
            continue;
        }

        // Write 0xFFFFFFFF to read the configs.
        libpci_write_reg32(bus, dev, fun, PCI_BASE_ADDRESS_0 + (i * 4), 0xFFFFFFFF);
        uint32_t cfg_base_addr = libpci_read_reg32(bus, dev, fun, PCI_BASE_ADDRESS_0 + (i * 4));

        if (cfg_base_addr == 0)
            /* no device here. */
        {
            continue;
        }

        cfg->base_addr_space[i] = cfg_base_addr & PCI_BASE_ADDRESS_SPACE;
        if (cfg->base_addr_space[i] == PCI_BASE_ADDRESS_SPACE_MEMORY) {
            cfg->base_addr_type[i] = (cfg_base_addr & PCI_BASE_ADDRESS_MEM_TYPE_MASK);
            cfg->base_addr_prefetchable[i] = (cfg_base_addr & PCI_BASE_ADDRESS_MEM_PREFETCH) > 0;
            cfg->base_addr_size_mask[i] = cfg_base_addr & PCI_BASE_ADDRESS_MEM_MASK;
            if (cfg->base_addr_type[i] == PCI_BASE_ADDRESS_MEM_TYPE_64) {
                /* Handle 64-bit addresses. */
                assert(i < 5);
                // Set up the next BAR entry to be 64H mode.
                cfg->base_addr_64H[i + 1] = true;
                // Set up this BAR entry to be in 64L mode.
                cfg->base_addr[i] = bios_base_addr & PCI_BASE_ADDRESS_MEM_MASK;
            } else {
                cfg->base_addr[i] = bios_base_addr & PCI_BASE_ADDRESS_MEM_MASK;
            }
        } else { /* PCI_BASE_ADDRESS_SPACE_IO */
            cfg->base_addr[i] = bios_base_addr & PCI_BASE_ADDRESS_IO_MASK;
            cfg->base_addr_size_mask[i] = cfg_base_addr & PCI_BASE_ADDRESS_IO_MASK;
            cfg->base_addr_type[i] = PCI_BASE_ADDRESS_MEM_TYPE_32;
        }

        /* Calculate size from size_mask. */
        cfg->base_addr_size[i] = BIT(CTZ(cfg->base_addr_size_mask[i]));

        // Write back the address set by the BIOS.
        libpci_write_reg32(bus, dev, fun, PCI_BASE_ADDRESS_0 + (i * 4), bios_base_addr);
    }
}
