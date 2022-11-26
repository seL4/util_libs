/*
 * Copyright 2021, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2021, Breakaway Consulting Pty. Ltd.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <platsupport/plat/gpt.h>
#include <platsupport/io.h>
#include <platsupport/irq.h>
#include <platsupport/fdt.h>

#include <utils/util.h>

#define CR_CLK_SRC_HIGH_FREQ (2 << 6)
#define CR_EN (1)
#define TICKS_PER_MICROSECOND 24
/* When no specific timeout is in place, interrupt at 1Hz */
#define DEFAULT_IRQ_PERIOD (TICKS_PER_MICROSECOND * 1000 * 1000)

#define MODULE_LABEL "tqma8xqp1gb.gpt: "
#define CLEANUP_FAIL_TEXT MODULE_LABEL "Failed to cleanup the GPT after failing to initialise it"

struct gpt_regs {
    uint32_t cr;
    uint32_t pr;
    uint32_t sr;
    uint32_t ir;
    uint32_t ocr1;
    uint32_t ocr2;
    uint32_t ocr3;
    uint32_t icr1;
    uint32_t icr2;
    uint32_t cnt;
};

static inline uint64_t ns_to_ticks(uint64_t ns)
{
    /* NOTE: this assumes that 'ns is less than 2 ** 64 / TICKS_PER_MICROSECOND
     * which seems reasonable given it would be a value of around 24 years.
     *
     * NOTE: this can return a value of more than 2 ** 32, the caller must check
     * that the returned tick value is actually usable in practise!
     */
    return (ns * TICKS_PER_MICROSECOND / 1000);
}

int gpt_get_time(gpt_t *gpt, uint64_t *time)
{
    assert(gpt != NULL);
    assert(time != NULL);

    /* returns the time in nanoseconds */
    uint64_t total_ticks = gpt->counted_ticks + gpt->regs->cnt;
    /* NOTE: This assume total ticks is never more than
     * 2 ** 64 / 1000. Which seems reasonable as that would
     * be an uptime of more than 24 years */
    uint64_t ns = total_ticks * 1000 / TICKS_PER_MICROSECOND;
    *time = ns;
    return 0;
}

int gpt_set_timeout(gpt_t *gpt, uint64_t ns, timeout_type_t type)
{
    uint64_t new_timeout;
    uint32_t extra_ticks;
    enum gpt_timeout_type timeout_type;

    assert(gpt != NULL);

    if (type == TIMEOUT_ABSOLUTE) {
        uint64_t now;
        gpt_get_time(gpt, &now);
        if (now > ns) {
            return EINVAL;
        }
        ns -= now;
        timeout_type = GPT_TIMEOUT_ONESHOT;
    } else if (type == TIMEOUT_PERIODIC) {
        timeout_type = GPT_TIMEOUT_PERIODIC;
    } else if (type == TIMEOUT_RELATIVE) {
        timeout_type = GPT_TIMEOUT_ONESHOT;
    } else {
        return EINVAL;
    }

    new_timeout = ns_to_ticks(ns);
    if (new_timeout > UINT32_MAX) {
        printf("returning einvali: new_timeout: %lx\n", new_timeout);
        return EINVAL;
    }
    extra_ticks = gpt->regs->cnt;
    gpt->regs->ocr1 = new_timeout;
    gpt->current_timeout = new_timeout;
    gpt->counted_ticks += extra_ticks;
    gpt->timeout_type = timeout_type;

    return 0;
}

int gpt_reset(gpt_t *gpt)
{
    assert(gpt != NULL);
    gpt->current_timeout = DEFAULT_IRQ_PERIOD;
    gpt->counted_ticks = 0;
    gpt->timeout_type = GPT_TIMEOUT_NONE;
    gpt->regs->ocr1 = gpt->current_timeout;
    return 0;
}

static int gpt_start(gpt_t *gpt)
{
    gpt->regs->pr = 0; /* no scaling */
    gpt->regs->ir = 1; /* interrupt zero enable */
    gpt->regs->ocr1 = gpt->current_timeout;
    gpt->regs->cr = CR_CLK_SRC_HIGH_FREQ | CR_EN;

    return 0;
}

static int gpt_stop(gpt_t *gpt)
{
    gpt->regs->cr = 0;
    return 0;
}

void gpt_destroy(gpt_t *gpt)
{
    assert(gpt != NULL);

    if (gpt->regs) {
        ZF_LOGF_IF(gpt_stop(gpt), MODULE_LABEL "failed to stop the GPT before de-allocating it");
        ps_io_unmap(&gpt->io_ops.io_mapper, (void *) gpt->regs, (size_t) gpt->pmem.length);
    }

    if (gpt->irq_id != PS_INVALID_IRQ_ID) {
        ZF_LOGF_IF(ps_irq_unregister(&gpt->io_ops.irq_ops, gpt->irq_id), MODULE_LABEL "failed to unregister IRQ");
    }

    ps_free(&gpt->io_ops.malloc_ops, sizeof * gpt, gpt);
}

static void handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    assert(data != NULL);
    gpt_t *gpt = data;

    /*
     * ack the interrupt by clearing the status register
     * Note: No need to read the status register; it can only
     * have the one output match bit set.
     *
     * Can then ack it with the interrupt controller
     */
    gpt->regs->sr = gpt->regs->sr;
    acknowledge_fn(ack_data);

    /* Add current_timeout */
    gpt->counted_ticks += gpt->current_timeout;

    if (gpt->timeout_type != GPT_TIMEOUT_NONE && gpt->user_callback) {
        gpt->user_callback(gpt->user_callback_token, LTIMER_TIMEOUT_EVENT);
    }

    if (gpt->timeout_type == GPT_TIMEOUT_ONESHOT) {
        /* got back to normal handling if we just have one-shot timer */
        uint32_t extra_ticks;
        extra_ticks = gpt->regs->cnt;
        gpt->regs->ocr1 = DEFAULT_IRQ_PERIOD;
        gpt->current_timeout = DEFAULT_IRQ_PERIOD;
        gpt->counted_ticks += extra_ticks;
        gpt->timeout_type = GPT_TIMEOUT_NONE;
    }
}

static int allocate_register_callback(pmem_region_t pmem, unsigned curr_num, size_t num_regs, void *token)
{
    assert(token != NULL);
    gpt_t *gpt = token;

    /* Should only be called once. I.e. only one register field */
    if (curr_num != 0) {
        ZF_LOGE(MODULE_LABEL "allocate_register_callback called with multiple set of registers");
        return EIO;
    }

    gpt->pmem = pmem;

    gpt->regs = ps_pmem_map(&gpt->io_ops, pmem, false, PS_MEM_NORMAL);
    if (!gpt->regs) {
        ZF_LOGE(MODULE_LABEL "failed to map in registers");
        return EIO;
    }

    return 0;
}

static int allocate_irq_callback(ps_irq_t irq, unsigned curr_num, size_t num_irqs, void *token)
{
    assert(token != NULL);
    gpt_t *gpt = token;

    /* Should only be called once. I.e. only one interrupt field */
    if (curr_num != 0) {
        ZF_LOGE(MODULE_LABEL "allocate_register_callback called with multiple set of registers");
        return EIO;
    }

    gpt->irq_id = ps_irq_register(&gpt->io_ops.irq_ops, irq, handle_irq, gpt);
    if (gpt->irq_id < 0) {
        ZF_LOGE(MODULE_LABEL "failed to register interrupt with the IRQ interface");
        return EIO;
    }

    return 0;
}

int gpt_init(gpt_t *gpt, const char *fdt_path, ps_io_ops_t ops, ltimer_callback_fn_t user_callback,
             void *user_callback_token)
{
    assert(gpt != NULL);

    ps_fdt_cookie_t *cookie = NULL;

    gpt->io_ops = ops;
    gpt->irq_id = PS_INVALID_IRQ_ID;
    gpt->user_callback = user_callback;
    gpt->user_callback_token = user_callback_token;

    int error = ps_fdt_read_path(&gpt->io_ops.io_fdt, &ops.malloc_ops, fdt_path, &cookie);
    if (error) {
        ZF_LOGE(MODULE_LABEL "failed to read path (%d, %s)", error, fdt_path);
        return ENODEV;
    }

    error = ps_fdt_walk_registers(&gpt->io_ops.io_fdt, cookie, allocate_register_callback, gpt);
    if (error) {
        ZF_LOGE(MODULE_LABEL "error allocating registers (%d, %s)", error, fdt_path);
        ZF_LOGF_IF(ps_fdt_cleanup_cookie(&gpt->io_ops.malloc_ops, cookie), CLEANUP_FAIL_TEXT);

        return ENODEV;
    }

    error = ps_fdt_walk_irqs(&gpt->io_ops.io_fdt, cookie, allocate_irq_callback, gpt);
    if (error) {
        ZF_LOGE(MODULE_LABEL "error allocating interrupts (%d, %s)", error, fdt_path);
        ZF_LOGF_IF(ps_fdt_cleanup_cookie(&ops.malloc_ops, cookie), CLEANUP_FAIL_TEXT);

        return ENODEV;
    }

    ZF_LOGF_IF(ps_fdt_cleanup_cookie(&gpt->io_ops.malloc_ops, cookie),
               MODULE_LABEL "failed to cleanup the FDT cookie after initialisation");

    gpt->counted_ticks = 0;
    gpt->current_timeout = DEFAULT_IRQ_PERIOD;
    gpt->timeout_type = GPT_TIMEOUT_NONE;

    gpt_start(gpt);

    return 0;
}
