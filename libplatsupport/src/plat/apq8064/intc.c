/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdint.h>
#include <stdio.h>
#include <platsupport/io.h>

#define INTCTL0 0x12100000
#define INTCTL1 0x12100800
#define INTCTL2 0x12101000
#define INTCTL3 0x12101800
#define INTCTL4 0x12102000
#define INTCTL5 0x12102800
#define INTCTL6 0x12103000
#define INTCTL7 0x12103800

struct intctl {
#define INTSELECT_IRQ   0x0
#define INTSELECT_FIQ   0x1
    uint32_t select[2];       /* +0x000 */
    uint32_t res0[2];
#define INTENABLE_ENABLE  0x1
#define INTENABLE_DISABLE 0x0
    uint32_t enable[2];       /* +0x010 */
    uint32_t res1[2];
    uint32_t enable_clear[2]; /* +0x020 */
    uint32_t res2[2];
    uint32_t enable_set[2];   /* +0x030 */
    uint32_t res3[2];
#define INTTYPE_EDGE   0x1
#define INTTYPE_LEVEL  0x0
    uint32_t type[2];         /* +0x040 */
    uint32_t res4[2];
#define INTPOL_NEG  0x1
#define INTPOL_POS  0x0
    uint32_t polariy[2];      /* +0x050 */
    uint32_t res5[2];
    uint32_t no_pend_val;     /* +0x060 */
#define INTMASTER_IRQEN  (BIT(0))
#define INTMASTER_FIQEN  (BIT(1))
    uint32_t master_enable;   /* +0x064 */
#define INTVIC_VECTOR_MODE 0x1
    uint32_t vic_config;      /* +0x068 */
#define INTSECURE          0x1
    uint32_t security;        /* +0x06C */
    uint32_t res6[4];
    uint32_t irq_status[2];   /* +0x080 */
    uint32_t res7[2];
    uint32_t fiq_status[2];   /* +0x090 */
    uint32_t res8[2];
    uint32_t raw_status[2];   /* +0x0A0 */
    uint32_t res9[2];
    uint32_t raw_clear[2];    /* +0x0B0 */
    uint32_t res10[2];
    uint32_t raw_soft_int[2]; /* +0x0C0 */
    uint32_t res11[2];
    uint32_t vec_rd;          /* +0x0D0 */
    uint32_t pend_rd;         /* +0x0D4 */
    uint32_t vec_wr;          /* +0x0D8 */
    uint32_t res12;
    uint32_t in_service;      /* +0x0E0 */
    uint32_t in_stack;        /* +0x0E4 */
    uint32_t test_bus_sel;    /* +0x0E8 */
    uint32_t ctl_config;      /* +0x0EC */
    uint32_t res13[4];
    uint32_t res14[64];
    uint32_t priority[64];    /* +0x200 */
    uint32_t res15[64];
    uint32_t vec_addr[64];    /* +0x400 */
    uint32_t res16[64];
};
typedef volatile struct intctl intctl_t;

#define PPSS_INTC           0x12080000
#define PPSSINTC_IRQ_OFFSET 0x000
#define PPSSINTC_FIQ_OFFSET 0x100
struct ppss_intc {
    uint32_t stat;    /* +0x00 */
    uint32_t rawstat; /* +0x04 */
    uint32_t seten;   /* +0x08 */
    uint32_t clren;   /* +0x0C */
    uint32_t soft;    /* +0x10 */
};
typedef volatile struct ppss_intc ppss_intc_t;

intctl_t* intctl[8];
ppss_intc_t* fiq;
ppss_intc_t* irq;

void
test_intc(ps_io_ops_t* o)
{
    void* vaddr[4];
    if (intctl[0] == NULL) {
        /* intc */
        vaddr[0] = ps_io_map(&o->io_mapper, INTCTL0, 0x1000, 0, PS_MEM_NORMAL);
        vaddr[1] = ps_io_map(&o->io_mapper, INTCTL2, 0x1000, 0, PS_MEM_NORMAL);
        vaddr[2] = ps_io_map(&o->io_mapper, INTCTL3, 0x1000, 0, PS_MEM_NORMAL);
        vaddr[3] = ps_io_map(&o->io_mapper, INTCTL4, 0x1000, 0, PS_MEM_NORMAL);
        assert(vaddr[0]);
        assert(vaddr[1]);
        assert(vaddr[2]);
        assert(vaddr[3]);
        intctl[0] = (intctl_t*)((uintptr_t)vaddr[0] + 0x000);
        intctl[1] = (intctl_t*)((uintptr_t)vaddr[0] + 0x800);
        intctl[2] = (intctl_t*)((uintptr_t)vaddr[1] + 0x000);
        intctl[3] = (intctl_t*)((uintptr_t)vaddr[1] + 0x800);
        intctl[4] = (intctl_t*)((uintptr_t)vaddr[2] + 0x000);
        intctl[5] = (intctl_t*)((uintptr_t)vaddr[2] + 0x800);
        intctl[6] = (intctl_t*)((uintptr_t)vaddr[3] + 0x000);
        intctl[7] = (intctl_t*)((uintptr_t)vaddr[3] + 0x800);
        /* ppss intc */
        vaddr[0] = ps_io_map(&o->io_mapper, PPSS_INTC, 0x1000, 0, PS_MEM_NORMAL);
        irq = (ppss_intc_t*)((uintptr_t)vaddr[0] + PPSSINTC_IRQ_OFFSET);
        fiq = (ppss_intc_t*)((uintptr_t)vaddr[0] + PPSSINTC_FIQ_OFFSET);
        printf("irq: 0x%x 0x%x 0x%x 0x%x 0x%x\n", irq->stat, irq->rawstat, irq->seten, irq->clren, irq->soft);
        printf("fiq: 0x%x 0x%x 0x%x 0x%x 0x%x\n", fiq->stat, fiq->rawstat, fiq->seten, fiq->clren, fiq->soft);
        irq->soft = 1;
        irq->seten = 0xffff;
        printf("irq: 0x%x 0x%x 0x%x 0x%x 0x%x\n", irq->stat, irq->rawstat, irq->seten, irq->clren, irq->soft);
    }
};
