/*
 * Copyright 2021, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2021, Breakaway Consulting Pty. Ltd.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <platsupport/io.h>
#include <platsupport/ltimer.h>

struct gpt_regs;

enum gpt_timeout_type {
    GPT_TIMEOUT_ONESHOT,
    GPT_TIMEOUT_PERIODIC,
    GPT_TIMEOUT_NONE,
};

typedef struct gpt {
    /* I/O ops */
    ps_io_ops_t io_ops;

    /* allocated resources */
    irq_id_t irq_id;
    pmem_region_t pmem;

    /* ltimer callback information */
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;

    /* registers */
    volatile struct gpt_regs *regs;

    /* internal state tracking data */
    uint64_t counted_ticks;
    uint64_t current_timeout;
    enum gpt_timeout_type timeout_type;
} gpt_t;

int gpt_init(gpt_t *gpt, char *fdt_path, ps_io_ops_t ops, ltimer_callback_fn_t user_callback,
             void *user_callback_token);
int gpt_get_time(gpt_t *gpt, uint64_t *time);
int gpt_set_timeout(gpt_t *gpt, uint64_t ns, timeout_type_t type);
int gpt_reset(gpt_t *gpt);
void gpt_destroy(gpt_t *gpt);
