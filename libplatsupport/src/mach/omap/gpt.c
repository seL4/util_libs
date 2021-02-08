/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>

#include <platsupport/timer.h>
#include <platsupport/mach/gpt.h>
#include <platsupport/io.h>
#include <platsupport/ltimer.h>
#include <platsupport/fdt.h>

typedef enum {

    /* Start or stop the timer */
    ST = 0,

    /* Autoreload mode */
    AR = 1,

    /* Prescale value
     * Timer is prescaled 2^(PTV+1).
     * EG: PTV = 3. Timer increases every 16 clock periods.
     */
    PTV = 2,

    /* Enable prescaler */
    PRE = 5,

    /* Compare enabled */
    CE = 6,

    /* Pulse-width-modulation output pin default setting when
     * counter is stopped or trigger output mode is set to no trigger
     *  0x0: Default value of PWM_out output: 0
     *  0x1: Default value of PWM_out output: 1
     */
    SCPWM = 7,

    /* Transition capture mode
     * 0x0: No capture
     * 0x1: Capture on rising edges of EVENT_CAPTURE pin.
     * 0x2: Capture on falling edges of EVENT_CAPTURE pin.
     * 0x3: Capture on both edges of EVENT_CAPTURE pin.
     */
    TCM = 8,

    /* Trigger output mode
     * 0x0: No trigger
     * 0x1: Overflow trigger
     * 0x2: Overflow and match trigger
     * 0x3: Reserved
     */
    TRG  = 10,

    /* Pulse or toggle select bit. Pulse 0. Toggle 1. */
    PT = 12,

    /* Capture mode select bit (first/second)
     * 0x0: Capture the first enabled capture event in TCAR1.
     * 0x1: Capture the second enabled capture event in TCAR2.
     */
    CAPT_MODE = 13,

    /* PWM output/event detection input pin direction control:
     * 0x0: Configures the pin as an output (needed when PWM mode is required)
     * 0x1: Configures the pin as an input (needed when capture mode is required)
     */
    GPO_CFG = 14
} gpt_control_reg;

typedef enum {

    /* Enable match interrupt */
    MAT_IT_ENA = 0,

    /* Enable overflow interrupt */
    OVF_IT_ENA = 1,

    /* Enable capture interrupt */
    TCAR_IT_ENA = 2

} gpt_int_en_reg;

typedef enum {
    /* General guide for all three flags:
     * Read 1: Interrupt pending
     * Read 0: No Interrupt pending
     * Write 1: Clear flag
     * Write 0: No change
     */

    /* match interrupt */
    MAT_IT_FLAG = 0,

    /* overflow interrupt */
    OVF_IT_FLAG = 1,

    /* capture interrupt */
    TCAR_IT_FLAG = 2

} gpt_int_stat_reg;

typedef enum {

    /* PWM output/event detection input pin direction control:
     * 0x0: Configures the pin as an output (needed when PWMmode is required)
     * 0x1: Configures the pin as an input (needed when capture mode is required)
     */
    AUTOIDLE = 0,

    /* Software reset. This bit is automatically reset by the hardware.
     * During reads, it always returns 0.
     * 0x0: Normal mode
     * 0x1: The module is reset.
     */
    SOFTRESET = 1,

    /* Software reset. This bit is automatically reset by the RW 0
     * hardware. During reads, it always returns 0.
     * 0x0: Normal mode
     * 0x1: The module is reset.
     */
    ENAWAKEUP = 3

} gpt_cfg_reg;

/* Memory map for GPT */
struct gpt_map {
    uint32_t tidr;   // GPTIMER_TIDR 0x00
    uint32_t padding1[3];
    uint32_t cfg;    // GPTIMER_CFG 0x10
    uint32_t tistat; // GPTIMER_TISTAT 0x14
    uint32_t tisr;   // GPTIMER_TISR 0x18
    uint32_t tier;   // GPTIMER_TIER 0x1C
    uint32_t twer;   // GPTIMER_TWER 0x20
    uint32_t tclr;   // GPTIMER_TCLR 0x24
    uint32_t tcrr;   // GPTIMER_TCRR 0x28
    uint32_t tldr;   // GPTIMER_TLDR 0x2C
    uint32_t ttgr;   // GPTIMER_TTGR 0x30
    uint32_t twps;   // GPTIMER_TWPS 0x34
    uint32_t tmar;   // GPTIMER_TMAR 0x38
    uint32_t tcar1;  // GPTIMER_TCAR1 0x3C
    uint32_t tsicr;  // GPTIMER_TSICR 0x40
    uint32_t tcar2;  // GPTIMER_TCAR2 0x44
    uint32_t tpir;   // GPTIMER_TPIR 0x48
    uint32_t tnir;   // GPTIMER_TNIR 0x4C
    uint32_t tcvr;   // GPTIMER_TCVR 0x50
    uint32_t tocr;   // GPTIMER_TOCR 0x54
    uint32_t towr;   // GPTIMER_TOWR 0x58
};

void gpt_start(gpt_t *gpt)
{
    assert(gpt != NULL && gpt->gpt_map != NULL);
    gpt->gpt_map->tclr |= BIT(ST);
}

void gpt_stop(gpt_t *gpt)
{
    assert(gpt != NULL && gpt->gpt_map != NULL);
    /* Disable timer. */
    gpt->gpt_map->tclr &= ~BIT(ST);
}

static void gpt_handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    gpt_t *gpt = data;
    uint32_t tisr = gpt->gpt_map->tisr;

    /* track timekeeping overflow */
    if (tisr & BIT(OVF_IT_FLAG)) {
        gpt->high_bits++;
    }

    /* ack any possible irqs */
    gpt->gpt_map->tisr = (BIT(OVF_IT_FLAG) | BIT(MAT_IT_FLAG) | BIT(TCAR_IT_FLAG));

    if (acknowledge_fn(ack_data)) {
        ZF_LOGE("Failed to acknowledge ps_irq");
    }

    if (gpt->user_callback) {
        if (tisr & BIT(OVF_IT_FLAG)) {
            gpt->user_callback(gpt->user_callback_token, LTIMER_OVERFLOW_EVENT);
        } else if (tisr & BIT(MAT_IT_FLAG)) {
            gpt->user_callback(gpt->user_callback_token, LTIMER_TIMEOUT_EVENT);
        } else {
            ZF_LOGE("Unknown interrupt neither overflow or match");
        }
    }
}

static bool gpt_ok_prescaler(uint32_t prescaler)
{
    if (prescaler > 7) {
        ZF_LOGE("Prescaler value set too large for device, value: %d, max 7", prescaler);
        return false;
    }

    return true;
}

static uint64_t gpt_ticks_to_ns(uint64_t ticks)
{
    return (ticks / CLK_MHZ) * NS_IN_US;
}

static uint64_t gpt_ns_to_ticks(uint64_t ns)
{
    return ns / NS_IN_US * CLK_MHZ;
}

uint64_t gpt_get_max(void)
{
    return gpt_ticks_to_ns(UINT32_MAX - 1);
}

static void gpt_init(gpt_t *gpt)
{
    /* Disable GPT. */
    gpt->gpt_map->tclr = 0;

    /* Perform a soft reset */
    gpt->gpt_map->cfg = BIT(SOFTRESET);

    while (!gpt->gpt_map->tistat); /* Wait for timer to reset */

    /* set prescaler */
    if (gpt->prescaler > 0) {
        gpt->gpt_map->tclr = (gpt->prescaler << PTV); /* Set the prescaler */
        gpt->gpt_map->tclr |= BIT(PRE); /* Enable the prescaler */
    }
}

/* Relative timeout driver for the gpt.
 *
 * This driver sets up the gpt for relative timeouts (oneshot or periodic) only.
 *
 * It works by setting the timer to interrupt on overflow and reloading the timer
 * with (0xFFFFFFFF - relative timeout).
 */

int rel_gpt_set_timeout(gpt_t *gpt, uint64_t ns, bool periodic)
{
    uint32_t reload = periodic ? BIT(AR) : 0;
    uint64_t ticks = gpt_ns_to_ticks(ns) / BIT(gpt->prescaler + 1);

    if (ticks >= UINT32_MAX) {
        /* too big for this timer implementation */
        ZF_LOGE("Timeout too big for timer, max %u, got %llu\n", UINT32_MAX - 1, ticks);
        return ETIME;
    }

    /* invert ticks - it's an upcounter and will interrupt on overflow */
    ticks = UINT32_MAX - ticks;

    gpt_init(gpt);

    /* Clear pending overflows. */
    gpt->gpt_map->tisr |= BIT(OVF_IT_FLAG);

    /* Set the reload value. */
    gpt->gpt_map->tldr = (uint32_t) ticks;

    /* Reset the read register. */
    gpt->gpt_map->tcrr = (uint32_t) ticks;

    /* Enable interrupt on overflow. */
    gpt->gpt_map->tier |= BIT(OVF_IT_ENA);

    assert(!(gpt->gpt_map->tisr & BIT(OVF_IT_FLAG)));
    /* Set autoreload and start the timer. */
    gpt->gpt_map->tclr |= (reload | BIT(ST));

    /* success */
    return 0;
}

int rel_gpt_init(gpt_t *gpt, gpt_config_t config)
{
    if (!gpt_ok_prescaler(config.prescaler)) {
        return EINVAL;
    }

    gpt->prescaler = config.prescaler;

    gpt_init(gpt);

    return 0;
}

uint64_t abs_gpt_get_time(gpt_t *gpt)
{
    bool overflow;
    uint64_t ticks;

    overflow = !!(gpt->gpt_map->tisr & OVF_IT_FLAG);
    /* high bits */
    ticks = ((uint64_t)(gpt->high_bits + overflow)) << 32llu;
    /* low bits */
    ticks += gpt->gpt_map->tcrr;
    ticks = ticks * BIT(gpt->prescaler + 1);

    return gpt_ticks_to_ns(ticks);
}

int abs_gpt_init(gpt_t *gpt, gpt_config_t config)
{
    if (!gpt_ok_prescaler(config.prescaler)) {
        return EINVAL;
    }

    gpt->prescaler = config.prescaler;

    /* enable interrupt on overflow. */
    gpt->gpt_map->tier |= BIT(OVF_IT_ENA);

    /* set the reload value. */
    gpt->gpt_map->tldr = 0u;

    /* reset the read register. */
    gpt->gpt_map->tcrr = 0u;

    /* clear pending irqs. */
    gpt->gpt_map->tisr |= BIT(OVF_IT_FLAG | MAT_IT_FLAG | TCAR_IT_FLAG);

    gpt->gpt_map->tclr |= (BIT(CE) | BIT(AR));

    gpt_init(gpt);

    return 0;
}

static int allocate_register_callback(pmem_region_t pmem, unsigned curr_num, size_t num_regs, void *token)
{
    gpt_t *gpt = token;
    assert(num_regs == 1 && curr_num == 0);
    void *vaddr = ps_pmem_map(&gpt->ops, pmem, false, PS_MEM_NORMAL);
    if (vaddr == NULL) {
        return EIO;
    }
    gpt->gpt_map = vaddr;
    gpt->timer_pmem = pmem;
    return 0;
}

static int allocate_irq_callback(ps_irq_t irq, unsigned curr_num, size_t num_irqs, void *token)
{
    gpt_t *gpt = token;
    /* Device should only have one interrupt */
    if (num_irqs != 1) {
        return ENODEV;
    }
    assert(curr_num == 0);
    irq_id_t irq_id = ps_irq_register(&gpt->ops.irq_ops, irq, gpt_handle_irq, gpt);
    if (irq_id < 0) {
        return EIO;
    }
    gpt->irq_id = irq_id;
    return 0;
}

void gpt_destroy(gpt_t *gpt)
{
    int error;

    /* pre-set INVALID_IRQ_ID before init and do not run if not initialised */
    if (gpt->irq_id != PS_INVALID_IRQ_ID) {
        error = ps_irq_unregister(&gpt->ops.irq_ops, gpt->irq_id);
        ZF_LOGE_IF(error, "Unable to un-register timer gpt irq")
    }

    /* check if pwm_map is NULL and do not run if not initialised */
    if (gpt->gpt_map != NULL) {
        gpt_stop(gpt);
        ps_pmem_unmap(&gpt->ops, gpt->timer_pmem, (void *) gpt->gpt_map);
    }
}

int gpt_create(gpt_t *gpt, ps_io_ops_t ops, char *fdt_path, ltimer_callback_fn_t user_cb_fn, void *user_cb_token)
{
    int error;

    if (gpt == NULL || fdt_path == NULL) {
        return EINVAL;
    }

    /* Set up gpt */
    gpt->ops = ops;
    gpt->user_callback = user_cb_fn;
    gpt->user_callback_token = user_cb_token;

    /* Set up variables that will be set by callback */
    gpt->gpt_map = NULL;
    gpt->irq_id = PS_INVALID_IRQ_ID;

    /* Gather FDT info */
    ps_fdt_cookie_t *cookie = NULL;
    error = ps_fdt_read_path(&ops.io_fdt, &ops.malloc_ops, fdt_path, &cookie);
    if (error) {
        gpt_destroy(gpt);
        return error;
    }

    /* walk the registers and allocate them */
    error = ps_fdt_walk_registers(&ops.io_fdt, cookie, allocate_register_callback, gpt);
    if (error) {
        gpt_destroy(gpt);
        return error;
    }

    /* walk the interrupts and allocate the first */
    error = ps_fdt_walk_irqs(&ops.io_fdt, cookie, allocate_irq_callback, gpt);
    if (error) {
        gpt_destroy(gpt);
        return error;
    }

    error = ps_fdt_cleanup_cookie(&ops.malloc_ops, cookie);
    if (error) {
        gpt_destroy(gpt);
        return error;
    }

    return 0;
}
