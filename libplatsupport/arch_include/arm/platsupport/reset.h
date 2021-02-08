/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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

int reset_sys_init(ps_io_ops_t *io_ops, void *dependencies, reset_sys_t *reset);

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
