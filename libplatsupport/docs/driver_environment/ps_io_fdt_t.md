<!--
  Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause

  @TAG(DATA61_BSD)
-->

# FDT interface

`libplatsupport` provides interfaces and utility functions to interact with the
flattened device tree (FDT) of a platform.

<https://github.com/seL4/util_libs/blob/master/libplatsupport/include/platsupport/io.h>

## Usage

The interface provides one function:

```c
char *ps_io_fdt_get(const ps_io_fdt_t *io_fdt);
```

`io_fdt` is a handle to the instance of the interface being called. This is
typically constructed as part of a system environment and then provided to the
caller to use.

The function is expected to operate synchronously, which means that their
effects are visible once the function has returned.

`ps_io_fdt_get` will retrieve the copy of the FDT that the interface has saved
and return it to the user. The copy follows the shallow copy semantics in that
it is only a reference to the FDT that the interface holds.

The FDT can be walked manually or it can be passed to a number of utility
functions for parsing and mapping resources.

### Device tree parsing utility functions

<https://github.com/seL4/util_libs/blob/master/libplatsupport/include/platsupport/fdt.h>

These utility functions cover the common FDT use cases, for example, reading
the `regs` and `interrupts/interrupts-extended` properties to map registers and
allocate hardware interrupts.

The functions are designed in such a way that they parse the properties and
invoke a callback on each instance in the properties field. Each callback is
given information about the current instance, the total number of instances and
a user-supplied token. It is up to the driver providing the callback function as
to what is done with the property information passed to it, for example, it
could use the information to allocate appropriate hardware resources.

The key functions in this interface are described below.

```c
int ps_fdt_read_path(ps_io_fdt_t *io_fdt, ps_malloc_ops_t *malloc_ops, const char *path, ps_fdt_cookie_t **ret_cookie);
```

Given a device tree path, this function will return a cookie that can be used in
other functions in this library.

```c
int ps_fdt_walk_registers(ps_io_fdt_t *io_fdt, ps_fdt_cookie_t *cookie, reg_walk_cb_fn_t callback, void *token);
```

This function walks the `regs` property of the device node (if any)
corresponding to the input cookie and invokes a callback function at each
register instance of the field.

```c
int ps_fdt_walk_irqs(ps_io_fdt_t *io_fdt, ps_fdt_cookie_t *cookie, irq_walk_cb_fn_t callback, void *token);
```

This function walks the `interrupts/interrupts-extended` field of the device
node (if any) corresponding to the cookie passed in and invokes a callback
function at each interrupt instance of the field.

To illustrate this interface, consider a snippet of the device tree from the NVIDIA Jetson TX2.

```
ahci-sata@3507000 {
    compatible = "nvidia,tegra186-ahci-sata";
    reg = < 0x00 0x3507000 0x00 0x2000 0x00 0x3501000 0x00 0x6000 0x00 0x3500000 0x00 0x1000 0x00 0x3a90 000 0x00 0x10000 >;
    reg-names = "sata-ahci","sata-config","sata-ipfs","sata-aux";
    interrupts = < 0x00 0xc5 0x04 >;
    ...
};
```

The `regs` property consists of a sequence of *address* and *size* cells. The
parent node of this device (not shown) specifies that the number of address and
size cells per register instance is 2.  So when we call the register walking
function it would read the first instance in the register property (which is
`<0x00 0x3507000 0x00 0x2000>` in this example) and invoke the callback with
this information.  It would then invoke the callback again with the next
instance (`<0x00 0x3501000 0x00 0x6000>`), and continue until the whole of the
`regs` property has been processed.

For convenience's sake, there are two functions which can be used to map
resources without supplying an explicit callback. These can be useful when the
structure of the targeted node in the FDT is known beforehand.

```c
void *ps_fdt_index_map_register(ps_io_ops_t *io_ops, ps_fdt_cookie_t *cookie, unsigned offset, pmem_region_t *ret_pmem);

void ps_fdt_index_register_irq(ps_io_ops_t *io_ops, ps_fdt_cookie_t *cookie, unsigned offset, irq_callback_fn_t irq_callback, void *irq_callback_data);
```

### Interrupt parsing

Note that parsing the `interrupts` property is a little special. Depending on
the platform and device, the `interrupts` property of a device may have a
special encoding format.

For example, the Arm GICv2 specifies that each `interrupts` instance takes 3
cells, the first cell describes the type of interrupt (e.g. SPI or PPI), the
second cell describes the interrupt number, and the third cell describes the
flags of the interrupt. On the GICv3, however, each instance can take 3 or 4
cells as multiple formats are specified, and so processing the property
requires a different process.

Our approach to determining the interrupt property encoding is similar to
Linux's approach. We follow the parent node's interrupt controller `phandle` to
the interrupt controller node that is responsible for the target device. The
`compatible` string of the interrupt controller is checked to determine what
type of interrupt controller it is. The parsing is then delegated to a parser
module that understands the interrupt controller and its expected `interrupts`
property format.

More information about the interrupt encodings is available in Linux's interrupt
bindings documentation:
<https://github.com/torvalds/linux/tree/master/Documentation/devicetree/bindings/interrupt-controller>.

## Implementation details

The implementation must keep a valid copy of the FDT that reflects all the
devices or a part of the devices on the running platform. The implementation is
not strictly required to support x86/pc99 or any platforms where the device
setup on the platform can be dynamic and/or hotswappable.

## Potential future changes

For now, the interface only contains a function to retrieve the FDT of the
platform. This may seem to be a unnecessary abstraction since we only provide
one function, but this design allows us to easily extend the functionality, if
need be, in the future.
