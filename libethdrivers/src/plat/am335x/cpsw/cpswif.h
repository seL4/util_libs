/* @TAG(CUSTOM) */

/**
 * @file - cpswif.h
 * Prototypes for CPSW Ethernet interface.
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * Copyright (c) 2010 Texas Instruments Incorporated
 *
 */
#include <lwip/netif.h>
#include <platsupport/io.h>
#include <ethdrivers/raw.h>

#ifndef __CPSWIF_H__
#define __CPSWIF_H__

/***************************************************************************/
/*
 * Configurations for AM335x
 */
#ifdef CONFIG_PLAT_AM335X
#include <sel4platsupport/plat/hw/soc_AM335x.h>

#define MAX_CPSW_INST                   1
#define CPSW0_SS_REGS                   SOC_CPSW_SS_REGS
#define CPSW0_MDIO_REGS                 SOC_CPSW_MDIO_REGS
#define CPSW0_WR_REGS                   SOC_CPSW_WR_REGS
#define CPSW0_CPDMA_REGS                SOC_CPSW_CPDMA_REGS
#define CPSW0_ALE_REGS                  SOC_CPSW_ALE_REGS
#define CPSW0_CPPI_RAM_REGS             SOC_CPSW_CPPI_RAM_REGS
#define CPSW0_PORT_0_REGS               SOC_CPSW_PORT_0_REGS
#define CPSW0_PORT_1_REGS               SOC_CPSW_PORT_1_REGS
#define CPSW0_SLIVER_1_REGS             SOC_CPSW_SLIVER_1_REGS
#define CPSW0_PORT_2_REGS               SOC_CPSW_PORT_2_REGS
#define CPSW0_SLIVER_2_REGS             SOC_CPSW_SLIVER_2_REGS

/* CPPI RAM size in bytes */
#ifndef SIZE_CPPI_RAM
#define SIZE_CPPI_RAM                            0x2000
#endif

#define PORT_1                                   0x0
#define PORT_2                                   0x1
#define PORT_0_MASK                              0x1
#define PORT_1_MASK                              0x2
#define PORT_2_MASK                              0x4
#define HOST_PORT_MASK                           PORT_0_MASK
#define SLAVE_PORT_MASK(slv_port_num)            (1 << slv_port_num)
#define PORT_MASK                                (0x7)
#define INDV_PORT_MASK(slv_port_num)             (1 << slv_port_num)

#define ENTRY_TYPE                               0x30
#define ENTRY_TYPE_IDX                           7
#define ENTRY_FREE                               0

/* MDIO input and output frequencies in Hz */
#define MDIO_FREQ_INPUT                          125000000
#define MDIO_FREQ_OUTPUT                         1000000

#define CPDMA_BUF_DESC_OWNER                     0x20000000
#define CPDMA_BUF_DESC_SOP                       0x80000000
#define CPDMA_BUF_DESC_EOP                       0x40000000
#define CPDMA_BUF_DESC_EOQ                       0x10000000
#define CPDMA_BUF_DESC_FROM_PORT                 0x70000
#define CPDMA_BUF_DESC_FROM_PORT_SHIFT           16
#define CPDMA_BUF_DESC_TO_PORT(port)             ((port << 16) | 0x100000)
#define CPDMA_BD_LEN_MASK                        0xFFFF
#define CPDMA_BD_PKTLEN_MASK                     0xFFFF

#define MAX_TRANSFER_UNIT                        1500
#define PBUF_LEN_MAX                             1520

#define MIN_PKT_LEN                              60

#ifndef FALSE
# define FALSE 0
#endif
#ifndef TRUE
# define TRUE 1
#endif

#ifdef evmAM335x
#define CPSW0_PORT_1_PHY_ADDR           0
#define CPSW0_PORT_1_PHY_GIGABIT        TRUE

#elif defined(beaglebone)
#define CPSW0_PORT_1_PHY_ADDR           0
#define CPSW0_PORT_1_PHY_GIGABIT        FALSE

#elif defined(evmskAM335x)
#define CPSW0_PORT_1_PHY_ADDR           0
#define CPSW0_PORT_2_PHY_ADDR           1
#define CPSW0_PORT_1_PHY_GIGABIT        TRUE
#define CPSW0_PORT_2_PHY_GIGABIT        TRUE
#endif

#define LWIP_PRINTF                     printf

#else
#error Unsupported EVM !!!
#endif

#define MAX_SLAVEPORT_PER_INST          2

#define SIZE_CPPI_RAM                            0x2000

#define VPTR_CPSW_CPPI(cpsw_base) ((cpsw_base) + 0x2000)
#define VPTR_CPSW_CPDMA(cpsw_base) ((cpsw_base) + 0x800)

struct EthVirtAddr {
    uintptr_t eth_mmio_ctr_reg;
    uintptr_t eth_mmio_prcm_reg;
    uintptr_t eth_mmio_cpsw_reg;
};

/**
 * Slave port information
 */
struct cpswport {
    u32_t port_base;
    u32_t sliver_base;
    u32_t phy_addr;

    /* The PHY is capable of GitaBit or Not */
    u32_t phy_gbps;
} cpswport;

/*****************************************************************************/
/**
 * Helper struct to hold private data used to operate the ethernet interface.
 */
struct cpswportif {
    /* CPSW instance number */
    u32_t inst_num;

    /* CPSW port number */
    u32_t port_num;

    u8_t eth_addr[6];
} cpswportif;

/**
 * CPSW instance information
 */
struct cpswinst {
    /* Base addresses */
    u32_t ss_base;
    u32_t mdio_base;
    u32_t wrpr_base;
    u32_t ale_base;
    u32_t cpdma_base;
    u32_t cppi_ram_base;
    u32_t host_port_base;

    /* Slave port information */
    struct cpswport port[MAX_SLAVEPORT_PER_INST];
} cpswinst;

/* TX and RX Buffer descriptor data structure (used by the device) */
struct descriptor {
    uintptr_t next;
    volatile uint32_t bufptr;
    volatile uint32_t bufoff_len;
    volatile uint32_t flags_pktlen;
};

struct beaglebone_eth_data {
    struct cpswportif *cpswPortIf;
    struct cpswinst   *cpswinst;
    uintptr_t tx_ring_phys;
    uintptr_t rx_ring_phys;
    volatile struct descriptor *tx_ring;
    volatile struct descriptor *rx_ring;
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
    struct EthVirtAddr iomm_address;
};

extern u32_t cpswif_netif_status(struct netif *netif);
extern u32_t cpswif_link_status(struct eth_driver *driver, u32_t inst_num, u32_t slv_port_num);
extern err_t cpswif_init(struct eth_driver *driver);

#endif /* _CPSWIF_H__ */
