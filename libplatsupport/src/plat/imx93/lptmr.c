/*
 * Copyright 2024, Indan Zupancic
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Low Power Timer driver.
 * The i.MX93 has two of them, LPTMR1 and LPTMR2.
 */
#include <platsupport/gen_config.h>
#include <platsupport/io.h>
#include <platsupport/irq.h>
#include <platsupport/ltimer.h>
#include <utils/util.h>

/* Use Arm Generic Timer if available: */
#if !(defined(CONFIG_EXPORT_PCNT_USER) && defined(CONFIG_EXPORT_PTMR_USER))

#define LPTMR1_ADDR 0x44300000
#define LPTMR2_ADDR 0x424D0000

#define LPTMR1_IRQ  (32 + 18)
#define LPTMR2_IRQ  (32 + 67)

#define LPTMR_CSR_TCF   (1u << 7) // Timer Compare Flag (W1C)
#define LPTMR_CSR_TIE   (1u << 6) // Timer Interrupt Enable
#define LPTMR_CSR_TFC   (1u << 2) // Timer Free-running Counter
#define LPTMR_CSR_TMS   (1u << 1) // Timer Mode Selection
#define LPTMR_CSR_TEN   (1u << 0) // Timer Enable

#define LPTMR_PSR_PRESCALE(n) ((((n) - 1) & 0xf) << 3) // Prescale value in bits
#define LPTMR_PSR_PBYP (1u << 2) // Prescaler and Glitch Filter Bypass
/* Prescaler and Glitch Filter Clock Select */
#define LPTMR_PSR_PCS_32kHz 2 // ipg_clk_32kHz: Fixed 32.768 kHz
#define LPTMR_PSR_PCS_24MHz 0 // ipg_clk_irclk: From CCM

#ifdef CONFIG_LIB_PLAT_SUPPORT_LPTMR_CLOCK_32kHZ
#define LPTMR_PSR       (LPTMR_PSR_PCS_32kHz | LPTMR_PSR_PBYP)
#define FREQ            32768
#elif defined(CONFIG_LIB_PLAT_SUPPORT_LPTMR_CLOCK_24MHz)
#define LPTMR_PSR       (LPTMR_PSR_PCS_24MHz | LPTMR_PSR_PRESCALE(7))
#define FREQ            (24000000 / 128)
#else
#error "LPTMR: Unknown clock source"
#endif

#define NS_PER_COUNT    (NS_IN_S / FREQ)

struct lptmr_regs {
    uint32_t csr; // Control Status
    uint32_t psr; // Prescaler and Glitch Filter
    uint32_t cmr; // Compare
    uint32_t cnr; // Counter
};

struct lptmr {
    volatile struct lptmr_regs *clock_reg;
    volatile struct lptmr_regs *timeout_reg;
    uint64_t last_count;
    uint64_t high_bits;

    /* Current timer info */
    bool periodic;

    ltimer_callback_fn_t cb_fn;
    void *cb_token;

    ps_io_ops_t ops;
    irq_id_t irq_id[2];
};

static void init(volatile struct lptmr_regs *reg, uint32_t compare)
{
    /* Disable timer before configuring it: */
    reg->csr = 0;
    reg->psr = LPTMR_PSR;
    reg->cmr = compare;
    reg->csr = LPTMR_CSR_TIE | LPTMR_CSR_TFC | LPTMR_CSR_TCF;
    /* Do not alter CSR[5:1] when setting TEN: */
    reg->csr |= LPTMR_CSR_TEN;
}

static int reset(void *data)
{
    struct lptmr *t = data;

    /* IRQ comes one clock later after a match: */
    init(t->clock_reg, ~0);
    /* Disable timeout timer, ack any pending IRQ */
    t->timeout_reg->csr = LPTMR_CSR_TCF;

    t->high_bits = 0;
    t->last_count = 0;
    t->periodic = false;
    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    struct lptmr *t = data;
    uint64_t v, s;
    uint32_t count;

    /* Write CNR to sync count value: */
    t->clock_reg->cnr = 0;
    /* Now read the counter value: */
    count = t->clock_reg->cnr;

    v = t->high_bits | count;

    /* Handle overflow */
    if (v < t->last_count) {
        v += 1ull << 32;
    }
    t->last_count = v;

    /* Convert seconds exactly: */
    s = v / FREQ;
    v = v % FREQ;
    *time = s * NS_IN_S + v * NS_PER_COUNT;
    return 0;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    struct lptmr *t = data;
    uint64_t now = 0;
    uint64_t v, s;

    switch (type) {
    case TIMEOUT_PERIODIC:
        t->periodic = true;
        break;
    case TIMEOUT_ABSOLUTE:
        t->periodic = false;
        get_time(t, &now);
        if (now <= ns) {
            ns -= now;
        } else {
            ns = 0;
        }
        break;
    case TIMEOUT_RELATIVE:
        t->periodic = false;
        break;
    default:
        return EINVAL;
    }
    /* Convert seconds exactly: */
    s = ns / NS_IN_S;
    ns = ns % NS_IN_S;
    /* Convert to counts and round up: */
    v = s * FREQ + (ns + NS_PER_COUNT - 1) / NS_PER_COUNT;
    if (v > UINT32_MAX) {
        return EINVAL;
    }
    init(t->timeout_reg, v);
    return 0;
}

static void clock_irq(void *data, ps_irq_acknowledge_fn_t ack_fn, void *ack_data)
{
    struct lptmr *t = data;

    /* Ack the IRQ */
    t->clock_reg->csr |= LPTMR_CSR_TCF;

    /* Increment high bits only here: */
    t->high_bits += 1ull << 32;

    /* Ack the IRQ at CPU level */
    ack_fn(ack_data);
}

static void timeout_irq(void *data, ps_irq_acknowledge_fn_t ack_fn, void *ack_data)
{
    struct lptmr *t = data;

    if (t->periodic) {
        /* Ack the IRQ at device level */
        t->timeout_reg->csr |= LPTMR_CSR_TCF;
    } else {
        /* Stop timer */
        t->timeout_reg->csr = LPTMR_CSR_TCF;
    }
    /* Ack the IRQ at CPU level */
    ack_fn(ack_data);

    /* Call user callback: */
    if (t->cb_fn) {
        t->cb_fn(t->cb_token, LTIMER_TIMEOUT_EVENT);
    }
}

static void destroy(void *data)
{
    struct lptmr *t = data;

    t->clock_reg->csr = 0;
    t->timeout_reg->csr = 0;
    ps_io_unmap(&t->ops.io_mapper, (void *)t->clock_reg, 4096);
    ps_io_unmap(&t->ops.io_mapper, (void *)t->timeout_reg, 4096);
    for (int i = 0; i < 2; ++i) {
        ps_irq_unregister(&t->ops.irq_ops, t->irq_id[i]);
    }
    ps_free(&t->ops.malloc_ops, sizeof(*t), (void **)&t);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t cb_fn, void *cb_token)
{
    struct lptmr *t;
    int error;
    pmem_region_t pmem = {.type = PMEM_TYPE_DEVICE, .length = 4096};
    ps_irq_t irq = {.type = PS_INTERRUPT};

    error = ps_calloc(&ops.malloc_ops, 1, sizeof(*t), (void **)&t);
    if (error) {
        return error;
    }
    pmem.base_addr = LPTMR1_ADDR;
    t->clock_reg = ps_pmem_map(&ops, pmem, false, PS_MEM_NORMAL);
    if (!t->clock_reg) {
        return EIO;
    }
    pmem.base_addr = LPTMR2_ADDR;
    t->timeout_reg = ps_pmem_map(&ops, pmem, false, PS_MEM_NORMAL);
    if (!t->timeout_reg) {
        return EIO;
    }
    irq.irq.number = LPTMR1_IRQ;
    t->irq_id[0] = ps_irq_register(&ops.irq_ops, irq, clock_irq, t);
    if (t->irq_id[0] < 0) {
        return EIO;
    }
    irq.irq.number = LPTMR2_IRQ;
    t->irq_id[1] = ps_irq_register(&ops.irq_ops, irq, timeout_irq, t);
    if (t->irq_id[1] < 0) {
        return EIO;
    }
    t->ops = ops;
    t->cb_fn = cb_fn;
    t->cb_token = cb_token;
    ltimer->get_time = get_time;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;
    ltimer->data = t;

    reset(t);
    return 0;
}

#endif // !Arm Generic Timer
