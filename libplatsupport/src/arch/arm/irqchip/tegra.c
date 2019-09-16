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
#include <string.h>
#include <stdbool.h>

#include <utils/util.h>

#include "../../../irqchip.h"

/* Force the _allocated_irqs section to be created even if no modules are defined. */
static USED SECTION("_ps_irqchips") struct {} dummy_ps_irqchips;
/* Definitions so that we can find the exposed IRQ information */
extern ps_irqchip_t *__start__ps_irqchips[];
extern ps_irqchip_t *__stop__ps_irqchips[];

#define ARM_GIC_COMPAT_STRING "arm,cortex-a9-gic"
#define ARM_GIC_COMPAT_STRLEN sizeof(ARM_GIC_COMPAT_STRING)

static int parse_tegra_ictlr_interrupts(char *dtb_blob, int node_offset, int intr_controller_phandle,
                                        irq_walk_cb_fn_t callback, void *token)
{
    /* Look for the ARM GIC parser module and call their function.
     *
     * The tegra interrupt controllers have the same encodings as the ARM GIC, they just forward any
     * interrupts to the GIC. */
    for (ps_irqchip_t **irqchip = __start__ps_irqchips; irqchip < __stop__ps_irqchips; irqchip++) {
        for (char **compatible_str = (*irqchip)->compatible_list; *compatible_str != NULL; compatible_str++) {
            /* To account for the NULL byte */
            size_t compat_str_len = strlen(*compatible_str) + 1;
            size_t compare_length = (compat_str_len < ARM_GIC_COMPAT_STRLEN) ? compat_str_len
                                    : ARM_GIC_COMPAT_STRLEN;
            if (strncmp(ARM_GIC_COMPAT_STRING, *compatible_str, compare_length)) {
                return (*irqchip)->parser_fn(dtb_blob, node_offset, intr_controller_phandle, callback, token);
            }
        }
    }
    ZF_LOGE("Couldn't find the ARM GIC parser module!");
    return -EINVAL;
}

char *tegra_ictlr_compatible_list[] = {
    "nvidia,tegra210-ictlr",
    "nvidia,tegra124-ictlr",
    "nvidia,tegra30-ictlr",
    NULL
};
DEFINE_IRQCHIP_PARSER(tegra_ictlr, tegra_ictlr_compatible_list, parse_tegra_ictlr_interrupts);
