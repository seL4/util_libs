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

#include <platsupport/clock.h>

/*
 * This is here just to make the compiler happy. The actual clock driver is
 * located inside libplatsupportports in the projects_libs repository.
 */
clk_t *ps_clocks = NULL;
freq_t ps_freq_default = {0};
