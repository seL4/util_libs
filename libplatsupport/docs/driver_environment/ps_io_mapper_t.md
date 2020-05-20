<!--
  Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

# Memory Mapped I/O interface

`ps_io_mapper_t` is an interface for providing access to device memory (for
memory mapped I/O) by creating memory mappings in an address space.

<https://github.com/seL4/util_libs/blob/master/libplatsupport/include/platsupport/io.h>

## Usage

The interface provides two functions:

```c
void * ps_io_map(const ps_io_mapper_t *io_mapper, uintptr_t paddr, size_t size, int cached, ps_mem_flags_t flags)
void ps_io_unmap(const ps_io_mapper_t *io_mapper, void *vaddr, size_t size)
```

`io_mapper` is a handle to the instance of the interface being called. This is
typically constructed as part of a system environment and then provided to the
caller to use.

Both functions are expected to operate synchronously, which means that their
effects are visible once the function has returned.

`ps_io_map` will try to create a mapping region of size `size` and starting
physical address of `paddr`. The architecture specific mapping attributes will
be set based on the values of `cached` and `flags`. The return value indicates
the virtual address of `paddr` in the new mapping or `NULL` will be returned if
a mapping was not created. The values that the `flags` parameter can take are
currently:

```c
typedef enum ps_mem_flags {
    PS_MEM_NORMAL, /* No hints, consider 'normal' memory */
    PS_MEM_HR,     /* Host typically reads */
    PS_MEM_HW      /* Host typically writes */
} ps_mem_flags_t;
```

`ps_io_unmap` will likely remove a mapping created by `ps_io_map`. The mapping
to remove is identified by its virtual address and size, `vaddr` and `size`.
There is no return value. Implementations are not required to remove the
mapping when this is called, but a caller is not allowed to access the mapping
after `ps_io_unmap` is called.

Callers of this interface are expected to know the address of physical memory
that they intend to access. Anonymous style mappings, where the physical
address is unspecified, should be provided by a different interface.

`paddr` and `size` will typically describe a region that can be constructed
from valid hardware frame sizes.  If this is not the case then the created
mapping should cover the entire region requested but the return value will
specify the virtual address corresponding to the `paddr` requested.

It is up to the implementation of this interface how to handle access control.
If the caller tries to create a mapping with different mapping attributes than
what it has access rights for it is up to the implementation to return a `NULL`
mapping, or to return an address for a mapping that may result in a memory
access fault when accessed.

It is expected that device memory mappings are created during device
initialisation and will not be created in a fastpath scenario where performance
is more critical.

## Implementation details

A goal for this interface is to allow implementations for both static and
dynamic environments. An example of a static environment implementation is
where all of the mappings are created during program initialisation and mapping
calls to this interface simply return the existing mappings, while unmapping
calls have no observable effect. An example of a dynamic environment
implementation is where a mapping is created or destroyed in direct response to
calls to this interface.

In some platforms, it is possible that register physical addresses can exist in
chunks that are smaller than the smallest hardware page, therefore, it is not
feasible to expect any implementation to validate that all physical addresses
being mapped are valid. Additionally, certain device registers may have
additional protections that are not managed by a virtual memory system and
likely outside of any implementation's responsibilities.

Virtual addresses returned by an implementation likely exist in an address
space with other address allocations from other interfaces and therefore the
implementation would need to be aware of these and not return address ranges
that would overlap with other addresses. An implementation should use a common
address space allocator for obtaining valid virtual address regions to then map
physical memory into.

Access control policies for access to underlying memory can be implemented. The
interface can currently only return `NULL` to indicate that a mapping was not
created.  If a physical region requested is invalid the implementation is
allowed to return `NULL` to indicate no mapping was created, or to return a
virtual address that has less access rights than requested or even a virtual
address that is not mapped to the physical memory requested. It is expected
that this interface is used on architectures with memory management systems
that generate memory faults when a program accesses memory invalid permissions.

Implementations are not required to perform any accounting internally. Calls to
`ps_io_unmap` that do not correspond to earlier calls to `ps_io_map` can be
ignored.

## Potential future changes

The interface currently does not provide a mechanism for callers to provide
exact hardware mapping attributes and must specify them via the `cached` and
`flags` arguments. It may be preferable to explicitly provide these attributes
to avoid ambiguity.

There is currently no way for the caller to be synchronously informed about
what caused a `NULL` value to be returned.  This does not enable a caller to
respond differently to different error conditions. Such error conditions could
be generic or implementation defined.
