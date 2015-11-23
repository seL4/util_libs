/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __UTILS_H
#define __UTILS_H

#include <assert.h>
#include <autoconf.h>
#include <stdbool.h>

#include <utils/arith.h>
#include <utils/assume.h>
#include <utils/attribute.h>
#include <utils/builtin.h>
#include <utils/compile_time.h>
#include <utils/debug.h>
#include <utils/formats.h>
#include <utils/list.h>
#include <utils/page.h>
#include <utils/print.h>
#include <utils/sglib.h>
#include <utils/stringify.h>
#include <utils/stack.h>
#include <utils/time.h>
#include <utils/math.h>
#include <utils/ansi.h>
#include <utils/xml.h>

#ifndef ZF_LOG_LEVEL
#define ZF_LOG_LEVEL ZF_LOG_ERROR
#endif /* ZF_LOG_LEVEL */

#include <utils/zf_log.h>

/* deprecated, use the following instead:
 *
 * ZF_LOGV -- verbose
 * ZF_LOGD -- debug
 * ZF_LOGI -- info
 * ZF_LOGW -- warning
 * ZF_LOGE -- error
 * ZF_LOGF -- fatal
 * 
 * setting ZF_LOG_LEVEL to ZF_LOG_VERBOSE will display 
 * all ZF_LOG output, settings it to ZF_LOG_FATAL will 
 * only display fatal outputs. 
 */
#define LOG_ERROR(args...) ZF_LOGE(args)
#define LOG_INFO(args...) ZF_LOGI(args)

#endif /* __UTILS_H */
