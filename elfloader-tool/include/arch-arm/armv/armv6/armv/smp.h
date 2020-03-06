/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <autoconf.h>
#include <elfloader/gen_config.h>

#if CONFIG_MAX_NUM_NODES > 1

#error SMP is not supported on ARMv6

#endif
