/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <platsupport/gen_config.h>
#include <errno.h>

#include <platsupport/timer.h>
#include <platsupport/plat/hpet.h>
#include <platsupport/plat/acpi/acpi.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <utils/attribute.h>
#include <utils/util.h>
#include <utils/fence.h>

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
    return (uint64_t *)((uintptr_t)vaddr + GENERAL_CONFIG_REG);
}

static inline uint64_t *hpet_get_main_counter(void *vaddr)
{
    return (uint64_t *)((uintptr_t)vaddr + MAIN_COUNTER_REG);
}

static inline uint64_t *hpet_get_cap_id(void *vaddr)
{
    return (uint64_t *)((uintptr_t)vaddr + CAP_ID_REG);
}

static inline hpet_timer_t *hpet_get_hpet_timer(void *vaddr, unsigned int timer)
{
    return ((hpet_timer_t *)((uintptr_t)vaddr + TIMERS_OFFSET)) + timer;
}

int hpet_start(const hpet_t *hpet)
{

    hpet_timer_t *timer = hpet_get_hpet_timer(hpet->base_addr, 0);
    /* enable the global timer */
    /* volatile is used here to try and prevent the compiler from satisfying this
       bitwise operation via byte only reads and writes. */
    volatile uint64_t *general_config = hpet_get_general_config(hpet->base_addr);
    *general_config |= BIT(ENABLE_CNF);

    /* make sure the comparator is 0 before we turn time0 on*/
    timer->comparator = 0llu;
    COMPILER_MEMORY_RELEASE();

    /* turn timer0 on */
    timer->config |= BIT(TN_INT_ENB_CNF);

    /* ensure the compiler sends the writes to the hardware */
    COMPILER_MEMORY_RELEASE();
    return 0;
}

int hpet_stop(const hpet_t *hpet)
{
    hpet_timer_t *timer = hpet_get_hpet_timer(hpet->base_addr, 0);

    /* turn off timer0 */
    timer->config &= ~(BIT(TN_INT_ENB_CNF));

    /* turn the global timer off */
    *hpet_get_general_config(hpet->base_addr) &= ~BIT(ENABLE_CNF);

    /* ensure the compiler sends the writes to the hardware */
    COMPILER_MEMORY_RELEASE();
    return 0;
}

uint64_t hpet_get_time(const hpet_t *hpet)
{
    uint64_t time;

    do {
        time = *hpet_get_main_counter(hpet->base_addr);
        COMPILER_MEMORY_ACQUIRE();
        /* race condition on 32-bit systems: check the bottom 32 bits didn't overflow */
    } while (CONFIG_WORD_SIZE == 32
             && ((uint32_t)(time >> 32llu)) != ((uint32_t *)hpet_get_main_counter(hpet->base_addr))[1]);

    return time * hpet->period_ns;
}

int hpet_set_timeout(const hpet_t *hpet, uint64_t absolute_ns)
{
    hpet_timer_t *timer = hpet_get_hpet_timer(hpet->base_addr, 0);
    uint64_t absolute_fs = absolute_ns / hpet->period_ns;

    timer->comparator = absolute_fs;
    COMPILER_MEMORY_RELEASE();

    if (hpet_get_time(hpet) > absolute_ns) {
        return ETIME;
    }

    return 0;
}

bool hpet_supports_fsb_delivery(void *vaddr)
{
    hpet_timer_t *timer0 = hpet_get_hpet_timer(vaddr, 0);
    uint32_t timer0_config_low = timer0->config;
    return !!(timer0_config_low & BIT(TN_FSB_INT_DEL_CAP));
}

uint32_t hpet_ioapic_irq_delivery_mask(void *vaddr)
{
    hpet_timer_t *timer0 = hpet_get_hpet_timer(vaddr, 0);
    uint32_t irq_mask = timer0->config >> TN_INT_ROUTE_CAP;
    return irq_mask;
}

uint32_t hpet_level(void *vaddr)
{
    hpet_timer_t *timer0 = hpet_get_hpet_timer(vaddr, 0);
    return timer0->config & BIT(TN_INT_TYPE_CNF);
}

int hpet_init(hpet_t *hpet, hpet_config_t config)
{
    hpet->base_addr = config.vaddr;
    hpet_timer_t *hpet_timer = hpet_get_hpet_timer(hpet->base_addr, 0);

    uint32_t timer0_config_low = (uint32_t) hpet_timer->config;

    /* check that this timer is edge triggered */
    if (timer0_config_low & BIT(TN_INT_TYPE_CNF)) {
        ZF_LOGE("This driver expects the timer to be edge triggered");
        return -1;
    }

    /* check that this timer is 64 bit */
    if (!(timer0_config_low & BIT(TN_SIZE_CAP))) {
        ZF_LOGE("This driver expects hpet timer0 to be 64bit");
        return -1;
    }

    if (config.ioapic_delivery) {
        /* Check if this IO/APIC offset is valid */
        uint32_t irq_mask = hpet_timer->config >> TN_INT_ROUTE_CAP;
        if (!(BIT(config.irq) & irq_mask)) {
            ZF_LOGE("IRQ %d not in the support mask 0x%x", config.irq, irq_mask);
            return -1;
        }
        /* Remove any legacy replacement route so our interrupts go where we want them
         * NOTE: PIT will cease to function from here on */
        *hpet_get_general_config(hpet->base_addr) &= ~BIT(LEG_RT_CNF);
        /* Make sure we're not deliverying by MSI */
        hpet_timer->config &= ~BIT(TN_FSB_EN_CNF);
        /* Put the IO/APIC offset in (this is called an irq, but in reality it is
         * an index into whichever IO/APIC the HPET delivers to */
        hpet_timer->config &= ~(MASK(5) << TN_INT_ROUTE_CNF);
        hpet_timer->config |= config.irq << TN_INT_ROUTE_CNF;
    } else {
        /* check that this timer supports front size bus delivery */
        if (!(timer0_config_low & BIT(TN_FSB_INT_DEL_CAP))) {
            ZF_LOGE("Requested fsb delivery, but timer0 does not support");
            return ENOSYS;
        }

        /* set timer 0 to delivery interrupts via the front side bus (using MSIs) */
        hpet_timer->config |= BIT(TN_FSB_EN_CNF);

        /* set up the message address register and message value register so we receive
         * MSIs for timer 0*/
        hpet_timer->fsb_irr =
            /* top 32 bits is the message address register */
            ((0x0FEEllu << FIXED) << 32llu)
            /* bottom 32 bits is the message value register */
            | config.irq;
    }
    COMPILER_MEMORY_RELEASE();

    /* read the period of the timer (its in femptoseconds) and calculate no of ticks per ns */
    uint32_t tick_period_fs = (uint32_t)(*hpet_get_cap_id(hpet->base_addr) >> 32llu);
    hpet->period_ns = tick_period_fs / 1000000;

    return 0;
}

int hpet_parse_acpi(acpi_t *acpi, pmem_region_t *region)
{
    if (!acpi || !region) {
        ZF_LOGE("arguments cannot be NULL");
        return EINVAL;
    }

    acpi_hpet_t *header = (acpi_hpet_t *) acpi_find_region(acpi, ACPI_HPET);
    if (header == NULL) {
        ZF_LOGE("Could not find HPET ACPI header");
        return ENOSYS;
    }

    region->base_addr = header->base_address.address;
    region->length = header->header.length;
    return 0;
}
