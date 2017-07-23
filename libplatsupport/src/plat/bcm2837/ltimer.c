/* Implementation of a logical timer for omap platforms
 *
 * We use two GPTS: one for the time and relative timeouts, the other
 * for absolute timeouts.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/spt.h>
#include <platsupport/pmem.h>
#include <utils/util.h>

#define NV_TMR_ID TMR0

typedef struct {
    spt_t spt;
    void *vaddr;
    ps_io_ops_t ops;
    uint64_t period;
} spt_ltimer_t;

static pmem_region_t pmem =
{
    .type = PMEM_TYPE_DEVICE,
    .base_addr = SP804_TIMER_PADDR,
    .length = PAGE_SIZE_4K
};

size_t get_num_irqs(void *data)
{
    return 1;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n == 0);
    irq->irq.number = SP804_TIMER_IRQ;
    irq->type = PS_INTERRUPT;
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return 1;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *paddr)
{
    assert(n == 0);
    *paddr = pmem;
    return 0;
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    spt_ltimer_t *spt_ltimer = data;
    spt_handle_irq(&spt_ltimer->spt);
    if (spt_ltimer->period > 0) {
        spt_set_timeout(&spt_ltimer->spt, spt_ltimer->period);
    }
    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    spt_ltimer_t *spt_ltimer = data;
    *time = spt_get_time(&spt_ltimer->spt);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    spt_ltimer_t *spt_ltimer = data;
    spt_ltimer->period = 0;

    switch (type) {
    case TIMEOUT_ABSOLUTE: {
        uint64_t time = spt_get_time(&spt_ltimer->spt);
        if (time >= ns) {
            return ETIME;
        }
        return spt_set_timeout(&spt_ltimer->spt, ns - time);
    }
    case TIMEOUT_PERIODIC:
        spt_ltimer->period = ns;
        /* fall through */
    case TIMEOUT_RELATIVE:
        return spt_set_timeout(&spt_ltimer->spt, ns);
    }

    return EINVAL;
}

static int reset(void *data)
{
    assert(data != NULL);
    spt_ltimer_t *spt_ltimer = data;
    spt_start(&spt_ltimer->spt);
    spt_stop(&spt_ltimer->spt);
    return 0;
}

static void destroy(void *data)
{
    assert(data);
    spt_ltimer_t *spt_ltimer = data;
    if (spt_ltimer->vaddr) {
        spt_stop(&spt_ltimer->spt);
        ps_pmem_unmap(&spt_ltimer->ops, pmem, spt_ltimer->vaddr);
    }
    ps_free(&spt_ltimer->ops.malloc_ops, sizeof(spt_ltimer), spt_ltimer);
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

    error = ps_calloc(&ops.malloc_ops, 1, sizeof(spt_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    spt_ltimer_t *spt_ltimer = ltimer->data;
    spt_ltimer->ops = ops;
    spt_ltimer->vaddr = ps_pmem_map(&ops, pmem, false, PS_MEM_NORMAL);
    if (spt_ltimer->vaddr == NULL) {
        destroy(ltimer->data);
    }

    /* setup spt */
    spt_config_t config = {
        .vaddr = spt_ltimer->vaddr,
    };

    spt_init(&spt_ltimer->spt, config);
    spt_start(&spt_ltimer->spt);
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
