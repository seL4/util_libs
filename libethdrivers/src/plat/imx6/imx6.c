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

#define IRQ_MASK    (NETIRQ_RXF | NETIRQ_TXF | NETIRQ_EBERR)
#define BUF_SIZE    MAX_PKT_SIZE
#define DMA_ALIGN   32

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

/* we basically extend 'struct eth_driver' here */
typedef struct {
    struct eth_driver eth_drv;
    void *mapped_peripheral;
    irq_id_t irq_id;
    struct enet *enet;
    struct phy_device *phy;
    uintptr_t tx_ring_phys;
    uintptr_t rx_ring_phys;
    volatile struct descriptor *tx_ring;
    volatile struct descriptor *rx_ring;
    unsigned int rx_size;
    unsigned int tx_size;
    void **rx_cookies; /* Array of rx_size elements of type 'void *' */
    unsigned int rx_remain;
    unsigned int tx_remain;
    void **tx_cookies;
    unsigned int *tx_lengths;
    /* Track where the head and tail of the queues are for enqueueing buffers
     * and checking for completions.
     */
    unsigned int rdt, rdh, tdt, tdh;
} imx6_eth_driver_t;

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

static imx6_eth_driver_t *imx6_eth_driver(struct eth_driver *driver)
{
    assert(driver);

    /* we have simply extended the structure */
    assert(driver == driver->eth_data);
    return (imx6_eth_driver_t *)driver;
}

static void get_mac(struct eth_driver *driver, uint8_t *mac)
{
    assert(driver);
    assert(mac);

    imx6_eth_driver_t *dev = imx6_eth_driver(driver);
    assert(dev);
    struct enet *enet = dev->enet;
    assert(enet);

    uint64_t mac_u64 = enet_get_mac(enet);
    /* MAC is big endian u64, 0x0000aabbccddeeff means aa:bb:cc:dd:ee:ff */
    for (unsigned int i = 0; i < 6; i++) {
        mac[5 - i] = (uint8_t)mac_u64;
        mac_u64 >>= 8;
    }
}

static void low_level_init(struct eth_driver *driver, uint8_t *mac, int *mtu)
{
    assert(driver);

    if (mac) {
        get_mac(driver, mac);
    }

    if (mtu) {
        *mtu = MAX_PKT_SIZE;
    }
}

static void fill_rx_bufs(imx6_eth_driver_t *dev)
{
    assert(dev);

    void *cb_cookie = dev->eth_drv.cb_cookie;
    ethif_raw_allocate_rx_buf cb_alloc = dev->eth_drv.i_cb.allocate_rx_buf;
    if (!cb_alloc) {
        /* The function may not be set up (yet), in this case we can't do
         * anything. If lwip is used, this can be either lwip_allocate_rx_buf()
         * or lwip_pbuf_allocate_rx_buf() from src/lwip.c
         */
        ZF_LOGW("callback allocate_rx_buf not set, can't allocate %d buffers",
                dev->rx_remain);
    } else {
        __sync_synchronize();
        while (dev->rx_remain > 0) {

            /* request a buffer */
            void *cookie = NULL;
            uintptr_t phys = cb_alloc(cb_cookie, BUF_SIZE, &cookie);
            if (!phys) {
                /* There are no buffers left. This can happen if the pool is too
                 * small because CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS
                 * is less than CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT.
                 */
                break;
            }

            int next_rdt = (dev->rdt + 1) % dev->rx_size;
            dev->rx_cookies[dev->rdt] = cookie;
            dev->rx_ring[dev->rdt].phys = phys;
            dev->rx_ring[dev->rdt].len = 0;

            __sync_synchronize();

            dev->rx_ring[dev->rdt].stat = RXD_EMPTY
                                          | (next_rdt == 0 ? RXD_WRAP : 0);
            dev->rdt = next_rdt;
            dev->rx_remain--;
        }
        __sync_synchronize();
    }

    if (dev->rdt != dev->rdh) {
        struct enet *enet = dev->enet;
        assert(enet);
        if (!enet_rx_enabled(enet)) {
            enet_rx_enable(enet);
        }
    }
}

static void free_desc_ring(imx6_eth_driver_t *dev)
{
    assert(dev);

    ps_dma_man_t *dma_man = &(dev->eth_drv.io_ops.dma_manager);
    assert(dma_man);

    if (dev->rx_ring) {
        dma_unpin_free(
            dma_man,
            (void *)dev->rx_ring,
            sizeof(struct descriptor) * dev->rx_size);
        dev->rx_ring = NULL;
    }

    if (dev->tx_ring) {
        dma_unpin_free(
            dma_man,
            (void *)dev->tx_ring,
            sizeof(struct descriptor) * dev->tx_size);
        dev->tx_ring = NULL;
    }

    ps_malloc_ops_t *malloc_ops = &(dev->eth_drv.io_ops.malloc_ops);
    assert(malloc_ops);

    if (dev->rx_cookies) {
        ps_free(
            malloc_ops,
            sizeof(void *) * dev->rx_size,
            dev->rx_cookies);
        dev->rx_cookies = NULL;
    }

    if (dev->tx_cookies) {
        ps_free(
            malloc_ops,
            sizeof(void *) * dev->tx_size,
            dev->tx_cookies);
        dev->tx_cookies = NULL;
    }

    if (dev->tx_lengths) {
        ps_free(
            malloc_ops,
            sizeof(void *) * dev->tx_size,
            dev->tx_lengths);
        dev->tx_lengths = NULL;
    }
}

static int initialize_desc_ring(imx6_eth_driver_t *dev)
{
    assert(dev);

    int ret;

    /* Allocate uncached memory for RX/TX DMA descriptor rings. The function
     * ensures that the cache is cleaned and invalidated for this area, so there
     * are no surprises.
     */
    ps_dma_man_t *dma_man = &(dev->eth_drv.io_ops.dma_manager);
    assert(dma_man);

    dma_addr_t rx_ring = dma_alloc_pin(
                             dma_man,
                             sizeof(struct descriptor) * dev->rx_size,
                             0, // uncached
                             dev->eth_drv.dma_alignment);
    if (!rx_ring.phys) {
        ZF_LOGE("Failed to allocate rx_ring");
        goto error;
    }
    dev->rx_ring = rx_ring.virt;
    dev->rx_ring_phys = rx_ring.phys;

    dma_addr_t tx_ring = dma_alloc_pin(
                             dma_man,
                             sizeof(struct descriptor) * dev->tx_size,
                             0, // uncached
                             dev->eth_drv.dma_alignment);
    if (!tx_ring.phys) {
        ZF_LOGE("Failed to allocate tx_ring");
        goto error;
    }
    dev->tx_ring = tx_ring.virt;
    dev->tx_ring_phys = tx_ring.phys;

    ps_malloc_ops_t *malloc_ops = &(dev->eth_drv.io_ops.malloc_ops);
    assert(malloc_ops);

    ret = ps_calloc(
              malloc_ops,
              dev->rx_size,
              sizeof(void *),
              (void **)&dev->rx_cookies);
    if (ret) {
        ZF_LOGE("ps_calloc() failed for rx_cookies, code %d", ret);
        goto error;
    }

    ret = ps_calloc(
              malloc_ops,
              dev->tx_size,
              sizeof(void *),
              (void **)&dev->tx_cookies);

    if (ret) {
        ZF_LOGE("ps_calloc() failed for tx_cookies, code %d", ret);
        goto error;
    }

    ret = ps_calloc(
              malloc_ops,
              dev->tx_size,
              sizeof(void *),
              (void **)&dev->tx_lengths);
    if (ret) {
        ZF_LOGE("ps_calloc() failed for tx_lengths, code %d", ret);
        goto error;
    }

    /* Remaining needs to be 2 less than size as we cannot actually enqueue
     * size many descriptors, since then the head and tail pointers would be
     * equal, indicating empty.
     */
    assert(dev->rx_size > 2);
    dev->rx_remain = dev->rx_size - 2;

    assert(dev->tx_size > 2);
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

error:
    free_desc_ring(dev);
    return -1;
}

static void complete_rx(imx6_eth_driver_t *dev)
{
    assert(dev);

    void *cb_cookie = dev->eth_drv.cb_cookie;
    ethif_raw_rx_complete cb_complete = dev->eth_drv.i_cb.rx_complete;
    assert(cb_complete);

    unsigned int rdt = dev->rdt;
    while (dev->rdh != rdt) {
        unsigned int status = dev->rx_ring[dev->rdh].stat;
        /* Ensure no memory references get ordered before we checked the
         * descriptor was written back
         */
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
        cb_complete(cb_cookie, 1, &cookie, &len);
    }

    if (dev->rdt != dev->rdh) {
        struct enet *enet = dev->enet;
        assert(enet);
        if (!enet_rx_enabled(enet)) {
            enet_rx_enable(enet);
        }
    }
}

static void complete_tx(imx6_eth_driver_t *dev)
{
    assert(dev);

    void *cb_cookie = dev->eth_drv.cb_cookie;
    ethif_raw_tx_complete cb_complete = dev->eth_drv.i_cb.tx_complete;
    assert(cb_complete);

    while (dev->tdh != dev->tdt) {
        for (unsigned int i = 0; i < dev->tx_lengths[dev->tdh]; i++) {
            if (dev->tx_ring[(i + dev->tdh) % dev->tx_size].stat & TXD_READY) {
                /* not all parts complete */
                return;
            }
        }
        /* do not let memory loads happen before our checking of the descriptor
         * write back
         */
        __sync_synchronize();
        /* increase where we believe tdh to be */
        void *cookie = dev->tx_cookies[dev->tdh];
        dev->tx_remain += dev->tx_lengths[dev->tdh];
        dev->tdh = (dev->tdh + dev->tx_lengths[dev->tdh]) % dev->tx_size;
        /* give the buffer back */
        cb_complete(cb_cookie, cookie);
    }

    if (dev->tdh != dev->tdt) {
        struct enet *enet = dev->enet;
        assert(enet);
        if (!enet_tx_enabled(enet)) {
            enet_tx_enable(enet);
        }
    }
}

static void print_state(struct eth_driver *driver)
{
    assert(driver);

    imx6_eth_driver_t *dev = imx6_eth_driver(driver);
    struct enet *enet = dev->enet;
    assert(enet);

    enet_print_mib(enet);
}

static void handle_irq(struct eth_driver *driver, int irq)
{
    assert(driver);

    imx6_eth_driver_t *dev = imx6_eth_driver(driver);
    assert(dev);
    struct enet *enet = dev->enet;
    assert(enet);

    uint32_t e = enet_clr_events(enet, IRQ_MASK);
    if (e & NETIRQ_TXF) {
        complete_tx(dev);
    }
    if (e & NETIRQ_RXF) {
        complete_rx(dev);
        fill_rx_bufs(dev);
    }
    if (e & NETIRQ_EBERR) {
        ZF_LOGE("Error: System bus/uDMA");
        // ethif_print_state(netif_get_eth_driver(netif));
        assert(0);
        while (1);
    }
}

/* This is a platsuport IRQ interface IRQ handler wrapper for handle_irq() */
static void eth_irq_handle(void *ctx, ps_irq_acknowledge_fn_t acknowledge_fn,
                           void *ack_data)
{
    if (!ctx) {
        ZF_LOGE("IRQ handler got ctx=NULL");
        assert(0);
        return;
    }

    struct eth_driver *driver = (struct eth_driver *)ctx;

    /* handle_irq doesn't really expect an IRQ number */
    handle_irq(driver, 0);

    int ret = acknowledge_fn(ack_data);
    if (ret) {
        ZF_LOGE("Failed to acknowledge the Ethernet device's IRQ, code %d", ret);
    }
}

static void raw_poll(struct eth_driver *driver)
{
    assert(driver);

    imx6_eth_driver_t *dev = imx6_eth_driver(driver);
    assert(dev);

    complete_rx(dev);
    complete_tx(dev);
    fill_rx_bufs(dev);
}

static int raw_tx(struct eth_driver *driver, unsigned int num, uintptr_t *phys,
                  unsigned int *len, void *cookie)
{
    assert(driver);

    imx6_eth_driver_t *dev = imx6_eth_driver(driver);
    assert(dev);

    /* Ensure we have room */
    if (dev->tx_remain < num) {
        /* try and complete some */
        complete_tx(dev);
        if (dev->tx_remain < num) {
            ZF_LOGE("TX queue lacks space, have %d, need %d", dev->tx_remain, num);
            return ETHIF_TX_FAILED;
        }
    }

    __sync_synchronize();
    for (unsigned int i = 0; i < num; i++) {
        unsigned int ring = (dev->tdt + i) % dev->tx_size;
        dev->tx_ring[ring].len = len[i];
        dev->tx_ring[ring].phys = phys[i];
        __sync_synchronize();
        dev->tx_ring[ring].stat = TXD_READY
                                  | (ring + 1 == dev->tx_size ? TXD_WRAP : 0)
                                  | (i + 1 == num ? TXD_ADDCRC | TXD_LAST : 0);
    }
    dev->tx_cookies[dev->tdt] = cookie;
    dev->tx_lengths[dev->tdt] = num;
    dev->tdt = (dev->tdt + num) % dev->tx_size;
    dev->tx_remain -= num;
    __sync_synchronize();

    struct enet *enet = dev->enet;
    assert(enet);
    if (!enet_tx_enabled(enet)) {
        enet_tx_enable(enet);
    }

    return ETHIF_TX_ENQUEUED;
}

static uint64_t obtain_mac_from_ocotp(ps_io_mapper_t *io_mapper)
{
    assert(io_mapper);

    struct ocotp *ocotp = ocotp_init(io_mapper);
    if (!ocotp) {
        ZF_LOGE("Failed to initialize OCOTP to read MAC");
        return 0;
    }

    uint64_t mac = ocotp_get_mac(ocotp);
    ocotp_free(ocotp, io_mapper);
    if (0 == mac) {
        ZF_LOGE("Failed to get MAC from OCOTP");
        return 0;
    }

    return mac;
}

static int init_device(imx6_eth_driver_t *dev)
{
    assert(dev);

    int ret;

    uint64_t mac = obtain_mac_from_ocotp(&(dev->eth_drv.io_ops.io_mapper));
    if (0 == mac) {
        ZF_LOGE("Failed to get MAC from OCOTP");
        return -1;
    }

    ZF_LOGI("using MAC: %02x:%02x:%02x:%02x:%02x:%02x",
            (uint8_t)(mac >> 40),
            (uint8_t)(mac >> 32),
            (uint8_t)(mac >> 24),
            (uint8_t)(mac >> 16),
            (uint8_t)(mac >> 8),
            (uint8_t)(mac));

    ret = initialize_desc_ring(dev);
    if (ret) {
        ZF_LOGE("Failed to allocate descriptor rings, code %d", ret);
        return -1;
    }
    /* Allocation successful, need to free rings on any error from now own. */

    /* Initialise ethernet pins, also does a PHY reset */
    ret = setup_iomux_enet(&(dev->eth_drv.io_ops));
    if (ret) {
        ZF_LOGE("Failed to setup IOMUX for ENET, code %d", ret);
        goto error;
    }

    /* Initialise the RGMII interface, clears and masks all interrupts */
    dev->enet = enet_init(
                    dev->mapped_peripheral,
                    dev->tx_ring_phys,
                    dev->rx_ring_phys,
                    BUF_SIZE,
                    mac,
                    &(dev->eth_drv.io_ops));
    if (!dev->enet) {
        ZF_LOGE("Failed to initialize RGMII");
        /* currently no way to properly clean up enet */
        assert(!"enet cannot be cleaned up");
        goto error;
    }

    /* Non-Promiscuous mode means that only traffic relevant for us is made
     * visible by the hardware, everything else is discarded automatically. We
     * will only see packets addressed to our MAC and broadcast/multicast
     * packets. This is usually all that is needed unless the upper layer
     * implements functionality beyond a "normal" application scope, e.g.
     * switching or monitoring.
     */
    enet_prom_enable(dev->enet);

    /* Initialise the phy library */
    miiphy_init();
    /* Initialise the phy */
    phy_micrel_init();
    /* Connect the phy to the ethernet controller */
    unsigned int phy_mask = 0xffffffff;
    dev->phy = fec_init(phy_mask, dev->enet);
    if (!dev->phy) {
        ZF_LOGE("Failed to initialize fec");
        goto error;
    }

    enet_set_speed(
        dev->enet,
        dev->phy->speed,
        (dev->phy->duplex == DUPLEX_FULL) ? 1 : 0);

    ZF_LOGI("Link speed: %d Mbps, %s-duplex",
            dev->phy->speed,
            (dev->phy->duplex == DUPLEX_FULL) ? "full" : "half");

    udelay(100000); /* why? */

    /* Start the controller, all interrupts are still masked here */
    enet_enable(dev->enet);

    /* This could also enable receiving, so the controller must be enabled. */
    fill_rx_bufs(dev);

    /* Ensure no unused interrupts are pending. */
    enet_clr_events(dev->enet, ~((uint32_t)IRQ_MASK));

    /* Enable interrupts for the events we care about. This unmask them, some
     * could already be pending here and trigger immediately.
     */
    enet_enable_events(dev->enet, IRQ_MASK);

    /* done */
    return 0;

error:
    /* ToDo: free dev->phydev if set */
    free_desc_ring(dev);
    return -1;
}

static int allocate_register_callback(pmem_region_t pmem, unsigned curr_num,
                                      size_t num_regs, void *token)
{
    assert(token);
    imx6_eth_driver_t *dev = (imx6_eth_driver_t *)token;

    /* we support only one peripheral, ignore others gracefully */
    if (curr_num != 0) {
        ZF_LOGW("Ignoring peripheral register bank #%d at 0x%"PRIx64,
                curr_num, pmem.base_addr);
        return 0;
    }

    dev->mapped_peripheral = ps_pmem_map(
                                 &(dev->eth_drv.io_ops),
                                 pmem,
                                 false,
                                 PS_MEM_NORMAL);
    if (!dev->mapped_peripheral) {
        ZF_LOGE("Failed to map the Ethernet device");
        return -EIO;
    }

    return 0;
}

static int allocate_irq_callback(ps_irq_t irq, unsigned curr_num,
                                 size_t num_irqs, void *token)
{
    assert(token);
    imx6_eth_driver_t *dev = (imx6_eth_driver_t *)token;

    unsigned target_num = config_set(CONFIG_PLAT_IMX8MQ_EVK) ? 2 : 0;
    if (curr_num != target_num) {
        ZF_LOGW("Ignoring interrupt #%d with value %d", curr_num, irq);
        return 0;
    }

    dev->irq_id = ps_irq_register(
                      &(dev->eth_drv.io_ops.irq_ops),
                      irq,
                      eth_irq_handle,
                      dev);
    if (dev->irq_id < 0) {
        ZF_LOGE("Failed to register the Ethernet device's IRQ");
        return -EIO;
    }

    return 0;
}

int ethif_imx_init_module(ps_io_ops_t *io_ops, const char *device_path)
{
    int ret;
    imx6_eth_driver_t *driver = NULL;

    ret = ps_calloc(
              &io_ops->malloc_ops,
              1,
              sizeof(*driver),
              (void **) &driver);
    if (ret) {
        ZF_LOGE("Failed to allocate memory for the Ethernet driver, code %d", ret);
        ret = -ENOMEM;
        goto error;
    }

    driver->eth_drv.i_fn = (struct raw_iface_funcs) {
        .raw_handleIRQ   = handle_irq,
        .print_state     = print_state,
        .low_level_init  = low_level_init,
        .raw_tx          = raw_tx,
        .raw_poll        = raw_poll,
        .get_mac         = get_mac
    };

    driver->eth_drv.eth_data = driver; /* use simply extend the structure */
    driver->eth_drv.io_ops = *io_ops;
    driver->eth_drv.dma_alignment = DMA_ALIGN;
    driver->tx_size = CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT;
    driver->rx_size = CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT;

    ps_fdt_cookie_t *cookie = NULL;
    ret = ps_fdt_read_path(
              &io_ops->io_fdt,
              &io_ops->malloc_ops,
              device_path,
              &cookie);
    if (ret) {
        ZF_LOGE("Failed to read the path of the Ethernet device, code %d", ret);
        ret = -ENODEV;
        goto error;
    }

    ret = ps_fdt_walk_registers(
              &io_ops->io_fdt,
              cookie,
              allocate_register_callback,
              driver);
    if (ret) {
        ZF_LOGE("Failed to walk the Ethernet device's registers and allocate them, code %d", ret);
        ret = -ENODEV;
        goto error;
    }

    ret = ps_fdt_walk_irqs(
              &io_ops->io_fdt,
              cookie,
              allocate_irq_callback,
              driver);
    if (ret) {
        ZF_LOGE("Failed to walk the Ethernet device's IRQs and allocate them, code %d", ret);
        ret = -ENODEV;
        goto error;
    }

    ret = ps_fdt_cleanup_cookie(&io_ops->malloc_ops, cookie);
    if (ret) {
        ZF_LOGE("Failed to free the cookie used to allocate resources, code %d", ret);
        ret = -ENODEV;
        goto error;
    }

    ret = init_device(driver);
    if (ret) {
        ZF_LOGE("Failed to initialise the Ethernet driver, code %d", ret);
        ret = -ENODEV;
        goto error;
    }

    ret = ps_interface_register(
              &io_ops->interface_registration_ops,
              PS_ETHERNET_INTERFACE,
              driver,
              NULL);
    if (ret) {
        ZF_LOGE("Failed to register Ethernet driver interface, code %d", ret);
        ret = -ENODEV;
        goto error;
    }

    return 0;

error:
    ZF_LOGI("Cleaning up failed driver initialization");

    if (driver) {
        ps_free(&io_ops->malloc_ops, sizeof(*driver), driver);
    }

    return ret;

}

static const char *compatible_strings[] = {
    /* Other i.MX platforms may also be compatible but the platforms that have
     * been tested are the SABRE Lite (i.MX6Quad) and i.MX8MQ Evaluation Kit
     */
    "fsl,imx6q-fec",
    "fsl,imx8mq-fec",
    NULL
};

PS_DRIVER_MODULE_DEFINE(imx_fec, compatible_strings, ethif_imx_init_module);
