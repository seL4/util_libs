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

#include "clock.h"
#include <string.h>
#include <assert.h>
#include "../../services.h"

/* Fixed clocks */
static freq_t
_fixed_clk_get_freq(clk_t* clk)
{
    return clk->req_freq;
}

static freq_t
_fixed_clk_set_freq(clk_t* clk, freq_t hz UNUSED)
{
    return clk_get_freq(clk);
}

void
_fixed_clk_recal(clk_t* clk UNUSED)
{
    assert(0);
}

clk_t*
_fixed_clk_init(clk_t* clk)
{
    return clk;
}

/* Default clocks. Simply report the recorded default frequency */
freq_t
_default_clk_get_freq(clk_t* clk)
{
    assert(clk->id >= 0 && clk->id < NCLOCKS);
    return ps_freq_default[clk->id];
}

freq_t
_default_clk_set_freq(clk_t* clk, freq_t hz UNUSED)
{
    return clk_get_freq(clk);
}

void
_default_clk_recal(clk_t* clk UNUSED)
{
    assert(0);
}

clk_t*
_default_clk_init(clk_t* clk)
{
    return clk;
}

static clk_t*
get_clock_default(clock_sys_t* clock_sys UNUSED, enum clk_id id)
{
    if (id < 0 || id >= NCLOCKS) {
        return NULL;
    } else {
        clk_t* clk;
        clk = ps_clocks[id];
        /* Destroy original clock links */
        clk->clk_sys = clock_sys;
        clk->init = _default_clk_init;
        clk->get_freq = _default_clk_get_freq;
        clk->set_freq = _default_clk_set_freq;
        clk->recal = _default_clk_recal;
        return clk;
    }
}

static int
gate_enable_default(clock_sys_t* clock_sys UNUSED, enum clock_gate gate UNUSED, enum clock_gate_mode mode UNUSED)
{
    /* Assume the gate is already enabled */
    return 0;
}

int
clock_sys_init_default(clock_sys_t* clock_sys)
{
    clock_sys->get_clock = &get_clock_default;
    clock_sys->gate_enable = &gate_enable_default;
    /* Clock subsystem is invalid if there is not private data attached */
    clock_sys->priv = (void*)0xdeadbeef;
    return 0;
}

int
clock_sys_set_default_freq(enum clk_id id, freq_t hz)
{
    if (id < 0 || id >= NCLOCKS) {
        return -1;
    } else {
        ps_freq_default[id] = hz;
        return 0;
    }
}

clk_t*
ps_get_clock(clock_sys_t* sys, enum clk_id id)
{
    if (id < 0 || id >= NCLOCKS) {
        return NULL;
    } else {
        clk_t* clk;
        clk = ps_clocks[id];
        assert(clk);
        assert(ps_clocks[id]->init);
        clk->clk_sys = sys;
        return clk_init(clk);
    }
}

void
clk_print_tree(clk_t* clk, const char* prefix)
{
    int depth = strlen(prefix);
    char new_prefix[depth + 2];
    strcpy(&new_prefix[0], prefix);
    new_prefix[depth] = '|';
    new_prefix[depth + 1] = '\0';
    while (clk != NULL) {
        const char* units[] = {"hz", "Khz", "Mhz", "Ghz"};
        const char** u = units;
        freq_t freq;
        int freqh, freql;
        freq = clk_get_freq(clk);
        /* Find frequency with appropriate units */
        while (freq > 10000 * 1000) {
            freq /= 1000;
            u++;
        }
        freqh = freq / 1000;
        freql = freq % 1000;
        /* Generate tree graphics */
        if (clk->sibling == NULL) {
            strcpy(new_prefix + depth, " ");
        }
        if (freqh == 0) {
            printf("%s\\%s (%03d %s)\n", new_prefix, clk->name, freql, u[0]);
        } else if (freql == 0) {
            printf("%s\\%s (%d %s)\n", new_prefix, clk->name, freqh, u[1]);
        } else {
            printf("%s\\%s (%d.%03d %s)\n", new_prefix, clk->name, freqh, freql, u[1]);
        }
        clk_print_tree(clk->child, new_prefix);
        clk = clk->sibling;
    }
}

void
clk_register_child(clk_t* parent, clk_t* child)
{
    /* Lets make sure that we were initialised correctly
     * to avoid tree loops */
    if (child->parent != NULL) {
        /* If we are registered with a parent */
        clk_t* sibling = parent->child;
        /* Make sure that we are a sibling of the parent's child */
        while (sibling != child) {
            assert(sibling);
            sibling = sibling->sibling;
        }
    }
    if (child->parent == NULL) {
        child->parent = parent;
        child->sibling = parent->child;
        parent->child = child;
    } else if (child->parent != parent) {
        printf("%s->%s\n | %s already has parent %s",
               child->name, parent->name, child->name,
               child->parent->name);
        assert(!"Changing parents not supported yet");
    }
}

clk_t
clk_generate_fixed_clk(enum clk_id id, freq_t frequency)
{
    clk_t ret = { _CLK_OPS(id, "Fixed clock", fixed_clk, NULL) };
    ret.req_freq = frequency;
    return ret;
}
