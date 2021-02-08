/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdint.h>
#include <utils/arith.h>

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
