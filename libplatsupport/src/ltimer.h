/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <platsupport/ltimer.h>
#include <platsupport/irq.h>
#include <platsupport/fdt.h>

/* Per-platform ltimer IRQ handling function type */
typedef int (*ltimer_handle_irq_fn_t)(void *data, ps_irq_t *irq);

typedef struct {
    ltimer_t *ltimer;
    ps_irq_t *irq;
    ltimer_handle_irq_fn_t irq_handler;
} timer_callback_data_t;

/*
 * This is a simple wrapper around the per-platform ltimer IRQ handling function.
 *
 * This is called as a callback function from the IRQ interface when the user asks it to handle IRQs.
 *
 * Note that this wrapper assumes that the interrupts are level triggered.
 */
static inline void handle_irq_wrapper(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    assert(data);

    timer_callback_data_t *callback_data = (timer_callback_data_t *) data;

    ltimer_t *ltimer = callback_data->ltimer;
    ps_irq_t *irq = callback_data->irq;
    ltimer_handle_irq_fn_t irq_handler = callback_data->irq_handler;

    int UNUSED error = irq_handler(ltimer->data, irq);
    assert(!error);

    error = acknowledge_fn(ack_data);
    assert(!error);
}

static int helper_fdt_alloc_simple(
    ps_io_ops_t *ops, char *fdt_path,
    unsigned reg_choice, unsigned irq_choice,
    void **vmap, pmem_region_t *pmem, irq_id_t *irq_id,
    irq_callback_fn_t handler, void *handler_token
)
{
    assert(fdt_path != NULL);
    assert(vmap != NULL && pmem != NULL && irq_id != NULL);

    int error;
    void *temp_vmap;
    pmem_region_t temp_pmem;
    irq_id_t temp_irq_id;

    *vmap = NULL;
    *irq_id = PS_INVALID_IRQ_ID;

    /* Gather FDT info */
    ps_fdt_cookie_t *cookie = NULL;
    error = ps_fdt_read_path(&ops->io_fdt, &ops->malloc_ops, fdt_path, &cookie);
    if (error) {
        ZF_LOGE("Simple FDT helper failed to read path (%d, %s)", error, fdt_path);
        return error;
    }

    temp_vmap = ps_fdt_index_map_register(ops, cookie, reg_choice, &temp_pmem);
    if (temp_vmap == NULL) {
        ZF_LOGE("Simple FDT helper failed to map registers");
        return ENODEV;
    }

    temp_irq_id = ps_fdt_index_register_irq(ops, cookie, irq_choice, handler, handler_token);
    if (temp_irq_id <= PS_INVALID_IRQ_ID) {
        ZF_LOGE("Simple FDT helper failed to register irqs (%d)", temp_irq_id);
        return temp_irq_id;
    }

    error = ps_fdt_cleanup_cookie(&ops->malloc_ops, cookie);
    if (error) {
        ZF_LOGE("Simple FDT helper failed to clean up cookie (%d)", error);
        return error;
    }

    *vmap = temp_vmap;
    *pmem = temp_pmem;
    *irq_id = temp_irq_id;

    return 0;
}

static int get_resolution_dummy(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int create_ltimer_simple(
    ltimer_t *ltimer, ps_io_ops_t ops, size_t sz,
    int (*get_time)(void *data, uint64_t *time),
    int (*set_timeout)(void *data, uint64_t ns, timeout_type_t type),
    int (*reset)(void *data),
    void (*destroy)(void *data)
)
{
    assert(ltimer != NULL);

    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution_dummy;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;
    ltimer->get_nth_irq = NULL;
    ltimer->get_nth_pmem = NULL;
    ltimer->get_num_irqs = NULL;
    ltimer->get_num_pmems = NULL;

    int error = ps_calloc(&ops.malloc_ops, 1, sz, &ltimer->data);
    if (error) {
        ZF_LOGE("Unable to allocate ltimer data");
        return error;
    }
    assert(ltimer->data != NULL);

    return 0;
}
