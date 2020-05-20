<!--
  Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

# Direct Memory Access (DMA) management interface

`ps_dma_man_t` is an interface for software to allocate and provide direct
memory access (DMA) memory for devices to use.

<https://github.com/seL4/util_libs/blob/master/libplatsupport/include/platsupport/io.h>

## Usage

DMA communication allows devices to directly access memory. This enables
communication between hardware devices and software to happen asynchronously.
This DMA management interface is how software can manage the memory used in DMA
transactions.

A DMA transaction works as follows:

1. Software allocates some memory to be shared with the hardware.
2. The memory is initialised by the software. If the transaction involves
   sending data to the hardware then this data can be written into the memory.
3. The memory gets 'pinned' which means that the software's system environment
   is not supposed to swap the backing memory out in dynamic environments. Once
   the memory has been pinned, the software is free to tell the hardware about
   the memory so that it can be used.
4. The software hands the memory region over to the hardware by communicating
   with it over some interface.  Once the memory is handed over to the
   hardware, the software is not expected to modify it.
5. The hardware is free to access the memory which it can do by reading or
   writing to it directly.
6. Once the hardware is ready to release the memory back to the software it
   indicates that it has finished with the memory via some interface.
7. The software can now access the modified data by reading the memory that may
   have been updated by the hardware.
8. The software can 'unpin' the memory to indicate to the system environment
   that the memory is not going to be used by the hardware anymore and it may
   choose to swap it out.
9. When the software is finished with the memory it can free the memory and it
   gets given back to the DMA manager to potentially reallocate elsewhere.

This transaction protocol relies on the software and hardware synchronising
access to the shared memory correctly.  In all cases the memory access is
controlled by the software. Sending data from the hardware to software requires
the software to initially provide DMA memory for the hardware to then write
into.

To have memory safety, the hardware should only access memory within the
regions provided to it by the software.  This requires the software to
communicate addresses to the hardware correctly, and also for the hardware to
only access memory that it has been given by the software. Without special
hardware architecture mechanisms such as a hardware memory management unit, the
software and hardware components need to be trusted to behave correctly.

These hardware protection mechanisms are known as IOMMU on x86 and SMMU on Arm
and provide a mechanism for system software to control memory access by
hardware devices to both protect against devices and drivers that are
untrustworthy, or have errors.  On systems with these mechanisms, the 'pin' and
'unpin' operations can allow the system environment software to create device
mappings of the memory and provide an address for the software to then provide
to the hardware.  In this scenario, in cases where the hardware tries to access
memory that is outside of the allocated DMA regions it will generate a
protection fault instead of accessing the memory.

Support for hardware memory protection mechanisms is intended to be handled by
the interface implementation and the users of this interface do not need to be
aware of this.

Scatter-gather is a common mechanism used to specify how a single data stream
can be constructed from a sequence of buffers. In a scenario where a device
supports scatter-gather, multiple DMA allocations can be used for all of the
buffers.  Essentially, scatter-gather becomes a mechanism implemented on top of
this interface.

```c
void * ps_dma_alloc(const ps_dma_man_t *dma_man, size_t size, int align, int cache, ps_mem_flags_t flags)
void ps_dma_free(const ps_dma_man_t *dma_man, void *addr, size_t size)
```

The two functions, `ps_dma_alloc` and `ps_dma_free`, allocate and free memory
that can be used in DMA transactions.  The `size` and `alignment` arguments are
used to specify the size and position of the allocated region that will be
compatible with hardware requirements. These methods also handle mapping and
unmapping the DMA regions into the software address space. The return value
specifies the address of the allocated memory in the caller's address space.
If an allocation is not possible then `NULL` will be returned.  Memory mapping
attributes for the caller's mappings are specified by the `cache` and `flags`
arguments.  It is architecture dependent whether hardware memory accesses are
cache coherent. In situations where DMA is not cache coherent, the DMA memory
allocation can either be mapped uncached, or cached but then cache operations
are used to ensure that the uncached hardware access sees a consistent view of
memory contents.  On architectures where DMA is cache coherent then cached
mappings do not require cache maintenance operations.

```c
uintptr_t ps_dma_pin(ps_dma_man_t *dma_man, void *addr, size_t size)
void ps_dma_unpin(ps_dma_man_t *dma_man, void *addr, size_t size)
```

The two functions `ps_dma_pin` and `ps_dma_unpin` indicate to the system
environment that the memory can be accessed by the hardware. The pinning
operation is required because DMA access faults cannot be trapped and restarted
by a system environment the same way that regular memory faults can be trapped
and restarted for on-demand paging. `ps_dma_pin` gets provided with `addr` and
`size` which describe a region of memory that has previously been allocated.
The return value is an address that the software can provide to the hardware
for accessing the memory.  `ps_dma_unpin` will inform the system environment
that the hardware is finished accessing the memory. `addr` and `size` specify
the memory range to unpin.

```c
void ps_dma_cache_op(const ps_dma_man_t *dma_man, void *addr, size_t size, dma_cache_op_t op)
void ps_dma_cache_clean(const ps_dma_man_t *dma_man, void *addr, size_t size)
void ps_dma_cache_invalidate(const ps_dma_man_t *dma_man, void *addr, size_t size)
void ps_dma_cache_clean_invalidate(const ps_dma_man_t *dma_man, void *addr, size_t size)
```

The above functions perform cache operations that may be required to make the
contents of memory visible to hardware or software. A clean operation will
cause data in the cache to be written back to memory, invalidate will cause
data in the cache to be marked invalid so that future reads will load the data
in from memory, and clean/invalidate will clean the cache and then invalidate
the cache. In a scenario where the software access is cached and the hardware
access is uncached, a clean operation would be required before releasing DMA
regions to the device and an invalidate operation would be required before
accessing the DMA memory after getting the descriptor back from the device.

## Implementation details

It is up to the implementation when it chooses to create and destroy mappings
for the caller and for the device hardware.  The interface guarantees that the
memory is accessible for the caller between matching calls to `ps_dma_alloc`
and `ps_dma_free` and accessible to the hardware between calls to `ps_dma_pin`
and `ps_dma_unpin`.  Static implementations of this interface may leave all
mappings in place for the lifetime of the system and simply return addresses to
the caller when it uses the interface.

It is up to the implementation whether it implements hardware protection
mechanisms (IOMMU/SMMU). The interface does not provide a mechanism for error
handling faults caused by invalid hardware access and so this feature is
essentially hidden from the caller.

## Potential future changes

Some devices cannot handle 64-bit addresses for DMA but there is no way to
specify this as a constraint in the original allocation call. This could likely
be implemented as an additional flag in the allocation interface and the
implementation would need to be able to provide 32-bit memory allocations.
Implementations that are able to use a hardware MMU to create device mappings
likely will not be affected by this issue as they can create mappings less than
`2^32` that can access physical memory located higher than `2^32`.

Interface support for dealing with scatter-gather lists may be added in the
future in response to performance optimisations.  Callers can currently
assemble scatter-gather lists from a collection of allocated DMA regions.
