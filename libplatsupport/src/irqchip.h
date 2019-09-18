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
#include <platsupport/fdt.h>
#include <utils/util.h>

/* Macro to streamline process of declaring new interrupt parsing modules */
#define DEFINE_IRQCHIP_PARSER(instance, compatible_strings, parser_func)  \
    static ps_irqchip_t instance = {                                      \
        .compatible_list = compatible_strings,                            \
        .parser_fn = parser_func                                          \
    };                                                                    \
    USED SECTION("_ps_irqchips") ps_irqchip_t *instance##_ptr = &instance

/*
 * Expected function type of the interrupt parsing functions.
 *
 * Given offset of the device node in the FDT, this should read the interrupts
 * property, parse it and call the callback function on each interrupt instance
 * of the property.
 *
 * @param dtb_blob A blob of a platform's FDT.
 * @param node_offset Offset to the device node to be parsed.
 * @param intr_controller_phandle Phandle to the interrupt controller that handles the interrupts of the device.
 * @param callback Pointer to a callback function that is called at each interrupt instance in the property.
 * @param token Pointer to a token that is passed into the callback each time it is called.
 *
 * @returns 0 on success, otherwise an error code
 */
typedef int (*ps_irqchip_parse_fn_t)(char *dtb_blob, int node_offset, int intr_controller_phandle,
                                     irq_walk_cb_fn_t callback, void *token);

/*
 * Struct describing a IRQ parser module.
 */
typedef struct ps_irqchip {
    /* Array of compatibility strings for interrupt
     * controllers that the module can handle, should be NULL terminated, e.g.
     * foo = { "abc", NULL } */
    char **compatible_list;
    /* Pointer to the parser function for this module */
    ps_irqchip_parse_fn_t parser_fn;
} ps_irqchip_t;

/*
 * Helper function that retrieves a pointer to the interrupts property of a device.
 *
 * @param dtb_blob A blob of a platform's FDT.
 * @param node_offset Offset to the desired device node.
 * @param is_extended Pointer to a boolean that will have true written to it if the 'interrupts-extended' property was found, false otherwise
 * @param prop_len Pointer to an int that will have the total length of the interrupts property written to it
 */
static inline const void *get_interrupts_prop(char *dtb_blob, int node_offset, bool *is_extended, int *prop_len)
{
    ZF_LOGF_IF(!is_extended || !prop_len, "Ret args are NULL!");
    const void *interrupts_prop = fdt_getprop(dtb_blob, node_offset, "interrupts-extended", prop_len);
    if (interrupts_prop) {
        *is_extended = true;
        return interrupts_prop;
    }

    interrupts_prop = fdt_getprop(dtb_blob, node_offset, "interrupts", prop_len);
    if (interrupts_prop) {
        *is_extended = false;
        return interrupts_prop;
    }

    return NULL;
}
