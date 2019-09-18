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

#define DEFINE_IRQCHIP_PARSER(instance, compatible_strings, parser_func)  \
    static ps_irqchip_t instance = {                                      \
        .compatible_list = compatible_strings,                            \
        .parser_fn = parser_func                                          \
    };                                                                    \
    USED SECTION("_ps_irqchips") ps_irqchip_t *instance##_ptr = &instance

typedef int (*ps_irqchip_parse_fn_t)(char *dtb_blob, int node_offset, int intr_controller_phandle,
                                     irq_walk_cb_fn_t callback, void *token);

typedef struct ps_irqchip {
    char **compatible_list;
    ps_irqchip_parse_fn_t parser_fn;
} ps_irqchip_t;

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
