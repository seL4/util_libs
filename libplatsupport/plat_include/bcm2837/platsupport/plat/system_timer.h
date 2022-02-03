/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2022, Technology Innovation Institute

 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <utils/util.h>

#define BCM_SYSTEM_TIMER_CHANNEL    (1)
#define BCM_SYSTEM_TIMER_FREQ       (MHZ)
#define BCM_SYSTEM_TIMER_FDT_PATH   "/soc/timer@7e003000"
#define BCM_SYSTEM_TIMER_REG_CHOICE (0)
/* Note: Assumes device tree doesn't have IRQ entries for channels
 * 0 and 2 which are used by VideoCore.
 */
#define BCM_SYSTEM_TIMER_IRQ_CHOICE (0)
