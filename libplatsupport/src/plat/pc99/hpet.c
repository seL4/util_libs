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

#include <autoconf.h>
#include <errno.h>

#include <platsupport/timer.h>
#include <platsupport/plat/hpet.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <utils/attribute.h>
#include <utils/util.h>
#include <utils/fence.h>

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

    /* Timer state */
    double period_ns;
    uint64_t period;
    uint32_t periodic;
    uint32_t irq;
} hpet_t;

/* HPET timer config bits - these can't be changed, but allow us to
 * find out details of the timer */

enum {
    /* 0 is reserved */
    /* 0 if edge triggered, 1 if level triggered. */
    TN_INT_TYPE_CNF = 1,
    /* Set to 1 to cause an interrupt when main timer hits comparator for this timer */
    TN_INT_ENB_CNF = 2,
    /* If this bit is 1 you can write a 1 to it for periodic interrupts,
     * or a 0 for non-periodic interrupts */
    TN_TYPE_CNF = 3,
    /* If this bit is 1, hardware supports periodic mode for this timer */
    TN_PER_INT_CAP = 4,
    /* 1 = timer is 64 bit, 0 = timer is 32 bit */
    TN_SIZE_CAP = 5,
    /* Writing 1 to this bit allows software to directly set a periodic timers accumulator */
    TN_VAL_SET_CNF = 6,
    /* 7 is reserved */
    /* Set this bit to force the timer to be a 32-bit timer (only works on a 64-bit timer) */
    TN_32MODE_CNF = 8,
    /* 5 bit wide field (9:13). Specifies routing for IO APIC if using */
    TN_INT_ROUTE_CNF = 9,
    /* Set this bit to force interrupt delivery to the front side bus, don't use the IO APIC */
    TN_FSB_EN_CNF = 14,
    /* If this bit is one, bit TN_FSB_EN_CNF can be set */
    TN_FSB_INT_DEL_CAP = 15,
    /* Bits 16:31 are reserved */
    /* Read-only 32-bit field that specifies which routes in the IO APIC this timer can be configured
       to take */
    TN_INT_ROUTE_CAP = 32
};

/* General HPET config bits */
enum {
    /* 1 if main counter is running and interrupts are enabled */
    ENABLE_CNF = 0,
    /* 1 if LegacyReplacementRoute is being used */
    LEG_RT_CNF = 1
};

/* MSI registers - used to configure front side bus delivery of the
 * HPET interrupt. This allows us to avoid writing an I/O APIC driver.
 *
 * For details see section 10.10 "APIC message passing mechanism
 * and protocol (P6 family,pentium processors)" in "Intel 64 and IA-32
 * Architectures Software Developers Manual, Volume 3 (3A, 3B & 3C),
 * System Programming Guide" */
/* Message value register layout */
enum {
    /* 0:7 irq_vector */
    IRQ_VECTOR = 0,
    /* 8:10 */
    DELIVERY_MODE = 8,
    /* 11:13 reserved */
    LEVEL_TRIGGER = 14,
    TRIGGER_MODE = 15,
    /* 16:32 reserved */
};

/* Message address register layout */
enum {
    /* 0:1 reserved */
    DESTINATION_MODE = 2,
    REDIRECTION_HINT = 3,
    /* 4:11 reserved */
    /* 12:19 Destination ID */
    DESTINATION_ID = 12,
    /* 20:31 Fixed value 0x0FEE */
    FIXED = 20
};

#define CAP_ID_REG 0x0
#define GENERAL_CONFIG_REG 0x10
#define MAIN_COUNTER_REG 0xF0
#define TIMERS_OFFSET 0x100

static inline uint64_t *hpet_get_general_config(void *vaddr)
{
    return (uint64_t*)((uintptr_t)vaddr + GENERAL_CONFIG_REG);
}

static inline uint64_t *hpet_get_main_counter(void *vaddr)
{
    return (uint64_t*)((uintptr_t)vaddr + MAIN_COUNTER_REG);
}

static inline uint64_t *hpet_get_cap_id(void *vaddr)
{
    return (uint64_t*)((uintptr_t)vaddr + CAP_ID_REG);
}

static inline hpet_timer_t *hpet_get_hpet_timer(void *vaddr, unsigned int timer)
{
    return ((hpet_timer_t*)((uintptr_t)vaddr + TIMERS_OFFSET)) + timer;
}

static int
hpet_start(const pstimer_t* device)
{

    hpet_t *hpet = (hpet_t *) device->data;
    hpet_timer_t *timer = hpet_get_hpet_timer(hpet->base_addr, 0);
    /* enable the global timer */
    *hpet_get_general_config(hpet->base_addr) |= BIT(ENABLE_CNF);

    /* make sure the comparator is 0 before we turn time0 on*/
    timer->comparator = 0llu;
    COMPILER_MEMORY_RELEASE();

    /* turn timer0 on */
    timer->config |= BIT(TN_INT_ENB_CNF);

    /* ensure the compiler sends the writes to the hardware */
    COMPILER_MEMORY_RELEASE();
    return 0;
}

static int
hpet_stop(const pstimer_t* device)
{
    hpet_t *hpet = (hpet_t *) device->data;

    hpet->periodic = false;
    hpet_timer_t *timer = hpet_get_hpet_timer(hpet->base_addr, 0);

    /* turn off timer0 */
    timer->config &= ~(BIT(TN_INT_ENB_CNF));

    /* turn the global timer off */
    *hpet_get_general_config(hpet->base_addr) &= ~BIT(ENABLE_CNF);

    /* ensure the compiler sends the writes to the hardware */
    COMPILER_MEMORY_RELEASE();
    return 0;
}

static uint64_t
hpet_get_time(const pstimer_t* device)
{
    uint64_t time;
    hpet_t *hpet = (hpet_t *) device->data;

    do {
        time = *hpet_get_main_counter(hpet->base_addr);
        COMPILER_MEMORY_ACQUIRE();
        /* race condition on 32-bit systems: check the bottom 32 bits didn't overflow */
    } while (CONFIG_WORD_SIZE == 32 && ((uint32_t) (time >> 32llu)) != ((uint32_t *)hpet_get_main_counter(hpet->base_addr))[1]);

    return time * hpet->period_ns;
}

int
hpet_oneshot_absolute(const pstimer_t *device, uint64_t absolute_ns)
{
    hpet_t *hpet = (hpet_t *) device->data;
    hpet_timer_t *timer = hpet_get_hpet_timer(hpet->base_addr, 0);
    uint64_t absolute_fs = absolute_ns / hpet->period_ns;

    hpet->periodic = false;
    timer->comparator = absolute_fs;
    COMPILER_MEMORY_RELEASE();

    if (hpet_get_time(device) > absolute_ns) {
        return ETIME;
    }

    return 0;
}

static int
hpet_oneshot_relative(const pstimer_t* device, uint64_t relative_ns)
{
    return hpet_oneshot_absolute(device, hpet_get_time(device) + relative_ns);
}

static int
hpet_periodic(const pstimer_t* device, uint64_t ns)
{
    hpet_t *hpet = (hpet_t *) device->data;

    hpet->period = ns;

    int error = hpet_oneshot_relative(device, ns);
    if (error != 0) {
        hpet->periodic = false;
        return error;
    }

    hpet->periodic = true;

    return 0;
}

static void
hpet_handle_irq(const pstimer_t* device, uint32_t irq UNUSED)
{
    /* hpet does not need an ack, kernel acks in APIC for us */

    /* but if we are periodic, need to set the next irq */
    hpet_t *hpet = (hpet_t *) device->data;

    if (hpet->periodic) {
        int error = hpet_periodic(device, hpet->period);
        if (error != 0) {
            ZF_LOGE("Repeat periodic timeout failed. Period: %"PRIu64"", hpet->period);
            assert(error == 0);
        }
    }

}

static uint32_t
hpet_get_nth_irq(const pstimer_t *device, uint32_t n)
{
    uint32_t irq = 0;

    if (n == 0) {
        irq = ((hpet_t *) device)->irq;
    }

    assert(n == 0);
    return irq;
}

static pstimer_t singleton_timer;
static hpet_t singleton_hpet;

bool
hpet_supports_fsb_delivery(void *vaddr)
{
    hpet_timer_t *timer0 = hpet_get_hpet_timer(vaddr, 0);
    uint32_t timer0_config_low = timer0->config;
    return !!(timer0_config_low & BIT(TN_FSB_INT_DEL_CAP));
}

uint32_t
hpet_ioapic_irq_delivery_mask(void *vaddr)
{
    hpet_timer_t *timer0 = hpet_get_hpet_timer(vaddr, 0);
    uint32_t irq_mask = timer0->config >> TN_INT_ROUTE_CAP;
    return irq_mask;
}

pstimer_t *
hpet_get_timer(hpet_config_t *config)
{

    pstimer_t *timer = &singleton_timer;
    hpet_t *hpet = &singleton_hpet;

    timer->data = (void *) hpet;
    timer->start = hpet_start;
    timer->stop = hpet_stop;
    timer->get_time = hpet_get_time;
    timer->oneshot_absolute = hpet_oneshot_absolute;
    timer->oneshot_relative = hpet_oneshot_relative;
    timer->periodic = hpet_periodic;
    timer->handle_irq = hpet_handle_irq;
    timer->get_nth_irq = hpet_get_nth_irq;

    timer->properties = (timer_properties_t) {
        .upcounter = true,
        .timeouts = true,
        .periodic_timeouts = true,
        .relative_timeouts = true,
        .absolute_timeouts = true,
        .bit_width = 64,
        .irqs = 1
    };

    hpet->base_addr = config->vaddr;
    hpet_timer_t *hpet_timer = hpet_get_hpet_timer(hpet->base_addr, 0);

    uint32_t timer0_config_low = (uint32_t) hpet_timer->config;

    /* check that this timer is edge triggered */
    if (timer0_config_low & BIT(TN_INT_TYPE_CNF)) {
        ZF_LOGE("This driver expects the timer to be edge triggered");
        return NULL;
    }

    /* check that this timer is 64 bit */
    if (!(timer0_config_low & BIT(TN_SIZE_CAP))) {
        ZF_LOGE("This driver expects hpet timer0 to be 64bit");
        return NULL;
    }

    hpet->irq = config->irq;

    if (config->ioapic_delivery) {
        /* Check if this IO/APIC offset is valid */
        uint32_t irq_mask = hpet_timer->config >> TN_INT_ROUTE_CAP;
        if (!(BIT(config->irq) & irq_mask)) {
            ZF_LOGE("IRQ %d not in the support mask 0x%x", config->irq, irq_mask);
            return NULL;
        }
        /* Remove any legacy replacement route so our interrupts go where we want them
         * NOTE: PIT will cease to function from here on */
        *hpet_get_general_config(hpet->base_addr) &= ~BIT(LEG_RT_CNF);
        /* Make sure we're not deliverying by MSI */
        hpet_timer->config &= ~BIT(TN_FSB_EN_CNF);
        /* Put the IO/APIC offset in (this is called an irq, but in reality it is
         * an index into whichever IO/APIC the HPET delivers to */
        hpet_timer->config &= ~(MASK(5) << TN_INT_ROUTE_CNF);
        hpet_timer->config |= config->irq << TN_INT_ROUTE_CNF;
    } else {
        /* check that this timer supports front size bus delivery */
        if (!(timer0_config_low & BIT(TN_FSB_INT_DEL_CAP))) {
            ZF_LOGE("Requested fsb delivery, but timer0 does not support");
            return NULL;
        }

        /* set timer 0 to delivery interrupts via the front side bus (using MSIs) */
        hpet_timer->config |= BIT(TN_FSB_EN_CNF);

        /* set up the message address register and message value register so we receive
         * MSIs for timer 0*/
        hpet_timer->fsb_irr =
            /* top 32 bits is the message address register */
            ((0x0FEEllu << FIXED) << 32llu)
            /* bottom 32 bits is the message value register */
            | config->irq;
    }
    COMPILER_MEMORY_RELEASE();

    /* read the period of the timer (its in femptoseconds) and calculate no of ticks per ns */
    uint32_t tick_period_fs = (uint32_t) (*hpet_get_cap_id(hpet->base_addr) >> 32llu);
    hpet->period_ns = tick_period_fs / 1000000.0f;

    return timer;
}
