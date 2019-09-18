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

#include <assert.h>
#include <stdbool.h>

#include <platsupport/fdt.h>

#include "irqchip.h"

/* Force the _allocated_irqs section to be created even if no modules are defined. */
static USED SECTION("_ps_irqchips") struct {} dummy_ps_irqchips;
/* Definitions so that we can find the exposed IRQ information */
extern ps_irqchip_t *__start__ps_irqchips[];
extern ps_irqchip_t *__stop__ps_irqchips[];

/* Private internal struct */
struct ps_fdt_cookie {
    int node_offset;
};

int ps_fdt_read_path(ps_io_fdt_t *io_fdt, ps_malloc_ops_t *malloc_ops, char *path, ps_fdt_cookie_t **ret_cookie)
{
    if (!path || !ret_cookie) {
        return -EINVAL;
    }

    int error = ps_calloc(malloc_ops, 1, sizeof(**ret_cookie), (void **) ret_cookie);
    if (error) {
        return -ENOMEM;
    }

    char *dtb_blob = ps_io_fdt_get(io_fdt);
    if (!dtb_blob) {
        ZF_LOGF_IF(ps_fdt_cleanup_cookie(malloc_ops, *ret_cookie), "Failed to cleanup FDT cookie");
        return -EINVAL;
    }

    int node_offset = fdt_path_offset(dtb_blob, path);
    if (node_offset < 0) {
        ZF_LOGF_IF(ps_fdt_cleanup_cookie(malloc_ops, *ret_cookie), "Failed to cleanup FDT cookie");
        return node_offset;
    }

    (*ret_cookie)->node_offset = node_offset;

    return 0;
}

int ps_fdt_cleanup_cookie(ps_malloc_ops_t *malloc_ops, ps_fdt_cookie_t *cookie)
{
    if (!malloc_ops || !cookie) {
        return -EINVAL;
    }

    return ps_free(malloc_ops, sizeof(*cookie), cookie);
}

int ps_fdt_walk_registers(ps_io_fdt_t *io_fdt, ps_fdt_cookie_t *cookie, reg_walk_cb_fn_t callback, void *token)
{
    if (!io_fdt || !callback || !cookie) {
        return -EINVAL;
    }

    char *dtb_blob = ps_io_fdt_get(io_fdt);
    if (!dtb_blob) {
        return -EINVAL;
    }

    int node_offset = cookie->node_offset;

    /* NOTE: apparently fdt_parent_offset is expensive to use,
     * maybe manipulate the path to get the parent's node path instead? */
    int parent_offset = fdt_parent_offset(dtb_blob, node_offset);
    if (parent_offset < 0) {
        return parent_offset;
    }

    /* get the number of address and size cells */
    int num_address_cells = fdt_address_cells(dtb_blob, parent_offset);
    if (num_address_cells < 0) {
        return num_address_cells;
    }
    int num_size_cells = fdt_address_cells(dtb_blob, parent_offset);
    if (num_size_cells < 0) {
        return num_size_cells;
    }

    if (num_address_cells == 0 || num_size_cells == 0) {
        /* this isn't really a standard device, so we just return */
        return -FDT_ERR_NOTFOUND;
    }

    int prop_len = 0;
    const void *reg_prop = fdt_getprop(dtb_blob, node_offset, "reg", &prop_len);
    if (!reg_prop) {
        /* The error is written to the variable passed in */
        return prop_len;
    }
    int total_cells = prop_len / sizeof(uint32_t);

    /* sanity check */
    assert(total_cells % (num_address_cells + num_size_cells) == 0);

    int stride = num_address_cells + num_size_cells;
    int num_regs = total_cells / stride;

    for (int i = 0; i < num_regs; i++) {
        pmem_region_t curr_pmem = {0};
        const void *curr = reg_prop + (i * stride * sizeof(uint32_t));
        curr_pmem.type = PMEM_TYPE_DEVICE;
        curr_pmem.base_addr = READ_CELL(num_address_cells, curr, 0);
        curr_pmem.length = READ_CELL(num_size_cells, curr, num_address_cells);
        int error = callback(curr_pmem, i, num_regs, token);
        if (error) {
            return error;
        }
    }

    return 0;
}

static inline ps_irqchip_t **find_compatible_irq_controller(char *dtb_blob, int intr_controller_offset)
{
    for (ps_irqchip_t **irqchip = __start__ps_irqchips; irqchip < __stop__ps_irqchips; irqchip++) {
        for (char **compatible_str = (*irqchip)->compatible_list; *compatible_str != NULL; compatible_str++) {
            if (fdt_node_check_compatible(dtb_blob, intr_controller_offset, *compatible_str) == 0) {
                return irqchip;
            }
        }
    }

    return NULL;
}

int ps_fdt_walk_irqs(ps_io_fdt_t *io_fdt, ps_fdt_cookie_t *cookie, irq_walk_cb_fn_t callback, void *token)
{
    if (!io_fdt || !callback || !cookie) {
        return -EINVAL;
    }

    char *dtb_blob = ps_io_fdt_get(io_fdt);
    if (!dtb_blob) {
        return -EINVAL;
    }

    int node_offset = cookie->node_offset;

    /* check that this node actually has interrupts */
    const void *intr_addr = fdt_getprop(dtb_blob, node_offset, "interrupts", NULL);
    if (!intr_addr) {
        intr_addr = fdt_getprop(dtb_blob, node_offset, "interrupts-extended", NULL);
        if (!intr_addr) {
            return -FDT_ERR_NOTFOUND;
        }
    }

    /* NOTE: apparently fdt_parent_offset is expensive to use,
     * maybe manipulate the path to get the parent's node path instead? */
    int parent_offset = fdt_parent_offset(dtb_blob, node_offset);
    if (parent_offset < 0) {
        return parent_offset;
    }

    /* get the interrupt controller of the node */
    int curr_offset = parent_offset;
    bool found_controller = false;
    const void *intr_parent_prop;
    while (curr_offset >= 0 && !found_controller) {
        intr_parent_prop = fdt_getprop(dtb_blob, curr_offset, "interrupt-parent", NULL);
        if (!intr_parent_prop) {
            /* move up a level
             * NOTE: fdt_parent_offset is apparently expensive to use,
             * maybe avoid using fdt_parent_offset and manipulate the path instead? */
            curr_offset = fdt_parent_offset(dtb_blob, curr_offset);
        } else {
            found_controller = true;
        }
    }

    if (!found_controller) {
        /* something is really wrong with the FDT */
        return -FDT_ERR_BADSTRUCTURE;
    }

    uint32_t intr_controller_phandle = READ_CELL(1, intr_parent_prop, 0);
    ZF_LOGF_IF(intr_controller_phandle == 0,
               "Failed to get the phandle of the interrupt controller of this node");
    int intr_controller_offset = fdt_node_offset_by_phandle(dtb_blob, intr_controller_phandle);
    ZF_LOGF_IF(intr_controller_offset < 0, "Failed to get the offset of the interrupt controller");

    /* check the compatible string against our list of support interrupt controllers */
    ps_irqchip_t **irqchip = find_compatible_irq_controller(dtb_blob, intr_controller_offset);
    if (irqchip == NULL) {
        ZF_LOGE("Could not find a parser for this particular interrupt controller");
        return -ENOENT;
    }

    /* delegate to the interrupt controller specific code */
    int error = (*irqchip)->parser_fn(dtb_blob, node_offset, intr_controller_phandle, callback, token);
    if (error) {
        ZF_LOGE("Failed to parse and walk the interrupt field");
        return error;
    }

    return 0;
}
