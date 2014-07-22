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

#include <utils/arith.h>
#include <utils/assume.h>
#include <utils/attribute.h>
#include <utils/builtin.h>
#include <utils/compile_time.h>
#include <utils/debug.h>
#include <utils/formats.h>
#include <utils/page.h>
#include <utils/stringify.h>
#include <utils/stack.h>
#include <utils/time.h>

#ifdef NDEBUG
#define LOG_ERROR(format, ...)
#define LOG_INFO(format, ...)
#else
#define LOG_ERROR(format, ...) printf("ERROR:%s:%d: "format"\n", __func__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(format, ...)  printf("INFO :%s:%d: "format"\n", __func__, __LINE__, ##__VA_ARGS__)
#endif /* NDEBUG */

#endif /* __UTILS_H */
