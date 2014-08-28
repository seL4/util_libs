/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_IO_H__
#define __PLATSUPPORT_IO_H__

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <utils/util.h>
#include <sys/types.h>

/* For clock.h and mux.h */
typedef struct ps_io_ops ps_io_ops_t;

#ifdef ARCH_ARM
#include <platsupport/clock.h>
#include <platsupport/mux.h>
#endif

/**
 * Memory usage hints. These indicate how memory is expected to be used
 * allowing for better memory attributes or caching to be done.
 * For example, memory that is only written to would be best mapped
 * write combining if the architecture supports it.
 */
typedef enum ps_mem_flags {
    PS_MEM_NORMAL, /* No hints, consider 'normal' memory */
    PS_MEM_HR,     /* Host typically reads */
    PS_MEM_HW      /* Host typically writes */
} ps_mem_flags_t;

/**
 * Map the region of memory at the requested physical address
 *
 * @param cookie Cookie for the I/O Mapper
 * @param paddr physical address to map.
 * @param size amount of bytes to map.
 * @param cached Whether region should be mapped cached or not
 * @param flags Memory usage flags
 * @return the virtual address at which the data at paddr can be accessed or NULL on failure
 */
typedef void *(*ps_io_map_fn_t)(void* cookie, uintptr_t paddr, size_t size, int cached, ps_mem_flags_t flags);

/**
 * Unmap a previously mapped I/O memory region
 *
 * @param cookie Cookie for the I/O Mapper
 * @param vaddr a virtual address that has been returned by io_map
 * @param size the same size in bytes this memory was mapped in with originally
 */
typedef void (*ps_io_unmap_fn_t)(void *cookie, void *vaddr, size_t size);

typedef struct ps_io_mapper {
    void *cookie;
    ps_io_map_fn_t io_map_fn;
    ps_io_unmap_fn_t io_unmap_fn;
} ps_io_mapper_t;

static inline void *
ps_io_map(ps_io_mapper_t *io_mapper, uintptr_t paddr, size_t size, int cached, ps_mem_flags_t flags)
{
    assert(io_mapper);
    assert(io_mapper->io_map_fn);
    return io_mapper->io_map_fn(io_mapper->cookie, paddr, size, cached, flags);
}

static inline void
ps_io_unmap(ps_io_mapper_t *io_mapper, void *vaddr, size_t size)
{
    assert(io_mapper);
    assert(io_mapper->io_unmap_fn);
    io_mapper->io_unmap_fn(io_mapper->cookie, vaddr, size);
}

/**
 * Perform an architectural I/O 'in' operation (aka I/O ports on x86)
 *
 * @param cookie Cookie to the underlying I/O handler
 * @param port Port to perform the 'in' on
 * @param io_size Size in bytes of the I/O operation
 * @param result Location to store the results. If io_size < 4 then unused bytes will be zeroed
 *
 * @return Returns 0 on success
 */
typedef int (*ps_io_port_in_fn_t) (void* cookie, uint32_t port, int io_size, uint32_t *result);

/**
 * Perform an architectural I/O 'out' operation (aka I/O ports on x86)
 *
 * @param cookie Cookie to the underlying I/O handler
 * @param port Port to perform the 'out' on
 * @param io_size Size in bytes of the I/O operation
 * @param val Value to send to the I/O port
 *
 * @return Returns 0 on success
 */
typedef int (*ps_io_port_out_fn_t)(void* cookie, uint32_t port, int io_size, uint32_t val);

typedef struct ps_io_port_ops {
    void *cookie;
    ps_io_port_in_fn_t io_port_in_fn;
    ps_io_port_out_fn_t io_port_out_fn;
} ps_io_port_ops_t;

static inline int
ps_io_port_in(ps_io_port_ops_t *port_ops, uint32_t port, int io_size, uint32_t *result)
{
    assert(port_ops);
    assert(port_ops->io_port_in_fn);
    return port_ops->io_port_in_fn(port_ops->cookie, port, io_size, result);
}

static inline int
ps_io_port_out(ps_io_port_ops_t *port_ops, uint32_t port, int io_size, uint32_t val)
{
    assert(port_ops);
    assert(port_ops->io_port_out_fn);
    return port_ops->io_port_out_fn(port_ops->cookie, port, io_size, val);
}

typedef enum dma_cache_op {
    DMA_CACHE_OP_CLEAN,
    DMA_CACHE_OP_INVALIDATE,
    DMA_CACHE_OP_CLEAN_INVALIDATE
} dma_cache_op_t;

/**
 * Allocate a dma memory buffer. Must be contiguous in physical and virtual address,
 * but my cross page boundaries. It is also guaranteed that this memory region can
 * be pinned
 *
 * @param cookie Cookie for the dma manager
 * @param size Size in bytes of the dma memory region
 * @param align Alignemnt in bytes of the dma region
 * @param cached Whether the region should be mapped cached or not
 * @param flags Memory access flags
 *
 * @return NULL on failure, otherwise virtual address of allocation
 */
typedef void* (*ps_dma_alloc_fn_t)(void *cookie, size_t size, int align, int cached, ps_mem_flags_t flags);

/**
 * Free a previously allocated dma memory buffer
 *
 * @param cookie Cookie for the dma manager
 * @param addr Virtual address of the memory buffer as given by the dma_alloc function
 * @param size Original size of the allocated buffer
 */
typedef void (*ps_dma_free_fn_t)(void *cookie, void *addr, size_t size);

/**
 * Pin a piece of memory. This ensures it is resident and has a translation until
 * it is unpinned. You should not pin a memory range that overlaps with another
 * pinned range. If pinning is successfull memory is guaranteed to be contiguous
 * in physical memory.
 *
 * @param cookie Cookie for the dma manager
 * @param addr Address of the memory to pin
 * @param size Range of memory to pin
 *
 * @return 0 if memory could not be pinned, otherwise physical address
 */
typedef uintptr_t (*ps_dma_pin_fn_t)(void *cookie, void *addr, size_t size);

/**
 * Unpin a piece of memory. You should only unpin the exact same range
 * that was pinned, do not partially unpin a range or unpin memory that
 * was never pinned.
 *
 * @param cookie Cookie for the dma manager
 * @param addr Address of the memory to unpin
 * @param size Range of the memory to unpin
 */
typedef void (*ps_dma_unpin_fn_t)(void *cookie, void *addr, size_t size);

/**
 * Perform a cache operation on a dma memory region. Pinned and unpinned
 * memory can have cache operations performed on it
 *
 * @param cookie Cookie for the dma manager
 * @param addr Start address to perform the cache operation on
 * @param size Size of the range to perform the cache operation on
 * @param op Cache operation to perform
 */
typedef void (*ps_dma_cache_op_fn_t)(void *cookie, void *addr, size_t size, dma_cache_op_t op);

typedef struct ps_dma_man {
    void *cookie;
    ps_dma_alloc_fn_t dma_alloc_fn;
    ps_dma_free_fn_t dma_free_fn;
    ps_dma_pin_fn_t dma_pin_fn;
    ps_dma_unpin_fn_t dma_unpin_fn;
    ps_dma_cache_op_fn_t dma_cache_op_fn;
} ps_dma_man_t;

static inline void *
ps_dma_alloc(ps_dma_man_t *dma_man, size_t size, int align, int cache, ps_mem_flags_t flags)
{
    assert(dma_man);
    assert(dma_man->dma_alloc_fn);
    return dma_man->dma_alloc_fn(dma_man->cookie, size, align, cache, flags);
}

static inline void
ps_dma_free(ps_dma_man_t *dma_man, void *addr, size_t size)
{
    assert(dma_man);
    assert(dma_man->dma_free_fn);
    dma_man->dma_free_fn(dma_man->cookie, addr, size);
}

static inline uintptr_t
ps_dma_pin(ps_dma_man_t *dma_man, void *addr, size_t size)
{
    assert(dma_man);
    assert(dma_man->dma_pin_fn);
    return dma_man->dma_pin_fn(dma_man->cookie, addr, size);
}

static inline void
ps_dma_unpin(ps_dma_man_t *dma_man, void *addr, size_t size)
{
    assert(dma_man);
    assert(dma_man->dma_unpin_fn);
    dma_man->dma_unpin_fn(dma_man->cookie, addr, size);
}

static inline void
ps_dma_cache_op(ps_dma_man_t *dma_man, void *addr, size_t size, dma_cache_op_t op)
{
    assert(dma_man);
    assert(dma_man->dma_cache_op_fn);
    dma_man->dma_cache_op_fn(dma_man->cookie, addr, size, op);
}

static inline void
ps_dma_cache_clean(ps_dma_man_t *dma_man, void *addr, size_t size)
{
    ps_dma_cache_op(dma_man, addr, size, DMA_CACHE_OP_CLEAN);
}

static inline void
ps_dma_cache_invalidate(ps_dma_man_t *dma_man, void *addr, size_t size)
{
    ps_dma_cache_op(dma_man, addr, size, DMA_CACHE_OP_INVALIDATE);
}

static inline void
ps_dma_cache_clean_invalidate(ps_dma_man_t *dma_man, void *addr, size_t size)
{
    ps_dma_cache_op(dma_man, addr, size, DMA_CACHE_OP_CLEAN_INVALIDATE);
}

/* Struct to collect all the different I/O operations together. This should contain
 * everything a driver needs to function */
struct ps_io_ops {
    ps_io_mapper_t io_mapper;
    ps_io_port_ops_t io_port_ops;
    ps_dma_man_t dma_manager;
#ifdef ARCH_ARM
    clock_sys_t clock_sys;
    mux_sys_t mux_sys;
#endif
};

#endif /* __PLATSUPPORT_IO_H__ */
