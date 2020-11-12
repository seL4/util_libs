<!--
  Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause

  @TAG(DATA61_BSD)
-->

# Architectural I/O operations

`ps_io_port_ops_t` is an interface for performing architectural I/O operations,
i.e. device I/O that cannot be performed via memory-mapped IO. One example of
this is I/O Ports on x86.

<https://github.com/seL4/util_libs/blob/master/libplatsupport/include/platsupport/io.h>

## Usage

The interface provides two functions:

```c
int ps_io_port_in(const ps_io_port_ops_t *port_ops, uint32_t port, int io_size, uint32_t *result)
int ps_io_port_out(const ps_io_port_ops_t *port_ops, uint32_t port, int io_size, uint32_t val)
```

`port_ops` is a handle to the instance of the interface being called. This is
typically constructed as part of a system environment and then provided to the
caller to use.

Both functions are expected to operate synchronously, which means that their
effects are visible once the function has returned.

`ps_io_port_in` will perform a blocking read operation on a port. `result` will
be set to the result of the read.  `io_size` specifies the size, in bytes, of
the read and cannot be larger than 4. The `port` contains the architectural
address of the port that the operation applies to. The return value is used to
indicate if the operation succeeded.

`ps_io_port_out` will perform a blocking write operation on a port. `val`
contains the value to be written and `io_size` the size, in bytes, of the data
to be written and also cannot be larger than 4. For values of `io_size` less
than 4, the input value, `val`, will be cast to a variable of the smaller size
and data that is removed in the cast will be ignored.  The return value is used
to indicate if the operation succeeded.

Acceptable values of `port` depend on the implementation and an instance of
this interface is expected to return an error for port values that are
unrecognised.  Valid ports may change over the lifetime of the interface if the
implementation can dynamically add or remove ports.

## Implementation details

While it would be possible to use this interface for devices that use
memory-mapped I/O (MMIO) it is not a design goal and it would be better to use
the `ps_io_mapper_t` interface instead.

It is expected that these in and out operations complete within a reasonable
amount of time and that the interface functions are effectively non-blocking.

An implementation may provide its own mechanisms for dynamically changing and
querying available ports. This would require a custom interface that the
implementation provides.  Such an interface could be used by the caller or by
the system environment but this would be defined by the implementation.

## Potential future changes

This interface is currently intended for handling x86 I/O Port operations via
the `in8, in16, in32, out8, out16, out32` instructions as these are protected
operations and must be performed in a privileged processor mode.  Arm could
have a use for this interface to allow performing secure monitor calls which is
how communication with software running in the "Secure" security state is
performed.  Supporting this would require extending the interface to allow for
providing more arguments than a single 32-bit value.

It may be necessary to provide a consistent way of querying which ports an
interface instance supports.  Currently it is assumed that the caller knows
this implicitly. This may be due to providing a list of ports during the
construction of the interface, or some shared configuration.

Performance optimisations such as batching operations may be needed in the
future. Currently we assume that these ports are used for low bandwidth
configuration, and any operations that require higher data transfer bandwidth
use an MMIO interface.
