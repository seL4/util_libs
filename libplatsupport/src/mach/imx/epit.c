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

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>

#include <platsupport/mach/epit.h>
#include <platsupport/plat/timer.h>

/* EPIT CONTROL REGISTER BITS */
typedef enum {
    /*
     * This bit enables the EPIT.
     */
    EN = 0,

    /*
     * By setting this bit, then when EPIT is disabled (EN=0), then
     * both Main Counter and Prescaler Counter freeze their count at
     * current count values.
     */
    ENMOD = 1,

    /*
     *  This bit enables the generation of interrupt when a compare
     *  event occurs
     */
    OCIEN = 2,

    /*
     * This bit is cleared by hardware reset. It controls whether the
     * counter runs in free running mode OR set and forget mode.
     */
    RLD = 3,

    /*
     * Bits 4 - 15 determine the prescaler value by which the clock is divided
     * before it goes into the counter.
     *
     * The prescaler used is the value in these bits + 1. ie:
     *
     * 0x00 divide by 1
     * 0x01 divide by 2
     * 0x10 divide by 3
     *  .
     *  .
     *  .
     * 0xFFF divide by 4096
     *
     */
    PRESCALER = 4,

    /*
     * This bit controls the counter data when the modulus register is
     * written. When this bit is set, all writes to the load register
     * will overwrite the counter contents and the counter will
     * subsequently start counting down from the programmed value.
     */
    IOVW = 17,

    /*
     * These bits select the clock input used to run the counter. After
     * reset, the system functional clock is selected. The input clock
     * can also be turned off if these bits are set to 00. This field
     * value should only be changed when the EPIT is disabled.
     */
    CLKSRC = 24
} epit_control_reg;

enum IPGConstants {
    IPG_CLK = 1, IPG_CLK_HIGHFREQ = 2, IPG_CLK_32K = 3
};

/* Memory map for EPIT (Enhanced Periodic Interrupt Timer). */
struct epit_map {
    /* epit control register */
    uint32_t epitcr;
    /* epit status register */
    uint32_t epitsr;
    /* epit load register */
    uint32_t epitlr;
    /* epit compare register */
    uint32_t epitcmpr;
    /* epit counter register */
    uint32_t epitcnt;
};

int epit_stop(epit_t *epit)
{
    /* Disable timer irq. */
    epit->epit_map->epitcr &= ~(BIT(EN));
    return 0;
}

int epit_set_timeout(epit_t *epit, uint64_t ns, bool periodic)
{
    /* Set counter modulus - this effectively sets the timeouts to us but doesn't
     * overflow as fast. */
    uint64_t counterValue =  (uint64_t) (IPG_FREQ / (epit->prescaler + 1)) * (ns / 1000ULL);
    if (counterValue >= (1ULL << 32)) {
        ZF_LOGE("ns too high %llu\n", ns);
        /* Counter too large to be stored in 32 bits. */
        return EINVAL;
    }


    /* configure it and turn it on */
    uint32_t reload_val = periodic ? BIT(RLD) : 0;
    epit->epit_map->epitcr = reload_val | (IPG_CLK << CLKSRC) | /* Clock source = IPG */
                             (epit->prescaler << PRESCALER) | /* Set the prescaler */
                             BIT(IOVW) | /* Overwrite counter immediately on write */
                             BIT(OCIEN) | /* Enable interrupt on comparison event */
                             BIT(ENMOD) | /* Count from modulus on restart */
                             BIT(EN);

    /* hardware has a race condition where the epitlr won't be set properly
     * - keep trying until it works
     */
    epit->epit_map->epitlr = counterValue;
    while (epit->epit_map->epitlr != counterValue) {
        epit->epit_map->epitlr = counterValue;
    }

    return 0;
}

int epit_handle_irq(epit_t *epit)
{
    if (epit->epit_map->epitsr) {
        /* ack the irq */
        epit->epit_map->epitsr = 1;
    }

    return 0;
}

int epit_init(epit_t *epit, epit_config_t config)
{
    if (epit == NULL) {
        ZF_LOGE("Epit cannot be NULL, must be preallocated");
        return EINVAL;
    }

    /* check the irq */
    if (config.irq != EPIT1_INTERRUPT && config.irq != EPIT2_INTERRUPT) {
        ZF_LOGE("Invalid irq %u for epit, expected %u or %u\n", config.irq,
                EPIT1_INTERRUPT, EPIT2_INTERRUPT);
        return EINVAL;
    }

    epit->epit_map = (volatile struct epit_map*)config.vaddr;
    epit->prescaler = config.prescaler;

    /* Disable EPIT. */
    epit->epit_map->epitcr = 0;

    /* Interrupt when compare with 0. */
    epit->epit_map->epitcmpr = 0;

    return 0;
}
