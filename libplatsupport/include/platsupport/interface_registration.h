/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <errno.h>

#include <utils/util.h>
#include <platsupport/interface_types.h>

#define __PS_INTERFACE_REG_VALID_ARGS(func) \
    if (!interface_registration_ops) { ZF_LOGE("interface_ops is NULL!"); return -EINVAL; } \
    if (!interface_registration_ops->cookie) { ZF_LOGE("cookie in interface_ops is NULL!"); return -EINVAL; } \
    if (!interface_registration_ops->func) { ZF_LOGE(#func " is not supported!"); return -ENOSYS; }

typedef int (*ps_interface_register_fn_t)(void *cookie, ps_interface_type_t interface_type, void *interface_instance,
                                          char **properties);

typedef int (*ps_interface_unregister_fn_t)(void *cookie, ps_interface_type_t interface_type, void *interface_instance);

#define PS_INTERFACE_FOUND_MATCH 0
#define PS_INTERFACE_NO_MATCH 1

/*
 * Returns:
 *  - Negative on error
 *  - PS_INTERFACE_FOUND_MATCH on match
 *  - PS_INTERFACE_NO_MATCH on no match
 */
typedef int (*ps_interface_search_handler_fn_t)(void *handler_data, void *interface_instance, char **properties);

typedef int (*ps_interface_find_fn_t)(void *cookie, ps_interface_type_t interface_type,
                                      ps_interface_search_handler_fn_t handler, void *handler_data);

typedef struct {
    void *cookie;
    ps_interface_register_fn_t interface_register_fn;
    ps_interface_unregister_fn_t interface_unregister_fn;
    ps_interface_find_fn_t interface_find_fn;
} ps_interface_registration_ops_t;

static inline int ps_interface_register(ps_interface_registration_ops_t *interface_registration_ops,
                                        ps_interface_type_t interface_type, void *interface_instance, char **properties)
{
    __PS_INTERFACE_REG_VALID_ARGS(interface_register_fn);
    return interface_registration_ops->interface_register_fn(interface_registration_ops->cookie, interface_type,
                                                             interface_instance, properties);
}

static inline int ps_interface_unregister(ps_interface_registration_ops_t *interface_registration_ops,
                                          ps_interface_type_t interface_type, void *interface_instance)
{
    __PS_INTERFACE_REG_VALID_ARGS(interface_unregister_fn);
    return interface_registration_ops->interface_unregister_fn(interface_registration_ops->cookie,
                                                               interface_type, interface_instance);
}

static inline int ps_interface_find(ps_interface_registration_ops_t *interface_registration_ops,
                                    ps_interface_type_t interface_type, ps_interface_search_handler_fn_t handler,
                                    void *handler_data)
{
    __PS_INTERFACE_REG_VALID_ARGS(interface_find_fn);
    return interface_registration_ops->interface_find_fn(interface_registration_ops->cookie, interface_type,
                                                         handler, handler_data);
}
