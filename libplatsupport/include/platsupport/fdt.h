/*
 * Copyright 2019, Data61
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

#include <libfdt.h>
#include <platsupport/irq.h>
#include <platsupport/io.h>
#include <platsupport/pmem.h>

/* We make an assumption that the size of the address/size cells are no larger than 2
 * RISCV does have 128 bits but there are no platforms that use that bit length (yet) */
#define READ_CELL32(addr) fdt32_ld(addr)
#define READ_CELL64(addr) fdt64_ld(addr)
#define READ_CELL(size, addr, offset) (size == 2 ? READ_CELL64(addr + (offset * sizeof(uint32_t))) : \
                                                   READ_CELL32(addr + (offset * sizeof(uint32_t))))

typedef int (*reg_walk_cb_fn_t)(pmem_region_t pmem, unsigned curr_num, size_t num_regs, void *token);

typedef int (*irq_walk_cb_fn_t)(ps_irq_t irq, unsigned curr_num, size_t num_irqs, void *token);

typedef struct ps_fdt_cookie ps_fdt_cookie_t;

int ps_fdt_read_path(ps_io_fdt_t *io_fdt, ps_malloc_ops_t *malloc_ops, char *path, ps_fdt_cookie_t **ret_cookie);

int ps_fdt_cleanup_cookie(ps_malloc_ops_t *malloc_ops, ps_fdt_cookie_t *cookie);

int ps_fdt_walk_registers(ps_io_fdt_t *io_fdt, ps_fdt_cookie_t *cookie, reg_walk_cb_fn_t callback, void *token);

int ps_fdt_walk_irqs(ps_io_fdt_t *io_fdt, ps_fdt_cookie_t *cookie, irq_walk_cb_fn_t callback, void *token);
