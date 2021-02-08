/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* seconds */
#define SEC_IN_MINUTE 60llu
#define NS_IN_MINUTE (SEC_IN_MINUTE*NS_IN_S)

/* milliseconds */
#define MS_IN_S 1000llu

/* microseconds */
#define US_IN_MS 1000llu
#define US_IN_S  1000000llu

/* nanoseconds */
#define NS_IN_US 1000llu
#define NS_IN_MS 1000000llu
#define NS_IN_S  1000000000llu

/* picoseconds */
#define PS_IN_NS 1000llu
#define PS_IN_US 1000000llu
#define PS_IN_MS 1000000000llu
#define PS_IN_S  1000000000000llu

/* femptoseconds */
#define FS_IN_PS 1000llu
#define FS_IN_NS 1000000llu
#define FS_IN_US 1000000000llu
#define FS_IN_MS 1000000000000llu
#define FS_IN_S  1000000000000000llu
