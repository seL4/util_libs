/*
 * Copyright 2017, Data61
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

#include <stdint.h>
#include <assert.h>
#include <utils/util.h>

struct clock;
struct clock_sys;

typedef struct clock clk_t;
typedef struct clock_sys clock_sys_t;

#include <platsupport/plat/clock.h>

enum clock_gate_mode {
    CLKGATE_ON,
    CLKGATE_IDLE,
    CLKGATE_SLEEP,
    CLKGATE_OFF
};

struct clock_sys {
    clk_t* (*get_clock)(clock_sys_t* clock_sys, enum clk_id id);
    int (*gate_enable)(clock_sys_t* clock_sys, enum clock_gate gate, enum clock_gate_mode mode);
    void* priv;
};

#include <platsupport/io.h>

struct clock {
    enum clk_id id;
    /// Name of the clock
    const char* name;
    /// Clock specific private data
    void *priv;
    /// The requested frequency (not the actual frequency)
    freq_t req_freq;
    /* For requesting a freq change up the tree. This should
     * be NULL until clk_register_child has been called */
    clk_t* parent;
    /// Provide linked list for this clock's parent.
    clk_t* sibling;
    /// For signalling a freq change down the tree
    clk_t* child;
    /* Changing parents and initialisation requires access to the clock
     * subsystem. A reference is stored here for convenience */
    clock_sys_t* clk_sys;
    /// Clock specific functions
    clk_t* (*init)(clk_t* clk);
    freq_t (*get_freq)(clk_t*);
    freq_t (*set_freq)(clk_t*, freq_t hz);
    void (*recal)(clk_t*);
};

/**
 * Determine if a clock subsystem structure is valid
 * @param[in] clock_sys  A handle to the clock subsystem
 * @return               1 if the structure is valid;
 */
static inline int clock_sys_valid(const clock_sys_t* clock_sys)
{
    return clock_sys && clock_sys->priv;
}

/**
 * Initialise the clock subsystem
 * @parm[in] io_ops      A handle to io operations that may be used for
 *                       initialisation. If NULL is passed, the clock system
 * @param[out] clock_sys On success, clk_sys will contain a handle
 *                       to the clocking subsystem
 * @return               0 on success
 */
int clock_sys_init(ps_io_ops_t* io_ops, clock_sys_t* clock_sys);

/**
 * Initialise a clock subsystem that does not support clock configuration
 * and reports a static set of default clock frequencies.
 * The use of this initialisation method is not advised as the accuracy
 * of default clock frequencies are heavily tied to what the bootloader
 * may (or may not) have configured.
 * @param[out] clock_sys On success, clk_sys will contain a handle
 *                       to the clocking subsystem
 * @return               0 on success
 */
int clock_sys_init_default(clock_sys_t* clock_sys);

/**
 * Override the default frequency for a clock.
 * This function may be used in conjunction with clock_sys_init_default.
 * Again, the use of these methods are not advised.
 * @param[in] id         The ID of the clock to configure
 * @param[in] hz         The frequency of the clock
 * @return               0 on success
 */
int clock_sys_set_default_freq(enum clk_id id, freq_t hz);

/**
 * Initialise and acquire a system clock
 * @param[in] clock_sys  A handle to the clock subsystem
 * @param[in] id         The ID of the clock to acquire
 * @return               On success, a handle to the acquired clock.
 *                       Otherwise, NULL.
 */
static inline clk_t* clk_get_clock(clock_sys_t* clock_sys, enum clk_id id)
{
    clk_t * clk;
    assert(clock_sys);
    assert(clock_sys->get_clock);
    clk = clock_sys->get_clock(clock_sys, id);
    return clk;
};

/**
 * prints a list of initialised clocks
 * @param[in] clock_sys  A handle to the clock subsystem
 */
void clk_print_clock_tree(clock_sys_t* clock_sys);

/**
 * Configure the gating mode of a clock
 * @param[in] clock_sys  A handle to the clock subsystem
 * @param[in] gate       The ID of the gate to control
 * @param[in] mode       The mode at which the clock should be gated
 * @return               0 on success;

 */
static inline int clk_gate_enable(clock_sys_t* clock_sys, enum clock_gate gate,
                                  enum clock_gate_mode mode)
{
    assert(clock_sys);
    assert(clock_sys->gate_enable);
    return clock_sys->gate_enable(clock_sys, gate, mode);
}

/**
 * Set the clock frequency.
 * @param[in] clk    The clock to set the frequency of.
 * @param[in] hz     Hz to set the clk to
 * @return           The hz the clock was set to
 *                   (may not exactly match input param)
 */
static inline freq_t clk_set_freq(clk_t* clk, freq_t hz)
{
    assert(clk);
    assert(clk->set_freq);
    return clk->set_freq(clk, hz);
}

/**
 * Set the clock frequency.
 * @param[in] clk    The clock to set the frequency of.
 * @param[in] hz     Hz to set the clk to
 * @return           The hz the clock was set to
 *                   (may not exactly match input param)
 */
static inline freq_t clk_get_freq(clk_t* clk)
{
    assert(clk);
    assert(clk->get_freq);
    return clk->get_freq(clk);
}

/**
 * Register a clock as a child of another
 * If the parent clock frequency ever changes, the recal function for the
 * child clock will be called.
 * @param[in] parent  The parent of this clock relationship
 * @param[in] child   The child of this clock relationship
 */
void clk_register_child(clk_t* parent, clk_t* child);

/**
 * Initialise a clock structure which reports a fixed frequency
 * @param[in] id    The ID for the clock
 * @param[in] freq  The frequency of the fixed clock
 * @return          A populated clk_t structure ready for use
 */
clk_t clk_generate_fixed_clk(enum clk_id id, freq_t frequency);

