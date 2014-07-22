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
 * Fri 22 Nov 2013 04:08:03 EST */

#ifndef __LIB_PCI_SUPPORT_LIBRARY_IOREG_H__
#define __LIB_PCI_SUPPORT_LIBRARY_IOREG_H__

#include <stdint.h>

#ifndef MASK
    #define MASK(n) (BIT(n)-1ul)
#endif

#ifndef BIT
    #define BIT(n) (1ul<<(n))
#endif

/* Generic IOPort in/out functions. */

void libpci_out32(uint32_t port_no, uint32_t val);
uint32_t libpci_in32(uint32_t port_no);
void libpci_out16(uint32_t port_no, uint32_t val);
uint32_t libpci_in16(uint32_t port_no);
void libpci_out8(uint32_t port_no, uint32_t val);
uint32_t libpci_in8(uint32_t port_no);
void libpci_out(uint32_t port_no, uint32_t val, uint8_t size);
uint32_t libpci_in(uint32_t port_no, uint8_t size);

/* IOPort device access functions. */

uint32_t libpci_read_reg32(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg);
void libpci_write_reg32(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg, uint32_t val);
uint16_t libpci_read_reg16(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg);
void libpci_write_reg16(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg, uint16_t val);
uint16_t libpci_read_reg8(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg);
void libpci_write_reg8(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg, uint8_t val);
uint32_t libpci_read_reg(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg, uint8_t size);
void libpci_write_reg(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg, uint32_t val, uint8_t size);

/* Reverse look up which device is associated with a given IOPort number. */
void libpci_portno_reverse_lookup(uint32_t port_no, uint8_t *bus, uint8_t *dev, uint8_t *fun,
                                  uint8_t *reg);

#endif /* __LIB_PCI_SUPPORT_LIBRARY_IOREG_H__ */
