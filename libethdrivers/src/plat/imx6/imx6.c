/*
 * Copyright 2017, DornerWorks
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2020, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <platsupport/driver_module.h>
#include <platsupport/fdt.h>
#include <ethdrivers/gen_config.h>
#include <ethdrivers/imx6.h>
#include <ethdrivers/raw.h>
#include <ethdrivers/helpers.h>
#include <string.h>
#include <utils/util.h>
#include "enet.h"
#include "ocotp_ctrl.h"
#include "uboot/fec_mxc.h"
#include "uboot/miiphy.h"
#include "uboot/mx6qsabrelite.h"
#include "uboot/micrel.h"
#include "unimplemented.h"

#define DEFAULT_MAC "\x00\x19\xb8\x00\xf0\xa3"

#define BUF_SIZE MAX_PKT_SIZE
#define DMA_ALIGN 32

struct descriptor {
    /* NOTE: little endian packing: len before stat */
#if BYTE_ORDER == LITTLE_ENDIAN
    uint16_t len;
    uint16_t stat;
#elif BYTE_ORDER == BIG_ENDIAN
    uint16_t stat;
    uint16_t len;
#else
#error Could not determine endianess
#endif
    uint32_t phys;
};

struct imx6_eth_data {
    struct enet *enet;
    uintptr_t tx_ring_phys;
    uintptr_t rx_ring_phys;
    volatile struct descriptor *tx_ring;
    volatile struct descriptor *rx_ring;
    unsigned int rx_size;
    unsigned int tx_size;
    void **rx_cookies;                   // Array (of rx_size elements) of type 'void *'
    unsigned int rx_remain;
    unsigned int tx_remain;
    void **tx_cookies;
    unsigned int *tx_lengths;
    /* track where the head and tail of the queues are for
     * enqueueing buffers / checking for completions */
    unsigned int rdt, rdh, tdt, tdh;
};

int setup_iomux_enet(ps_io_ops_t *io_ops);

/* Receive descriptor status */
#define RXD_EMPTY     BIT(15) /* Buffer has no data. Waiting for reception. */
#define RXD_OWN0      BIT(14) /* Receive software ownership. R/W by user */
#define RXD_WRAP      BIT(13) /* Next buffer is found in ENET_RDSR */
#define RXD_OWN1      BIT(12) /* Receive software ownership. R/W by user */
#define RXD_LAST      BIT(11) /* Last buffer in frame. Written by the uDMA. */
#define RXD_MISS      BIT( 8) /* Frame does not match MAC (promiscuous mode) */
#define RXD_BROADCAST BIT( 7) /* frame is a broadcast frame */
#define RXD_MULTICAST BIT( 6) /* frame is a multicast frame */
#define RXD_BADLEN    BIT( 5) /* Incoming frame was larger than RCR[MAX_FL] */
#define RXD_BADALIGN  BIT( 4) /* Frame length does not align to a byte */
#define RXD_CRCERR    BIT( 2) /* The frame has a CRC error */
#define RXD_OVERRUN   BIT( 1) /* FIFO overrun */
#define RXD_TRUNC     BIT( 0) /* Receive frame > TRUNC_FL */

#define RXD_ERROR    (RXD_BADLEN  | RXD_BADALIGN | RXD_CRCERR |\
                      RXD_OVERRUN | RXD_TRUNC)

/* Transmit descriptor status */
#define TXD_READY     BIT(15) /* buffer in use waiting to be transmitted */
#define TXD_OWN0      BIT(14) /* Receive software ownership. R/W by user */
#define TXD_WRAP      BIT(13) /* Next buffer is found in ENET_TDSR */
#define TXD_OWN1      BIT(12) /* Receive software ownership. R/W by user */
#define TXD_LAST      BIT(11) /* Last buffer in frame. Written by the uDMA. */
#define TXD_ADDCRC    BIT(10) /* Append a CRC to the end of the frame */
#define TXD_ADDBADCRC BIT( 9) /* Append a bad CRC to the end of the frame */

static void low_level_init(struct eth_driver *driver, uint8_t *mac, int *mtu)
{
    struct imx6_eth_data *dev = (struct imx6_eth_data *)driver->eth_data;
    enet_get_mac(dev->enet, mac);
    *mtu = MAX_PKT_SIZE;
}

static void fill_rx_bufs(struct eth_driver *driver)
{
    struct imx6_eth_data *dev = (struct imx6_eth_data *)driver->eth_data;
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
        dev->rx_ring[dev->rdt].phys = phys;
        dev->rx_ring[dev->rdt].len = 0;

        __sync_synchronize();
        dev->rx_ring[dev->rdt].stat = RXD_EMPTY | (next_rdt == 0 ? RXD_WRAP : 0);
        dev->rdt = next_rdt;
        dev->rx_remain--;
    }
    __sync_synchronize();
    if (dev->rdt != dev->rdh && !enet_rx_enabled(dev->enet)) {
        enet_rx_enable(dev->enet);
    }
}

static void enable_interrupts(struct imx6_eth_data *dev)
{
    struct enet *enet = dev->enet;
    assert(enet);
    enet_enable_events(enet, 0);
    enet_clr_events(enet, (uint32_t) ~(NETIRQ_RXF | NETIRQ_TXF | NETIRQ_EBERR));
    enet_enable_events(enet, (uint32_t) NETIRQ_RXF | NETIRQ_TXF | NETIRQ_EBERR);
}

static void free_desc_ring(struct imx6_eth_data *dev, ps_dma_man_t *dma_man)
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

static int initialize_desc_ring(struct imx6_eth_data *dev, ps_dma_man_t *dma_man)
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

    /* zero both rings */
    for (unsigned int i = 0; i < dev->tx_size; i++) {
        dev->tx_ring[i] = (struct descriptor) {
            .phys = 0,
            .len = 0,
            .stat = (i + 1 == dev->tx_size) ? TXD_WRAP : 0
        };
    }
    for (unsigned int i = 0; i < dev->rx_size; i++) {
        dev->rx_ring[i] = (struct descriptor) {
            .phys = 0,
            .len = 0,
            .stat = (i + 1 == dev->rx_size) ? RXD_WRAP : 0
        };
    }
    __sync_synchronize();

    return 0;
}

static void complete_rx(struct eth_driver *eth_driver)
{
    struct imx6_eth_data *dev = (struct imx6_eth_data *)eth_driver->eth_data;
    unsigned int rdt = dev->rdt;
    while (dev->rdh != rdt) {
        unsigned int status = dev->rx_ring[dev->rdh].stat;
        /* Ensure no memory references get ordered before we checked the descriptor was written back */
        __sync_synchronize();
        if (status & RXD_EMPTY) {
            /* not complete yet */
            break;
        }
        void *cookie = dev->rx_cookies[dev->rdh];
        unsigned int len = dev->rx_ring[dev->rdh].len;
        /* update rdh */
        dev->rdh = (dev->rdh + 1) % dev->rx_size;
        dev->rx_remain++;
        /* Give the buffers back */
        eth_driver->i_cb.rx_complete(eth_driver->cb_cookie, 1, &cookie, &len);
    }
    if (dev->rdt != dev->rdh && !enet_rx_enabled(dev->enet)) {
        enet_rx_enable(dev->enet);
    }
}

static void complete_tx(struct eth_driver *driver)
{
    struct imx6_eth_data *dev = (struct imx6_eth_data *)driver->eth_data;
    while (dev->tdh != dev->tdt) {
        unsigned int i;
        for (i = 0; i < dev->tx_lengths[dev->tdh]; i++) {
            if (dev->tx_ring[(i + dev->tdh) % dev->tx_size].stat & TXD_READY) {
                /* not all parts complete */
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
    if (dev->tdh != dev->tdt && !enet_tx_enabled(dev->enet)) {
        enet_tx_enable(dev->enet);
    }
}

static void print_state(struct eth_driver *eth_driver)
{
    struct imx6_eth_data *eth_data = (struct imx6_eth_data *)eth_driver->eth_data;
    enet_print_mib(eth_data->enet);
}

static void handle_irq(struct eth_driver *driver, int irq)
{
    struct imx6_eth_data *eth_data = (struct imx6_eth_data *)driver->eth_data;
    struct enet *enet = eth_data->enet;
    uint32_t e;
    e = enet_clr_events(enet, NETIRQ_RXF | NETIRQ_TXF | NETIRQ_EBERR);
    if (e & NETIRQ_TXF) {
        complete_tx(driver);
    }
    if (e & NETIRQ_RXF) {
        complete_rx(driver);
        fill_rx_bufs(driver);
    }
    if (e & NETIRQ_EBERR) {
        printf("Error: System bus/uDMA\n");
        //ethif_print_state(netif_get_eth_driver(netif));
        assert(0);
        while (1);
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
    struct imx6_eth_data *dev = (struct imx6_eth_data *)driver->eth_data;
    struct enet *enet = dev->enet;
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
        dev->tx_ring[ring].len = len[i];
        dev->tx_ring[ring].phys = phys[i];
        __sync_synchronize();
        dev->tx_ring[ring].stat = TXD_READY | (ring + 1 == dev->tx_size ? TXD_WRAP : 0) | (i + 1 == num ? TXD_ADDCRC |
                                                                                           TXD_LAST : 0);
    }
    dev->tx_cookies[dev->tdt] = cookie;
    dev->tx_lengths[dev->tdt] = num;
    dev->tdt = (dev->tdt + num) % dev->tx_size;
    dev->tx_remain -= num;
    __sync_synchronize();
    if (!enet_tx_enabled(enet)) {
        enet_tx_enable(enet);
    }

    return ETHIF_TX_ENQUEUED;
}

static void get_mac(struct eth_driver *driver, uint8_t *mac)
{
    struct enet *enet = ((struct imx6_eth_data *)driver->eth_data)->enet;
    enet_get_mac(enet, (unsigned char *)mac);
}

static struct raw_iface_funcs iface_fns = {
    .raw_handleIRQ = handle_irq,
    .print_state = print_state,
    .low_level_init = low_level_init,
    .raw_tx = raw_tx,
    .raw_poll = raw_poll,
    .get_mac = get_mac
};

int ethif_imx6_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config)
{
    struct ocotp *ocotp = NULL;
    int err;
    struct enet *enet;
    struct imx6_eth_data *eth_data = NULL;
    uint8_t mac[6];

    if (config == NULL) {
        LOG_ERROR("Cannot get platform info; Passed in Config Pointer NULL");
        goto error;
    }

    struct arm_eth_plat_config *plat_config = (struct arm_eth_plat_config *)config;

    eth_data = (struct imx6_eth_data *)malloc(sizeof(struct imx6_eth_data));
    if (eth_data == NULL) {
        LOG_ERROR("Failed to allocate eth data struct");
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

    /* initialise the eFuse controller so we can get a MAC address */
    ocotp = ocotp_init(&io_ops.io_mapper);
    if (!ocotp) {
        LOG_ERROR("Failed to initialize ocotp");
        goto error;
    }

    /* Initialise ethernet pins, also does a PHY reset */
    err = setup_iomux_enet(&io_ops);
    if (err) {
        LOG_ERROR("Failed to setup iomux enet");
        goto error;
    }
    /* Initialise the phy library */
    miiphy_init();
    /* Initialise the phy */
    phy_micrel_init();
    /* Initialise the RGMII interface */
    enet = enet_init((struct desc_data) {
        .tx_phys = eth_data->tx_ring_phys, .rx_phys = eth_data->rx_ring_phys, .rx_bufsize = BUF_SIZE
    }, &io_ops);
    if (!enet) {
        LOG_ERROR("Failed to initialize RGMII");
        /* currently no way to properly clean up enet */
        assert(!"enet cannot be cleaned up");
        goto error;
    }
    eth_data->enet = enet;

    /* Non-Promiscuous mode means that only traffic relevant for us is made
     * visible by the hardware, everything else is discarded automatically. We
     * will only see packets addressed to our MAC and broadcast/multicast
     * packets. This is usually all that is needed unless the upper layer
     * implements functionality beyond a "normal" application scope, e.g.
     * switching or monitoring.
     */
    if (plat_config->prom_mode) {
        enet_prom_enable(enet);
    } else {
        enet_prom_disable(enet);
    }

    if (ocotp == NULL || ocotp_get_mac(ocotp, mac)) {
        memcpy(mac, DEFAULT_MAC, 6);
    }

    enet_set_mac(enet, mac);

    /* Connect the phy to the ethernet controller */
    unsigned phy_mask = 0xffffffff;
    if (fec_init(phy_mask, enet)) {
        LOG_ERROR("Failed to initialize fec");
        goto error;
    }

    /* Start the controller */
    enet_enable(enet);

    fill_rx_bufs(eth_driver);
    enable_interrupts(eth_data);

    /* done */
    return 0;
error:
    if (ocotp) {
        ocotp_free(ocotp, &io_ops.io_mapper);
    }
    if (eth_data) {
        free_desc_ring(eth_data, &io_ops.dma_manager);
        free(eth_data);
    }
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

    unsigned target_num = 0;
    callback_args_t *args = token;
    if (config_set(CONFIG_PLAT_IMX6)) {
        target_num = 0;
    } else if (config_set(CONFIG_PLAT_IMX8MQ_EVK)) {
        target_num = 2;
    }

    if (curr_num == target_num) {
        args->irq_id = ps_irq_register(&args->io_ops->irq_ops, irq, eth_irq_handle, args->eth_driver);
        if (args->irq_id < 0) {
            ZF_LOGE("Failed to register the Ethernet device's IRQ");
            return -EIO;
        }
    }

    return 0;
}

int ethif_imx_init_module(ps_io_ops_t *io_ops, const char *device_path)
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
    error = ps_fdt_read_path(&io_ops->io_fdt, &io_ops->malloc_ops, device_path, &cookie);
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

    error = ethif_imx6_init(eth_driver, *io_ops, &plat_config);
    if (error) {
        ZF_LOGE("Failed to initialise the Ethernet driver");
        return -ENODEV;
    }

    return ps_interface_register(&io_ops->interface_registration_ops, PS_ETHERNET_INTERFACE, eth_driver, NULL);
}

static const char *compatible_strings[] = {
    /* Other i.MX platforms may also be compatible but the platforms that have
     * been tested are the SABRE Lite (i.MX6Quad) and i.MX8MQ Evaluation Kit */
    "fsl,imx6q-fec",
    "fsl,imx8mq-fec",
    NULL
};
PS_DRIVER_MODULE_DEFINE(imx_fec, compatible_strings, ethif_imx_init_module);
