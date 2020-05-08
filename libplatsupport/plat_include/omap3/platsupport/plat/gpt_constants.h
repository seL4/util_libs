/*
 * Copyright 2017, Data61
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

#define GPT1_DEVICE_PATH    "/ocp@68000000/timer@48318000"
#define GPT2_DEVICE_PATH    "/ocp@68000000/timer@49032000"

/* confiugured by uboot. This is only correct for GPT1-9 */
#define CLK_HZ 13000000llu
#define CLK_MHZ (CLK_HZ / US_IN_S)
