/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define GPT1_DEVICE_PATH    "/ocp@68000000/timer@48318000"
#define GPT2_DEVICE_PATH    "/ocp@68000000/timer@49032000"

/* confiugured by uboot. This is only correct for GPT1-9 */
#define CLK_HZ 13000000llu
#define CLK_MHZ (CLK_HZ / US_IN_S)
