/*
 * Copyright 2020, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#pragma once

#include "common.h"

#define CONFIG_SYS_CACHELINE_SIZE 64

#ifdef CONFIG_SYS_CACHELINE_SIZE
#define ARCH_DMA_MINALIGN   CONFIG_SYS_CACHELINE_SIZE
#else
#define ARCH_DMA_MINALIGN   16
#endif

#define CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT 128
#define CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT 128

#define FDT_ADDR_T_NONE (-1U)

#define GPIOD_REQUESTED     (1 << 0)    /* Requested/claimed */
#define GPIOD_IS_OUT        (1 << 1)    /* GPIO is an output */
#define GPIOD_IS_IN         (1 << 2)    /* GPIO is an input */
#define GPIOD_ACTIVE_LOW    (1 << 3)    /* value has active low */
#define GPIOD_IS_OUT_ACTIVE (1 << 4)    /* set output active */

#define TX2_PADDR 0x02490000
#define TX2_DEFAULT_MAC "\x00\x04\x4b\xc5\x67\x70"
// #define TX2_DEFAULT_MAC "\x00\x04\x4b\xa5\x90\xeb"


/* descriptor 0, 1 and 2 need to be written to in order to trigger dma */
struct eqos_desc {
    uint32_t des0; /* address of the packet */
    uint32_t des1;
    uint32_t des2; /* length of packet */
    uint32_t des3; /* flags (interrupt, own, ld, etc) and length of packet */
};

#define EQOS_DESCRIPTORS_TX 256
#define EQOS_DESCRIPTORS_RX 256

/* descriptor flags */
#define EQOS_DESC2_IOC      BIT(31)
#define EQOS_DESC3_OWN      BIT(31)
#define EQOS_DESC3_FD       BIT(29)
#define EQOS_DESC3_LD       BIT(28)
#define EQOS_DESC3_BUF1V    BIT(24)
#define DWCEQOS_DMA_RDES3_INTE    BIT(30)

#define TXIRQ 1
#define RXIRQ 2

#define EQOS_ALIGN(x,a)      __ALIGN_MASK((x),(typeof(x))(a)-1)
#define EQOS_MAX_PACKET_SIZE    EQOS_ALIGN(1568, ARCH_DMA_MINALIGN)

struct tx2_eth_data {
    void *eth_dev;
    uintptr_t tx_ring_phys;
    uintptr_t rx_ring_phys;
    volatile struct eqos_desc *tx_ring;
    volatile struct eqos_desc *rx_ring;
    unsigned int rx_size;
    unsigned int tx_size;
    void **rx_cookies;
    unsigned int rx_remain;
    unsigned int tx_remain;
    void **tx_cookies;
    unsigned int *tx_lengths;
    /* track where the head and tail of the queues are for
     * enqueueing buffers / checking for completions */
    unsigned int rdt, rdh, tdt, tdh;
};
