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
#include "../../common.h"


#define MXO_SRC_CLK_INV    (1U << 12)
#define MXO_SRC_BRANCH_ENA (1U << 11)
#define PXO_SRC_CLK_INV    (1U << 10)
#define PXO_SRC_BRANCH_ENA (1U <<  9)
#define CLK_INV            (1U <<  5)
#define CLK_BRANCH_ENA     (1U <<  4)
#define SRC_SEL            (1U <<  0)
#define SRC_SEL_PXO        (0U <<  0)
#define SRC_SEL_MXO        (1U <<  0)
#define PPSS_TIMER0_CLK_CTL 0x0090258C
#define PPSS_TIMER1_CLK_CTL 0x00902590

#define REG_READ(base, offset)     *(uint32_t*)((uintptr_t)base + (offset))
#define REG_WRITE(base, offset, v) *(uint32_t*)((uintptr_t)base + (offset)) = v

struct clock_sys_regs {
    void* block0;
    void* block1;
    void* block2;
    void* block3;
};


struct clock_sys_regs clk_sys_regs;

static inline uint32_t reg_read(uintptr_t addr)
{
    uint32_t* reg;
    switch (addr & 0x3000) {
    case 0x0000:
        reg = (uint32_t*)clk_sys_regs.block0;
        break;
    case 0x1000:
        reg = (uint32_t*)clk_sys_regs.block1;
        break;
    case 0x2000:
        reg = (uint32_t*)clk_sys_regs.block2;
        break;
    case 0x3000:
        reg = (uint32_t*)clk_sys_regs.block3;
        break;
    default:
        printf("Invalid memory access: 0x%x\n", (uint32_t)addr);
        return 0;
    }

    if (addr & 0x3) {
        printf("Unaligned memory access: 0x%x\n", (uint32_t)addr);
    }
    addr &= 0xfff;
    addr /= 4;
    return reg[addr];
}

static inline void reg_write(uintptr_t addr, uint32_t v)
{
    uint32_t* reg;
    switch (addr & 0x3000) {
    case 0x0000:
        reg = (uint32_t*)clk_sys_regs.block0;
        break;
    case 0x1000:
        reg = (uint32_t*)clk_sys_regs.block1;
        break;
    case 0x2000:
        reg = (uint32_t*)clk_sys_regs.block2;
        break;
    case 0x3000:
        reg = (uint32_t*)clk_sys_regs.block3;
        break;
    default:
        printf("Invalid memory access: 0x%x\n", (uint32_t)addr);
        return;
    }

    if (addr & 0x3) {
        printf("Unaligned memory access: 0x%x\n", (uint32_t)addr);
    }
    addr &= 0xfff;
    addr /= 4;
    reg[addr] = v;
}

void test(void)
{
    reg_write(PPSS_TIMER0_CLK_CTL, CLK_BRANCH_ENA);
    reg_write(PPSS_TIMER1_CLK_CTL, CLK_BRANCH_ENA);
}

static struct clock master_clk = { CLK_OPS_DEFAULT(MASTER) };
static struct clock pxo_clk    = { CLK_OPS_DEFAULT(PXO)    };
static struct clock tcxo_clk   = { CLK_OPS_DEFAULT(TCXO)   };
static struct clock wcnxo_clk  = { CLK_OPS_DEFAULT(WCNXO)  };
static struct clock slpxo_clk  = { CLK_OPS_DEFAULT(SLPXO)  };

static int
apq8064_gate_enable(clock_sys_t* sys, enum clock_gate gate, enum clock_gate_mode mode)
{
    (void)sys;
    (void)gate;
    (void)mode;
    return 0;
}

void
clk_print_clock_tree(clock_sys_t* sys)
{
    clk_t *clk = clk_get_clock(sys, CLK_MASTER);
    clk_print_tree(clk, "");
}

static int apq8064_clock_sys_init_common(clock_sys_t* clk_sys)
{
    clk_sys->gate_enable = &apq8064_gate_enable;
    clk_sys->get_clock = &ps_get_clock;
    return 0;
}

int
clock_sys_init(ps_io_ops_t* io_ops, clock_sys_t* clk_sys)
{
    struct clock_sys_regs* d = &clk_sys_regs;
    MAP_IF_NULL(io_ops, APQ8064_CLK_CTL0, d->block0);
    MAP_IF_NULL(io_ops, APQ8064_CLK_CTL1, d->block1);
    MAP_IF_NULL(io_ops, APQ8064_CLK_CTL2, d->block2);
    MAP_IF_NULL(io_ops, APQ8064_CLK_CTL3, d->block3);
    clk_sys->priv = d;
    return 0;
}


int
apq8064_clock_sys_init(void* clk_ctl_base0, void* clk_ctl_base1,
                       void* clk_ctl_base2, void* clk_ctl_base3,
                       clock_sys_t* clk_sys)
{
    struct clock_sys_regs* d = &clk_sys_regs;
    if (clk_ctl_base0) {
        d->block0 = clk_ctl_base0;
    }
    if (clk_ctl_base1) {
        d->block1 = clk_ctl_base1;
    }
    if (clk_ctl_base2) {
        d->block2 = clk_ctl_base2;
    }
    if (clk_ctl_base3) {
        d->block3 = clk_ctl_base3;
    }
    clk_sys->priv = d;
    return apq8064_clock_sys_init_common(clk_sys);
}

clk_t* ps_clocks[] = {
    [CLK_MASTER]   = &master_clk,
    [CLK_PXO   ]   = &pxo_clk,
    [CLK_WCNXO ]   = &wcnxo_clk,
    [CLK_TCXO  ]   = &tcxo_clk,
    [CLK_SLPXO ]   = &slpxo_clk
};

freq_t ps_freq_default[] = {
    [CLK_MASTER]   = 24 * MHZ,
    [CLK_PXO   ]   = 27 * MHZ,
    [CLK_WCNXO ]   = 48 * MHZ,
    [CLK_TCXO  ]   = 19200 * KHZ,
    [CLK_SLPXO ]   = 32768
};
