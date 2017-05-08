/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include <stdio.h>
#include <assert.h>
#include <pci/pci.h>
#include <pci/ioreg.h>

void libpci_out(uint32_t port_no, uint32_t val, uint8_t size) {
    assert(size == 1 || size == 2 || size == 4);
    int UNUSED ret = libpci_iowrite(port_no, val, size);
    assert(ret == seL4_NoError);
}

uint32_t libpci_in(uint32_t port_no, uint8_t size) {
    assert(size == 1 || size == 2 || size == 4);
    uint32_t val = 0;
    int UNUSED ret = libpci_ioread(port_no, &val, size);
    assert(ret == seL4_NoError);
    return val;
}

void libpci_out32(uint32_t port_no, uint32_t val) {
    libpci_out(port_no, val, 4);
}

uint32_t libpci_in32(uint32_t port_no) {
    return libpci_in(port_no, 4);
}

void libpci_out16(uint32_t port_no, uint32_t val) {
    libpci_out(port_no, val, 2);
}

uint32_t libpci_in16(uint32_t port_no) {
    return libpci_in(port_no, 2);
}

void libpci_out8(uint32_t port_no, uint32_t val) {
    libpci_out(port_no, val, 1);
}

uint32_t libpci_in8(uint32_t port_no) {
    return libpci_in(port_no, 1);
}

uint32_t libpci_read_reg32(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg) {
    reg &= ~MASK(2);
    libpci_out32(PCI_CONF_PORT_ADDR, 0x80000000 | bus << 16 | dev << 11 | fun << 8 | reg);
    return libpci_in32(PCI_CONF_PORT_DATA);
}

void libpci_write_reg32(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg, uint32_t val) {
    reg &= ~MASK(2);
    libpci_out32(PCI_CONF_PORT_ADDR, 0x80000000 | bus << 16 | dev << 11 | fun << 8 | reg);
    libpci_out32(PCI_CONF_PORT_DATA, val);
}

uint16_t libpci_read_reg16(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg) {
    reg &= ~MASK(1);
    libpci_out32(PCI_CONF_PORT_ADDR, 0x80000000 | bus << 16 | dev << 11 | fun << 8 | (reg & ~MASK(2)));
    return ( libpci_in32(PCI_CONF_PORT_DATA) >> ((reg & MASK(2)) * 8) ) & 0xFFFF;
}

void libpci_write_reg16(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg, uint16_t val) {
    reg &= ~MASK(1);
    libpci_out32(PCI_CONF_PORT_ADDR, 0x80000000 | bus << 16 | dev << 11 | fun << 8 | reg);
    libpci_out16(PCI_CONF_PORT_DATA, val);
}

uint16_t libpci_read_reg8(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg) {
    libpci_out32(PCI_CONF_PORT_ADDR, 0x80000000 | bus << 16 | dev << 11 | fun << 8 | (reg & ~MASK(2)));
    return ( libpci_in32(PCI_CONF_PORT_DATA) >> ((reg & MASK(2)) * 8) ) & 0xFF;
}

void libpci_write_reg8(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg, uint8_t val) {
    libpci_out32(PCI_CONF_PORT_ADDR, 0x80000000 | bus << 16 | dev << 11 | fun << 8 | reg);
    libpci_out8(PCI_CONF_PORT_DATA, val);
}

uint32_t libpci_read_reg(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg, uint8_t size) {
    switch (size) {
    case 1: return (uint32_t) libpci_read_reg8(bus, dev, fun, reg);
    case 2: return (uint32_t) libpci_read_reg16(bus, dev, fun, reg);
    case 4: return (uint32_t) libpci_read_reg32(bus, dev, fun, reg);
    default:
        assert(!"libpci_read_reg: unsupported size.");
        return 0xFFFFFFFF;
    }
}

void libpci_write_reg(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg, uint32_t val, uint8_t size) {
    switch (size) {
    case 1: libpci_write_reg8(bus, dev, fun, reg, val); return;
    case 2: libpci_write_reg16(bus, dev, fun, reg, val); return;
    case 4: libpci_write_reg32(bus, dev, fun, reg, val); return;
    default:
        assert(!"libpci_write_reg: unsupported size.");
        return;
    }
}

void libpci_portno_reverse_lookup(uint32_t port_no, uint8_t *bus, uint8_t *dev, uint8_t *fun,
                                  uint8_t *reg) {
    if (bus) {
        *bus = (port_no >> 16) & MASK(8);
    }
    if (dev) {
        *dev = (port_no >> 11) & MASK(5);
    }
    if (fun) {
        *fun = (port_no >> 8) & MASK(3);
    }
    if (reg) {
        *reg = port_no & MASK(8);
    }
}
