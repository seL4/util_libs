/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _PLATSUPPORT_PC99_HPET_H
#define _PLATSUPPORT_PC99_HPET_H

#include <stdint.h>

#include <platsupport/timer.h>

typedef struct PACKED {
    /* vaddr that HPET_BASE (parsed from acpi tables) is mapped in to.*/
    void *vaddr;
    /* irq number of hpet interrupts */
    uint32_t irq;
    /* Use IOAPIC instead of FSB delivery */
    int ioapic_delivery;
} hpet_config_t;

pstimer_t *hpet_get_timer(hpet_config_t *config);

/* Queries the HPET device mapped at the provided vaddr and returns
 * whether timer0 (the timer used by hpet_get_timer) supports fsb
 * deliver */
bool hpet_supports_fsb_delivery(void *vaddr);

/* Queries the HPET device mapped at the provided vaddr and returns
 * the mask of interrupts supported by IOAPIC delivery */
uint32_t hpet_ioapic_irq_delivery_mask(void *vaddr);

#endif /* _PLATSUPPORT_PC99_HPET_H */
