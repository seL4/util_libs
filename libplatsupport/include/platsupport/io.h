/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <utils/util.h>
#include <sys/types.h>
#include <errno.h>
/* For clock.h and mux.h */
typedef struct ps_io_ops ps_io_ops_t;

#ifdef CONFIG_ARCH_ARM
#include <platsupport/clock.h>
#include <platsupport/mux.h>
#endif
#include <platsupport/irq.h>

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
ps_io_map(const ps_io_mapper_t *io_mapper, uintptr_t paddr, size_t size, int cached, ps_mem_flags_t flags)
{
    assert(io_mapper);
    assert(io_mapper->io_map_fn);
    return io_mapper->io_map_fn(io_mapper->cookie, paddr, size, cached, flags);
}

static inline void
ps_io_unmap(const ps_io_mapper_t *io_mapper, void *vaddr, size_t size)
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
ps_io_port_in(const ps_io_port_ops_t *port_ops, uint32_t port, int io_size,
              uint32_t *result)
{
    assert(port_ops);
    assert(port_ops->io_port_in_fn);
    return port_ops->io_port_in_fn(port_ops->cookie, port, io_size, result);
}

static inline int
ps_io_port_out(const ps_io_port_ops_t *port_ops, uint32_t port, int io_size,
               uint32_t val)
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
 * @param align Alignment in bytes of the dma region
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
 * pinned range. If pinning is successful memory is guaranteed to be contiguous
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
ps_dma_alloc(const ps_dma_man_t *dma_man, size_t size, int align, int cache,
             ps_mem_flags_t flags)
{
    assert(dma_man);
    assert(dma_man->dma_alloc_fn);
    return dma_man->dma_alloc_fn(dma_man->cookie, size, align, cache, flags);
}

static inline void
ps_dma_free(const ps_dma_man_t *dma_man, void *addr, size_t size)
{
    assert(dma_man);
    assert(dma_man->dma_free_fn);
    dma_man->dma_free_fn(dma_man->cookie, addr, size);
}

static inline uintptr_t
ps_dma_pin(const ps_dma_man_t *dma_man, void *addr, size_t size)
{
    assert(dma_man);
    assert(dma_man->dma_pin_fn);
    return dma_man->dma_pin_fn(dma_man->cookie, addr, size);
}

static inline void
ps_dma_unpin(const ps_dma_man_t *dma_man, void *addr, size_t size)
{
    assert(dma_man);
    assert(dma_man->dma_unpin_fn);
    dma_man->dma_unpin_fn(dma_man->cookie, addr, size);
}

static inline void
ps_dma_cache_op(const ps_dma_man_t *dma_man, void *addr, size_t size,
                dma_cache_op_t op)
{
    assert(dma_man);
    assert(dma_man->dma_cache_op_fn);
    dma_man->dma_cache_op_fn(dma_man->cookie, addr, size, op);
}

static inline void
ps_dma_cache_clean(const ps_dma_man_t *dma_man, void *addr, size_t size)
{
    ps_dma_cache_op(dma_man, addr, size, DMA_CACHE_OP_CLEAN);
}

static inline void
ps_dma_cache_invalidate(const ps_dma_man_t *dma_man, void *addr, size_t size)
{
    ps_dma_cache_op(dma_man, addr, size, DMA_CACHE_OP_INVALIDATE);
}

static inline void
ps_dma_cache_clean_invalidate(const ps_dma_man_t *dma_man, void *addr,
                              size_t size)
{
    ps_dma_cache_op(dma_man, addr, size, DMA_CACHE_OP_CLEAN_INVALIDATE);
}

/*
 * Allocate some heap memory for the driver to use. Basically malloc.
 *
 * @param cookie     Cookie for the allocator.
 * @param size       Amount of bytes to allocate.
 * @param[out] ptr   Pointer to store the result in.
 *
 * @return 0 on success, errno on error.
 */
typedef int (*ps_malloc_fn_t)(void *cookie, size_t size, void **ptr);

/*
 * Allocate and zero some heap memory for the driver to use. Basically calloc.
 *
 * @param cookie     Cookie for the allocator.
 * @param nmemb      Amount of element to allocate.
 * @param size       Size of each element in bytes.
 * @param[out] ptr   Pointer to store the result in.
 *
 * @return 0 on success, errno on error.
 */
typedef int (*ps_calloc_fn_t)(void *cookie, size_t nmemb, size_t size, void **ptr);

/*
 * Free allocated heap memory.
 *
 * @param ptr        Pointer previously returned by alloc or calloc.
 * @param size       Amount of bytes to free.
 * @param cookie     Cookie for the allocator.
 *
 * @return 0 on success, errno on error.
 */
typedef int (*ps_free_fn_t)(void *cookie, size_t size, void *ptr);

typedef struct {
    ps_malloc_fn_t malloc;
    ps_calloc_fn_t calloc;
    ps_free_fn_t free;
    void *cookie;
} ps_malloc_ops_t;

static inline int ps_malloc(const ps_malloc_ops_t *ops, size_t size,
                            void **ptr)
{
    if (ops == NULL) {
        ZF_LOGE("ops cannot be NULL");
        return EINVAL;
    }

    if (ops->malloc == NULL) {
        ZF_LOGE("not implemented");
        return ENOSYS;
    }

    if (size == 0) {
        /* nothing to do */
        ZF_LOGW("called with size 0");
        return 0;
    }

    if (ptr == NULL) {
        ZF_LOGE("ptr cannot be NULL");
        return EINVAL;
    }

    return ops->malloc(ops->cookie, size, ptr);
}

static inline int ps_calloc(const ps_malloc_ops_t *ops, size_t nmemb,
                            size_t size, void **ptr)
{

    if (ops == NULL) {
        ZF_LOGE("ops cannot be NULL");
        return EINVAL;
    }

    if (ops->calloc == NULL) {
        ZF_LOGE("not implemented");
        return ENOSYS;
    }

    if (size == 0 || nmemb == 0) {
        /* nothing to do */
        ZF_LOGW("called no bytes to allocate");
        return 0;
    }

    if (ptr == NULL) {
        ZF_LOGE("ptr cannot be NULL");
        return EINVAL;
    }

    return ops->calloc(ops->cookie, nmemb, size, ptr);
}

static inline int ps_free(const ps_malloc_ops_t *ops, size_t size, void *ptr)
{
    if (ops == NULL) {
        ZF_LOGE("ops cannot be NULL");
        return EINVAL;
    }

    if (ops->free == NULL) {
        ZF_LOGE("not implemented");
        return ENOSYS;
    }

    if (ptr == NULL) {
        ZF_LOGE("ptr cannot be NULL");
        return EINVAL;
    }

    return ops->free(ops->cookie, size, ptr);
}

/*
 * Retrieves a copy of the FDT.
 *
 * @param cookie     Cookie for the FDT interface.
 *
 * @return A pointer to a FDT object, NULL on error.
 */
typedef char *(*ps_io_fdt_get_fn_t)(void *cookie);

typedef struct ps_fdt {
    void *cookie;
    ps_io_fdt_get_fn_t get_fn;
} ps_io_fdt_t;

static inline char *ps_io_fdt_get(const ps_io_fdt_t *io_fdt)
{
    if (io_fdt == NULL) {
        ZF_LOGE("fdt cannot be NULL");
        return NULL;
    }

    if (io_fdt->get_fn == NULL) {
        ZF_LOGE("not implemented");
        return NULL;
    }

    return io_fdt->get_fn(io_fdt->cookie);
}

/* Struct to collect all the different I/O operations together. This should contain
 * everything a driver needs to function */
struct ps_io_ops {
    ps_io_mapper_t io_mapper;
    ps_io_port_ops_t io_port_ops;
    ps_dma_man_t dma_manager;
    ps_io_fdt_t io_fdt;
#ifdef CONFIG_ARCH_ARM
    clock_sys_t clock_sys;
    mux_sys_t mux_sys;
#endif
    ps_malloc_ops_t malloc_ops;
    ps_irq_ops_t irq_ops;
};

/**
 * In place reads/writes of device register bitfields
 *
 * eg, where var = 0x12345678
 *
 * read_masked(&var, 0x0000FFFF) ==> 0x00005678
 * write_masked(&var, 0x0000FFFF, 0x000000CC) ==> var = 0x123400CC
 * read_bits(&var, 8, 4) ==> 0x6
 * write_bits(&var, 8, 4, 0xC) ==> var = 0x12345C78
 */
static inline uint32_t
read_masked(volatile uint32_t* addr, uint32_t mask)
{
    assert(addr);
    return *addr & mask;
}

static inline void
write_masked(volatile uint32_t* addr, uint32_t mask, uint32_t value)
{
    assert(addr);
    assert((value & mask) == value);
    *addr = read_masked(addr, ~mask) | value;
}

static inline uint32_t
read_bits(volatile uint32_t* addr, unsigned int first_bit, unsigned int nbits)
{
    assert(addr);
    assert(first_bit < 32);
    assert(nbits <= 32 - first_bit);
    return (*addr >> first_bit) & MASK(nbits);
}

static inline void
write_bits(volatile uint32_t* addr, unsigned int first_bit, unsigned int nbits, uint32_t value)
{
    assert(addr);
    assert(first_bit < 32);
    assert(nbits <= 32 - first_bit);
    write_masked(addr, MASK(nbits) << first_bit, value << first_bit);
}

/*
 * Populate a malloc ops with stdlib malloc wrappers.
 */
int ps_new_stdlib_malloc_ops(ps_malloc_ops_t *ops);
