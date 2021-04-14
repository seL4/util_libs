/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <platsupport/src.h>

#define SRC_PADDR           0x20D8000
#define SRC_SIZE            0x1000

struct src_regs {
    uint32_t scr;   /* 0x000 32 R/W 00000521h */
    uint32_t sbmr1; /* 0x004 32  R  00000000h */
    uint32_t srsr;  /* 0x008 32 R/W 00000001h */
    uint32_t res1[2];
    uint32_t sisr;  /* 0x014 32  R  00000000h */
    uint32_t simr;  /* 0x018 32 R/W 0000001Fh */
    uint32_t sbmr2; /* 0x01C 32  R  00000000h */
    uint32_t gpr1;  /* 0x020 32 R/W 00000000h */
    uint32_t gpr2;  /* 0x024 32 R/W 00000000h */
    uint32_t gpr3;  /* 0x028 32 R/W 00000000h */
    uint32_t gpr4;  /* 0x02C 32 R/W 00000000h */
    uint32_t gpr5;  /* 0x030 32 R/W 00000000h */
    uint32_t gpr6;  /* 0x034 32 R/W 00000000h */
    uint32_t gpr7;  /* 0x038 32 R/W 00000000h */
    uint32_t gpr8;  /* 0x03C 32 R/W 00000000h */
    uint32_t gpr9;  /* 0x040 32 R/W 00000000h */
    uint32_t gpr10; /* 0x044 32 R/W 00000000h */
};
typedef volatile struct src_regs src_regs_t;

static src_regs_t *src_regs;

static inline void src_set_regs(src_dev_t *d, src_regs_t *r)
{
    d->priv = (void *)r;
}

static inline src_regs_t *src_get_regs(src_dev_t *d)
{
    return (src_regs_t *)d->priv;
}

void reset_controller_assert_reset(src_dev_t *dev, enum src_rst_id id)
{
    src_regs_t *regs;
    int reset_bit;
    static const int reset_map[] = {
        [SRCRST_CORE3]      = 16, [SRCRST_CORE2]   = 15,
        [SRCRST_CORE1]      = 14, [SRCRST_CORE0]   = 13,
        [SRCRST_SW_IPU2]    = 12, [SRCRST_EIM]     = 11,
        [SRCRST_SW_OPEN_VG] =  4, [SRCRST_SW_IPU1] =  3,
        [SRCRST_SW_VPU]     =  2, [SRCRST_SW_GPU]  =  1
    };
    if (id >= 0 && id < ARRAY_SIZE(reset_map)) {
        reset_bit = reset_map[id];
        regs = src_get_regs(dev);
        /* Reset the subsystem */
        regs->scr |= BIT(reset_bit);
        while (regs->scr & BIT(reset_bit));
    }
}

int reset_controller_init(enum src_id id, ps_io_ops_t *ops, src_dev_t *dev)
{
    assert(sizeof(struct src_regs) == 0x48);
    if (id < 0 || id >= NSRC) {
        return -1;
    }

    src_regs = ps_io_map(&ops->io_mapper, SRC_PADDR, SRC_SIZE, 0, PS_MEM_NORMAL);
    if (src_regs == NULL) {
        return -1;
    }
    src_set_regs(dev, src_regs);

    return 0;
}
