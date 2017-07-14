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

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>

#include <platsupport/timer.h>
#include <platsupport/mach/gpt.h>

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

typedef struct gpt {
    volatile struct gpt_map *gpt_map;
    uint64_t counter_start;
    uint32_t irq;
    gpt_id_t id;
    uint32_t prescaler;
    uint32_t high_bits;
} gpt_t;

/* common timer interface functions */
int gpt_timer_start(const pstimer_t *timer);
int gpt_timer_stop(const pstimer_t *timer);
void gpt_handle_irq(const pstimer_t *timer, uint32_t irq);
uint32_t gpt_get_nth_irq(const pstimer_t *timer, uint32_t n);

/* common helpers */
bool gpt_ok_prescaler(uint32_t prescaler);
timer_properties_t gpt_common_properties(void);
uint64_t gpt_ticks_to_ns(uint64_t ns);
uint64_t gpt_ns_to_ticks(uint64_t ticks);
/* do a soft reset such that this gpt is ready to go, set prescaler */
void gpt_init(gpt_t *gpt);
