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

#define SPI_INTERRUPTS 0
#define PPI_INTERRUPTS 1
#define EXTENDED_SPI_INTERRUPTS 2
#define EXTENDED_PPI_INTERRUPTS 3

#define PPI_START 16
#define SPI_START 32

#define INT_FLAG_EDGE_TRIGGERED 1
#define INT_FLAG_LEVEL_TRIGGERED 4

/* The Linux documentation states that the #interrupt-cells property should be either 3 or 4 */
#define INTERRUPT_CELL_COUNT 3
#define INTERRUPT_AFFINITY_CELL_COUNT 4

/* These offsets are in the context of the normal 'interrupts' property,
 * for the 'interrupts-extended' property, we add one to each */
typedef enum {
    INT_TYPE_OFFSET,
    INT_OFFSET,
    INT_FLAG_OFFSET,
    INT_AFFINITY_OFFSET,
} interrupt_cell_offset;

typedef enum {
    EXT_INT_CONTROLLER_OFFSET,
    EXT_INT_TYPE_OFFSET,
    EXT_INT_OFFSET,
    EXT_INT_FLAG_OFFSET,
    EXT_INT_AFFINITY_OFFSET
} ext_interrupt_cell_offset;

/* Note for extended interrupts, we expect the common case that the interrupt controller phandles
 * for each block in the property is the same as the v3 GIC's phandle */
static int parse_arm_gicv3_interrupts(char *dtb_blob, int node_offset, int intr_controller_phandle,
                                      irq_walk_cb_fn_t callback, void *token)
{
    bool is_extended = false;
    int prop_len = 0;
    const void *interrupts_prop = get_interrupts_prop(dtb_blob, node_offset, &is_extended, &prop_len);
    assert(interrupts_prop != NULL);
    int total_cells = prop_len / sizeof(uint32_t);

    /*
     * Check for the number of interrupt cells, two cases: either 3 or 4. The extra cell describes PPI
     * affinity if the interrupt is a PPI interrupt.
     */
    int intr_controller_offset = fdt_node_offset_by_phandle(dtb_blob, intr_controller_phandle);
    if (intr_controller_offset < 0) {
        ZF_LOGE("Can't find the interrupt controller's node offset");
        return intr_controller_offset;
    }
    const void *interrupt_cells_prop = fdt_getprop(dtb_blob, intr_controller_offset, "#interrupt-cells", NULL);
    if (!interrupt_cells_prop) {
        ZF_LOGE("No '#interrupt-cells' property!");
        return -EINVAL;
    }
    uint32_t num_interrupt_cells = READ_CELL(1, interrupt_cells_prop, 0);
    if (num_interrupt_cells != INTERRUPT_CELL_COUNT && num_interrupt_cells != INTERRUPT_AFFINITY_CELL_COUNT) {
        ZF_LOGE("This GICv3 interrupt controller has an invalid interrupt cell count!");
        return -EINVAL;
    }

    int stride = num_interrupt_cells + (is_extended == true);
    int num_interrupts = total_cells / stride;

    assert(total_cells % stride == 0);

    for (int i = 0; i < num_interrupts; i++) {
        ps_irq_t curr_irq = {0};
        const void *curr = interrupts_prop + (i * stride * sizeof(uint32_t));
        uint32_t irq_type = 0;
        uint32_t irq = 0;
        uint32_t UNUSED irq_flag = 0;
        uint32_t UNUSED irq_core_affinity_phandle = 0;

        if (is_extended) {
            if (READ_CELL(1, curr, EXT_INT_CONTROLLER_OFFSET) != intr_controller_phandle) {
                /* Bail, we hit the uncommon case where this device has interrupts routed to
                 * different interrupt controllers */
                return -EINVAL;
            }
            irq_type = READ_CELL(1, curr, EXT_INT_TYPE_OFFSET);
            irq = READ_CELL(1, curr, EXT_INT_OFFSET);
            irq_flag = READ_CELL(1, curr, EXT_INT_FLAG_OFFSET);
            if (num_interrupt_cells == INTERRUPT_AFFINITY_CELL_COUNT) {
                irq_core_affinity_phandle = READ_CELL(1, curr, EXT_INT_AFFINITY_OFFSET);
            }
        } else {
            irq_type = READ_CELL(1, curr, INT_TYPE_OFFSET);
            irq = READ_CELL(1, curr, INT_OFFSET);
            irq_flag = READ_CELL(1, curr, INT_FLAG_OFFSET);
            if (num_interrupt_cells == INTERRUPT_CELL_COUNT) {
                irq_core_affinity_phandle = READ_CELL(1, curr, INT_AFFINITY_OFFSET);
            }
        }

        /* Assumption: The parser currently treats the extended SPI/PPI interrupts as the same
         * as the normal interrupts */
        if (irq_type == SPI_INTERRUPTS || irq_type == EXTENDED_SPI_INTERRUPTS) {
            curr_irq.type = PS_INTERRUPT;
            curr_irq.irq.number = irq + SPI_START;
        } else if (irq_type == PPI_INTERRUPTS || irq_type == EXTENDED_PPI_INTERRUPTS) {
            /* TODO Parse PPI interrupts */
            ZF_LOGE("Found an PPI interrupt in the GICv3 parser, skipping");
        } else {
            ZF_LOGE("Invalid IRQ type");
            return -EINVAL;
        }

        int error = callback(curr_irq, i, num_interrupts, token);
        if (error) {
            return error;
        }
    }

    return 0;
}

char *arm_gicv3_compatible_list[] = {
    "arm,gic-v3",
    NULL
};
DEFINE_IRQCHIP_PARSER(arm_gicv3, arm_gicv3_compatible_list, parse_arm_gicv3_interrupts);
