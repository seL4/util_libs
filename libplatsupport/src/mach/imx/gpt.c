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

#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

/* The GPT status register is w1c (write 1 to clear), and there are 6 status bits in the iMX
   status register, so writing the value 0b111111 = 0x3F will clear it. */
#define GPT_STATUS_REGISTER_CLEAR 0x3F

#define CLEANUP_FAIL_TEXT "Failed to cleanup the GPT after failing to initialise it"

/* GPT CONTROL REGISTER BITS */
typedef enum {
    /*
     * This bit enables the GPT.
     */
    EN = 0,

    /*
     * When GPT is disabled (EN=0), then
     * both Main Counter and Prescaler Counter freeze their count at
     * current count values. The ENMOD bit determines the value of
     * the GPT counter when Counter is enabled again (if the EN bit is set).
     *
     *   If the ENMOD bit is 1, then the Main Counter and Prescaler Counter
     *   values are reset to 0 after GPT is enabled (EN=1).
     *
     *   If the ENMOD bit is 0, then the Main Counter and Prescaler Counter
     *   restart counting from their frozen values after GPT is enabled (EN=1).
     *
     *   If GPT is programmed to be disabled in a low power mode (STOP/WAIT), then
     *   the Main Counter and Prescaler Counter freeze at their current count
     *   values when the GPT enters low power mode.
     *
     *   When GPT exits low power mode, the Main Counter and Prescaler Counter start
     *   counting from their frozen values, regardless of the ENMOD bit value.
     *
     *   Setting the SWR bit will clear the Main Counter and Prescalar Counter values,
     *   regardless of the value of EN or ENMOD bits.
     *
     *   A hardware reset resets the ENMOD bit.
     *   A software reset does not affect the ENMOD bit.
     */
    ENMOD = 1,

    /*
     * This read/write control bit enables the operation of the GPT
     *  during debug mode
     */
    DBGEN = 2,

    /*
     *  This read/write control bit enables the operation of the GPT
     *  during wait mode
     */
    WAITEN = 3,

    /*
     * This read/write control bit enables the operation of the GPT
     *  during doze mode
     */
    DOZEN = 4,

    /*
     * This read/write control bit enables the operation of the GPT
     *  during stop mode
     */
    STOPEN = 5,

    /*
     * bits 6-8 -  These bits selects the clock source for the
     *  prescaler and subsequently be used to run the GPT counter.
     *  the following sources are available on i.MX 7 board:
     *  000: no clock
     *  001: peripheral clock
     *  010: high frequency reference clock
     *  011: external clock (CLKIN)
     *  100: low frequency reference clock 32 kHZ
     *  101: crystal oscillator as reference clock 24 MHz
     *  others: reserved
     *  by default the peripheral clock is used.
     *
     *  For imx6 :
     *  000: no clock
     *  001: peripheral clock
     *  010: high frequency reference clock
     *  011: external clock (CLKIN)
     *  100: low frequency reference clock
     *  101: crystal oscillator divided by 8 as reference clock
     *  111: crystal osscillator as reference clock
     */
    CLKSRC = 6,

    /*
     * Freerun or Restart mode.
     *
     * 0 Restart mode
     * 1 Freerun mode
     */
    FRR = 9,

    /* for i.MX7 only
     * enable the 24 MHz clock input from crystal
     * a hardware reset resets the EN_24M bit.
     * a software reset dose not affect the EN_24M bit.
     * 0: disabled
     * 1: enabled
     */
    EN_24M = 10,

    /*
     * Software reset.
     *
     * This bit is set when the module is in reset state and is cleared
     * when the reset procedure is over. Writing a 1 to this bit
     * produces a single wait state write cycle. Setting this bit
     * resets all the registers to their default reset values except
     * for the EN, ENMOD, STOPEN, DOZEN, WAITEN and DBGEN bits in this
     *  control register.
     */
    SWR = 15,

    /* Input capture channel operating modes */
    IM1 = 16, IM2 = 18,

    /* Output compare channel operating modes */
    OM1 = 20, OM2 = 23, OM3 = 26,

    /* Force output compare channel bits */
    FO1 = 29, FO2 = 30, FO3 = 31

} gpt_control_reg;

/* bits in the interrupt/status regiser */
enum gpt_interrupt_register_bits {

    /* Output compare interrupt enable bits */
    OF1IE = 0, OF2IE = 1, OF3IE = 2,

    /* Input capture interrupt enable bits */
    IF1IE = 3, IF2IE = 4,

    /* Rollover interrupt enabled */
    ROV = 5,
};

/* Memory map for GPT. */
struct gpt_map {
    /* gpt control register */
    uint32_t gptcr;
    /* gpt prescaler register */
    uint32_t gptpr;
    /* gpt status register */
    uint32_t gptsr;
    /* gpt interrupt register */
    uint32_t gptir;
    /* gpt output compare register 1 */
    uint32_t gptcr1;
    /* gpt output compare register 2 */
    uint32_t gptcr2;
    /* gpt output compare register 3 */
    uint32_t gptcr3;
    /* gpt input capture register 1 */
    uint32_t gpticr1;
    /* gpt input capture register 2 */
    uint32_t gpticr2;
    /* gpt counter register */
    uint32_t gptcnt;
};

int gpt_start(gpt_t *gpt)
{
    gpt->gpt_map->gptcr |= BIT(EN);
    gpt->high_bits = 0;
    return 0;
}

int gpt_stop(gpt_t *gpt)
{
    /* Disable timer. */
    gpt->gpt_map->gptcr &= ~(BIT(EN));
    gpt->high_bits = 0;
    return 0;
}

static void gpt_handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    assert(data != NULL);
    gpt_t *gpt = data;
    /* we've only set the GPT to interrupt on overflow */
    if (gpt->gpt_map->gptcr & BIT(FRR)) {
        /* free-run mode, we should only enable the rollover interrupt */
        if (gpt->gpt_map->gptsr & BIT(ROV)) {
            gpt->high_bits++;
        }
    }
    /* clear the interrupt status register */
    gpt->gpt_map->gptsr = GPT_STATUS_REGISTER_CLEAR;
    /* acknowledge the interrupt and call the user callback if any */
    ZF_LOGF_IF(acknowledge_fn(ack_data), "Failed to acknowledge the interrupt from the GPT");
    if (gpt->user_callback) {
        gpt->user_callback(gpt->user_callback_token, LTIMER_OVERFLOW_EVENT);
    }
}

uint64_t gpt_get_time(gpt_t *gpt)
{
    // Rollover of 32-bit counter can happen while we are reading it,
    // We need to read in the following order (volatile modifier on gpt_map should ensure this):
    // - GPT_SR[ROV]
    // - GPT_CNT
    // - GPT_SR[ROV]
    // If GPT_SR[ROV] bit is unchanged, then the GPT_CNT read is valid and can be used.
    // If GPT_SR[ROV] bit has changed, then the rollover happened sometime between the first
    // read and the third read.  In this case GPT_CNT would have had a value of 0 at some point
    // during our reads and so use 0.
    uint32_t rollover_st = gpt->gpt_map->gptsr & BIT(ROV);
    uint32_t low_bits = gpt->gpt_map->gptcnt;
    uint32_t rollover_st2 = gpt->gpt_map->gptsr & BIT(ROV);
    if (rollover_st != rollover_st2) {
        low_bits = 0;
    }

    // gpt->high_bits is the number of times the timer has overflowed and is managed by this driver.
    // If GPT_SR[ROV] is set then gpt_handle_irq hasn't had a chance to run yet and gpt->high_bits
    // is still recording the old number of overflows. We increment our own local copy and leave
    // the driver copy to be updated by gpt_handle_irq.
    // This driver requires that the IRQ handler function won't be called while other driver functions
    // are executing.
    uint32_t high_bits = gpt->high_bits;
    if (rollover_st2) {
        /* irq has come in */
        high_bits++;
    }

    uint64_t value = ((uint64_t) high_bits << 32llu) + low_bits;
    /* convert to ns */
    uint64_t ns = (value / (uint64_t)GPT_FREQ) * NS_IN_US * (gpt->prescaler + 1);
    return ns;
}

static int allocate_register_callback(pmem_region_t pmem, unsigned curr_num, size_t num_regs, void *token)
{
    assert(token != NULL);
    /* Should only be called once. I.e. only one register field */
    assert(curr_num == 0);
    gpt_t *gpt = token;
    gpt->gpt_map = (volatile struct gpt_map *) ps_pmem_map(&gpt->io_ops, pmem, false, PS_MEM_NORMAL);
    if (!gpt->gpt_map) {
        ZF_LOGE("Failed to map in registers for the GPT");
        return EIO;
    }
    gpt->timer_pmem = pmem;
    return 0;
}

static int allocate_irq_callback(ps_irq_t irq, unsigned curr_num, size_t num_irqs, void *token)
{
    assert(token != NULL);
    /* Should only be called once. I.e. only one interrupt field */
    assert(curr_num == 0);
    gpt_t *gpt = token;
    gpt->irq_id = ps_irq_register(&gpt->io_ops.irq_ops, irq, gpt_handle_irq, gpt);
    if (gpt->irq_id < 0) {
        ZF_LOGE("Failed to register the GPT interrupt with the IRQ interface");
        return EIO;
    }
    return 0;
}

int gpt_init(gpt_t *gpt, gpt_config_t config)
{
    /* Initialise the structure */
    gpt->io_ops = config.io_ops;
    gpt->user_callback = config.user_callback;
    gpt->user_callback_token = config.user_callback_token;
    gpt->irq_id = PS_INVALID_IRQ_ID;
    gpt->prescaler = config.prescaler;

    /* Read the timer's path in the DTB */
    ps_fdt_cookie_t *cookie = NULL;
    int error = ps_fdt_read_path(&gpt->io_ops.io_fdt, &gpt->io_ops.malloc_ops, config.device_path, &cookie);
    if (error) {
        ZF_LOGF_IF(ps_fdt_cleanup_cookie(&gpt->io_ops.malloc_ops, cookie), CLEANUP_FAIL_TEXT);
        ZF_LOGF_IF(gpt_destroy(gpt), CLEANUP_FAIL_TEXT);
        return ENODEV;
    }

    /* Walk the registers and allocate them */
    error = ps_fdt_walk_registers(&gpt->io_ops.io_fdt, cookie, allocate_register_callback, gpt);
    if (error) {
        ZF_LOGF_IF(ps_fdt_cleanup_cookie(&gpt->io_ops.malloc_ops, cookie), CLEANUP_FAIL_TEXT);
        ZF_LOGF_IF(gpt_destroy(gpt), CLEANUP_FAIL_TEXT);
        return ENODEV;
    }

    /* Walk the interrupts and allocate the first */
    error = ps_fdt_walk_irqs(&gpt->io_ops.io_fdt, cookie, allocate_irq_callback, gpt);
    if (error) {
        ZF_LOGF_IF(ps_fdt_cleanup_cookie(&gpt->io_ops.malloc_ops, cookie), CLEANUP_FAIL_TEXT);
        ZF_LOGF_IF(gpt_destroy(gpt), CLEANUP_FAIL_TEXT);
        return ENODEV;
    }

    ZF_LOGF_IF(ps_fdt_cleanup_cookie(&gpt->io_ops.malloc_ops, cookie),
               "Failed to cleanup the FDT cookie after initialising the GPT");

    uint32_t gptcr = 0;
    if (gpt == NULL) {
        return EINVAL;
    }

    /* Disable GPT. */
    gpt->gpt_map->gptcr = 0;
    gpt->gpt_map->gptsr = GPT_STATUS_REGISTER_CLEAR;

    /* Configure GPT. */
    gpt->gpt_map->gptcr = 0 | BIT(SWR); /* Reset the GPT */
    /* SWR will be 0 when the reset is done */
    while (gpt->gpt_map->gptcr & BIT(SWR));
    /* GPT can do more but for this just set it as free running  so we can tell the time */
    gptcr = BIT(FRR) | BIT(ENMOD);

#ifdef CONFIG_PLAT_IMX7
    /* eanble the 24MHz source and select the oscillator as CLKSRC */
    gptcr |= (BIT(EN_24M) | (5u << CLKSRC));
#else
    gptcr |= BIT(CLKSRC);
#endif

    gpt->gpt_map->gptcr = gptcr;
    gpt->gpt_map->gptir = BIT(ROV); /* Interrupt when the timer overflows */

    /* The prescaler register has two parts when the 24 MHz clocksource is used.
     * The 24MHz crystal clock is devided by the (the top 15-12 bits + 1) before
     * it is fed to the CLKSRC field.
     * The clock selected by the CLKSRC is divided by the (the 11-0 bits + ) again.
     * For unknown reason, when the prescaler for the 24MHz clock is set to zero, which
     * is valid according to the manual, the GPTCNT register does not work. So we
     * set the value at least to 1, using a 12MHz clocksource.
     */

#ifdef CONFIG_PLAT_IMX7
    gpt->gpt_map->gptpr = config.prescaler | (1u << 12);
#else
    gpt->gpt_map->gptpr = config.prescaler; /* Set the prescaler */
#endif

    gpt->high_bits = 0;

    return 0;
}

int gpt_destroy(gpt_t *gpt)
{
    if (gpt->gpt_map) {
        ZF_LOGF_IF(gpt_stop(gpt), "Failed to stop the GPT before de-allocating it");
        ps_io_unmap(&gpt->io_ops.io_mapper, (void *) gpt->gpt_map, (size_t) gpt->timer_pmem.length);
    }

    if (gpt->irq_id != PS_INVALID_IRQ_ID) {
        ZF_LOGF_IF(ps_irq_unregister(&gpt->io_ops.irq_ops, gpt->irq_id), "Failed to unregister IRQ");
    }

    return 0;
}

int gpt_set_timeout(gpt_t  *gpt, uint64_t ns, bool periodic)
{
    uint32_t gptcr = 0;
    uint64_t counter_value = (uint64_t)(GPT_FREQ / (gpt->prescaler + 1)) * (ns / 1000ULL);
    if (counter_value >= (1ULL << 32)) {
        /* Counter too large to be stored in 32 bits. */
        ZF_LOGW("ns too high %llu, going to be capping it\n", ns);
        counter_value = UINT32_MAX;
    }

    gpt->gpt_map->gptcr = 0;
    gpt->gpt_map->gptsr = GPT_STATUS_REGISTER_CLEAR;
    gpt->gpt_map->gptcr = BIT(SWR);
    while (gpt->gpt_map->gptcr & BIT(SWR));
    gptcr = (periodic ? 0 : BIT(FRR));

#ifdef CONFIG_PLAT_IMX7
    gptcr |= BIT(EN_24M) | (5u << CLKSRC);
#else
    gptcr |= BIT(CLKSRC);
#endif

    gpt->gpt_map->gptcr = gptcr;
    gpt->gpt_map->gptcr1 = (uint32_t)counter_value;
    while (gpt->gpt_map->gptcr1 != counter_value) {
        gpt->gpt_map->gptcr1 = (uint32_t)counter_value;
    }

#ifdef CONFIG_PLAT_IMX7
    gpt->gpt_map->gptpr = gpt->prescaler | BIT(12);
#else
    gpt->gpt_map->gptpr = gpt->prescaler; /* Set the prescaler */
#endif

    gpt->gpt_map->gptir = 1;
    gpt->gpt_map->gptcr |= BIT(EN);

    return 0;
}
