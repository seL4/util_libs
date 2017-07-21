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

#include <platsupport/timer.h>

#define RTC0_PADDR 0xF8003000
#define RTC1_PADDR 0xF8004000

#define RTC0_INTERRUPT 44
#define RTC1_INTERRUPT 40

typedef enum rtc_id {
    RTC0,
    RTC1,
    NUM_RTCS = 2,
} rtc_id_t;

static inline void *rtc_get_paddr(rtc_id_t id) {
    switch (id) {
    case RTC0:
        return (void *) RTC0_PADDR;
    case RTC1:
        return (void *) RTC1_PADDR;
    default:
        return NULL;
    }
}

static inline long rtc_get_irq(rtc_id_t id) {
    switch (id) {
    case RTC0:
        return RTC0_INTERRUPT;
    case RTC1:
        return RTC1_INTERRUPT;
    default:
        return 0;
    }
}

typedef struct {
    int timer_id;
    void *vaddr;
    uint32_t irq;
} rtc_t;

typedef struct {
    void *vaddr;
    rtc_id_t id;
} rtc_config_t;

static UNUSED timer_properties_t rtc_props = {
    .upcounter = true,
    .timeouts = false,
    .bit_width = 32,
    .irqs = 0
};

int rtc_stop(rtc_t *rtc);
int rtc_start(rtc_t *rtc);
uint32_t rtc_get_time(rtc_t *rtc);
int rtc_init(rtc_t *rtc, rtc_config_t config);
