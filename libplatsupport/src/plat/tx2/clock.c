/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <platsupport/clock.h>
#include <platsupport/plat/clock.h>

/* NVIDIA interface */
#include <tx2bpmp/bpmp.h> /* struct mrq_clk_request, struct mrq_clk_response */

#include "../../services.h"

/*
 * Having the parent pointer filled in for the clk_t structure currently
 * doesn't mean anything at the moment. Recalibration is handled by the BPMP
 * co-processor and it all happens behind the scenes.
 */

typedef struct tx2_clk {
    ps_io_ops_t *io_ops;
    void *car_vaddr;
    struct tx2_bpmp bpmp;
} tx2_clk_t;

static inline bool check_valid_gate(enum clock_gate gate)
{
    return (CLK_GATE_FUSE <= gate && gate < NCLKGATES);
}

static inline bool check_valid_clk_id(enum clk_id id)
{
    return (CLK_PLLC_OUT_ISP <= id && id < NCLOCKS);
}

static int tx2_car_gate_enable(clock_sys_t *clock_sys, enum clock_gate gate, enum clock_gate_mode mode)
{
    if (!check_valid_gate(gate)) {
        ZF_LOGE("Invalid clock gate!");
        return -EINVAL;
    }

    if (mode == CLKGATE_IDLE || mode == CLKGATE_SLEEP) {
        ZF_LOGE("Idle and sleep gate modes are not supported");
        return -EINVAL;
    }

    uint32_t command = (mode == CLKGATE_ON ? CMD_CLK_ENABLE : CMD_CLK_DISABLE);

    /* Setup the message and make a call to BPMP */
    struct mrq_clk_request req = { .cmd_and_id = (command << 24) | gate };
    struct mrq_clk_response res = {0};
    tx2_clk_t *clk = clock_sys->priv;

    int bytes_recvd = tx2_bpmp_call(&clk->bpmp, MRQ_CLK, &req, sizeof(req), &res, sizeof(res));
    if (bytes_recvd < 0) {
        return -EIO;
    }

    return 0;
}

static freq_t tx2_car_get_freq(clk_t *clk)
{
    struct mrq_clk_request req = { .cmd_and_id = (CMD_CLK_GET_RATE << 24) | clk->id };
    struct mrq_clk_response res = {0};
    tx2_clk_t *tx2_clk = clk->clk_sys->priv;

    int bytes_recvd = tx2_bpmp_call(&tx2_clk->bpmp, MRQ_CLK, &req, sizeof(req), &res, sizeof(&res));
    if (bytes_recvd < 0) {
        return 0;
    }

    return (freq_t) res.clk_get_rate.rate;
}

static freq_t tx2_car_set_freq(clk_t *clk, freq_t hz)
{
    struct mrq_clk_request req = { .cmd_and_id = (CMD_CLK_SET_RATE << 24) | clk->id };
    req.clk_set_rate.rate = hz;
    struct mrq_clk_response res = {0};
    tx2_clk_t *tx2_clk = clk->clk_sys->priv;

    int bytes_recvd = tx2_bpmp_call(&tx2_clk->bpmp, MRQ_CLK, &req, sizeof(req), &res, sizeof(&res));
    if (bytes_recvd < 0) {
        return 0;
    }

    clk->req_freq = hz;

    return (freq_t) res.clk_set_rate.rate;
}

static clk_t *tx2_car_get_clock(clock_sys_t *clock_sys, enum clk_id id)
{
    if (!check_valid_clk_id(id)) {
        ZF_LOGE("Invalid clock ID");
        return NULL;
    }

    clk_t *ret_clk = NULL;
    tx2_clk_t *tx2_clk = clock_sys->priv;

    int error = ps_calloc(&tx2_clk->io_ops->malloc_ops, 1, sizeof(*ret_clk), (void **) &ret_clk);
    if (error) {
        ZF_LOGE("Failed to allocate memory for the clock structure");
        return NULL;
    }

    bool clock_initialised = false;

    /* Enable the clock while we're at it, clk_id is also a substitute for clock_gate */
    error = tx2_car_gate_enable(clock_sys, id, CLKGATE_ON);
    if (error) {
        goto fail;
    }

    clock_initialised = true;

    /* Get info about this clock so we can fill it in */
    struct mrq_clk_request req = { .cmd_and_id = (CMD_CLK_GET_ALL_INFO << 24) | id };
    struct mrq_clk_response res = {0};
    char *clock_name = NULL;
    int bytes_recvd = tx2_bpmp_call(&tx2_clk->bpmp, MRQ_CLK, &req, sizeof(req), &res, sizeof(res));
    if (bytes_recvd < 0) {
        ZF_LOGE("Failed to initialise the clock");
        goto fail;
    }
    size_t clk_name_len = strlen((char *) res.clk_get_all_info.name) + 1;
    error = ps_calloc(&tx2_clk->io_ops->malloc_ops, 1, sizeof(char) * clk_name_len, (void **) &clock_name);
    if (error) {
        ZF_LOGE("Failed to allocate memory for the name of the clock");
        goto fail;
    }
    strncpy(clock_name, (char *) res.clk_get_all_info.name, clk_name_len);

    ret_clk->name = (const char *) clock_name;

    /* There's no need for the init nor the recal functions as we're already
     * doing it now and that the BPMP handles the recalibration for us */
    ret_clk->get_freq = tx2_car_get_freq;
    ret_clk->set_freq = tx2_car_set_freq;

    ret_clk->id = id;
    ret_clk->clk_sys = clock_sys;

    return ret_clk;

fail:
    if (ret_clk) {
        if (ret_clk->name) {
            ps_free(&tx2_clk->io_ops->malloc_ops, sizeof(char) * clk_name_len, (void *) ret_clk->name);
        }

        ps_free(&tx2_clk->io_ops->malloc_ops, sizeof(*ret_clk), (void *) ret_clk);
    }

    if (clock_initialised) {
        ZF_LOGF_IF(tx2_car_gate_enable(clock_sys, id, CLKGATE_OFF),
                   "Failed to disable clock following failed clock initialisation operation");
    }

    return NULL;
}

int clock_sys_init(ps_io_ops_t *io_ops, clock_sys_t *clock_sys)
{
    if (!io_ops || !clock_sys) {
        if (!io_ops) {
            ZF_LOGE("null io_ops argument");
        }

        if (!clock_sys) {
            ZF_LOGE("null clock_sys argument");
        }

        return -EINVAL;
    }

    int error = 0;
    bool bpmp_initialised = false;

    error = ps_calloc(&io_ops->malloc_ops, 1, sizeof(tx2_clk_t), (void **) &clock_sys->priv);
    if (error) {
        ZF_LOGE("Failed to allocate memory for clock sys internal structure");
        error = -ENOMEM;
        goto fail;
    }

    tx2_clk_t *clk = clock_sys->priv;

    void *car_vaddr = NULL;
    MAP_IF_NULL(io_ops, TX2_CLKCAR, car_vaddr);
    if (car_vaddr == NULL) {
        ZF_LOGE("Failed to map tx2 CAR registers");
        error = -ENOMEM;
        goto fail;
    }

    clk->car_vaddr = car_vaddr;

    error = tx2_bpmp_init(io_ops, &clk->bpmp);
    if (error) {
        goto fail;
    }

    bpmp_initialised = true;

    clk->io_ops = io_ops;

    clock_sys->gate_enable = &tx2_car_gate_enable;
    clock_sys->get_clock = &tx2_car_get_clock;

    return 0;

fail:

    if (car_vaddr) {
        ps_io_unmap(&io_ops->io_mapper, car_vaddr, TX2_CLKCAR_SIZE);
    }

    if (clock_sys->priv) {
        ps_free(&io_ops->malloc_ops, sizeof(tx2_clk_t), (void *) clock_sys->priv);
    }

    if (bpmp_initialised) {
        ZF_LOGF_IF(tx2_bpmp_destroy(io_ops, &clk->bpmp),
                   "Failed to cleanup after a failed clock system initialisation");
    }

    return error;
}

void clk_print_clock_tree(clock_sys_t *sys)
{
    /* TODO Implement this function. The manual doesn't really give us a nice
     * diagram of the clock hierarchy, but there is information about it. It's
     * kind of hard to find however and would require scrolling through the
     * manual and constructing it by hand. */
    ZF_LOGE("Unimplemented");
}

/* This is here just to make the compiler happy. We don't have a static array
 * as there are just too many clocks and each of them share the same functions.
 */
clk_t *ps_clocks = NULL;
/* TODO Figure out the default frequencies of the clocks */
freq_t ps_freq_default = {0};
