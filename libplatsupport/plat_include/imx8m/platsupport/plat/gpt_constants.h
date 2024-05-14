/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#ifdef USE_IMX8MP_GPT_PATH
#define GPT1_PATH "/soc@0/bus@30000000/timer@302d0000"
#define GPT2_PATH "/soc@0/bus@30000000/timer@302e0000"
#else
#define GPT1_PATH "/gpt@302d0000"
#define GPT2_PATH "/gpt@302e0000"
#endif

#define GPT_PRESCALER       (0)

