/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platsupport/clock.h>

/*
 * This is here just to make the compiler happy. The actual clock driver is
 * located inside libplatsupportports in the projects_libs repository.
 */
clk_t *ps_clocks = NULL;
freq_t ps_freq_default = {0};
