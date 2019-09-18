<!--
  Copyright 2019, Data61
  Commonwealth Scientific and Industrial Research Organisation (CSIRO)
  ABN 41 687 119 230.

  This software may be distributed and modified according to the terms of
  the BSD 2-Clause license. Note that NO WARRANTY is provided.
  See "LICENSE_BSD2.txt" for details.

  @TAG(DATA61_BSD)
-->
# FDT

`libplatsupport` provides interfaces and utility functions to interact with the
flattened device tree (FDT) of a platform.

## `ps_io_fdt`

**[Link to header](https://github.com/seL4/util_libs/blob/master/libplatsupport/include/platsupport/io.h#L387)**

This interface provides an abstraction over a platform's FDT. For now, the
interface only contains a function to retrieve the FDT of the platform. This
may seem to be a unnecessary abstraction as we only provide one function, but
this design allows us to easily extend the functionality if need be in the
future.

## DTB parsing utility functions

**[Link to header](https://github.com/seL4/util_libs/blob/master/libplatsupport/include/platsupport/fdt.h)**

These utility functions cover the common use cases of using the FDT, for
example, reading the `regs` and `interrupts/interrupts-extended` properties to
map registers and allocate hardware interrupts. The functions are designed in
such a way that it parses the properties and calls a callback on each instance
in the properties field. The callback would be given information about the
current instance, the total number of instances and a user-supplied token. It
could use the information to allocate hardware resources.

To illustrate this process, consider a snippet of the DTS from the TX2.

```
ahci-sata@3507000 {
    compatible = "nvidia,tegra186-ahci-sata";
    reg = < 0x00 0x3507000 0x00 0x2000 0x00 0x3501000 0x00 0x6000 0x00 0x3500000 0x00 0x1000 0x00 0x3a90
000 0x00 0x10000 >;
    reg-names = "sata-ahci","sata-config","sata-ipfs","sata-aux";
    interrupts = < 0x00 0xc5 0x04 >;
    ...
};
```

The parent node of this device describes that the number of address cells and
size cells is 2. So when we call the register walking function, the function
would encapsulate the first instance of the register property which is `< 0x00
0x3507000 0x00 0x2000>` and then call the callback with it. Afterwards, it
would then call the callback with the next instance `< 0x00 0x3501000 0x00
0x6000>` and so on.

### Interrupts parsing

A thing to note is that interrupt parsing is a little special. Depending on the
platform and device, the interrupts property of a device may have a special
encoding format.

For example, the ARM GIC specifies that each instance takes 3 cells, the first
cell describes whether or not the interrupt is SPI or PPI, the second cell
describes the interrupt number, and the third cell desribes the flags of the
interrupt. Whereas on the GICv3, each instance could take 3 or 4 cells.

The approach that we take when figuring out the interrupt property encoding is
similar to Linux's approach. We follow the parent node's interrupt controller
phandle to the interrupt controller node that is responsible for the target
device. The compatible string of the interrupt controller is then checked to
find out what type of interrupt controller it is. The parsing is then delegated
to a parser module which understands the interrupt controller.

You can read more about the interrupt encodings from Linux's interrupt bindings
documentation
[here](https://github.com/torvalds/linux/tree/master/Documentation/devicetree/bindings/interrupt-controller).
