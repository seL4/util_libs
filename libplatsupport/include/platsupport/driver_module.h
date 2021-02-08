/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <utils/util.h>
#include <platsupport/io.h>

/*
 * These macros are used to check if compat_list or init_func
 * passed to PS_DRIVER_MODULE_DEFINE are not NULL or 0
 */
#define PS_INVALID_ADDR_0 0,
#define PS_INVALID_ADDR_NULL 0,
#define __third_arg(_, __, val, ...) val
#define IS_NULL(compat_list, init_func) \
    __IS_NULL(PS_INVALID_##compat_list, PS_INVALID_##init_func)
// arg1 and arg2 will expand to real argument only when the pointers are NULL or 0
// thus selecting the thrid arg will tell us whether one of them is NULL
#define __IS_NULL(arg1, arg2) __third_arg(arg1 arg2 INVALID, INVALID, VALID)

#define PS_DRIVER_MODULE_DEFINE(name, compat_list, init_func)                   \
    __PS_DRIVER_MODULE_DEFINE(IS_NULL(ADDR_##compat_list, ADDR_##init_func),    \
            name, compat_list, init_func)
#define __PS_DRIVER_MODULE_DEFINE(null, name, compat_list, init_func)           \
    ____PS_DRIVER_MODULE_DEFINE(null, name, compat_list, init_func)
#define ____PS_DRIVER_MODULE_DEFINE(null, name, compat_list, init_func)         \
    PS_DRIVER_MODULE_DEFINE_##null(name, compat_list, init_func)
#define PS_DRIVER_MODULE_DEFINE_INVALID(name, compat_list, init_func)           \
    "compat_list and init_func must not be NULL"
#define PS_DRIVER_MODULE_DEFINE_VALID(name, compat_list, init_func)             \
    static ps_driver_module_t name = {                                          \
        .compatible_list = compat_list,                                         \
        .init = init_func                                                       \
    };                                                                          \
    USED SECTION("_driver_modules") ps_driver_module_t *name##_ptr = &name;

#define PS_DRIVER_INIT_SUCCESS 0
#define PS_DRIVER_INIT_DEFER 1

/*
 * Returns:
 *  - negative on error
 *  - PS_DRIVER_INIT_SUCCESS on success
 *  - PS_DRIVER_INIT_DEFER when you want to defer this initialisation function until later
 */
typedef int (*ps_driver_init_fn_t)(ps_io_ops_t *io_ops, const char *device_path);

typedef struct {
    const char **compatible_list;
    ps_driver_init_fn_t init;
} ps_driver_module_t;
