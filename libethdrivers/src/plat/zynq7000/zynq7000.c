/*
 * Copyright 2017, DornerWorks, Ltd.
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "unimplemented.h"
#include "io.h"
#include <platsupport/driver_module.h>
#include <platsupport/fdt.h>
#include <ethdrivers/gen_config.h>
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
    uintptr_t dummy_tx_bd_phys;
    uintptr_t dummy_rx_bd_phys;
    volatile struct emac_bd *dummy_tx_bd;
    volatile struct emac_bd *dummy_rx_bd;
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

static void free_desc_ring(struct zynq7000_eth_data *dev, ps_dma_man_t *dma_man)
{

    if (dev->rx_ring != NULL) {
        dma_unpin_free(dma_man, (void *)dev->rx_ring, sizeof(struct emac_bd) * dev->rx_size);
        dev->rx_ring = NULL;
    }

    if (dev->tx_ring != NULL) {
        dma_unpin_free(dma_man, (void *)dev->tx_ring, sizeof(struct emac_bd) * dev->tx_size);
        dev->tx_ring = NULL;
    }

#ifdef CONFIG_PLAT_ZYNQMP
    if (dev->dummy_rx_bd != NULL) {
        dma_unpin_free(dma_man, (void *)dev->dummy_rx_bd, sizeof(struct emac_bd));
        dev->dummy_rx_bd = NULL;
    }

    if (dev->dummy_tx_bd != NULL) {
        dma_unpin_free(dma_man, (void *)dev->dummy_tx_bd, sizeof(struct emac_bd));
        dev->dummy_tx_bd = NULL;
    }
#endif

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

#ifdef CONFIG_PLAT_ZYNQMP
    dma_addr_t dummy_rx_ring = dma_alloc_pin(dma_man, sizeof(struct emac_bd), 0, ARCH_DMA_MINALIGN);
    if (!dummy_rx_ring.phys) {
        LOG_ERROR("Failed to allocate dummy rx_ring");
        free_desc_ring(dev, dma_man);
        return -1;
    }
    dev->dummy_rx_bd = dummy_rx_ring.virt;
    dev->dummy_rx_bd_phys = dummy_rx_ring.phys;

    dma_addr_t dummy_tx_ring = dma_alloc_pin(dma_man, sizeof(struct emac_bd), 0, ARCH_DMA_MINALIGN);
    if (!dummy_tx_ring.phys) {
        LOG_ERROR("Failed to allocate dummy tx_ring");
        free_desc_ring(dev, dma_man);
        return -1;
    }
    dev->dummy_tx_bd = dummy_tx_ring.virt;
    dev->dummy_tx_bd_phys = dummy_tx_ring.phys;

    ps_dma_cache_clean_invalidate(dma_man, dummy_rx_ring.virt, sizeof(struct emac_bd));
    ps_dma_cache_clean_invalidate(dma_man, dummy_tx_ring.virt, sizeof(struct emac_bd));
#endif

    dev->rx_cookies = malloc(sizeof(void *) * dev->rx_size);
    dev->tx_cookies = malloc(sizeof(void *) * dev->tx_size);
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

    /* initialise both rings */
    for (unsigned int i = 0; i < dev->tx_size; i++) {
        dev->tx_ring[i] = (struct emac_bd) {
            .addr = 0,
            .status = ZYNQ_GEM_TXBUF_USED_MASK,
#ifdef CONFIG_PLAT_ZYNQMP
            .addr_hi = 0
#endif
        };
    }

    dev->tx_ring[dev->tx_size - 1].status |= ZYNQ_GEM_TXBUF_WRAP_MASK;

    for (unsigned int i = 0; i < dev->rx_size; i++) {
        dev->rx_ring[i] = (struct emac_bd) {
            .addr = ZYNQ_GEM_RXBUF_NEW_MASK,
            .status = 0,
#ifdef CONFIG_PLAT_ZYNQMP
            .addr_hi = 0,
#endif
        };
    }

    dev->rx_ring[dev->rx_size - 1].addr |= ZYNQ_GEM_RXBUF_WRAP_MASK;

#ifdef CONFIG_PLAT_ZYNQMP
    /* initialise the dummy rings */
    dev->dummy_tx_bd->addr = 0;
    dev->dummy_tx_bd->addr_hi = 0;
    dev->dummy_tx_bd->status = ZYNQ_GEM_TXBUF_USED_MASK | ZYNQ_GEM_TXBUF_WRAP_MASK
                                                        | ZYNQ_GEM_TXBUF_LAST_MASK;

    dev->dummy_rx_bd->addr = ZYNQ_GEM_RXBUF_NEW_MASK | ZYNQ_GEM_RXBUF_WRAP_MASK;
    dev->dummy_rx_bd->addr_hi = 0;
    dev->dummy_rx_bd->status = 0;
#endif

    __sync_synchronize();

    return 0;
}

static void fill_rx_bufs(struct eth_driver *driver)
{
    struct zynq7000_eth_data *dev = (struct zynq7000_eth_data *)driver->eth_data;
    __sync_synchronize();

    while (dev->rx_remain > 0) {

        /* request a buffer */
        void *cookie = NULL;
        int next_rdt = (dev->rdt + 1) % dev->rx_size;

        uintptr_t phys = driver->i_cb.allocate_rx_buf ? driver->i_cb.allocate_rx_buf(driver->cb_cookie, BUF_SIZE, &cookie) : 0;
        if (!phys) {
            break;
        }

        dev->rx_cookies[dev->rdt] = cookie;

        dev->rx_ring[dev->rdt].status = 0;

        /* Remove the used bit so the controller knows this descriptor is
         * available to be written to */
        dev->rx_ring[dev->rdt].addr &= ~(ZYNQ_GEM_RXBUF_NEW_MASK | ZYNQ_GEM_RXBUF_ADD_MASK);
        dev->rx_ring[dev->rdt].addr |= (phys & ZYNQ_GEM_RXBUF_ADD_MASK);
#ifdef CONFIG_PLAT_ZYNQMP
        dev->rx_ring[dev->rdt].addr_hi = (phys >> 32);
#endif

        __sync_synchronize();

        dev->rdt = next_rdt;
        dev->rx_remain--;
    }

    __sync_synchronize();

    if (dev->rdt != dev->rdh && !zynq_gem_recv_enabled(dev->eth_dev)) {
        zynq_gem_recv_enable(dev->eth_dev);
    }
}

static void complete_rx(struct eth_driver *eth_driver)
{

    struct zynq7000_eth_data *dev = (struct zynq7000_eth_data *)eth_driver->eth_data;
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
        void *cookie = dev->rx_cookies[dev->rdh];
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

static void complete_tx(struct eth_driver *driver)
{

    struct zynq7000_eth_data *dev = (struct zynq7000_eth_data *)driver->eth_data;

    while (dev->tdh != dev->tdt) {
        unsigned int i;

        for (i = 0; i < dev->tx_lengths[dev->tdh]; i++) {
            int ring_pos = (i + dev->tdh) % dev->tx_size;

            if (i == 0 && !(dev->tx_ring[ring_pos].status & ZYNQ_GEM_TXBUF_USED_MASK)) {
                /* not all parts complete */
                return;
            }

            dev->tx_ring[ring_pos].status &= (ZYNQ_GEM_TXBUF_USED_MASK | ZYNQ_GEM_TXBUF_WRAP_MASK);
            dev->tx_ring[ring_pos].status |= ZYNQ_GEM_TXBUF_USED_MASK;
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

    if (dev->tdh != dev->tdt) {
        zynq_gem_start_send(dev->eth_dev);
    }
}

static void handle_irq(struct eth_driver *driver, int irq)
{
    struct zynq7000_eth_data *eth_data = (struct zynq7000_eth_data *)driver->eth_data;
    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)eth_data->eth_dev->iobase;

    // Clear Interrupts
    u32 isr = readl(&regs->isr);
    writel(isr, &regs->isr);

    if (isr & ZYNQ_GEM_IXR_TXCOMPLETE) {
        /* Clear TX Status register */
        u32 val = readl(&regs->txsr);
        writel(val, &regs->txsr);

        complete_tx(driver);
    }

    if (isr & ZYNQ_GEM_IXR_FRAMERX) {
        /* Clear RX Status register */
        u32 val = readl(&regs->rxsr);
        writel(val, &regs->rxsr);

        complete_rx(driver);
        fill_rx_bufs(driver);
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

static void print_state(struct eth_driver *eth_driver)
{
    printf("Zynq7000: print_state not implemented\n");
}

static void low_level_init(struct eth_driver *driver, uint8_t *mac, int *mtu)
{
    printf("Zynq7000: low_level_init not implemented\n");
}

static int raw_tx(struct eth_driver *driver, unsigned int num, uintptr_t *phys, unsigned int *len, void *cookie)
{

    struct zynq7000_eth_data *dev = (struct zynq7000_eth_data *)driver->eth_data;

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
        dev->tx_ring[ring].addr = (phys[i] & 0xFFFFFFFF);
#ifdef CONFIG_PLAT_ZYNQMP
        dev->tx_ring[ring].addr_hi = (phys[i] >> 32);
#endif
        dev->tx_ring[ring].status &= ~(ZYNQ_GEM_TXBUF_USED_MASK | ZYNQ_GEM_TXBUF_FRMLEN_MASK | ZYNQ_GEM_TXBUF_LAST_MASK);
        dev->tx_ring[ring].status |= (len[i] & ZYNQ_GEM_TXBUF_FRMLEN_MASK);
        if (i == (num - 1)) {
            dev->tx_ring[ring].status |= ZYNQ_GEM_TXBUF_LAST_MASK;
        }
    }

    dev->tx_cookies[dev->tdt] = cookie;
    dev->tx_lengths[dev->tdt] = num;
    dev->tdt = (dev->tdt + num) % dev->tx_size;
    dev->tx_remain -= num;

    __sync_synchronize();

    zynq_gem_start_send(dev->eth_dev);

    return ETHIF_TX_ENQUEUED;
}

static void raw_poll(struct eth_driver *driver)
{
    complete_rx(driver);
    complete_tx(driver);
    fill_rx_bufs(driver);
}

static void get_mac(struct eth_driver *driver, uint8_t *mac)
{
    struct eth_device *eth_dev = ((struct zynq7000_eth_data *)driver->eth_data)->eth_dev;
    memcpy(mac, eth_dev->enetaddr, 6);
}


static struct raw_iface_funcs iface_fns = {
    .raw_handleIRQ = handle_irq,
    .print_state = print_state,
    .low_level_init = low_level_init,
    .raw_tx = raw_tx,
    .raw_poll = raw_poll,
    .get_mac = get_mac
};

int ethif_zynq7000_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config)
{
    int err;
    struct arm_eth_plat_config *plat_config = (struct arm_eth_plat_config *)config;
    struct zynq7000_eth_data *eth_data = NULL;
    struct eth_device *eth_dev;

    printf("ethif_zynq7000_init: Start\n");

    eth_data = (struct zynq7000_eth_data *)malloc(sizeof(struct zynq7000_eth_data));
    if (eth_data == NULL) {
        LOG_ERROR("Failed to allocate eth data struct");
        goto error;
    }

    if (config == NULL) {
        LOG_ERROR("Cannot get platform info; Passed in Config Pointer NULL");
        goto error;
    }
    uint32_t base_addr = (uint32_t)((uintptr_t)plat_config->buffer_addr);

    eth_data->tx_size = CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT;
    eth_data->rx_size = CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT;
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

    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)eth_dev->iobase;

    /* Initialize the buffer descriptor registers */
    writel((uint32_t)eth_data->tx_ring_phys, &regs->txqbase);
    writel((uint32_t)eth_data->rx_ring_phys, &regs->rxqbase);

#ifdef CONFIG_PLAT_ZYNQMP
    writel((uint32_t)(eth_data->tx_ring_phys >> 32), &regs->upper_txqbase);
    writel((uint32_t)(eth_data->rx_ring_phys >> 32), &regs->upper_rxqbase);
    writel((uint32_t)(eth_data->dummy_tx_bd_phys), &regs->transmit_q1_ptr);
    writel((uint32_t)(eth_data->dummy_rx_bd_phys), &regs->receive_q1_ptr);
#endif

    zynq_gem_init(eth_dev);

    if (plat_config->prom_mode) {
        zynq_gem_prom_enable(eth_dev);
    } else {
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

typedef struct {
    void *addr;
    ps_io_ops_t *io_ops;
    struct eth_driver *eth_driver;
    int irq_id;
} callback_args_t;

static int allocate_register_callback(pmem_region_t pmem, unsigned curr_num, size_t num_regs, void *token)
{
    if (token == NULL) {
        ZF_LOGE("Expected a token!");
        return -EINVAL;
    }

    callback_args_t *args = token;
    if (curr_num == 0) {
        args->addr = ps_pmem_map(args->io_ops, pmem, false, PS_MEM_NORMAL);
        if (!args->addr) {
            ZF_LOGE("Failed to map the Ethernet device");
            return -EIO;
        }
    }

    return 0;
}

static int allocate_irq_callback(ps_irq_t irq, unsigned curr_num, size_t num_irqs, void *token)
{
    if (token == NULL) {
        ZF_LOGE("Expected a token!");
        return -EINVAL;
    }

    callback_args_t *args = token;
    if (curr_num == 0) {
        args->irq_id = ps_irq_register(&args->io_ops->irq_ops, irq, eth_irq_handle, args->eth_driver);
        if (args->irq_id < 0) {
            ZF_LOGE("Failed to register the Ethernet device's IRQ");
            return -EIO;
        }
    }

    return 0;
}

int ethif_zynq7000_init_module(ps_io_ops_t *io_ops, const char *dev_path)
{
    struct arm_eth_plat_config plat_config;
    struct eth_driver *eth_driver;

    int error = ps_calloc(&io_ops->malloc_ops, 1, sizeof(*eth_driver), (void **) &eth_driver);
    if (error) {
        ZF_LOGE("Failed to allocate memory for the Ethernet driver");
        return -ENOMEM;
    }

    ps_fdt_cookie_t *cookie = NULL;
    callback_args_t args = { .io_ops = io_ops, .eth_driver = eth_driver };
    error = ps_fdt_read_path(&io_ops->io_fdt, &io_ops->malloc_ops, (char *) dev_path, &cookie);
    if (error) {
        ZF_LOGE("Failed to read the path of the Ethernet device");
        return -ENODEV;
    }

    error = ps_fdt_walk_registers(&io_ops->io_fdt, cookie, allocate_register_callback, &args);
    if (error) {
        ZF_LOGE("Failed to walk the Ethernet device's registers and allocate them");
        return -ENODEV;
    }

    error = ps_fdt_walk_irqs(&io_ops->io_fdt, cookie, allocate_irq_callback, &args);
    if (error) {
        ZF_LOGE("Failed to walk the Ethernet device's IRQs and allocate them");
        return -ENODEV;
    }

    error = ps_fdt_cleanup_cookie(&io_ops->malloc_ops, cookie);
    if (error) {
        ZF_LOGE("Failed to free the cookie used to allocate resources");
        return -ENODEV;
    }

    /* Setup the config and hand initialisation off to the proper
     * initialisation method */
    plat_config.buffer_addr = args.addr;
    plat_config.prom_mode = 1;

    error = ethif_zynq7000_init(eth_driver, *io_ops, &plat_config);
    if (error) {
        ZF_LOGE("Failed to initialise the Ethernet driver");
        return -ENODEV;
    }

    return ps_interface_register(&io_ops->interface_registration_ops, PS_ETHERNET_INTERFACE, eth_driver, NULL);
}

static const char *compatible_strings[] = {
    "cdns,zynq-gem",
    "cdns,gem",
    NULL
};
PS_DRIVER_MODULE_DEFINE(zynq7000_gem, compatible_strings, ethif_zynq7000_init_module);
