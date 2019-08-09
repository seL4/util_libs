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
/* Implementation of a logical timer for pc99 platforms
 *
 * We try to use the HPET, but if that doesn't work we use the PIT.
 */
#include <platsupport/plat/timer.h>
#include <platsupport/arch/tsc.h>
#include <platsupport/pmem.h>
#include <utils/util.h>
#include <platsupport/plat/acpi/acpi.h>
#include <platsupport/plat/hpet.h>

#include "../../ltimer.h"

/* This is duplicated from constants.h in libsel4 for the moment. Interrupt allocation
   shouldn't be happening here in this driver, until that is fixed this hack is needed */
#define IRQ_OFFSET (0x20 + 16)

typedef enum {
    HPET,
    PIT
} pc99_timer_t;

typedef struct {
    pc99_timer_t type;
    /* we are either using the HPET or the PIT */
    union {
        struct {
             hpet_t device;
             pmem_region_t region;
             uint64_t period;
             hpet_config_t config;
        } hpet;
        struct {
            pit_t device;
            uint32_t freq;
            /* the PIT can only set short timeouts - if we have
             * set intermediate irqs we track when the actual timeout is due here */
            uint64_t abs_time;
        } pit;
    };
    ps_irq_t irq;
    ps_io_ops_t ops;
    irq_id_t irq_id;
    timer_callback_data_t callback_data;
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
} pc99_ltimer_t;

static size_t get_num_irqs(void *data)
{
    assert(data != NULL);

    /* both PIT and HPET only have one irq */
    return 1;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{

    assert(data != NULL);
    assert(irq != NULL);
    assert(n == 0);

    pc99_ltimer_t *pc99_ltimer = data;
    *irq = pc99_ltimer->irq;
    return 0;
}

static size_t get_num_pmems(void *data)
{
    assert(data != NULL);
    pc99_ltimer_t *pc99_ltimer = data;

    return pc99_ltimer->type == HPET ? 1 : 0;
}

static int hpet_ltimer_get_nth_pmem(void *data, size_t n, pmem_region_t *pmem)
{
    assert(data != NULL);
    assert(pmem != NULL);
    assert(n == 0);

    pc99_ltimer_t *pc99_ltimer = data;
    *pmem = pc99_ltimer->hpet.region;
    return 0;
}

static int pit_ltimer_handle_irq(pc99_ltimer_t *pc99_ltimer, ps_irq_t *irq)
{
    if (!pc99_ltimer->pit.abs_time) {
        /* nothing to do */
        return 0;
    }

    uint64_t time = tsc_get_time(pc99_ltimer->pit.freq);
    if (time > pc99_ltimer->pit.abs_time) {
        /* we're done here */
        pc99_ltimer->pit.abs_time = 0;
        return 0;
    }

    /* otherwise need to set another irq */
    uint64_t ns = MIN(pc99_ltimer->pit.abs_time - time, PIT_MAX_NS);
    return pit_set_timeout(&pc99_ltimer->pit.device, ns, false);
}

static int hpet_ltimer_handle_irq(pc99_ltimer_t *pc99_ltimer, ps_irq_t *irq)
{
    /* our hpet driver doesn't do periodic timeouts, so emulate them here */
    if (pc99_ltimer->hpet.period > 0) {
        // try a few times to set a timeout. If we continuously get ETIME then we have
        // no choice but to panic as there is no meaningful error we can return here
        // that will allow the user to work out what happened and recover
        // The whole time we are doing this we are of course losing time as we have to keep on
        // retrying the timeout with a new notion of the current time, but there is nothing
        // better we can do with this interface
        int retries = 10;
        int error;
        do {
            error = hpet_set_timeout(&pc99_ltimer->hpet.device,
                hpet_get_time(&pc99_ltimer->hpet.device) + pc99_ltimer->hpet.period);
            retries--;
        } while (error == ETIME && retries > 0);
        if (error == ETIME) {
            ZF_LOGF("Failed to reprogram periodic timeout. Unable to continue");
        }
        if (error != 0) {
            ZF_LOGF("Unexpected error when reprogramming periodic timeout. Unable to continue.");
        }
    }
    return 0;
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    pc99_ltimer_t *pc99_ltimer = data;

    int error = pc99_ltimer->type == PIT ? pit_ltimer_handle_irq(pc99_ltimer, irq)
                                         : hpet_ltimer_handle_irq(pc99_ltimer, irq);
    if (error) {
        return error;
    }

    /* the only interrupts we get are from timeout interrupts */
    if (pc99_ltimer->user_callback) {
        pc99_ltimer->user_callback(pc99_ltimer->user_callback_token, LTIMER_TIMEOUT_EVENT);
    }
}

static int hpet_ltimer_get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    pc99_ltimer_t *pc99_ltimer = data;
    *time = hpet_get_time(&pc99_ltimer->hpet.device);
    return 0;
}

static int pit_ltimer_get_time(void *data, uint64_t *time)
{
    pc99_ltimer_t *pc99_ltimer = data;
    *time = tsc_get_time(pc99_ltimer->pit.freq);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int hpet_ltimer_set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    pc99_ltimer_t *pc99_ltimer = data;

    if (type == TIMEOUT_PERIODIC) {
        pc99_ltimer->hpet.period = ns;
    } else {
        pc99_ltimer->hpet.period = 0;
    }

    if (type != TIMEOUT_ABSOLUTE) {
        ns += hpet_get_time(&pc99_ltimer->hpet.device);
    }

    return hpet_set_timeout(&pc99_ltimer->hpet.device, ns);
}

static int pit_ltimer_set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    pc99_ltimer_t *pc99_ltimer = data;

    /* we are overriding any existing timeouts */
    pc99_ltimer->pit.abs_time = 0;

    uint64_t time = tsc_get_time(pc99_ltimer->pit.freq);
    switch (type) {
    case TIMEOUT_RELATIVE:
        if (ns > PIT_MAX_NS) {
            pc99_ltimer->pit.abs_time = ns + time;
            ns = PIT_MAX_NS;
        }
        break;
    case TIMEOUT_ABSOLUTE:
        if (ns <= time) {
            return ETIME;
        }
        pc99_ltimer->pit.abs_time = ns;
        ns = MIN(PIT_MAX_NS, ns - time);
        break;
    case TIMEOUT_PERIODIC:
        if (ns > PIT_MAX_NS) {
            ZF_LOGE("Periodic timeouts %u not implemented for PIT ltimer", (uint32_t) PIT_MAX_NS);
            return ENOSYS;
        }
        break;
    }

    int error = pit_set_timeout(&pc99_ltimer->pit.device, ns, type == TIMEOUT_PERIODIC);
    if (error == EINVAL && type == TIMEOUT_ABSOLUTE ) {
        /* we capped the value we set at the highest value for the PIT, however this
         * could still have been too small - in this case the absolute timeout has
         * already passed */
        return ETIME;
    }

    return error;
}

static int pit_ltimer_reset(void *data)
{
    assert(data != NULL);
    pc99_ltimer_t *pc99_ltimer = data;
    pit_cancel_timeout(&pc99_ltimer->pit.device);
    return 0;
}

static int hpet_ltimer_reset(void *data)
{
    assert(data != NULL);
    pc99_ltimer_t *pc99_ltimer = data;

    hpet_stop(&pc99_ltimer->hpet.device);
    hpet_start(&pc99_ltimer->hpet.device);
    pc99_ltimer->hpet.period = 0;
    return 0;
}

static void destroy(void *data)
{
    assert(data);

    pc99_ltimer_t *pc99_ltimer = data;

    if (pc99_ltimer->type == HPET && pc99_ltimer->hpet.config.vaddr) {
        hpet_stop(&pc99_ltimer->hpet.device);
        ps_pmem_unmap(&pc99_ltimer->ops, pc99_ltimer->hpet.region, pc99_ltimer->hpet.config.vaddr);
    } else {
        assert(pc99_ltimer->type == PIT);
        pit_cancel_timeout(&pc99_ltimer->pit.device);
    }

    if (pc99_ltimer->irq_id > PS_INVALID_IRQ_ID) {
        ZF_LOGF_IF(ps_irq_unregister(&pc99_ltimer->ops.irq_ops, pc99_ltimer->irq_id),
                   "Failed to clean-up the IRQ ID!");
    }

    ps_free(&pc99_ltimer->ops.malloc_ops, sizeof(pc99_ltimer), pc99_ltimer);
}

static inline int
ltimer_init_common(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    pc99_ltimer_t *pc99_ltimer = ltimer->data;
    pc99_ltimer->ops = ops;
    pc99_ltimer->user_callback = callback;
    pc99_ltimer->user_callback_token = callback_token;
    ltimer->destroy = destroy;

    /* setup the interrupts */
    pc99_ltimer->callback_data.ltimer = ltimer;
    pc99_ltimer->callback_data.irq = &pc99_ltimer->irq;
    pc99_ltimer->callback_data.irq_handler = handle_irq;
    pc99_ltimer->irq_id = ps_irq_register(&ops.irq_ops, pc99_ltimer->irq, handle_irq_wrapper,
                                          &pc99_ltimer->callback_data);
    if (pc99_ltimer->irq_id < 0) {
        return EIO;
    }

    return 0;
}

static int ltimer_hpet_init_internal(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback,
                                     void *callback_token)
{
    pc99_ltimer_t *pc99_ltimer = ltimer->data;

    int error = ltimer_init_common(ltimer, ops, callback, callback_token);
    if (error) {
        destroy(pc99_ltimer);
        return -1;
    }

    /* map in the paddr */
    pc99_ltimer->hpet.config.vaddr = ps_pmem_map(&ops, pc99_ltimer->hpet.region, false, PS_MEM_NORMAL);
    if (pc99_ltimer->hpet.config.vaddr == NULL) {
        destroy(pc99_ltimer);
        return -1;
    }

    ltimer->get_time = hpet_ltimer_get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = hpet_ltimer_set_timeout;
    ltimer->reset = hpet_ltimer_reset;

    /* check if we have requested the IRQ that collides with the PIT. This check is not
     * particularly robust, as the legacy PIT route does not *have* to live on pin 2
     * and to be more accurate we should check the ACPI tables instead, but that is
     * difficult to do here and we shall ignore as an unlikely case */
    if (pc99_ltimer->hpet.config.ioapic_delivery && pc99_ltimer->hpet.config.irq == 2) {
        /* put the PIT into a known state to disable it from counting and genering interrupts */
        pit_t temp_pit;
        /* the pit_init function declares that it may only be called once, we can only hope that
         * it hasn't been called before and carry on */
        error = pit_init(&temp_pit, ops.io_port_ops);
        if (!error) {
            error = pit_cancel_timeout(&temp_pit);
            if (error) {
                /* if we fail to operate on an initialized pit then assume nothing is sane and abort */
                ZF_LOGE("PIT command failed!");
                return error;
            } else {
                ZF_LOGI("Disabled PIT under belief it was using same interrupt as HPET, and "
                        "this driver does not support interrupt sharing.");
            }
        } else {
            ZF_LOGW("Could not ensure PIT was not counting on pin 2, you may get spurious interrupts");
        }
    }

    error = hpet_init(&pc99_ltimer->hpet.device, pc99_ltimer->hpet.config);
    if (!error) {
        error = hpet_start(&pc99_ltimer->hpet.device);
    }

    return error;
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int error = ltimer_default_describe(ltimer, ops);
    if (error) {
        return error;
    }

    pc99_ltimer_t *pc99_ltimer = ltimer->data;
    if (pc99_ltimer->type == PIT) {
        return ltimer_pit_init(ltimer, ops, callback, callback_token);
    } else {
        assert(pc99_ltimer->type == HPET);
        return ltimer_hpet_init_internal(ltimer, ops, callback, callback_token);
    }
}

int ltimer_hpet_init(ltimer_t *ltimer, ps_io_ops_t ops, ps_irq_t irq, pmem_region_t region,
                     ltimer_callback_fn_t callback, void *callback_token)
{
    int error = ltimer_hpet_describe(ltimer, ops, irq, region);
    if (error) {
        return error;
    }

    return ltimer_hpet_init_internal(ltimer, ops, callback, callback_token);
}

int ltimer_pit_init_freq(ltimer_t *ltimer, ps_io_ops_t ops, uint64_t freq, ltimer_callback_fn_t callback,
                         void *callback_token)
{
    int error = ltimer_pit_describe(ltimer, ops);
    if (error) {
        return error;
    }

    pc99_ltimer_t *pc99_ltimer = ltimer->data;

    error = ltimer_init_common(ltimer, ops, callback, callback_token);
    if (error) {
        destroy(pc99_ltimer);
        return error;
    }

    ltimer->get_time = pit_ltimer_get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = pit_ltimer_set_timeout;
    ltimer->reset = pit_ltimer_reset;
    pc99_ltimer->pit.freq = freq;
    return pit_init(&pc99_ltimer->pit.device, ops.io_port_ops);
}

int ltimer_pit_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int error = ltimer_pit_init_freq(ltimer, ops, 0, callback, callback_token);
    if (error) {
        return error;
    }

    /* now calculate the tsc freq */
    pc99_ltimer_t *pc99_ltimer = ltimer->data;
    pc99_ltimer->pit.freq = tsc_calculate_frequency_pit(&pc99_ltimer->pit.device);
    if (pc99_ltimer->pit.freq == 0) {
        ltimer_destroy(ltimer);
        return ENOSYS;
    }
    return 0;
}

uint32_t ltimer_pit_get_tsc_freq(ltimer_t *ltimer)
{
    pc99_ltimer_t *pc99_ltimer = ltimer->data;
    return pc99_ltimer->pit.freq;
}

int _ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops, acpi_t *acpi)
{
    pmem_region_t hpet_region;
    int error = hpet_parse_acpi(acpi, &hpet_region);

    if (!error) {
        ps_irq_t irq;
        error = ltimer_hpet_describe_with_region(ltimer, ops, hpet_region, &irq);
    }

    if (error) {
        /* HPET failed - use the pit */
        error = ltimer_pit_describe(ltimer, ops);
    }

    return error;
}

int ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    acpi_t *acpi = acpi_init(ops.io_mapper);
    return _ltimer_default_describe(ltimer, ops, acpi);
}

int ltimer_default_describe_with_rsdp(ltimer_t *ltimer, ps_io_ops_t ops, acpi_rsdp_t rsdp)
{
    acpi_t *acpi = acpi_init_with_rsdp(ops.io_mapper, rsdp);
    return _ltimer_default_describe(ltimer, ops, acpi);
}

int ltimer_hpet_describe_with_region(ltimer_t *ltimer, ps_io_ops_t ops, pmem_region_t region, ps_irq_t *irq)
{
    /* try to map the HPET to query its properties */
    void *vaddr = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
    if (vaddr == NULL) {
        return ENOSYS;
    }

    /* first try to use MSIs */
    if (hpet_supports_fsb_delivery(vaddr)) {
        irq->type = PS_MSI;
        irq->msi.pci_bus = 0;
        irq->msi.pci_dev = 0;
        irq->msi.pci_func = 0;
        irq->msi.handle = 0;
        irq->msi.vector = DEFAULT_HPET_MSI_VECTOR;
    } else {
     /* try a IOAPIC */
        irq->type = PS_IOAPIC;
        irq->ioapic.pin = FFS(hpet_ioapic_irq_delivery_mask(vaddr)) - 1;
        irq->ioapic.level = hpet_level(vaddr);
        /* HPET is always active high polarity */
        irq->ioapic.polarity = 1;
        /* HPET always delivers to the first I/O APIC */
        irq->ioapic.ioapic = 0;
        irq->ioapic.vector = 0; /* TODO how to work this out properly */
    }

    ps_pmem_unmap(&ops, region, vaddr);
    return ltimer_hpet_describe(ltimer, ops, *irq, region);
}

int ltimer_pit_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(pc99_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }

    pc99_ltimer_t *pc99_ltimer = ltimer->data;
    pc99_ltimer->type = PIT;
    if (config_set(CONFIG_IRQ_IOAPIC)) {
        /* Use the IOAPIC if we can */
        pc99_ltimer->irq = (ps_irq_t) { .type = PS_IOAPIC, .ioapic = { .ioapic = 0, .pin = PIT_INTERRUPT,
                                                                       .level = 0, .polarity = 0,
                                                                       .vector = PIT_INTERRUPT }};
    } else {
        /* Default to the PIC */
        pc99_ltimer->irq = (ps_irq_t) { .type = PS_INTERRUPT, .irq = { .number = PIT_INTERRUPT }};
    }
    pc99_ltimer->irq_id = PS_INVALID_IRQ_ID;
    ltimer->get_num_irqs = get_num_irqs;
    ltimer->get_num_pmems = get_num_pmems;
    ltimer->get_nth_irq = get_nth_irq;
    return 0;
}

int ltimer_hpet_describe(ltimer_t *ltimer, ps_io_ops_t ops, ps_irq_t irq, pmem_region_t region)
{
    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(pc99_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }

    pc99_ltimer_t *pc99_ltimer = ltimer->data;
    pc99_ltimer->type = HPET;
    pc99_ltimer->irq_id = PS_INVALID_IRQ_ID;
    ltimer->get_num_irqs = get_num_irqs;
    ltimer->get_nth_irq = get_nth_irq;
    ltimer->get_num_pmems = get_num_pmems;
    ltimer->get_nth_pmem = hpet_ltimer_get_nth_pmem;

    pc99_ltimer->hpet.region = region;
    pc99_ltimer->hpet.config.irq = irq.type == PS_MSI ? irq.msi.vector + IRQ_OFFSET : irq.ioapic.pin;
    pc99_ltimer->hpet.config.ioapic_delivery = (irq.type == PS_IOAPIC);
    pc99_ltimer->irq = irq;

    return 0;
}
