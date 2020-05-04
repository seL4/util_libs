/*
 * Copyright 2020, Data61
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

#include <utils/util.h>

#define PS_DRIVER_MODULE_DEFINE(name, compat_list, init_func)                   \
    static_assert(compat_list != NULL, "Supplied compatible_list is NULL!");    \
    static_assert(init_func != NULL, "Supplied init_func is NULL!");            \
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
