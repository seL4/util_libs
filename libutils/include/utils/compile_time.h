/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <assert.h>

/* 201112L indicates C11 */
#if __STDC_VERSION__ < 201112L && !defined(__cplusplus)
/* Use gcc extension for older C standards. */
#define compile_time_assert(name, expr) _Static_assert((expr), #name)
#else
#define compile_time_assert(name, expr) static_assert((expr), #name)
#endif
