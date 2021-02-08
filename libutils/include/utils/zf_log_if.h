/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <utils/zf_log.h>


/*  zf_logif.h:
 * This file contains some convenience macros built on top of the ZF_LOG
 * library, to reduce source size and improve single-line readability.
 *
 * ZF_LOG?_IF(condition, fmt, ...):
 *  These will call the relevant ZF_LOG?() macro if "condition" evaluates to
 *  true at runtime.
 *
 */

#define ZF_LOGD_IF(cond, fmt, ...) \
	if (cond) { ZF_LOGD("[Cond failed: %s]\n\t" fmt, #cond, ## __VA_ARGS__); }
#define ZF_LOGI_IF(cond, fmt, ...) \
	if (cond) { ZF_LOGI("[Cond failed: %s]\n\t" fmt, #cond, ## __VA_ARGS__); }
#define ZF_LOGW_IF(cond, fmt, ...) \
	if (cond) { ZF_LOGW("[Cond failed: %s]\n\t" fmt, #cond, ## __VA_ARGS__); }
#define ZF_LOGE_IF(cond, fmt, ...) \
	if (cond) { ZF_LOGE("[Cond failed: %s]\n\t" fmt, #cond, ## __VA_ARGS__); }
#define ZF_LOGF_IF(cond, fmt, ...) \
	if (cond) { ZF_LOGF("[Cond failed: %s]\n\t" fmt, #cond, ## __VA_ARGS__); }
