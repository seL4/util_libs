/*
 * Copyright 2020, Data61
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

#include <utils/util.h>

#include "../../../irqchip.h"

/*
 * Interrupt parser for the RISC-V PLIC.
 *
 * RISC-V PLIC interrupts specified as single-cell and each device node
 * may specify one more more interrupts that it uses.
 *
 * Most properties in the `interrupt-controller` is useless to us - the
 * kernel takes care of that.
 *
 * We just want to confirm that the `interrupt-controller` is specified as
 * we expect, and then return the interrupts specified in the device node.
 */

static int parse_riscv_plic_interrupts(char *dtb_blob, int node_offset, int intr_controller_phandle,
                                       irq_walk_cb_fn_t callback, void *token)
{
    bool is_extended = false;
    int prop_len = 0;
    const void *interrupts_prop = get_interrupts_prop(dtb_blob, node_offset, &is_extended, &prop_len);
    assert(interrupts_prop != NULL);
    int total_cells = prop_len / sizeof(uint32_t);

    if (is_extended) {
        ZF_LOGE("This parser doesn't understand extended interrupts");
        return ENODEV;
    }

    /* Make sure that the interrupt we are parsing is 1-cell format */
    int intr_controller_offset = fdt_node_offset_by_phandle(dtb_blob, intr_controller_phandle);
    if (intr_controller_offset < 0) {
        ZF_LOGE("Can't find the interrupt controller's node offset");
        return intr_controller_offset;
    }
    const void *interrupt_cells_prop = fdt_getprop(dtb_blob, intr_controller_offset, "#interrupt-cells", NULL);
    if (!interrupt_cells_prop) {
        ZF_LOGE("No '#interrupt-cells' property!");
        return ENODEV;
    }
    uint32_t num_interrupt_cells = READ_CELL(1, interrupt_cells_prop, 0);
    if (num_interrupt_cells != 1) {
        ZF_LOGE("This parser doesn't understand multi-cell interrupts");
        return ENODEV;
    }

    /* Loop through each cell and call the callback */
    for (int i = 0; i < total_cells; i++) {
        ps_irq_t irq = {
            .type = PS_INTERRUPT,
            .irq = {
                .number = READ_CELL(1, interrupts_prop, i)
            }
        };

        int error = callback(irq, i, total_cells, token);
        if (error) {
            return error;
        }
    }

    return 0;
}

char *riscv_plic_compatible_list[] = {
    "riscv,plic0",
    NULL
};
DEFINE_IRQCHIP_PARSER(riscv_plic, riscv_plic_compatible_list, parse_riscv_plic_interrupts);
