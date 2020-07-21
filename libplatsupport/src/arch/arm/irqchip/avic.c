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

#define FSL_AVIC_INT_CELL_COUNT 1

static int parse_fsl_avic_interrupts(char *dtb_blob, int node_offset, int intr_controller_phandle,
                                     irq_walk_cb_fn_t callback, void *token)
{
    bool is_extended = false;
    int prop_len = 0;
    const void *interrupts_prop = get_interrupts_prop(dtb_blob, node_offset, &is_extended, &prop_len);
    assert(interrupts_prop != NULL);
    int UNUSED total_cells = prop_len / sizeof(uint32_t);
    /* There's only one interrupt cell for this IRQ chip */
    assert(total_cells == FSL_AVIC_INT_CELL_COUNT);
    ps_irq_t irq = { .type = PS_INTERRUPT, .irq = { .number = READ_CELL(1, interrupts_prop, 0) }};
    int error = callback(irq, 0, FSL_AVIC_INT_CELL_COUNT, token);
    if (error) {
        return error;
    }
    return 0;
}

char *fsl_avic_compatible_list[] = {
    "fsl,imx31-avic",
    NULL
};
DEFINE_IRQCHIP_PARSER(fsl_avic, fsl_avic_compatible_list, parse_fsl_avic_interrupts);
