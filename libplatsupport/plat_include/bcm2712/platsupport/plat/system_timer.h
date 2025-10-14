/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <utils/util.h>

/*
 * The Device Tree by default for BCM2712 will contain the timer IRQs
 * for each channel. Channels 0 and 2 are used by the VideoCore, so
 * we use 1 instead.
 */
#define BCM_SYSTEM_TIMER_CHANNEL    (1)
#define BCM_SYSTEM_TIMER_FREQ       (MHZ)
#define BCM_SYSTEM_TIMER_FDT_PATH   "/soc@107c000000/timer@7c003000"
#define BCM_SYSTEM_TIMER_REG_CHOICE (0)
#define BCM_SYSTEM_TIMER_IRQ_CHOICE (1)
