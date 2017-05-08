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
#include <pci/pci_config.h>
#include <pci/virtual_pci.h>
#include <pci/virtual_device.h>
#include <pci/ioreg.h>

#define VDEV_DEBUG_VERBOSE 0
#if VDEV_DEBUG_VERBOSE
    #define dvprintf printf
#else
    #define dvprintf(...)
#endif

static uint8_t libpci_vdevice_rebase_callback_ioread(libpci_vdevice_t* vdevice, int offset) {
    assert(vdevice);
    assert(offset >= PCI_BASE_ADDRESS_0 && offset < PCI_BASE_ADDRESS_5 + 4);

    int index = (offset - PCI_BASE_ADDRESS_0) / 4;
    int byte_offset = (offset - PCI_BASE_ADDRESS_0) % 4;
    uint8_t* rebased_addr_ptr = (uint8_t*)(&vdevice->rebased_addr[index]);
    dvprintf("returning rebased_addr rebased_addr[%d] 0x%x\n", index, vdevice->rebased_addr[index]);
    return rebased_addr_ptr[byte_offset];
}

static void libpci_vdevice_rebase_callback_iowrite(libpci_vdevice_t* vdevice, int offset,
                                                   uint8_t val) {
    assert(vdevice);
    assert(offset >= PCI_BASE_ADDRESS_0 && offset < PCI_BASE_ADDRESS_5 + 4);

    int index = (offset - PCI_BASE_ADDRESS_0) / 4;
    int byte_offset = (offset - PCI_BASE_ADDRESS_0) % 4;
    uint8_t* rebased_addr_ptr = (uint8_t*)(&vdevice->rebased_addr[index]);
    uint8_t* rebased_mask_ptr = (uint8_t*)(&vdevice->rebased_writemask[index]);

    /* Set all bits that are writable to zero. */
    rebased_addr_ptr[byte_offset] &= ~rebased_mask_ptr[byte_offset];

    /* Write into all bits writable. */
    rebased_addr_ptr[byte_offset] |= val & rebased_mask_ptr[byte_offset];
}

void libpci_vdevice_enable(libpci_vdevice_t* self, uint8_t bus, uint8_t dev, uint8_t fun,
                           libpci_device_t* pdevice_passthrough) {
    assert(self);
    self->location_bus = bus;
    self->location_dev = dev;
    self->location_fun = fun;
    self->physical_device_passthrough = pdevice_passthrough;
    self->enabled = true;
}

void libpci_vdevice_disable(libpci_vdevice_t* self) {
    assert(self);
    self->enabled = false;
}

bool libpci_vdevice_match(libpci_vdevice_t* self, uint8_t bus, uint8_t dev, uint8_t fun) {
    assert(self);
    if(!self->enabled) {
        /* match nothing if we are disabled. */
        return false;
    }
    return (self->location_bus == bus &&
            self->location_dev == dev &&
            self->location_fun == fun);
}

void libpci_vdevice_set_mode(libpci_vdevice_t* self, int offset,
                             libpci_vdevice_mode_t m) {
    assert(self);
    assert(offset >= 0 && offset < PCI_STD_HEADER_SIZEOF);

    int sz = libpci_device_cfg_sizeof(offset);
    assert(sz > 0);

    for (int i = 0; i < sz; i++) {
        self->mode[offset + i] = m;
    }
}

void libpci_vdevice_rebase_addr_realdevice(libpci_vdevice_t* self,
                                           int base_addr_index,
                                           uint32_t base_addr,
                                           libpci_device_t* dev) {
    assert(self && dev);
    assert(base_addr_index >= 0 && base_addr_index < 6);
    assert(dev->cfg.base_addr_space[base_addr_index] == PCI_BASE_ADDRESS_SPACE_MEMORY);

    if (dev->cfg.base_addr_64H[base_addr_index]) {
        /* Rebasing the HWORD of a 64-bit address is not supported yet. */
        assert(!"Rebase HWORD of 64-bit address not supported.");
        return;
    }

    self->rebase_addr_virtdevice(self, base_addr_index, base_addr,
            dev->cfg.base_addr_size_mask[base_addr_index] & PCI_BASE_ADDRESS_MEM_MASK,
            dev->cfg.base_addr_prefetchable[base_addr_index],
            dev->cfg.base_addr_type[base_addr_index] == PCI_BASE_ADDRESS_MEM_TYPE_64);
    self->physical_device_passthrough = dev;
}

void libpci_vdevice_rebase_ioaddr_realdevice(libpci_vdevice_t* self,
                                             int base_addr_index,
                                             uint32_t base_addr,
                                             libpci_device_t* dev) {
    assert(self && dev);
    assert(base_addr_index >= 0 && base_addr_index < 6);
    assert(dev->cfg.base_addr_space[base_addr_index] == PCI_BASE_ADDRESS_SPACE_IO);

    self->rebase_ioaddr_virtdevice(self, base_addr_index, base_addr,
            dev->cfg.base_addr_size_mask[base_addr_index] & PCI_BASE_ADDRESS_IO_MASK);
    self->physical_device_passthrough = dev;
}


void libpci_vdevice_rebase_addr_virtdevice(libpci_vdevice_t* self,
                                           int base_addr_index,
                                           uint32_t base_addr,
                                           uint32_t size_mask,
                                           bool prefetch,
                                           bool LWord64) {
    assert(self);
    assert(base_addr_index >= 0 && base_addr_index < 6);

    if((size_mask & PCI_BASE_ADDRESS_MEM_MASK) != size_mask) {
        printf("ERROR: size mask invalid 0x%x", size_mask);
        assert(!"size mask invalid");
        return;
    }

    if ((base_addr & size_mask) != base_addr) {
        printf("ERROR: address alignment invalid 0x%x to mask 0x%x", base_addr, size_mask);
        assert(!"address alignment invalid ");
        return;
    }

    self->rebased_writemask[base_addr_index] = size_mask;
    self->rebased_addr[base_addr_index] = PCI_BASE_ADDRESS_SPACE_MEMORY;
    self->rebased_addr[base_addr_index] |= LWord64 ? PCI_BASE_ADDRESS_MEM_TYPE_64:
                                                     PCI_BASE_ADDRESS_MEM_TYPE_32;
    self->rebased_addr[base_addr_index] |= prefetch ? PCI_BASE_ADDRESS_MEM_PREFETCH : 0;
    self->rebased_addr[base_addr_index] |= base_addr;
    self->rebased_type[base_addr_index] = PCI_BASE_ADDRESS_SPACE_MEMORY;

    libpci_vdevice_mode_t m;
    m.mode = PCI_VDEVICE_MODE_REBASED_ADDR;
    m.callback_ioread = libpci_vdevice_rebase_callback_ioread;
    m.callback_iowrite = libpci_vdevice_rebase_callback_iowrite;
    self->set_mode(self, PCI_BASE_ADDRESS_0 + (base_addr_index * 4), m);
}

void libpci_vdevice_rebase_ioaddr_virtdevice(libpci_vdevice_t* self,
                                           int base_addr_index,
                                           uint32_t base_addr,
                                           uint32_t size_mask) {
    assert(self);
    assert(base_addr_index >= 0 && base_addr_index < 6);

    if((size_mask & PCI_BASE_ADDRESS_IO_MASK) != size_mask) {
        printf("ERROR: io size mask invalid 0x%x", size_mask);
        assert(!"io size mask invalid");
        return;
    }

    if ((base_addr & size_mask) != base_addr) {
        printf("ERROR: io address alignment invalid 0x%x to mask 0x%x", base_addr, size_mask);
        assert(!"io address alignment invalid ");
        return;
    }

    self->rebased_writemask[base_addr_index] = size_mask;
    self->rebased_addr[base_addr_index] = base_addr;
    self->rebased_addr[base_addr_index] |= PCI_BASE_ADDRESS_SPACE_IO;
    self->rebased_type[base_addr_index] = PCI_BASE_ADDRESS_SPACE_IO;

    libpci_vdevice_mode_t m;
    m.mode = PCI_VDEVICE_MODE_REBASED_ADDR;
    m.callback_ioread = libpci_vdevice_rebase_callback_ioread;
    m.callback_iowrite = libpci_vdevice_rebase_callback_iowrite;
    self->set_mode(self, PCI_BASE_ADDRESS_0 + (base_addr_index * 4), m);
}

uint32_t libpci_vdevice_ioread(libpci_vdevice_t* self, int offset, int size){
    assert(self);
    assert(size == 1 || size == 2 || size == 4);
    assert(offset >= 0);

    uint32_t result = 0;
    uint8_t* result_p = (uint8_t*)(&result);

    /* Check for attempted access to extended PCI space. */
    if ((offset + size) >= PCI_STD_HEADER_SIZEOF) {
        if (!self->allow_extended_pci_config_space) {
            printf("ERROR: device tried to access extended PCI config space offset %d, but "
                   "allow_extended_pci_config_space was disabled. This is most likely a "
                   "misconfiguration.\n", offset + size);
            assert(!"Device tried to access extended PCI config space.");
            return 0xFFFFFFFF;
        }
        /* Only supported mode for extended PCI is passthrough. */
        assert(self->physical_device_passthrough);
        return libpci_read_reg(self->physical_device_passthrough->bus,
                               self->physical_device_passthrough->dev,
                               self->physical_device_passthrough->fun,
                               offset, size);
    }

    /* Loop through each byte and handle accordingly. */
    for (int i = 0; i < size; i++) {
        libpci_vdevice_mode_t* m = &self->mode[offset + i];
        uint8_t result_byte = 0;

        switch (m->mode) {
        case PCI_VDEVICE_MODE_PASSTHROUGH:
            assert(self->physical_device_passthrough);
            result_byte = libpci_read_reg8(self->physical_device_passthrough->bus,
                                           self->physical_device_passthrough->dev,
                                           self->physical_device_passthrough->fun,
                                           offset + i);
            break;

        case PCI_VDEVICE_MODE_FATAL_ERROR:
            printf("PCI_VDEVICE_MODE_FATAL_ERROR triggered for offset %d size %d\n", offset, size);
            assert(!"PCI_VDEVICE_MODE_FATAL_ERROR triggered.");
            while(1);
            break;

        case PCI_VDEVICE_MODE_CALLBACK:
        case PCI_VDEVICE_MODE_REBASED_ADDR:
            assert(m->callback_ioread);
            result_byte = m->callback_ioread(self, offset + i);
            break;

        default:
            assert(!"unknown mode.");
            break;
        }
        result_p[i] = result_byte;
    }

    return result;
}

void libpci_vdevice_iowrite(libpci_vdevice_t* self, int offset, int size, uint32_t val) {
    assert(self);
    assert(size == 1 || size == 2 || size == 4);
    assert(offset >= 0);
    uint8_t* val_p = (uint8_t*) &val;

    /* Check for attempted access to extended PCI space. */
    if ((offset + size) >= PCI_STD_HEADER_SIZEOF) {
        if (!self->allow_extended_pci_config_space) {
            printf("ERROR: device tried to access extended PCI config space offset %d, but "
                   "allow_extended_pci_config_space was disabled. This is most likely a "
                   "misconfiguration.\n", offset + size);
            assert(!"Device tried to access extended PCI config space.");
            return;
        }
        /* Only supported mode for extended PCI is passthrough. */
        assert(self->physical_device_passthrough);
        libpci_write_reg(self->physical_device_passthrough->bus,
                         self->physical_device_passthrough->dev,
                         self->physical_device_passthrough->fun,
                         offset, val, size);
        return;
    }

    /* Special case handle the case when the entire range is under passthrough. */
    bool passthrough_entire = true;
    for (int i = 0; i < size; i++) {
        libpci_vdevice_mode_t* m = &self->mode[offset + i];
        if (m->mode != PCI_VDEVICE_MODE_PASSTHROUGH) {
            passthrough_entire = false;
            break;
        }
    }
    if (passthrough_entire) {
        assert(self->physical_device_passthrough);
        libpci_write_reg(self->physical_device_passthrough->bus,
                         self->physical_device_passthrough->dev,
                         self->physical_device_passthrough->fun,
                         offset, val, size);
        return;
    }

    /* Loop through each byte and handle accordingly. */
    for (int i = 0; i < size; i++) {
        libpci_vdevice_mode_t* m = &self->mode[offset + i];

        switch (m->mode) {
        case PCI_VDEVICE_MODE_PASSTHROUGH:
            assert(self->physical_device_passthrough);
            printf("    writing 0x%x into offset %d (total val = 0x%x)\n", val_p[i], offset + i, val);
            libpci_write_reg8(self->physical_device_passthrough->bus,
                              self->physical_device_passthrough->dev,
                              self->physical_device_passthrough->fun,
                              offset + i, val_p[i]);
            break;

        case PCI_VDEVICE_MODE_FATAL_ERROR:
            printf("PCI_VDEVICE_MODE_FATAL_ERROR triggered for offset %d size %d\n", offset, size);
            assert(!"PCI_VDEVICE_MODE_FATAL_ERROR triggered.");
            while(1);
            break;

        case PCI_VDEVICE_MODE_CALLBACK:
        case PCI_VDEVICE_MODE_REBASED_ADDR:
            assert(m->callback_iowrite);
            m->callback_iowrite(self, offset + i, ((uint8_t*)(&val))[i]);
            break;

        default:
            assert(!"unknown mode.");
            break;
        }
    }
}

void libpci_vdevice_init(libpci_vdevice_t* vd) {
    assert(vd);
    memset(vd, 0, sizeof(libpci_vdevice_t));
    vd->enable = libpci_vdevice_enable;
    vd->disable = libpci_vdevice_disable;
    vd->match = libpci_vdevice_match;
    vd->set_mode = libpci_vdevice_set_mode;
    vd->rebase_addr_realdevice = libpci_vdevice_rebase_addr_realdevice;
    vd->rebase_ioaddr_realdevice = libpci_vdevice_rebase_ioaddr_realdevice;
    vd->rebase_addr_virtdevice = libpci_vdevice_rebase_addr_virtdevice;
    vd->rebase_ioaddr_virtdevice = libpci_vdevice_rebase_ioaddr_virtdevice;
    vd->ioread = libpci_vdevice_ioread;
    vd->iowrite = libpci_vdevice_iowrite;
}
