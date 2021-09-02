/*
 * Copyright 2021, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2021, Breakaway Consulting Pty. Ltd.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <platsupport/ltimer.h>
#include <platsupport/plat/gpt.h>
#include <platsupport/io.h>

#define MODULE_LABEL "tqma8xqp1gb.ltimer: "
#define GPT_0_PATH "/gpt@0x5d140000"

static int get_time(void *data, uint64_t *time)
{
    return gpt_get_time((gpt_t *)data, time);
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    return gpt_set_timeout((gpt_t *)data, ns, type);
}

static int reset(void *data)
{
    return gpt_reset((gpt_t *)data);
}

static void destroy(void *data)
{
    gpt_destroy((gpt_t *)data);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    assert(ltimer != NULL);
    gpt_t *gpt;
    int error;

    error = ps_calloc(&ops.malloc_ops, 1, sizeof * gpt, (void **) &gpt);
    if (error) {
        return error;
    }

    error = gpt_init(gpt, GPT_0_PATH, ops, callback, callback_token);
    if (error != 0) {
        ps_free(&ops.malloc_ops, sizeof * gpt, (void *)gpt);
        return error;
    }

    ltimer->get_time = get_time;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;
    ltimer->data = gpt;

    return 0;
}
