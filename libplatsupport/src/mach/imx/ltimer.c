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

/* Implementation of a logical timer for imx platforms
 *
 * We use the EPIT2 for timeouts and GPT for clock.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/mach/epit.h>
#include <platsupport/mach/gpt.h>

#include <utils/util.h>

typedef struct {
    gpt_t gpt;
    void *gpt_vaddr;
    epit_t epit;
    void *epit_vaddr;
    ps_io_ops_t ops;
} imx_ltimer_t;

static ps_irq_t imx_ltimer_irqs[] = {
    {
        .type = PS_INTERRUPT,
        .irq.number = GPT1_INTERRUPT,
    },
    {
        .type = PS_INTERRUPT,
        .irq.number = EPIT2_INTERRUPT
    }
};

static pmem_region_t imx_ltimer_paddrs[] = {
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = GPT1_DEVICE_PADDR,
        .length = PAGE_SIZE_4K
    },
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = EPIT2_DEVICE_PADDR,
        .length = PAGE_SIZE_4K
    }
};

#define N_IRQS ARRAY_SIZE(imx_ltimer_irqs)
#define N_PADDRS ARRAY_SIZE(imx_ltimer_paddrs)

static size_t get_num_irqs(void *data)
{
    return N_IRQS;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < N_IRQS);
    *irq = imx_ltimer_irqs[n];
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return N_PADDRS;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *paddr)
{
    *paddr = imx_ltimer_paddrs[n];
    return 0;
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    imx_ltimer_t *imx_ltimer = data;
    switch (irq->irq.number) {
        case GPT1_INTERRUPT:
            gpt_handle_irq(&imx_ltimer->gpt);
            break;
        case EPIT2_INTERRUPT:
            epit_handle_irq(&imx_ltimer->epit);
            break;
        default:
            ZF_LOGE("Unknown irq");
            return EINVAL;
    }

    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    imx_ltimer_t *imx_ltimer = data;
    *time = gpt_get_time(&imx_ltimer->gpt);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    imx_ltimer_t *imx_ltimer = data;

    if (type == TIMEOUT_ABSOLUTE) {
        uint64_t current_time = gpt_get_time(&imx_ltimer->gpt);
        ns -= current_time;
    }

    return epit_set_timeout(&imx_ltimer->epit, ns, type == TIMEOUT_PERIODIC);
}

static int reset(void *data)
{
    assert(data != NULL);
    imx_ltimer_t *imx_ltimer = data;

    /* reset the timers */
    gpt_stop(&imx_ltimer->gpt);
    gpt_start(&imx_ltimer->gpt);
    epit_stop(&imx_ltimer->epit);
    return 0;
}

static void destroy(void *data)
{
    assert(data);

    imx_ltimer_t *imx_ltimer = data;

    if (imx_ltimer->epit_vaddr) {
        epit_stop(&imx_ltimer->epit);
        ps_io_unmap(&imx_ltimer->ops.io_mapper, imx_ltimer->epit_vaddr, PAGE_SIZE_4K);
    }

    if (imx_ltimer->gpt_vaddr) {
        gpt_stop(&imx_ltimer->gpt);
        ps_io_unmap(&imx_ltimer->ops.io_mapper, imx_ltimer->gpt_vaddr, PAGE_SIZE_4K);
    }

    ps_free(&imx_ltimer->ops.malloc_ops, sizeof(imx_ltimer), imx_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops)
{

    int error = ltimer_default_describe(ltimer, ops);
    if (error) {
        return error;
    }

    ltimer->handle_irq = handle_irq;
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    error = ps_calloc(&ops.malloc_ops, 1, sizeof(imx_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    imx_ltimer_t *imx_ltimer = ltimer->data;

    /* map the frames we need */
    imx_ltimer->ops = ops;
    imx_ltimer->gpt_vaddr = ps_pmem_map(&ops, imx_ltimer_paddrs[0], false, PS_MEM_NORMAL);
    if (imx_ltimer->gpt_vaddr == NULL) {
        return -1;
    }

    imx_ltimer->epit_vaddr = ps_pmem_map(&ops, imx_ltimer_paddrs[1], false, PS_MEM_NORMAL);
    if (!imx_ltimer->epit_vaddr) {
        ltimer_destroy(ltimer);
        return -1;
    }

    /* setup gpt */
    gpt_config_t gpt_config = {
        .vaddr = imx_ltimer->gpt_vaddr,
        .prescaler = 1
    };

    /* intitialise gpt for getting the time */
    error = gpt_init(&imx_ltimer->gpt, gpt_config);
    if (error) {
        ZF_LOGE("Failed to init gpt");
        ltimer_destroy(ltimer);
        return error;
    }

    error = gpt_start(&imx_ltimer->gpt);
    if (error) {
        ZF_LOGE("Failed to start gpt");
        ltimer_destroy(ltimer);
        return error;
    }

    /* initialise epit for timeouts */
    epit_config_t config = {
        .vaddr = imx_ltimer->epit_vaddr,
        .irq = EPIT2_INTERRUPT,
        .prescaler = 0
    };

    error = epit_init(&imx_ltimer->epit, config);
    if (error != 0) {
        ltimer_destroy(ltimer);
        ZF_LOGE("Failed to init epit");
        return error;
    }

       /* success! */
    return 0;
}

int ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    if (ltimer == NULL) {
        ZF_LOGE("Timer is NULL!");
        return EINVAL;
    }

    ltimer->get_num_irqs = get_num_irqs;
    ltimer->get_nth_irq = get_nth_irq;
    ltimer->get_num_pmems = get_num_pmems;
    ltimer->get_nth_pmem = get_nth_pmem;
    return 0;
}
