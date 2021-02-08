<!--
     Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)

     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Memory allocation interface

`ps_malloc_ops_t` is an interface to provide implementations for allocating
anonymous memory using malloc-like functions.

<https://github.com/seL4/util_libs/blob/master/libplatsupport/include/platsupport/io.h>

## Usage

The functions in this interface are very similar to POSIX functions `malloc`,
`calloc` and `free`. This interface is provided to enable allocation of memory
from an underlying memory allocator that supports allocating memory sizes
smaller than a hardware page.

```c
int ps_malloc(const ps_malloc_ops_t *ops, size_t size, void **ptr)
int ps_calloc(const ps_malloc_ops_t *ops, size_t nmemb, size_t size, void **ptr)
```

The additional argument `ptr` is used to return the allocated address. The
return value is then used for reporting whether the allocation failed and
provides an error code.  `ps_calloc` will clear the allocated memory and takes
an `nmemb` argument for multiple elements as `calloc` does.

```c
int ps_free(const ps_malloc_ops_t *ops, size_t size, void *ptr)
```

`ps_free` is used to free only memory allocations created by `ps_malloc` and
`ps_calloc`.  Compared to POSIX `free` this function takes an additional `size`
argument that can be used by the implementation for stricter checking of
freeing of memory regions. Additionally, a return value is provided to allow
reporting of errors.

## Implementation details

A valid implementation of this interface would be to call the underlying POSIX
functions `malloc`, `calloc`, and `free`.

Multiple instances of this interface could be used to track memory allocations
of different driver instances within the same software component to allow
destruction of a driver instance and reclamation of resources without having to
destroy the software component.

## Future changes

Adding a `realloc` like function may occur in the future.
