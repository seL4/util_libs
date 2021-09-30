/*
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/mach/mailbox.h>

// The actual mailbox base address is 0x3F00B880 but mappings have to be page
// aligned (actually DMA_PAGE_SIZE aligned).
#define MAILBOX_PADDR       0x3f00b000
#define MAILBOX_SIZE        0x1000
