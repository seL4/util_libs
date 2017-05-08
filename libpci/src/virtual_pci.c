/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pci/pci.h>
#include <pci/ioreg.h>
#include <pci/virtual_pci.h>
#include <pci/virtual_device.h>
#include <utils/zf_log.h>

bool libpci_virtual_pci_device_allow(libpci_virtual_pci_t* self, libpci_device_t *device) {
    assert(self);
    if (!device) {
        ZF_LOGD("device_allow error: NULL device!\n");
        return false;
    }
    if (!libpci_find_device_matching(device)) {
        ZF_LOGD("device_allow error: invalid device!\n");
        return false;
    }
    assert(self->num_allowed_devices + 1 < PCI_MAX_VDEVICES);
    libpci_passthrough_vdevice_t *vd = &self->allowed_devices[self->num_allowed_devices];
    vd->host_bus = device->bus;
    vd->host_dev = device->dev;
    vd->host_fun = device->fun;
    self->num_allowed_devices++;
    return true;
}

bool libpci_virtual_pci_device_allow_id(libpci_virtual_pci_t* self, uint16_t vendor_id, uint16_t device_id) {
    libpci_device_t* matched_devices[PCI_MAX_DEVICES];
    int nfound = libpci_find_device_all(vendor_id, device_id, matched_devices);
    for (int i = 0; i < nfound; i++) {
        bool ret = libpci_virtual_pci_device_allow(self, matched_devices[i]);
        if (!ret) return ret;
    }
    return true;
}

bool libpci_virtual_pci_device_disallow(libpci_virtual_pci_t* self, const libpci_device_t *device) {
    assert(self && device);
    for (uint32_t i = 0; i < self->num_allowed_devices; i++) {
        libpci_passthrough_vdevice_t *vd = &self->allowed_devices[i];
        if (vd->host_bus == device->bus &&
            vd->host_dev == device->dev &&
            vd->host_fun == device->fun) {
            vd->host_bus = PCI_HOST_BUS_INVALID;
            return true;
        }
    }
    return false;
}

bool libpci_virtual_pci_device_device_check(libpci_virtual_pci_t* self, uint8_t bus, uint8_t dev, uint8_t fun) {
    assert(self);
    if (self->override_allow_all_devices) {
        return true;
    }
    for (uint32_t i = 0; i < self->num_allowed_devices; i++) {
        libpci_passthrough_vdevice_t *vd = &self->allowed_devices[i];
        if (vd->host_bus == PCI_HOST_BUS_INVALID) continue;
        if (vd->host_bus == bus &&
            vd->host_dev == dev &&
            vd->host_fun == fun) {
            return true;
        }
    }
    return false;
}

libpci_vdevice_t* libpci_virtual_pci_vdevice_assign(libpci_virtual_pci_t* self) {
    assert(self);
    assert(self->num_virtual_devices + 1 < PCI_MAX_VDEVICES);
    libpci_vdevice_init(&self->virtual_devices[self->num_virtual_devices]);
    return &self->virtual_devices[self->num_virtual_devices++];
}

void libpci_virtual_pci_vdevice_resign(libpci_virtual_pci_t* self, libpci_vdevice_t* vdev) {
    assert(self && vdev);
    int index = (vdev - self->virtual_devices);
    assert(index >= 0 && index <= PCI_MAX_VDEVICES);
    self->virtual_devices[index].disable(&self->virtual_devices[index]);
}

libpci_vdevice_t* libpci_virtual_pci_vdevice_check(libpci_virtual_pci_t* self,
                                                   uint8_t bus, uint8_t dev, uint8_t fun) {
    assert(self);
    for(uint32_t i = 0; i < self->num_virtual_devices; i++) {
        libpci_vdevice_t *vd = &self->virtual_devices[i];
        if (vd->match(vd, bus, dev, fun)) {
            return vd;
        }
    }
    return NULL;
}

int libpci_virtual_pci_ioread(libpci_virtual_pci_t* self, uint32_t port_no, uint32_t* val, uint32_t size) {
    if (port_no >= PCI_CONF_PORT_ADDR && port_no < PCI_CONF_PORT_ADDR_END) {
        if (port_no + size > PCI_CONF_PORT_ADDR_END) {
            ZF_LOGD("vpci_ioread WARNING: portno + size = 0x%x invalid address.\n", port_no + size);
            return 1;
        }
        /* Emulate read addr. */
        *val = 0;
        memcpy(val, ((char*)&self->current_addr) + (port_no - PCI_CONF_PORT_ADDR), size);
        return 0;
    }
    if (port_no < PCI_CONF_PORT_DATA || port_no >= PCI_CONF_PORT_DATA_END) {
        ZF_LOGD("vpci_ioread WARNING: port_no 0x%x size %d invalid.\n", port_no, size);
        return 1;
    }

    /* Reverse lookup port_no to bus, dev, fun and reg. */
    uint8_t bus, dev, fun, reg;
    libpci_portno_reverse_lookup(self->current_addr, &bus, &dev, &fun, &reg);

    /* Find a matching virtual device. */
    libpci_vdevice_t *vd = self->vdevice_check(self, bus, dev, fun);
    if (vd) {
        uint32_t data_offset =  port_no - PCI_CONF_PORT_DATA;
        *val = vd->ioread(vd, reg + data_offset, size);
        return 0;
    }

    /* Find a matching passthrough device. */
    bool allowed = self->device_check(self, bus, dev, fun);
    if (!allowed) {
        // Disallowed device, we hide it from the virtual PCI config.
        // By returning a commonly accepted invalid value. (All 1 bits)
        ZF_LOGV("vpci_ioread WARNING: disallowed device %d %d %d.\n", bus, dev, fun);
        *val = PCI_INVALID_READ_VALUE;
        return 0;
    }

    // Address is allowed. Perform normal ioread.
    libpci_out32(PCI_CONF_PORT_ADDR, self->current_addr);
    int ret = libpci_ioread(port_no, val, size);
    return ret;
}

int libpci_virtual_pci_iowrite(libpci_virtual_pci_t* self, uint32_t port_no, uint32_t val, uint32_t size) {
    if (port_no >= PCI_CONF_PORT_ADDR && port_no < PCI_CONF_PORT_ADDR_END) {
        if (port_no + size > PCI_CONF_PORT_ADDR_END) {
            ZF_LOGD("vpci_ioread WARNING: portno + size = 0x%x invalid address.\n", port_no + size);
            return 1;
        }
        /* Emulated set addr. */
        memcpy(((char*)&self->current_addr) + (port_no - PCI_CONF_PORT_ADDR), &val, size);
        return 0;
    }
    if (port_no < PCI_CONF_PORT_DATA || port_no >= PCI_CONF_PORT_DATA_END) {
        ZF_LOGD("vpci_iowrite WARNING: port_no 0x%x size %d invalid.\n", port_no, size);
        return 1;
    }

    uint8_t bus, dev, fun, reg;
    libpci_portno_reverse_lookup(self->current_addr, &bus, &dev, &fun, &reg);

    /* Find a matching virtual device. */
    libpci_vdevice_t *vd = self->vdevice_check(self, bus, dev, fun);
    if (vd) {
        uint32_t data_offset =  port_no - PCI_CONF_PORT_DATA;
        vd->iowrite(vd, reg + data_offset, size, val);
        return 0;
    }

    /* Find a matching passthrough device. */
    bool allowed = self->device_check(self, bus, dev, fun);
    if (!allowed) {
        // Disallowed device, we hide it from the virtual PCI config.
        ZF_LOGV("vpci_iowrite WARNING: disallowed device %d %d %d.\n", bus, dev, fun);
        return 0;
    }

    // Address is allowed. Perform normal iowrite.
    libpci_out32(PCI_CONF_PORT_ADDR, self->current_addr);
    int ret = libpci_iowrite(port_no, val, size);
    return ret;
}

void libpci_virtual_pci_init(libpci_virtual_pci_t* vp) {
    assert(vp);

    /* initialise state */
    vp->num_allowed_devices = 0;
    vp->num_virtual_devices = 0;
    vp->override_allow_all_devices = false;
    vp->current_addr = PCI_INVALID_READ_VALUE;

    /* connect interface */
    vp->device_allow = libpci_virtual_pci_device_allow;
    vp->device_allow_id = libpci_virtual_pci_device_allow_id;
    vp->device_disallow = libpci_virtual_pci_device_disallow;
    vp->device_check = libpci_virtual_pci_device_device_check;
    vp->vdevice_assign = libpci_virtual_pci_vdevice_assign;
    vp->vdevice_resign = libpci_virtual_pci_vdevice_resign;
    vp->vdevice_check = libpci_virtual_pci_vdevice_check;
    vp->ioread = libpci_virtual_pci_ioread;
    vp->iowrite = libpci_virtual_pci_iowrite;
}
