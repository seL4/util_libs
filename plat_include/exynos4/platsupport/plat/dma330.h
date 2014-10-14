/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _PLATSUPPORT_PLAT_DMA330_H
#define _PLATSUPPORT_PLAT_DMA330_H


#define MDMA0_PADDR 0x10800000
#define MDMA1_PADDR 0x11C10000
#define PDMA0_PADDR 0x121A0000
#define PDMA1_PADDR 0x121B0000

#define DMA330_SIZE 0x1000
#define MDMA0_SIZE  DMA330_SIZE
#define MDMA1_SIZE  DMA330_SIZE
#define PDMA0_SIZE  DMA330_SIZE
#define PDMA1_SIZE  DMA330_SIZE

#define MDMA0_IRQ       65
#define MDMA1_IRQ      156
#define MDMA0_ABORT_IRQ 47
#define MDMA1_ABORT_IRQ 45
#define PDMA0_IRQ       66
#define PDMA1_IRQ       67

enum dma330_id {
    PL330_MDMA0,
    PL330_MDMA1,
    PL330_PDMA0,
    PL330_PDMA1,
    NPL330
};

static const uint32_t dma330_paddr[] = {
    [PL330_MDMA0] = MDMA0_PADDR,
    [PL330_MDMA1] = MDMA1_PADDR,
    [PL330_PDMA0] = PDMA0_PADDR,
    [PL330_PDMA1] = PDMA1_PADDR
};

static const uint32_t dma330_size[] = {
    [PL330_MDMA0] = MDMA0_SIZE,
    [PL330_MDMA1] = MDMA1_SIZE,
    [PL330_PDMA0] = PDMA0_SIZE,
    [PL330_PDMA1] = PDMA1_SIZE
};

static const int dma330_irq[] = {
    [PL330_MDMA0] = MDMA0_IRQ,
    [PL330_MDMA1] = MDMA1_IRQ,
    [PL330_PDMA0] = PDMA0_IRQ,
    [PL330_PDMA1] = PDMA1_IRQ
};

static const int dma330_abort_irq[] = {
    [PL330_MDMA0] = MDMA0_ABORT_IRQ,
    [PL330_MDMA1] = MDMA1_ABORT_IRQ,
    [PL330_PDMA0] = -1,
    [PL330_PDMA1] = -1
};


#endif /* _PLATSUPPORT_PLAT_DMA330_H */
