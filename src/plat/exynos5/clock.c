/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include "../../arch/arm/clock.h"
#include "../../services.h"
#include <assert.h>
#include <string.h>
#include <utils/util.h>


#define EXYNOS5_CMU_CPU_PADDR   0x10010000
#define EXYNOS5_CMU_CORE_PADDR  0x10014000
#define EXYNOS5_CMU_ACP_PADDR   0x10018000
#define EXYNOS5_CMU_ISP_PADDR   0x1001C000
#define EXYNOS5_CMU_TOP_PADDR   0x10020000
#define EXYNOS5_CMU_LEX_PADDR   0x10024000
#define EXYNOS5_CMU_R0X_PADDR   0x10028000
#define EXYNOS5_CMU_R1X_PADDR   0x1002C000
#define EXYNOS5_CMU_CDREX_PADDR 0x10030000

#define EXYNOS5_CMUX_SIZE       0x1000
#define EXYNOS5_CMU_CPU_SIZE   EXYNOS5_CMUX_SIZE
#define EXYNOS5_CMU_CORE_SIZE  EXYNOS5_CMUX_SIZE
#define EXYNOS5_CMU_ACP_SIZE   EXYNOS5_CMUX_SIZE
#define EXYNOS5_CMU_ISP_SIZE   EXYNOS5_CMUX_SIZE
#define EXYNOS5_CMU_TOP_SIZE   EXYNOS5_CMUX_SIZE
#define EXYNOS5_CMU_LEX_SIZE   EXYNOS5_CMUX_SIZE
#define EXYNOS5_CMU_R0X_SIZE   EXYNOS5_CMUX_SIZE
#define EXYNOS5_CMU_R1X_SIZE   EXYNOS5_CMUX_SIZE
#define EXYNOS5_CMU_CDREX_SIZE EXYNOS5_CMUX_SIZE



static struct clock_regs {
    void* cpu;
    void* core;
    void* acp;
    void* isp;
    void* top;
    void* lex;
    void* r0x;
    void* r1x;
    void* cdrex;
} clk_regs;

static struct clock master_clk;

static clk_t* clks[] = {
    [CLK_MASTER]   = &master_clk,
};

static const freq_t freq_default[] = {
    [CLK_MASTER]   = 24 * MHZ,
};

static clk_t*
exynos5_get_clock(clock_sys_t* sys, enum clk_id id)
{
    clk_t* clk = clks[id];
    assert(clk);
    clk->clk_sys = sys;
    return clk_init(clk);
}

static int
exynos5_gate_enable(clock_sys_t* clock_sys, enum clock_gate gate, enum clock_gate_mode mode)
{
    return -1;
}

static int
clock_sys_common_init(clock_sys_t* clock_sys){
    clock_sys->priv = (void*)&clk_regs;
    clock_sys->get_clock = &exynos5_get_clock;
    clock_sys->gate_enable = &exynos5_gate_enable;
    return 0;
}

int
exynos5_clock_sys_init(void* cpu, void* core, void* acp, void* isp, void* top,
                       void* lex, void* r0x,  void* r1x, void* cdrex,
                       clock_sys_t* clock_sys)
{
    struct clock_regs* regs = &clk_regs;
    if(cpu)   regs->cpu   = cpu;
    if(core)  regs->core  = core;
    if(acp)   regs->acp   = acp;
    if(isp)   regs->isp   = isp;
    if(top)   regs->top   = top;
    if(lex)   regs->lex   = lex;
    if(r0x)   regs->r0x   = r0x;
    if(r1x)   regs->r1x   = r1x;
    if(cdrex) regs->cdrex = cdrex;
    return clock_sys_common_init(clock_sys);
}

int
clock_sys_init(ps_io_ops_t* o, clock_sys_t* clock_sys){
    struct clock_regs* regs = &clk_regs;
    MAP_IF_NULL(o, EXYNOS5_CMU_CPU,   regs->cpu);
    MAP_IF_NULL(o, EXYNOS5_CMU_CORE,  regs->core);
    MAP_IF_NULL(o, EXYNOS5_CMU_ACP,   regs->acp);
    MAP_IF_NULL(o, EXYNOS5_CMU_ISP,   regs->isp);
    MAP_IF_NULL(o, EXYNOS5_CMU_TOP,   regs->top);
    MAP_IF_NULL(o, EXYNOS5_CMU_LEX,   regs->lex);
    MAP_IF_NULL(o, EXYNOS5_CMU_R0X,   regs->r0x);
    MAP_IF_NULL(o, EXYNOS5_CMU_R1X,   regs->r1x);
    MAP_IF_NULL(o, EXYNOS5_CMU_CDREX, regs->cdrex);
    return clock_sys_common_init(clock_sys);
}

void
clk_print_clock_tree(clock_sys_t* sys)
{
    clk_t *clk = clk_get_clock(sys, CLK_MASTER);
    clk_print_tree(clk, "");
    assert(!"Not implemented");
}


/* MASTER_CLK */
static freq_t
_master_get_freq(clk_t* clk)
{
    return clk->freq;
}

static freq_t
_master_set_freq(clk_t* clk, freq_t hz)
{
    /* Master clock frequency is fixed */
    (void)hz;
    return clk_get_freq(clk);
}

static void
_master_recal(clk_t* clk UNUSED)
{
    assert(0);
}

static clk_t*
_master_init(clk_t* clk)
{
    if (clk->priv == NULL) {
        clk->priv = (void*)&clk_regs;
    }
    clk->freq = freq_default[clk->id];
    return clk;
}

static struct clock master_clk = {
    .id = CLK_MASTER,
    CLK_OPS(master),
    .freq = 0,
    .priv = NULL,
};


