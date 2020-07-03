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

    int error = irq_handler(ltimer->data, irq);
    assert(!error);

    error = acknowledge_fn(ack_data);
    assert(!error);
}
