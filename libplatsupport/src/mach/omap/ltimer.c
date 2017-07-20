/* Implementation of a logical timer for omap platforms
 *
 * We use two GPTS: one for the time and relative timeouts, the other
 * for absolute timeouts.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/mach/gpt.h>
#include <platsupport/pmem.h>
#include <utils/util.h>

#define ABS_GPT 0
#define REL_GPT 1
#define NUM_GPTS 2

typedef struct {
    gpt_t gpts[NUM_GPTS];
    void *vaddrs[NUM_GPTS];
    ps_io_ops_t ops;
} omap_ltimer_t;

static size_t get_num_irqs(void *data)
{
    return NUM_GPTS;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < NUM_GPTS);
    irq->type = PS_INTERRUPT,
    irq->irq.number = omap_get_gpt_irq(n);
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return NUM_GPTS;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *paddr)
{
    assert(n < NUM_GPTS);
    paddr->type = PMEM_TYPE_DEVICE;
    paddr->base_addr = (uint64_t) (uint32_t) omap_get_gpt_paddr(n);
    paddr->length = PAGE_SIZE_4K;
    return 0;
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    omap_ltimer_t *omap_ltimer = data;

    int i = 0;
    if (irq->irq.number == omap_get_gpt_irq(0)) {
        i = 0;
    } else if (irq->irq.number == omap_get_gpt_irq(1)) {
        i = 1;
    } else {
        ZF_LOGE("Unknown irq");
        return EINVAL;
    }

    gpt_handle_irq(&omap_ltimer->gpts[i]);
    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    omap_ltimer_t *omap_ltimer = data;
    *time = abs_gpt_get_time(&omap_ltimer->gpts[ABS_GPT]);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    omap_ltimer_t *omap_ltimer = data;

    switch (type) {
    case TIMEOUT_ABSOLUTE:
        return abs_gpt_set_timeout(&omap_ltimer->gpts[ABS_GPT], ns);
    case TIMEOUT_RELATIVE:
        return rel_gpt_set_timeout(&omap_ltimer->gpts[REL_GPT], ns, false);
    case TIMEOUT_PERIODIC:
        return rel_gpt_set_timeout(&omap_ltimer->gpts[REL_GPT], ns, true);
    }
}

static int reset(void *data)
{
    assert(data != NULL);
    omap_ltimer_t *omap_ltimer = data;

    /* reset the timers */
    for (int i = 0; i < NUM_GPTS; i++) {
        gpt_stop(&omap_ltimer->gpts[i]);
        gpt_start(&omap_ltimer->gpts[i]);
    }
    return 0;
}

static void destroy(void *data)
{
    assert(data);

    omap_ltimer_t *omap_ltimer = data;
    for (int i = 0; i < NUM_GPTS; i++) {
        gpt_stop(&omap_ltimer->gpts[i]);
        pmem_region_t region;
        get_nth_pmem(data, i, &region);
        ps_pmem_unmap(&omap_ltimer->ops, region, omap_ltimer->vaddrs[i]);
    }
    ps_free(&omap_ltimer->ops.malloc_ops, sizeof(omap_ltimer), omap_ltimer);
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

    error = ps_calloc(&ops.malloc_ops, 1, sizeof(omap_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    omap_ltimer_t *omap_ltimer = ltimer->data;
    omap_ltimer->ops = ops;

    /* map the frames we need */
    for (int i = 0; i < NUM_GPTS; i++) {
        pmem_region_t region;
        int error = get_nth_pmem(ltimer->data, i, &region);
        assert(error == 0);
        omap_ltimer->vaddrs[i] = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
        if (omap_ltimer->vaddrs[i] == NULL) {
            destroy(ltimer->data);
            return -1;
        }
    }

    /* setup gpt */
    gpt_config_t config = {
        .vaddr = omap_ltimer->vaddrs[ABS_GPT],
        .prescaler = 1,
        .id = GPT1
    };

    /* intitialise gpt for getting the time */
    error = abs_gpt_init(&omap_ltimer->gpts[ABS_GPT], config);
    if (error) {
        ZF_LOGE("Failed to init gpt");
        destroy(ltimer->data);
        return error;
    }

    /* initialise GPT for timeouts */
    config.vaddr = omap_ltimer->vaddrs[REL_GPT];
    config.id = GPT2;

    error = rel_gpt_init(&omap_ltimer->gpts[REL_GPT], config);
    if (error != 0) {
        destroy(ltimer->data);
        ZF_LOGE("Failed to init gpt");
        return error;
    }

    /* start the gpts */
    for (int i = 0; i < NUM_GPTS; i++) {
        error = gpt_start(&omap_ltimer->gpts[i]);
        if (error) {
            ZF_LOGE("Failed to start gpt");
            destroy(ltimer->data);
            return error;
        }
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
