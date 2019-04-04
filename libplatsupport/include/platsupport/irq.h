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

#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <platsupport/io.h>

typedef enum irq_type {
    PS_NONE,
    PS_MSI,
    PS_IOAPIC,
    PS_INTERRUPT,
    PS_TRIGGER
} irq_type_t;

typedef struct {
    irq_type_t type;
    union {
        struct {
            long ioapic;
            long pin;
            long level;
            long polarity;
            long vector;
        } ioapic;
        struct {
           long pci_bus;
           long pci_dev;
           long pci_func;
           long handle;
           long vector;
        } msi;
        struct {
            long number;
        } irq;
        struct {
            long number;
            long trigger;
        } trigger;
    };
} ps_irq_t;
