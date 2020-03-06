/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <assembler.h>

.text

BEGIN_FUNC(psci_func)
    b abort
END_FUNC(psci_func)
