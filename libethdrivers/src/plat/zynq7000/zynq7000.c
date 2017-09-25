/*
 * Copyright 2017, DornerWorks, Ltd.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include "unimplemented.h"
#include "io.h"
#include <ethdrivers/zynq7000.h>
#include <ethdrivers/raw.h>
#include <ethdrivers/helpers.h>
#include <string.h>
#include <utils/util.h>
#include "zynq_gem.h"
#include "uboot/net.h"
#include "uboot/miiphy.h"
#include "uboot/phy.h"
#include <stdio.h>

#define BUF_SIZE MAX_PKT_SIZE

struct zynq7000_eth_data {
    struct eth_device *eth_dev;
    uintptr_t tx_ring_phys;
    uintptr_t rx_ring_phys;
    volatile struct emac_bd *tx_ring;
    volatile struct emac_bd *rx_ring;
    unsigned int rx_size;
    unsigned int tx_size;
    struct dma_buf_cookie *rx_cookies;
    unsigned int rx_remain;
    unsigned int tx_remain;
    void **tx_cookies;
    unsigned int *tx_lengths;
    /* track where the head and tail of the queues are for
     * enqueueing buffers / checking for completions */
    unsigned int rdt, rdh, tdt, tdh;
};

static void free_desc_ring(struct zynq7000_eth_data *dev, ps_dma_man_t *dma_man) {

    if (dev->rx_ring != NULL) {
        dma_unpin_free(dma_man, (void*)dev->rx_ring, sizeof(struct emac_bd) * dev->rx_size);
        dev->rx_ring = NULL;
    }

    if (dev->tx_ring != NULL) {
        dma_unpin_free(dma_man, (void*)dev->tx_ring, sizeof(struct emac_bd) * dev->tx_size);
        dev->tx_ring = NULL;
    }

    if (dev->rx_cookies != NULL) {
        free(dev->rx_cookies);
        dev->rx_cookies = NULL;
    }

    if (dev->tx_cookies != NULL) {
        free(dev->tx_cookies);
        dev->tx_cookies = NULL;
    }

    if (dev->tx_lengths != NULL) {
        free(dev->tx_lengths);
        dev->tx_lengths = NULL;
    }
}

static int initialize_desc_ring(struct zynq7000_eth_data *dev, ps_dma_man_t *dma_man)
{
    dma_addr_t rx_ring = dma_alloc_pin(dma_man, sizeof(struct emac_bd) * dev->rx_size, 0, ARCH_DMA_MINALIGN);
    if (!rx_ring.phys) {
        LOG_ERROR("Failed to allocate rx_ring");
        return -1;
    }

    dev->rx_ring = rx_ring.virt;
    dev->rx_ring_phys = rx_ring.phys;
    dma_addr_t tx_ring = dma_alloc_pin(dma_man, sizeof(struct emac_bd) * dev->tx_size, 0, ARCH_DMA_MINALIGN);
    if (!tx_ring.phys) {
        LOG_ERROR("Failed to allocate tx_ring");
        free_desc_ring(dev, dma_man);
        return -1;
    }

    ps_dma_cache_clean_invalidate(dma_man, rx_ring.virt, sizeof(struct emac_bd) * dev->rx_size);
    ps_dma_cache_clean_invalidate(dma_man, tx_ring.virt, sizeof(struct emac_bd) * dev->tx_size);

    dev->rx_cookies = malloc(sizeof(struct dma_buf_cookie) * dev->rx_size);
    dev->tx_cookies = malloc(sizeof(void*) * dev->tx_size);
    dev->tx_lengths = malloc(sizeof(unsigned int) * dev->tx_size);

    if (dev->rx_cookies == NULL || dev->tx_cookies == NULL || dev->tx_lengths == NULL) {

        if (dev->rx_cookies != NULL) {
            free(dev->rx_cookies);
        }

        if (dev->tx_cookies != NULL) {
            free(dev->tx_cookies);
        }

        if (dev->tx_lengths != NULL) {
            free(dev->tx_lengths);
        }

        LOG_ERROR("Failed to malloc");
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
        dev->tx_ring[i] = (struct emac_bd) {
            .addr = 0,
            .status = 0
        };
    }

    for (unsigned int i = 0; i < dev->rx_size; i++) {
        dev->rx_ring[i] = (struct emac_bd) {
            .addr = 0,
            .status = 0
        };
    }

    __sync_synchronize();

    return 0;
}

static void fill_rx_bufs(struct eth_driver *driver) {

    struct zynq7000_eth_data *dev = (struct zynq7000_eth_data*)driver->eth_data;
    __sync_synchronize();

    while (dev->rx_remain > 0) {

        /* request a buffer */
        struct dma_buf_cookie cookie_bufs;
        void *cookie = &cookie_bufs;
        int next_rdt = (dev->rdt + 1) % dev->rx_size;

        uintptr_t phys = driver->i_cb.allocate_rx_buf(driver->cb_cookie, BUF_SIZE, &cookie);
        if (!phys) {
            break;
        }

        dev->rx_cookies[dev->rdt].vbuf = cookie_bufs.vbuf;
        dev->rx_cookies[dev->rdt].pbuf = cookie_bufs.pbuf;

        /* If this is the last descriptor in the ring, set the wrap bit of the address (bit 1)
         *   so the controller knows to loop back around
         */
        dev->rx_ring[dev->rdt].status = 0;

        uint32_t mask = (next_rdt == 0 ? ZYNQ_GEM_RXBUF_WRAP_MASK : 0);
        dev->rx_ring[dev->rdt].addr = (phys & ZYNQ_GEM_RXBUF_ADD_MASK) | mask;

        __sync_synchronize();

        dev->rdt = next_rdt;
        dev->rx_remain--;
    }

    __sync_synchronize();

    if (dev->rdt != dev->rdh && !zynq_gem_recv_enabled(dev->eth_dev)) {
        zynq_gem_recv_enable(dev->eth_dev);
    }
}

static void complete_rx(struct eth_driver *eth_driver) {

    struct zynq7000_eth_data *dev = (struct zynq7000_eth_data*)eth_driver->eth_data;
    unsigned int rdt = dev->rdt;

    while (dev->rdh != rdt) {
        unsigned int status = dev->rx_ring[dev->rdh].status;
        unsigned int addr = dev->rx_ring[dev->rdh].addr;

        /* Ensure no memory references get ordered before we checked the descriptor was written back */
        __sync_synchronize();
        if (!(addr & ZYNQ_GEM_RXBUF_NEW_MASK)) {
            /* not complete yet */
            break;
        }

        // TBD: Need to handle multiple buffers for single frame?
        void *cookie = &dev->rx_cookies[dev->rdh];
        unsigned int len = status & ZYNQ_GEM_RXBUF_LEN_MASK;

        /* update rdh */
        dev->rdh = (dev->rdh + 1) % dev->rx_size;
        dev->rx_remain++;

        /* Give the buffers back */
        eth_driver->i_cb.rx_complete(eth_driver->cb_cookie, 1, &cookie, &len);
    }

    if (dev->rdt != dev->rdh && !zynq_gem_recv_enabled(dev->eth_dev)) {
        zynq_gem_recv_enabled(dev->eth_dev);
    }
}

static void complete_tx(struct eth_driver *driver) {

    struct zynq7000_eth_data *dev = (struct zynq7000_eth_data*)driver->eth_data;

    while (dev->tdh != dev->tdt) {
        unsigned int i;

        for (i = 0; i < dev->tx_lengths[dev->tdh]; i++) {
            int ring_pos = (i + dev->tdh) % dev->tx_size;

            if (!(dev->tx_ring[ring_pos].status & ZYNQ_GEM_TXBUF_USED_MASK)) {
                /* not all parts complete */
                return;
            }
        }

        /* do not let memory loads happen before our checking of the descriptor write back */
        __sync_synchronize();

        /* increase TX Descriptor head */
        void *cookie = dev->tx_cookies[dev->tdh];
        dev->tx_remain += dev->tx_lengths[dev->tdh];
        dev->tdh = (dev->tdh + dev->tx_lengths[dev->tdh]) % dev->tx_size;

        /* give the buffer back */
        driver->i_cb.tx_complete(driver->cb_cookie, cookie);
    }

    if(dev->tdh != dev->tdt) {
        uintptr_t txbase = dev->tx_ring_phys + (uintptr_t)(dev->tdh*sizeof(struct emac_bd));
        zynq_gem_start_send(dev->eth_dev, txbase);
    }
}

static void
handle_irq(struct eth_driver *driver, int irq)
{
    struct zynq7000_eth_data *eth_data = (struct zynq7000_eth_data*)driver->eth_data;
    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)eth_data->eth_dev->iobase;

    // Clear Interrupts
    u32 val = readl(&regs->isr);
    writel(val, &regs->isr);

    if(val & ZYNQ_GEM_IXR_TXCOMPLETE)
    {
      /* Clear TX Status register */
      val = readl(&regs->txsr);
      writel(val, &regs->txsr);

      complete_tx(driver);
    }

    if(val & ZYNQ_GEM_IXR_FRAMERX)
    {
      /* Clear RX Status register */
      val = readl(&regs->rxsr);
      writel(val, &regs->rxsr);

      complete_rx(driver);
      fill_rx_bufs(driver);
    }
}

static void print_state(struct eth_driver *eth_driver) {
    printf("Zynq7000: print_state not implemented\n");
}

static void
low_level_init(struct eth_driver *driver, uint8_t *mac, int *mtu)
{
    printf("Zynq7000: low_level_init not implemented\n");
}

static int raw_tx(struct eth_driver *driver, unsigned int num, uintptr_t *phys, unsigned int *len, void *cookie) {

    struct zynq7000_eth_data *dev = (struct zynq7000_eth_data*)driver->eth_data;

    /* Ensure we have room */
    if (dev->tx_remain < num) {
        /* try and complete some */
        complete_tx(driver);

        if (dev->tx_remain < num) {
            return ETHIF_TX_FAILED;
        }
    }

    unsigned int i;
    __sync_synchronize();

    uintptr_t txbase = dev->tx_ring_phys + (uintptr_t)(dev->tdt*sizeof(struct emac_bd));

    for (i = 0; i < num; i++) {

        unsigned int ring = (dev->tdt + i) % dev->tx_size;
        dev->tx_ring[ring].addr = phys[i];
        dev->tx_ring[ring].status = (len[i] & ZYNQ_GEM_TXBUF_FRMLEN_MASK) |
                   ZYNQ_GEM_TXBUF_LAST_MASK;

        __sync_synchronize();
    }

    unsigned int ring = (dev->tdt + i) % dev->tx_size;

    /* Dummy descriptor to mark it as the last in descriptor chain */
    dev->tx_ring[ring].addr = 0x0;
    dev->tx_ring[ring].status = ZYNQ_GEM_TXBUF_WRAP_MASK |
        ZYNQ_GEM_TXBUF_LAST_MASK|
        ZYNQ_GEM_TXBUF_USED_MASK;

    /* Increment num by 1 to account for the added last byte */
    num += 1;

    dev->tx_cookies[dev->tdt] = cookie;
    dev->tx_lengths[dev->tdt] = num;
    dev->tdt = (dev->tdt + num) % dev->tx_size;
    dev->tx_remain -= num;

    __sync_synchronize();

    zynq_gem_start_send(dev->eth_dev, txbase);

    return ETHIF_TX_ENQUEUED;
}

static void raw_poll(struct eth_driver *driver) {
    complete_rx(driver);
    complete_tx(driver);
    fill_rx_bufs(driver);
}

static struct raw_iface_funcs iface_fns = {
    .raw_handleIRQ = handle_irq,
    .print_state = print_state,
    .low_level_init = low_level_init,
    .raw_tx = raw_tx,
    .raw_poll = raw_poll
};

int ethif_zynq7000_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config) {
    int err;
    struct eth_plat_config *plat_config = (struct eth_plat_config *)config;
    struct zynq7000_eth_data *eth_data = NULL;
    uint32_t base_addr = (uint32_t)plat_config->buffer_addr;
    struct eth_device *eth_dev;

    printf("ethif_zynq7000_init: Start\n");

    eth_data = (struct zynq7000_eth_data*)malloc(sizeof(struct zynq7000_eth_data));
    if (eth_data == NULL) {
        LOG_ERROR("Failed to allocate eth data struct");
        goto error;
    }

    eth_data->tx_size = CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT;
    eth_data->rx_size = CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT;
    eth_driver->eth_data = eth_data;
    eth_driver->dma_alignment = ARCH_DMA_MINALIGN;
    eth_driver->i_fn = iface_fns;

    /* Initialize Descriptors */
    err = initialize_desc_ring(eth_data, &io_ops.dma_manager);
    if (err) {
        LOG_ERROR("Failed to allocate descriptor rings");
        goto error;
    }

#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII) || defined(CONFIG_PHYLIB)
	 miiphy_init();
#endif

#ifdef CONFIG_PHYLIB
    phy_init();
#endif

    zynq_set_gem_ioops(&io_ops);

    eth_dev = (struct eth_device *)zynq_gem_initialize(base_addr,
  				   CONFIG_ZYNQ_GEM_PHY_ADDR0,
  				   CONFIG_ZYNQ_GEM_EMIO0);
    if (NULL == eth_dev) {
        LOG_ERROR("Failed to initialize Zynq Ethernet Device");
        goto error;
    }
    eth_data->eth_dev = eth_dev;

    zynq_gem_setup_mac(eth_dev);

    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)eth_dev->iobase;

    /* Initialize the buffer descriptor registers */
    writel((uint32_t)eth_data->tx_ring_phys, &regs->txqbase);
    writel((uint32_t)eth_data->rx_ring_phys, &regs->rxqbase);

    zynq_gem_init(eth_dev);

    if (plat_config->prom_mode) {
        zynq_gem_prom_enable(eth_dev);
    }
    else {
        memcpy(eth_dev->enetaddr, plat_config->mac_addr, 6);
        zynq_gem_setup_mac(eth_dev);
        zynq_gem_prom_disable(eth_dev);
    }

    fill_rx_bufs(eth_driver);

    /* done */
    return 0;
error:
    if (eth_data != NULL) {
        free(eth_data);
    }
    free_desc_ring(eth_data, &io_ops.dma_manager);
    return -1;
}
