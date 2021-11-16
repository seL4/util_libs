/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#include <utils/util.h>

#include <platsupport/mach/epit.h>
#include <platsupport/plat/timer.h>

#define CLEANUP_FAIL_TEXT "Failed to cleanup the EPIT after failing to initialise it"

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

int epit_set_timeout_ticks(epit_t *epit, uint64_t counterValue, bool periodic)
{
    ZF_LOGF_IF(epit == NULL, "invalid epit provided");
    ZF_LOGF_IF(epit->epit_map == NULL, "uninitialised epit provided");

    if (counterValue >= (1ULL << 32)) {
        /* Counter too large to be stored in 32 bits. */
        ZF_LOGW("counterValue too high, going to be capping it\n");
        counterValue = UINT32_MAX;
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

int epit_set_timeout(epit_t *epit, uint64_t ns, bool periodic)
{
    if (epit->is_timestamp) {
        ZF_LOGW("Using the EPIT as a timeout timer when it is configured as a timestamp timer");
    }
    ZF_LOGF_IF(epit == NULL, "invalid epit provided");
    ZF_LOGF_IF(epit->epit_map == NULL, "uninitialised epit provided");
    /* Set counter modulus - this effectively sets the timeouts to us but doesn't
     * overflow as fast. */
    uint64_t counterValue = (uint64_t)(IPG_FREQ / (epit->prescaler + 1)) * (ns / 1000ULL);
    if (counterValue == 0) {
        return ETIME;
    }

    return epit_set_timeout_ticks(epit, counterValue, periodic);
}

uint64_t epit_get_time(epit_t *epit)
{
    if (!epit->is_timestamp) {
        ZF_LOGW("Using the EPIT as a timestamp timer when it is configured as a timeout timer");
    }
    ZF_LOGF_IF(epit == NULL, "invalid epit provided");
    ZF_LOGF_IF(epit->epit_map == NULL, "uninitialised epit provided");
    uint64_t ticks = (epit->high_bits + !!epit->epit_map->epitsr) << 32llu;
    ticks += (UINT32_MAX - epit->epit_map->epitcnt);
    return (ticks * 1000llu) / (IPG_FREQ / (epit->prescaler + 1));
}

static void epit_handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    assert(data != NULL);
    epit_t *epit = data;
    if (epit->epit_map->epitsr) {
        /* ack the irq */
        epit->epit_map->epitsr = 1;
        if (epit->is_timestamp) {
            epit->high_bits++;
        }
    }
    /* acknowledge the interrupt and call the user callback if any */
    ZF_LOGF_IF(acknowledge_fn(ack_data), "Failed to acknowledge the interrupt from the EPIT");
    if (epit->user_callback) {
        if (epit->is_timestamp) {
            epit->user_callback(epit->user_callback_token, LTIMER_OVERFLOW_EVENT);
        } else {
            epit->user_callback(epit->user_callback_token, LTIMER_TIMEOUT_EVENT);
        }
    }
}

static int allocate_register_callback(pmem_region_t pmem, unsigned curr_num, size_t num_regs, void *token)
{
    assert(token != NULL);
    /* Should only be called once. I.e. only one register field */
    assert(curr_num == 0);
    epit_t *epit = token;
    epit->epit_map = (volatile struct epit_map *) ps_pmem_map(&epit->io_ops, pmem, false, PS_MEM_NORMAL);
    if (!epit->epit_map) {
        ZF_LOGE("Failed to map in registers for the EPIT");
        return EIO;
    }
    epit->timer_pmem = pmem;
    return 0;
}

static int allocate_irq_callback(ps_irq_t irq, unsigned curr_num, size_t num_irqs, void *token)
{
    assert(token != NULL);
    /* Should only be called once. I.e. only one interrupt field */
    assert(curr_num == 0);
    epit_t *epit = token;
    epit->irq_id = ps_irq_register(&epit->io_ops.irq_ops, irq, epit_handle_irq, epit);
    if (epit->irq_id < 0) {
        ZF_LOGE("Failed to register the EPIT interrupt with the IRQ interface");
        return EIO;
    }
    return 0;
}

int epit_init(epit_t *epit, epit_config_t config)
{
    if (epit == NULL) {
        ZF_LOGE("Epit cannot be NULL, must be preallocated");
        return EINVAL;
    }

    /* Initialise the structure */
    epit->io_ops = config.io_ops;
    epit->user_callback = config.user_callback;
    epit->user_callback_token = config.user_callback_token;
    epit->irq_id = PS_INVALID_IRQ_ID;
    epit->prescaler = config.prescaler;
    epit->is_timestamp = config.is_timestamp;

    /* Read the timer's path in the DTB */
    ps_fdt_cookie_t *cookie = NULL;
    int error = ps_fdt_read_path(&epit->io_ops.io_fdt, &epit->io_ops.malloc_ops, config.device_path, &cookie);
    if (error) {
        ZF_LOGF_IF(ps_fdt_cleanup_cookie(&epit->io_ops.malloc_ops, cookie), CLEANUP_FAIL_TEXT);
        ZF_LOGF_IF(epit_destroy(epit), CLEANUP_FAIL_TEXT);
        return ENODEV;
    }

    /* Walk the registers and allocate them */
    error = ps_fdt_walk_registers(&epit->io_ops.io_fdt, cookie, allocate_register_callback, epit);
    if (error) {
        ZF_LOGF_IF(ps_fdt_cleanup_cookie(&epit->io_ops.malloc_ops, cookie), CLEANUP_FAIL_TEXT);
        ZF_LOGF_IF(epit_destroy(epit), CLEANUP_FAIL_TEXT);
        return ENODEV;
    }

    /* Walk the interrupts and allocate the first */
    error = ps_fdt_walk_irqs(&epit->io_ops.io_fdt, cookie, allocate_irq_callback, epit);
    if (error) {
        ZF_LOGF_IF(ps_fdt_cleanup_cookie(&epit->io_ops.malloc_ops, cookie), CLEANUP_FAIL_TEXT);
        ZF_LOGF_IF(epit_destroy(epit), CLEANUP_FAIL_TEXT);
        return ENODEV;
    }

    ZF_LOGF_IF(ps_fdt_cleanup_cookie(&epit->io_ops.malloc_ops, cookie),
               "Failed to cleanup the FDT cookie after initialising the GPT");

    /* Disable EPIT. */
    epit->epit_map->epitcr = 0;

    /* Interrupt when compare with 0. */
    epit->epit_map->epitcmpr = 0;

    return 0;
}

int epit_destroy(epit_t *epit)
{
    if (epit->epit_map) {
        ZF_LOGF_IF(epit_stop(epit), "Failed to stop the GPT before de-allocating it");
        ps_io_unmap(&epit->io_ops.io_mapper, (void *) epit->epit_map, (size_t) epit->timer_pmem.length);
    }

    if (epit->irq_id != PS_INVALID_IRQ_ID) {
        ZF_LOGF_IF(ps_irq_unregister(&epit->io_ops.irq_ops, epit->irq_id), "Failed to unregister IRQ");
    }

    return 0;
}
