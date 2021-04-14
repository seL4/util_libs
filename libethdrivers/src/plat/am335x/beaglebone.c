/*
 * Copyright 2016, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ethdrivers/raw.h>
#include <ethdrivers/helpers.h>
#include <string.h>
#include <utils/util.h>
#include <lwip/netif.h>
#include "cpsw/cpswif.h"
#include <ethdrivers/plat/cpsw.h>
#include <ethdrivers/plat/interrupt.h>
#include "lwiplib.h"

#define DEFAULT_MAC "\x00\x19\xb8\x00\xf0\xa3"

#define MAX_PKT_SIZE 1520
#define DMA_ALIGN 32

static void low_level_init(struct eth_driver *driver, uint8_t *mac, int *mtu)
{
    struct beaglebone_eth_data *eth_data = (struct beaglebone_eth_data *)driver->eth_data;
    struct cpswportif *cpswif = (struct cpswportif *) eth_data->cpswPortIf;

    for (int temp = 0; temp < LEN_MAC_ADDRESS; temp++) {
        mac[temp] = cpswif->eth_addr[temp];
    }

    *mtu = MAX_PKT_SIZE;
}

static void fill_rx_bufs(struct eth_driver *driver)
{
    struct beaglebone_eth_data *dev = (struct beaglebone_eth_data *)driver->eth_data;

    THREAD_MEMORY_RELEASE();

    while (dev->rx_remain > 0) {
        /* request a buffer */
        void *cookie;
        int next_rdt = (dev->rdt + 1) % dev->rx_size;
        uintptr_t phys = driver->i_cb.allocate_rx_buf ? driver->i_cb.allocate_rx_buf(driver->cb_cookie, MAX_PKT_SIZE,
                                                                                     &cookie) : 0;
        if (!phys) {
            break;
        }
        dev->rx_cookies[dev->rdt] = cookie;

        dev->rx_ring[dev->rdt].bufptr = phys;
        dev->rx_ring[dev->rdt].bufoff_len = PBUF_LEN_MAX;
        /* Mark the descriptor as owned by CPDMA to tell the CPSW hardware it can put
         * RX data into it
         */
        THREAD_MEMORY_RELEASE();

        dev->rx_ring[dev->rdt].flags_pktlen = CPDMA_BUF_DESC_OWNER;
        /* Set the next field in the hardware descriptor. The device will traverse the ring
         * using the next field to dump other RX data
         */
        THREAD_MEMORY_RELEASE();
        dev->rx_ring[dev->rdt].next = ((struct descriptor *) dev->rx_ring_phys) + next_rdt;

        dev->rdt = next_rdt;
        dev->rx_remain--;
    }

    THREAD_MEMORY_ACQUIRE();
}

static void free_desc_ring(struct beaglebone_eth_data *dev, ps_dma_man_t *dma_man)
{
    if (dev->rx_ring) {
        dma_unpin_free(dma_man, (void *)dev->rx_ring, sizeof(struct descriptor) * dev->rx_size);
        dev->rx_ring = NULL;
    }
    if (dev->tx_ring) {
        dma_unpin_free(dma_man, (void *)dev->tx_ring, sizeof(struct descriptor) * dev->tx_size);
        dev->tx_ring = NULL;
    }
    if (dev->rx_cookies) {
        free(dev->rx_cookies);
        dev->rx_cookies = NULL;
    }
    if (dev->tx_cookies) {
        free(dev->tx_cookies);
        dev->tx_cookies = NULL;
    }
    if (dev->tx_lengths) {
        free(dev->tx_lengths);
        dev->tx_lengths = NULL;
    }
}

static int initialize_desc_ring(struct beaglebone_eth_data *dev, ps_dma_man_t *dma_man)
{
    /* CPSW device has its own fixed memory area to save RX and TX ring descriptors */
    dma_addr_t tx_ring = {.virt = VPTR_CPSW_CPPI(dev->iomm_address.eth_mmio_cpsw_reg), .phys = CPSW0_CPPI_RAM_REGS};
    dma_addr_t rx_ring = {.virt = VPTR_CPSW_CPPI(dev->iomm_address.eth_mmio_cpsw_reg) + (SIZE_CPPI_RAM >> 1), .phys = CPSW0_CPPI_RAM_REGS + (SIZE_CPPI_RAM >> 1)};
    dev->rx_ring = rx_ring.virt;
    dev->rx_ring_phys = rx_ring.phys;

    ps_dma_cache_clean_invalidate(dma_man, rx_ring.virt, sizeof(struct descriptor) * dev->rx_size);
    ps_dma_cache_clean_invalidate(dma_man, tx_ring.virt, sizeof(struct descriptor) * dev->tx_size);

    dev->rx_cookies = malloc(sizeof(void *) * dev->rx_size);
    dev->tx_cookies = malloc(sizeof(void *) * dev->tx_size);
    dev->tx_lengths = malloc(sizeof(unsigned int) * dev->tx_size);
    if (!dev->rx_cookies || !dev->tx_cookies || !dev->tx_lengths) {
        if (dev->rx_cookies) {
            free(dev->rx_cookies);
        }
        if (dev->tx_cookies) {
            free(dev->tx_cookies);
        }
        if (dev->tx_lengths) {
            free(dev->tx_lengths);
        }
        ZF_LOGE("Failed to malloc");
        free_desc_ring(dev, dma_man);
        return -1;
    }
    dev->tx_ring = tx_ring.virt;
    dev->tx_ring_phys = tx_ring.phys;
    /* Remaining needs to be 2 less than size as we cannot actually enqueue size many descriptors,
     * since then the head and tail pointers would be equal, indicating empty. */
    dev->rx_remain = dev->rx_size - 2;
    dev->tx_remain = dev->tx_size - 2;

    dev->rdt = dev->rdh = dev->tdt = dev->tdh = 0;

    /* zero both rings */
    for (unsigned int i = 0; i < dev->tx_size; i++) {
        dev->tx_ring[i] = (struct descriptor) {
            .next = NULL,
            .bufptr = 0,
            .bufoff_len = 0,
            .flags_pktlen = 0
        };
    }
    for (unsigned int i = 0; i < dev->rx_size; i++) {
        dev->rx_ring[i] = (struct descriptor) {
            .next = NULL,
            .bufptr = 0,
            .bufoff_len = 0,
            .flags_pktlen = CPDMA_BUF_DESC_OWNER
        };
    }
    THREAD_MEMORY_FENCE();

    return 0;
}

static void complete_rx(struct eth_driver *eth_driver)
{
    struct beaglebone_eth_data *dev = (struct beaglebone_eth_data *)eth_driver->eth_data;
    unsigned int rdt = dev->rdt;

    while ((dev->rdh != rdt) && ((dev->rx_ring[dev->rdh].flags_pktlen & CPDMA_BUF_DESC_OWNER) != CPDMA_BUF_DESC_OWNER)) {
        int orig_rdh = dev->rdh;


        /* Ensure no memory references get ordered before we checked the descriptor was written back */
        THREAD_MEMORY_ACQUIRE();

        void *cookie = dev->rx_cookies[dev->rdh];
        unsigned int len = (dev->rx_ring[dev->rdh].flags_pktlen) & CPDMA_BD_PKTLEN_MASK;
        /* update rdh */
        dev->rdh = (dev->rdh + 1) % dev->rx_size;
        dev->rx_remain++;

        /* Give the buffers back */
        eth_driver->i_cb.rx_complete(eth_driver->cb_cookie, 1, &cookie, &len);

        /* Acknowledge that this packet is processed */
        CPSWCPDMARxCPWrite(VPTR_CPSW_CPDMA(dev->iomm_address.eth_mmio_cpsw_reg), 0,
                           (uintptr_t)(((volatile struct descriptor *) dev->rx_ring_phys) + (orig_rdh)));
        dev->rx_ring[orig_rdh].flags_pktlen = CPDMA_BUF_DESC_OWNER;
        CPSWCPDMARxHdrDescPtrWrite(VPTR_CPSW_CPDMA(dev->iomm_address.eth_mmio_cpsw_reg),
                                   ((struct descriptor *) dev->rx_ring_phys) + dev->rdh, 0);

    }
}

static void complete_tx(struct eth_driver *driver)
{
    struct beaglebone_eth_data *dev = (struct beaglebone_eth_data *)driver->eth_data;
    volatile u32_t cnt = 0xFFFF;

    int orig_tdh = dev->tdh;

    while (dev->tdh != dev->tdt && (((dev->tx_ring[dev->tdh].flags_pktlen) & CPDMA_BUF_DESC_SOP))) {
        unsigned int i;

        /* Make sure that the transmission is over */
        while (((dev->tx_ring[dev->tdh].flags_pktlen & CPDMA_BUF_DESC_OWNER)
                == CPDMA_BUF_DESC_OWNER) && ((--cnt) != 0));

        /* If CPDMA failed to transmit, give it a chance once more */
        if (0 == cnt) {
            CPSWCPDMATxHdrDescPtrWrite(VPTR_CPSW_CPDMA(dev->iomm_address.eth_mmio_cpsw_reg),
                                       (uintptr_t)(((volatile struct descriptor *) dev->tx_ring_phys) + (dev->tdh)), 0);
            return;
        }

        /* do not let memory loads happen before our checking of the descriptor write back */
        THREAD_MEMORY_RELEASE();
        /* increase where we believe tdh to be */
        void *cookie = dev->tx_cookies[dev->tdh];
        dev->tx_remain += dev->tx_lengths[dev->tdh];
        dev->tdh = (dev->tdh + dev->tx_lengths[dev->tdh]) % dev->tx_size;
        /* give the buffer back */
        driver->i_cb.tx_complete(driver->cb_cookie, cookie);

        dev->tx_ring[orig_tdh].flags_pktlen &= ~(CPDMA_BUF_DESC_SOP);
        dev->tx_ring[orig_tdh].flags_pktlen &= ~(CPDMA_BUF_DESC_EOP);

        /* Acknowledge CPSW */
        CPSWCPDMATxCPWrite(VPTR_CPSW_CPDMA(dev->iomm_address.eth_mmio_cpsw_reg), 0,
                           (uintptr_t)(((volatile struct descriptor *) dev->tx_ring_phys) + (dev->tdh - 1)));

    }
}

static int raw_tx(struct eth_driver *driver, unsigned int num, uintptr_t *phys, unsigned int *len, void *cookie)
{
    struct beaglebone_eth_data *dev = (struct beaglebone_eth_data *)driver->eth_data;

    /* Ensure we have room */
    if (num > dev->tx_remain) {
        return ETHIF_TX_FAILED;
    }

    if (len[0] < MIN_PKT_LEN) {
        len[0] = MIN_PKT_LEN;
    }

    dev->tx_ring[dev->tdt].flags_pktlen = len[0];
    dev->tx_ring[dev->tdt].flags_pktlen |= (CPDMA_BUF_DESC_SOP | CPDMA_BUF_DESC_OWNER);

    unsigned int i, ring;
    THREAD_MEMORY_RELEASE();
    for (i = 0; i < num; i++) {
        ring = (dev->tdt + i) % dev->tx_size;
        dev->tx_ring[ring].bufoff_len = len[i] & CPDMA_BD_LEN_MASK;
        dev->tx_ring[ring].bufptr = phys[i];
        dev->tx_ring[ring].next = ((volatile struct descriptor *) dev->tx_ring_phys) + (ring + 1);
        THREAD_MEMORY_RELEASE();
    }

    /* Indicate the end of the packet */
    dev->tx_ring[ring].next = NULL;
    dev->tx_ring[ring].flags_pktlen |= CPDMA_BUF_DESC_EOP;
    dev->tx_ring[ring].flags_pktlen &= ~CPDMA_BUF_DESC_EOQ;

    dev->tx_cookies[dev->tdt] = cookie;
    dev->tx_lengths[dev->tdt] = num;
    dev->tdt = (dev->tdt + num) % dev->tx_size;
    dev->tx_remain -= num;

    THREAD_MEMORY_RELEASE();

    /* For the first time, write the HDP with the filled bd */
    if (dev->tdt - num == 0) {
        CPSWCPDMATxHdrDescPtrWrite(VPTR_CPSW_CPDMA(dev->iomm_address.eth_mmio_cpsw_reg),
                                   (uintptr_t)((volatile struct descriptor *) dev->tx_ring_phys) + (dev->tdt - num), 0);
    } else {
        /**
         * Chain the bd's. If the DMA engine, already reached the end of the chain,
         * the EOQ will be set. In that case, the HDP shall be written again.
         */
        if (dev->tx_ring[ring].flags_pktlen & CPDMA_BUF_DESC_EOQ) {
            CPSWCPDMATxHdrDescPtrWrite(VPTR_CPSW_CPDMA(dev->iomm_address.eth_mmio_cpsw_reg),
                                       (uintptr_t)((volatile struct descriptor *) dev->tx_ring_phys) + (dev->tdt - num), 0);

        }

    }

    return ETHIF_TX_ENQUEUED;
}

static void handle_irq(struct eth_driver *driver, int irq)
{

    struct beaglebone_eth_data *eth_data = (struct beaglebone_eth_data *)driver->eth_data;

    if (irq == SYS_INT_3PGSWRXINT0) {
        complete_rx(driver);
        fill_rx_bufs(driver);
    } else if (irq == SYS_INT_3PGSWTXINT0) {
        complete_tx(driver);
    } else {
        ZF_LOGE("Unrecognised interrupt number %d\n", irq);
    }

}

static void print_state(void)
{
    /* TODO */
}

static void raw_poll(struct eth_driver *driver)
{
    complete_tx(driver);
    complete_rx(driver);
    fill_rx_bufs(driver);
}

static struct raw_iface_funcs iface_fns = {
    .raw_handleIRQ = handle_irq,
    .print_state = print_state,
    .low_level_init = low_level_init,
    .raw_tx = raw_tx,
    .raw_poll = raw_poll
};

int ethif_am335x_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config)
{
    uint8_t mac[LEN_MAC_ADDRESS];
    /* Number if tx and rx buffers is limited by the CPSW/CPPI buffer size */
    uint32_t max_rx_cppi = (SIZE_CPPI_RAM >> 1) / sizeof(struct descriptor);
    uint32_t max_tx_cppi = (SIZE_CPPI_RAM >> 1) / sizeof(struct descriptor);

    struct cpswinst *cpswinst_data = NULL;
    struct cpswportif *cpswPortIf = NULL;
    struct beaglebone_eth_data *eth_data = NULL;

    int err;
    struct EthVirtAddr *eth_addresses = (struct EthVirtAddr *) config;

    eth_data = (struct beaglebone_eth_data *)malloc(sizeof(struct beaglebone_eth_data));
    if (eth_data == NULL) {
        ZF_LOGE("Failed to allocate eth data struct");
        return -1;
    }

    cpswPortIf = (struct cpswportif *) calloc(MAX_CPSW_INST * MAX_SLAVEPORT_PER_INST, sizeof(struct cpswportif));
    if (cpswPortIf == NULL) {
        ZF_LOGE("Failed to allocate cpswPortIf");
        return -1;
    }

    cpswinst_data = (struct cpswinst_data *) calloc(MAX_CPSW_INST, sizeof(struct cpswinst));
    if (cpswinst_data == NULL) {
        ZF_LOGE("Failed to allocate cpswinst");
        return -1;
    }

    compile_time_assert("CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT  <= max_rx_cppi",
                        CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT <= (SIZE_CPPI_RAM >> 1) / sizeof(struct descriptor));
    compile_time_assert("CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT  <= max_tx_cppi",
                        CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT <= (SIZE_CPPI_RAM >> 1) / sizeof(struct descriptor));

    /* Trim the number of buffers requested to the maximum count the hardware can support */
    eth_data->rx_size = CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT;
    eth_data->tx_size = CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT;

    eth_driver->eth_data = eth_data;
    eth_driver->dma_alignment = DMA_ALIGN;
    eth_driver->i_fn = iface_fns;

    /* Tell the driver the mapped virtual addresses of the CPSW device */
    eth_data->iomm_address.eth_mmio_ctr_reg = eth_addresses->eth_mmio_ctr_reg;
    eth_data->iomm_address.eth_mmio_prcm_reg = eth_addresses->eth_mmio_prcm_reg;
    eth_data->iomm_address.eth_mmio_cpsw_reg = eth_addresses->eth_mmio_cpsw_reg;

    err = initialize_desc_ring(eth_data, &io_ops.dma_manager);
    if (err) {
        ZF_LOGE("Failed to allocate descriptor rings");
        return -1;
    }

    CPSWPinMuxSetup((uintptr_t) eth_addresses->eth_mmio_ctr_reg);

    CPSWClkEnable((uintptr_t) eth_addresses->eth_mmio_prcm_reg);

    EVMPortMIIModeSelect((uintptr_t) eth_addresses->eth_mmio_ctr_reg);

    /* Only need one port (port0) */
    EVMMACAddrGet((uintptr_t) eth_addresses->eth_mmio_ctr_reg, 0, mac);

    /* set MAC hardware address */
    for (int temp = 0; temp < LEN_MAC_ADDRESS; temp++) {
        cpswPortIf[0].eth_addr[temp] =
            mac[(LEN_MAC_ADDRESS - 1) - temp];
    }

    eth_data->cpswPortIf = cpswPortIf;
    eth_data->cpswinst   = cpswinst_data;

    fill_rx_bufs(eth_driver);

    /* Initialise CPSW interface */
    if (cpswif_init(eth_driver)) {
        ZF_LOGE("Failed to initialise cpswif\n");
        return -1;
    }

    /* done */
    return 0;
}
