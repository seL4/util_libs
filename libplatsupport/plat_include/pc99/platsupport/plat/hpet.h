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

#include <stdint.h>

#include <platsupport/timer.h>
#include <platsupport/plat/acpi/acpi.h>
#include <platsupport/pmem.h>

#define DEFAULT_HPET_MSI_VECTOR 0

typedef struct PACKED {
    /* vaddr that HPET_BASE (parsed from acpi tables) is mapped in to.*/
    void *vaddr;
    /* irq number of hpet interrupts */
    uint32_t irq;
    /* Use IOAPIC instead of FSB delivery */
    int ioapic_delivery;
} hpet_config_t;

/* hpet data structures / memory maps */
typedef struct hpet_timer {
    uint64_t config;
    uint64_t comparator;
    uint64_t fsb_irr;
    char padding[8];
} hpet_timer_t;

/* the hpet has one set of global config registers */
typedef struct hpet {
    /* Pointer to base address of memory mapped HPET region */
    void *base_addr;
    uint64_t period_ns;
} hpet_t;

static UNUSED timer_properties_t hpet_properties =
{
        .upcounter = true,
        .timeouts = true,
        .absolute_timeouts = true,
        .bit_width = 64,
        .irqs = 1
};

int hpet_init(hpet_t *hpet, hpet_config_t config);
int hpet_start(const hpet_t *hpet);
int hpet_stop(const hpet_t *hpet);
int hpet_set_timeout(const hpet_t *hept, uint64_t absolute_ns);
uint64_t hpet_get_time(const hpet_t *hpet);

/* Queries the HPET device mapped at the provided vaddr and returns
 * whether timer0 (the timer used by hpet_get_timer) supports fsb
 * deliver */
bool hpet_supports_fsb_delivery(void *vaddr);

/* Queries the HPET device mapped at the provided vaddr and returns
 * the mask of interrupts supported by IOAPIC delivery */
uint32_t hpet_ioapic_irq_delivery_mask(void *vaddr);

/* Queries the HPET device mapped at the provided vaddr and returns
 * the level bit */
uint32_t hpet_level(void *vaddr);

/*
 * Find the HPET details from the ACPI tables.
 *
 * @param ops  io_ops to use to parse and map acpi,
 * @param[out] region to populate with details,
 * @return     0 on success.
 */
int hpet_parse_acpi(acpi_t *acpi, pmem_region_t *region);

