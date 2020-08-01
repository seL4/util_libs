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

#include <utils/util.h>

#include "../../../irqchip.h"

#define ARM_GIC_INT_CELL_COUNT 3

#define SPI_IRQ_TYPE 0
#define PPI_IRQ_TYPE 1

/* These offsets are in the context of the normal 'interrupts' property,
 * for the 'interrupts-extended' property, we add one to each */
typedef enum {
    INT_TYPE_OFFSET,
    INT_OFFSET
} interrupt_cell_offset;

typedef enum {
    EXT_INT_CONTROLLER_OFFSET,
    EXT_INT_TYPE_OFFSET,
    EXT_INT_OFFSET
} ext_interrupt_cell_offset;

/* Note for extended interrupts, we expect the common case that the interrupt controller phandles
 * for each block in the property is the same as the GICs phandle */
static int parse_arm_gic_interrupts(char *dtb_blob, int node_offset, int intr_controller_phandle,
                                    irq_walk_cb_fn_t callback, void *token)
{
    bool is_extended = false;
    int prop_len = 0;
    const void *interrupts_prop = get_interrupts_prop(dtb_blob, node_offset, &is_extended, &prop_len);
    assert(interrupts_prop != NULL);
    int total_cells = prop_len / sizeof(uint32_t);

    int stride = ARM_GIC_INT_CELL_COUNT + (is_extended == true);
    int num_interrupts = total_cells / stride;

    assert(total_cells % stride == 0);

    for (int i = 0; i < num_interrupts; i++) {
        ps_irq_t curr_irq = {0};
        const void *curr = interrupts_prop + (i * stride * sizeof(uint32_t));
        curr_irq.type = PS_INTERRUPT;
        uint32_t irq_type = 0;
        uint32_t irq = 0;
        if (is_extended) {
            if (READ_CELL(1, curr, EXT_INT_CONTROLLER_OFFSET) != intr_controller_phandle) {
                /* Bail, we hit the uncommon case where this device has interrupts routed to
                 * different interrupt controllers */
                return -EINVAL;
            }
            irq_type = READ_CELL(1, curr, EXT_INT_TYPE_OFFSET);
            irq = READ_CELL(1, curr, EXT_INT_OFFSET);
        } else {
            irq_type = READ_CELL(1, curr, INT_TYPE_OFFSET);
            irq = READ_CELL(1, curr, INT_OFFSET);
        }
        curr_irq.irq.number = irq + (irq_type == SPI_IRQ_TYPE ? 32 : 0);
        int error = callback(curr_irq, i, num_interrupts, token);
        if (error) {
            return error;
        }
    }

    return 0;
}

char *arm_gic_compatible_list[] = {
    "arm,gic-400",
    "arm,cortex-a9-gic",
    "arm,cortex-a15-gic",
    NULL
};
DEFINE_IRQCHIP_PARSER(arm_gic, arm_gic_compatible_list, parse_arm_gic_interrupts);
