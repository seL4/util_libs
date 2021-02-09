/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <platsupport/driver_module.h>
#include <platsupport/fdt.h>
#include <ethdrivers/gen_config.h>
#include <ethdrivers/odroidc2.h>
#include <ethdrivers/raw.h>
#include <ethdrivers/helpers.h>
#include <utils/util.h>

#include "uboot/common.h"
#include "uboot/net.h"
#include "uboot/miiphy.h"
#include "io.h"
#include "uboot/netdev.h"
#include "unimplemented.h"
#include "uboot/designware.h"

#define BUF_SIZE 2048
#define MAC_LEN 6
#define DMA_ALIGN 64

struct descriptor {
    uint32_t txrx_status;
    uint32_t dmamac_cntl;
    uint32_t dmamac_addr;
    uint32_t dmamac_next;
};

struct odroidc2_eth_data {
    struct eth_device *eth_dev;
    uint8_t mac[MAC_LEN];
    uintptr_t tx_ring_phys;
    uintptr_t rx_ring_phys;
    volatile struct descriptor *tx_ring;
    volatile struct descriptor *rx_ring;
    unsigned int rx_size;
    unsigned int tx_size;
    void **rx_cookies; /* Array of rx_size elements of type 'void *' */
    void **tx_cookies; /* Array of tx_size elements of type 'void *' */
    unsigned int rx_remain;
    unsigned int tx_remain;
    unsigned int *tx_lengths;
    /* Indexes used to keep track of the head and tail of the descriptor queues */
    unsigned int rdt, rdh, tdt, tdh;
};

static bool enabled = false;

static void low_level_init(struct eth_driver *driver, uint8_t *mac, int *mtu)
{
    struct odroidc2_eth_data *dev = (struct odroidc2_eth_data *)driver->eth_data;
    memcpy(mac, dev->mac, MAC_LEN);
    *mtu = MAX_PKT_SIZE;
}

static void fill_rx_bufs(struct eth_driver *driver)
{
    struct odroidc2_eth_data *dev = (struct odroidc2_eth_data *)driver->eth_data;
    __sync_synchronize();
    while (dev->rx_remain > 0) {
        /* request a buffer */
        void *cookie = NULL;
        int next_rdt = (dev->rdt + 1) % dev->rx_size;

        // This fn ptr is either lwip_allocate_rx_buf or lwip_pbuf_allocate_rx_buf (in src/lwip.c)
        uintptr_t phys = driver->i_cb.allocate_rx_buf ? driver->i_cb.allocate_rx_buf(driver->cb_cookie, BUF_SIZE, &cookie) : 0;
        if (!phys) {
            // NOTE: This condition could happen if
            //       CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS < CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT
            break;
        }

        dev->rx_cookies[dev->rdt] = cookie;
        dev->rx_ring[dev->rdt].dmamac_addr = phys;
        dev->rx_ring[dev->rdt].dmamac_cntl = (MAC_MAX_FRAME_SZ & DESC_RXCTRL_SIZE1MASK) | DESC_RXCTRL_RXCHAIN;
        dev->rx_ring[dev->rdt].txrx_status = DESC_RXSTS_OWNBYDMA;

        __sync_synchronize();
        dev->rdt = next_rdt;
        dev->rx_remain--;
    }
    __sync_synchronize();

    /* NOTE Maybe check if receiving isn't enabled? */
    if (enabled) {
        designware_start_receive(dev->eth_dev);
    }
}

static void free_desc_ring(struct odroidc2_eth_data *dev, ps_dma_man_t *dma_man)
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

static int initialize_desc_ring(struct odroidc2_eth_data *dev, ps_dma_man_t *dma_man)
{
    dma_addr_t rx_ring = dma_alloc_pin(dma_man, sizeof(struct descriptor) * dev->rx_size, 0, DMA_ALIGN);
    if (!rx_ring.phys) {
        LOG_ERROR("Failed to allocate rx_ring");
        return -1;
    }
    dev->rx_ring = rx_ring.virt;
    dev->rx_ring_phys = rx_ring.phys;
    dma_addr_t tx_ring = dma_alloc_pin(dma_man, sizeof(struct descriptor) * dev->tx_size, 0, DMA_ALIGN);
    if (!tx_ring.phys) {
        LOG_ERROR("Failed to allocate tx_ring");
        free_desc_ring(dev, dma_man);
        return -1;
    }
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

    uintptr_t next_phys = dev->tx_ring_phys;

    for (unsigned int i = 0; i < dev->tx_size; i++) {
        next_phys += sizeof(struct descriptor);
        dev->tx_ring[i].dmamac_addr = 0;
        if (i == (dev->tx_size - 1)) {
            dev->tx_ring[i].dmamac_next = (uint32_t)(dev->tx_ring_phys & 0xFFFFFFFF);
        } else {
            dev->tx_ring[i].dmamac_next = (uint32_t)(next_phys & 0xFFFFFFFF);
        }
        dev->tx_ring[i].dmamac_cntl = DESC_TXCTRL_TXCHAIN;
        dev->tx_ring[i].txrx_status = 0;
    }

    next_phys = dev->rx_ring_phys;

    for (unsigned int i = 0; i < dev->rx_size; i++) {
        next_phys += sizeof(struct descriptor);
        dev->rx_ring[i].dmamac_addr = 0;
        if (i == (dev->rx_size - 1)) {
            dev->rx_ring[i].dmamac_next = (uint32_t)(dev->rx_ring_phys & 0xFFFFFFFF);
        } else {
            dev->rx_ring[i].dmamac_next = (uint32_t)(next_phys & 0xFFFFFFFF);
        }
        dev->rx_ring[i].dmamac_cntl = (MAC_MAX_FRAME_SZ & DESC_RXCTRL_SIZE1MASK) | DESC_RXCTRL_RXCHAIN;
        dev->rx_ring[i].txrx_status = 0;
    }

    __sync_synchronize();

    return 0;
}

static void complete_rx(struct eth_driver *eth_driver)
{
    struct odroidc2_eth_data *dev = (struct odroidc2_eth_data *)eth_driver->eth_data;
    unsigned int rdt = dev->rdt;
    while (dev->rdh != rdt) {
        unsigned int status = dev->rx_ring[dev->rdh].txrx_status;
        /* Ensure no memory references get ordered before we checked the descriptor was written back */
        __sync_synchronize();
        if (status & DESC_RXSTS_OWNBYDMA) {
            /* not complete yet */
            break;
        }
        void *cookie = dev->rx_cookies[dev->rdh];
        unsigned int len = (status & DESC_RXSTS_FRMLENMSK) >> DESC_RXSTS_FRMLENSHFT;
        /* update rdh */
        dev->rdh = (dev->rdh + 1) % dev->rx_size;
        dev->rx_remain++;
        /* Give the buffers back */
        eth_driver->i_cb.rx_complete(eth_driver->cb_cookie, 1, &cookie, &len);
    }
    /* NOTE Maybe re-enable the Ethernet device for RX if there are still descriptors? */
}

static void complete_tx(struct eth_driver *driver)
{
    struct odroidc2_eth_data *dev = (struct odroidc2_eth_data *)driver->eth_data;
    while (dev->tdh != dev->tdt) {
        unsigned int i;
        for (i = 0; i < dev->tx_lengths[dev->tdh]; i++) {
            if (dev->tx_ring[dev->tdh + i].txrx_status & DESC_TXSTS_OWNBYDMA) {
                /* Not yet complete */
                return;
            }
        }
        /* do not let memory loads happen before our checking of the descriptor write back */
        __sync_synchronize();
        /* increase where we believe tdh to be */
        void *cookie = dev->tx_cookies[dev->tdh];
        dev->tx_remain += dev->tx_lengths[dev->tdh];
        dev->tdh = (dev->tdh + dev->tx_lengths[dev->tdh]) % dev->tx_size;
        /* give the buffer back */
        driver->i_cb.tx_complete(driver->cb_cookie, cookie);
    }
    /* NOTE Maybe re-enable the Ethernet device for TX if there are still descriptors? */
}

static void print_state(struct eth_driver *eth_driver)
{
    ZF_LOGE("Not implemented");
}

static void handle_irq(struct eth_driver *driver, int irq)
{
    struct odroidc2_eth_data *eth_data = (struct odroidc2_eth_data *)driver->eth_data;
    uint32_t status = 0;
    designware_interrupt_status(eth_data->eth_dev, &status);
    designware_ack(eth_data->eth_dev, status);
    if (status & DMA_INTR_ENA_TIE) {
        complete_tx(driver);
    }
    if (status & DMA_INTR_ENA_RIE) {
        complete_rx(driver);
        fill_rx_bufs(driver);
    }
    if (status & DMA_INTR_ABNORMAL) {
        ZF_LOGE("Ethernet device crashed with an abnormal interrupt, reporting...");
        if (status & DMA_INTR_ENA_FBE) {
            ZF_LOGE("    Ethernet device fatal bus error");
        }
        if (status & DMA_INTR_ENA_UNE) {
            ZF_LOGE("    Ethernet device TX underflow");
        }
        ZF_LOGF("Done.");
    }
}

/* This is a platsuport IRQ interface IRQ handler wrapper for handle_irq() */
static void eth_irq_handle(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    ZF_LOGF_IF(data == NULL, "Passed in NULL for the data");
    struct eth_driver *driver = data;

    /* handle_irq doesn't really expect an IRQ number */
    handle_irq(driver, 0);

    int error = acknowledge_fn(ack_data);
    if (error) {
        LOG_ERROR("Failed to acknowledge the Ethernet device's IRQ");
    }
}

static void raw_poll(struct eth_driver *driver)
{
    complete_rx(driver);
    complete_tx(driver);
    fill_rx_bufs(driver);
}

static int raw_tx(struct eth_driver *driver, unsigned int num, uintptr_t *phys, unsigned int *len, void *cookie)
{
    struct odroidc2_eth_data *dev = (struct odroidc2_eth_data *)driver->eth_data;
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
    for (i = 0; i < num; i++) {
        unsigned int ring = (dev->tdt + i) % dev->tx_size;
        dev->tx_ring[ring].dmamac_addr = phys[i];
        dev->tx_ring[ring].dmamac_cntl = DESC_TXCTRL_TXCHAIN;
        dev->tx_ring[ring].dmamac_cntl |= (len[i] << DESC_TXCTRL_SIZE1SHFT) & DESC_TXCTRL_SIZE1MASK;
        if (i == 0) {
            dev->tx_ring[ring].dmamac_cntl |= DESC_TXCTRL_TXFIRST;
        }
        if (i == (num - 1)) {
            dev->tx_ring[ring].dmamac_cntl |= DESC_TXCTRL_TXLAST;
        }
        dev->tx_ring[ring].dmamac_cntl |= DESC_TXCTRL_TXINT;
        dev->tx_ring[ring].txrx_status = DESC_TXSTS_OWNBYDMA;
    }
    dev->tx_cookies[dev->tdt] = cookie;
    dev->tx_lengths[dev->tdt] = num;
    dev->tdt = (dev->tdt + num) % dev->tx_size;
    dev->tx_remain -= num;
    __sync_synchronize();

    /* NOTE Maybe check if it's in the middle of sending? */
    designware_start_send(dev->eth_dev);

    return ETHIF_TX_ENQUEUED;
}

static void get_mac(struct eth_driver *driver, uint8_t *mac)
{
    struct odroidc2_eth_data *eth_data = (struct odroidc2_eth_data *) driver->eth_data;
    memcpy(mac, eth_data->mac, MAC_LEN);
}

static struct raw_iface_funcs iface_fns = {
    .raw_handleIRQ = handle_irq,
    .print_state = print_state,
    .low_level_init = low_level_init,
    .raw_tx = raw_tx,
    .raw_poll = raw_poll,
    .get_mac = get_mac
};

int ethif_odroidc2_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config)
{
    int err;
    struct odroidc2_eth_data *eth_data = NULL;
    struct eth_device *eth_dev;

    if (config == NULL) {
        ZF_LOGE("Cannot get platform info; passed in config pointer NULL");
        goto error;
    }

    struct arm_eth_plat_config *plat_config = (struct arm_eth_plat_config *)config;
    unsigned long base_addr = (unsigned long)((uintptr_t) plat_config->buffer_addr);

    eth_data = (struct odroidc2_eth_data *)malloc(sizeof(struct odroidc2_eth_data));
    if (eth_data == NULL) {
        ZF_LOGE("Failed to allocate eth data struct");
        goto error;
    }

    eth_data->tx_size = CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT;
    eth_data->rx_size = CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT;
    eth_driver->eth_data = eth_data;
    eth_driver->dma_alignment = DMA_ALIGN;
    eth_driver->i_fn = iface_fns;

    err = initialize_desc_ring(eth_data, &io_ops.dma_manager);
    if (err) {
        LOG_ERROR("Failed to allocate descriptor rings");
        goto error;
    }

    /* Initialise the MII abstraction layer */
    miiphy_init();

    /* Initialise the actual Realtek PHY */
    phy_init();

    /* Do some more PHY init, setup structures */
    eth_dev = malloc(sizeof(*eth_dev));
    if (NULL == eth_dev) {
        ZF_LOGE("Failed to allocate eth_dev structure");
        goto error;
    }

    int ret = designware_initialize(base_addr, 0, eth_dev);
    if (ret != 0) {
        ZF_LOGE("Failed: designware_initialize.");
        goto error;
    }

    eth_data->eth_dev = eth_dev;

    /* We must read the MAC address out of the device after the register
     * addresses have been initialized (designware_initialize), but before
     * initializing the hardware (uboot_eth_dev.init), because initializing
     * the hardware involves a soft reset which will wipe the MAC address
     * which was configured by u-boot */

    ret = designware_read_hwaddr(eth_dev, eth_data->mac);
    if (ret != 0) {
        ZF_LOGE("Failed: designware_read_hwaddr.");
        goto error;
    }

    /* NOTE: This line determines what our MAC address will be, it is
     * internally programmed into the device during the next init step*/
    memcpy(eth_dev->enetaddr, eth_data->mac, 6);

    /* Bring up the interface */
    ret = designware_startup(eth_dev);
    if (ret != 0) {
        ZF_LOGE("Failed: designware_startup");
        goto error;
    }

    ret = designware_write_descriptors(eth_dev, eth_data->tx_ring_phys, eth_data->rx_ring_phys);
    if (ret != 0) {
        ZF_LOGE("Failed: designware_write_descriptors");
        goto error;
    }

    fill_rx_bufs(eth_driver);

    enabled = true;

    /* Last step, enable it */
    ret = designware_enable(eth_dev);
    if (ret != 0) {
        ZF_LOGE("Failed: designware_enable");
    }

    return 0;
error:
    if (eth_data) {
        free(eth_data);
    }
    if (eth_dev) {
        free(eth_dev);
    }
    free_desc_ring(eth_data, &io_ops.dma_manager);
    return -1;
}

int ethif_odroidc2_init_module(ps_io_ops_t *io_ops, const char *device_path)
{
    struct arm_eth_plat_config plat_config;
    struct eth_driver *eth_driver;

    int error = ps_calloc(&io_ops->malloc_ops, 1, sizeof(*eth_driver), (void **) &eth_driver);
    if (error) {
        ZF_LOGE("Failed to allocate memory for the Ethernet driver");
        return -ENOMEM;
    }

    ps_fdt_cookie_t *cookie = NULL;
    error = ps_fdt_read_path(&io_ops->io_fdt, &io_ops->malloc_ops, device_path, &cookie);
    if (error) {
        ZF_LOGE("Failed to read the path of the Ethernet driver");
        return -ENODEV;
    }

    void *mapped_addr = ps_fdt_index_map_register(io_ops, cookie, 0, NULL);
    if (mapped_addr == NULL) {
        ZF_LOGE("Failed to map the registers");
        return -ENODEV;
    }

    irq_id_t eth_irq_id = ps_fdt_index_register_irq(io_ops, cookie, 0, eth_irq_handle, eth_driver);
    if (eth_irq_id < 0) {
        ZF_LOGE("Failed to register the Ethernet's IRQ");
        return -ENODEV;
    }

    plat_config.buffer_addr = mapped_addr;
    plat_config.prom_mode = 1;

    error = ethif_odroidc2_init(eth_driver, *io_ops, &plat_config);
    if (error) {
        ZF_LOGE("Failed to initialise the Ethernet driver");
        return -ENODEV;
    }

    return ps_interface_register(&io_ops->interface_registration_ops, PS_ETHERNET_INTERFACE, eth_driver, NULL);
}

static const char *compatible_strings[] = {
    "amlogic,meson-gx-dwmac",
    "amlogic,meson-gxbb-dwmac",
    "snps,dwmac",
    NULL
};
PS_DRIVER_MODULE_DEFINE(odroidc2_ethernet, compatible_strings, ethif_odroidc2_init_module);
