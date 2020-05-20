<!--
  Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

# Interrupt handling interface

`ps_irq_ops_t` is an interface for registering interrupt handlers to be called
when hardware interrupts are received.

<https://github.com/seL4/util_libs/blob/master/libplatsupport/include/platsupport/irq.h>

## Usage

This interface allows interrupt handlers to be registered for different IRQs.
If an interrupt is received matching an IRQ that has a registered handler, the
handler can be called. The handler is given a function for acknowledging the
interrupt which it needs to call once the interrupt has been handled. Once the
interrupt has been acknowledged then the interrupt can be received again.

```c
irq_id_t ps_irq_register(ps_irq_ops_t *irq_ops, ps_irq_t irq, irq_callback_fn_t callback, void *callback_data)
int ps_irq_unregister(ps_irq_ops_t *irq_ops, irq_id_t irq_id)
```

Registering and unregistering interrupt handler actions are performed by
calling `ps_irq_register` and `ps_irq_unregister`. `irq` identifies an
interrupt to register the handler for. A `ps_irq_t` is a union that allows
different interrupt types to be passed into the interface.  An implementation
cannot be expected to be able to match any `irq` value to a hardware interrupt
due to different ways interrupts can be described across different
architectures. The `irq` value is also used to specify which attributes the
interrupt has. An example is the `irq.trigger` type which allows a handler to
be registered with a trigger that is edge based or level based for a particular
interrupt number.

Registering more than one interrupt handler for a single interrupt is not
allowed. This is because there is no generic way to handle acknowledgement of
the interrupt. If multiple handlers need to be called for a particular
interrupt, then an additional handler can be created that calls the multiple
other handlers and picks a policy for when to acknowledge the interrupt.

`ps_irq_unregister` causes the interrupt handler to be unregistered for a
particular interrupt. Multiple calls to `ps_irq_register` for the same `irq`
need to have matching calls to `ps_irq_unregister` before each new
`ps_irq_register`.  This is so that the implementation is able to detect errors
for mistakenly trying to register different handlers for the same hardware
interrupt.

```c
void (*irq_callback_fn_t)(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
int (*ps_irq_acknowledge_fn_t)(void *ack_data)
```

The callback function provided to `ps_irq_register` will be called with the
`data` parameter that was provided when the handler was registered.
Additionally `acknowledge_fn` and `ack_data` will be provided for acknowledging
the interrupt. The interrupt handler will not be called again until the ack
function is called. It is not required to call the ack function before the
callback function returns. This enables situations where sufficiently handling
the interrupt requires an event to happen asynchronously.

## Implementation details

Any interrupts that are received before an IRQ handler is registered do not
need to be tracked. It is expected that the caller registering an interrupt
handler is able to poll for any events that need to be processed after
registering the handler. The implementation is required to configure the
interrupt handling such that if the hardware generates an interrupt after the
handler is registered it will be possible to call the interrupt handler.

Given that it is possible for the `acknowledge_fn` given to the interrupt
handler to be called after the interrupt handler has returned, the
implementation will need to ensure it is able to keep track of any resources
allocated for the handler until the `acknowledge_fn` is called. If
`ps_irq_unregister` is called, any existing `acknowledge_fn`s for that `irq`
can be cleaned up.

Multiple values of `irq` could map to the same hardware interrupt. In these
cases it is not valid for a different interrupt handler to be registered for
different values. The implementation needs to reject any additional
`ps_irq_register` calls for these `irq`s.

Reconfiguring an interrupt is possible by first calling `ps_irq_unregister`,
followed by `ps_irq_register` on the same interrupt but with a different value
of `irq` that has different attributes.

An implementation that would like to case match over the different
`ps_irq_ops_t` types should be aware that this union is expected to get
additional types added that certain implementations are not aware of. An
implementation should be able to handle the case where it sees a `ps_irq_ops_t`
type that it is unable to handle, and fail accordingly.

## Potential future changes

`ps_irq_ops_t` will be extended when new types of hardware interrupt
configurations are supported.
