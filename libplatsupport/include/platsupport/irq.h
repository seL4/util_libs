/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <errno.h>

#define __PS_IRQ_VALID_ARGS(FUN) do {\
    if (!irq_ops) return -EINVAL;\
    if (!irq_ops->cookie) return -EINVAL;\
    if (!irq_ops->FUN) return -ENOSYS;\
} while(0)

#define PS_INVALID_IRQ_ID -1

typedef enum irq_type {
    PS_NONE,
    PS_MSI,
    PS_IOAPIC,
    PS_INTERRUPT,
    PS_TRIGGER,
    PS_PER_CPU,
    PS_OTHER,
} irq_type_t;

typedef enum irq_trigger_type {
    PS_LEVEL_TRIGGERED,
    PS_EDGE_TRIGGERED,
} irq_trigger_type_t;

typedef struct {
    irq_type_t type;
    union {
        struct {
            long ioapic;
            long pin;
            long level;
            long polarity;
            long vector;
        } ioapic;
        struct {
            long pci_bus;
            long pci_dev;
            long pci_func;
            long handle;
            long vector;
        } msi;
        struct {
            long number;
        } irq;
        struct {
            long number;
            long trigger;
        } trigger;
        struct {
            long number;
            long trigger;
            long cpu_idx;
        } cpu;
        void *other; /* Implementation-specific information. */
    };
} ps_irq_t;

typedef int irq_id_t;

/*
 * Acknowledges an interrupt.
 *
 * @param ack_data Implementation specific structure containing information needed to ACK an interrupt.
 *
 * @return 0 on success, otherwise an error code
 */
typedef int (*ps_irq_acknowledge_fn_t)(void *ack_data);

/*
 * Callback type that is accepted by implementations of the IRQ interface. The
 * callback is responsible for acknowledging the interrupt via the supplied
 * acknowledge function.
 *
 * Note that the acknowledge function pointer and its token, 'ack_data' can be
 * saved and called later. This might be useful in some contexts where you need
 * to perform some maintenance before acknowledging the IRQ. The lifetime of
 * the 'ack_data' token lasts until the acknowledge function is called. Thus
 * you cannot call the acknowledge function again.
 *
 * @param data Pointer to data which is passed into the callback function
 * @param acknowledge_fn Function pointer to an function used to acknowledge an interrupt
 * @param ack_data Data to be passed to 'acknowledge_fn'
 */
typedef void (*irq_callback_fn_t)(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data);

/*
 * Registers an interrupt with the interface and allocates data necessary to
 * keep track of the interrupt. Also associates a callback function with the
 * interrupt which is called when the interrupt arrives.
 *
 * Returns a valid IRQ ID on success that has a value >= 0, otherwise an error
 * code with a value < 0.
 *
 * @param cookie Cookie for the IRQ interface
 * @param irq Information about the interrupt that is to be registered
 * @param callback Callback function that is called when the interrupt arrives
 * @param callback_data Pointer that is to be passed into the callback function
 *
 * @return A valid IRQ ID on success that has a value >= 0, otherwise an error code
 */
typedef irq_id_t (*ps_irq_register_fn_t)(void *cookie, ps_irq_t irq, irq_callback_fn_t callback, void *callback_data);

/*
 * Unregisters a registered interrupt and deallocates any data that was
 * associated with the registered interrupt.
 *
 * @param cookie Cookie for the IRQ interface
 * @param irq_id An IRQ ID that was allocated by the IRQ interface
 *
 * @return 0 on success, otherwise an error code
 */
typedef int (*ps_irq_unregister_fn_t)(void *cookie, irq_id_t irq_id);

typedef struct {
    void *cookie;
    ps_irq_register_fn_t irq_register_fn;
    ps_irq_unregister_fn_t irq_unregister_fn;
} ps_irq_ops_t;

static inline irq_id_t ps_irq_register(ps_irq_ops_t *irq_ops, ps_irq_t irq, irq_callback_fn_t callback,
                                       void *callback_data)
{
    __PS_IRQ_VALID_ARGS(irq_register_fn);
    return irq_ops->irq_register_fn(irq_ops->cookie, irq, callback, callback_data);
}

static inline int ps_irq_unregister(ps_irq_ops_t *irq_ops, irq_id_t irq_id)
{
    __PS_IRQ_VALID_ARGS(irq_unregister_fn);
    return irq_ops->irq_unregister_fn(irq_ops->cookie, irq_id);
}
