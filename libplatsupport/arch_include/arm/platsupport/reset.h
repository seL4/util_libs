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

#include <platsupport/io.h>
#include <platsupport/plat/reset.h>

typedef enum reset_id reset_id_t;

typedef struct reset_sys {
    int (*reset_assert)(void *data, reset_id_t id);
    int (*reset_deassert)(void *data, reset_id_t id);
    void *data;
} reset_sys_t;

int reset_sys_init(ps_io_ops_t *io_ops, void *dependecies, reset_sys_t *reset);

static inline int reset_sys_assert(reset_sys_t *reset, reset_id_t id)
{
    if (!reset) {
        ZF_LOGE("Reset sub system is invalid!");
        return -EINVAL;
    }

    if (!reset->reset_assert) {
        ZF_LOGE("not implemented");
        return -ENOSYS;
    }

    return reset->reset_assert(reset->data, id);
}

static inline int reset_sys_deassert(reset_sys_t *reset, reset_id_t id)
{
    if (!reset) {
        ZF_LOGE("Reset sub system is invalid!");
        return -EINVAL;
    }

    if (!reset->reset_deassert) {
        ZF_LOGE("not implemented");
        return -ENOSYS;
    }

    return reset->reset_deassert(reset->data, id);
}
