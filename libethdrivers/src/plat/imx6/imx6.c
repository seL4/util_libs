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
#include <ethdrivers/plat/eth_plat.h>
#include <string.h>
#include <utils/util.h>
#include "enet.h"
#include "ocotp_ctrl.h"
#include "uboot/fec_mxc.h"
#include "uboot/miiphy.h"
#include "uboot/imx_board.h"
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

typedef struct {
    unsigned int cnt;
    unsigned int remain;
    unsigned int tail;
    unsigned int head;
    volatile struct descriptor *descr;
    uintptr_t phys;
    void **cookies; /* Array with tx/tx size elements of type 'void *' */
} ring_ctx_t;


/* we basically extend 'struct eth_driver' here */
struct imx6_eth_data {
    struct enet *enet;
    struct phy_device *phy;
    ring_ctx_t tx;
    ring_ctx_t rx;
    unsigned int *tx_lengths;
};

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

static struct imx6_eth_data *imx6_eth_driver(struct eth_driver *driver)
{
    assert(driver);

    /* we have simply extended the structure */
    return (struct imx6_eth_data *)driver->eth_data;
}

struct enet *get_enet_from_driver(struct eth_driver *driver)
{
    assert(driver);

    struct imx6_eth_data *dev = imx6_eth_driver(driver);
    assert(dev);
    return dev->enet;
}

static void get_mac(struct eth_driver *driver, uint8_t *mac)
{
    assert(driver);
    assert(mac);

    struct imx6_eth_data *dev = imx6_eth_driver(driver);
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

static void update_ring_slot(
    ring_ctx_t *ring,
    unsigned int idx,
    uintptr_t phys,
    uint16_t len,
    uint16_t stat)
{
    volatile struct descriptor *d = &(ring->descr[idx]);
    d->phys = phys;
    d->len = len;

    /* Ensure all writes to the descriptor complete, before we set the flags
     * that makes hardware aware of this slot.
     */
    __sync_synchronize();

    d->stat = stat;
}

static void fill_rx_bufs(struct eth_driver *driver)
{
    struct imx6_eth_data *dev = imx6_eth_driver(driver);
    assert(dev);

    ring_ctx_t *ring = &(dev->rx);

    void *cb_cookie = driver->cb_cookie;
    ethif_raw_allocate_rx_buf cb_alloc = driver->i_cb.allocate_rx_buf;
    if (!cb_alloc) {
        /* The function may not be set up (yet), in this case we can't do
         * anything. If lwip is used, this can be either lwip_allocate_rx_buf()
         * or lwip_pbuf_allocate_rx_buf() from src/lwip.c
         */
        ZF_LOGW("callback allocate_rx_buf not set, can't allocate %d buffers",
                ring->remain);
    } else {
        __sync_synchronize();
        while (ring->remain > 0) {
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
            uint16_t stat = RXD_EMPTY;
            int idx = ring->tail;
            int new_tail = idx + 1;
            if (new_tail == ring->cnt) {
                new_tail = 0;
                stat |= RXD_WRAP;
            }
            ring->cookies[idx] = cookie;
            update_ring_slot(ring, idx, phys, 0, stat);
            ring->tail = new_tail;
            /* There is a race condition if add/remove is not synchronized. */
            ring->remain--;
        }
        __sync_synchronize();
    }

    if (ring->tail != ring->head) {
        struct enet *enet = dev->enet;
        assert(enet);
        if (!enet_rx_enabled(enet)) {
            enet_rx_enable(enet);
        }
    }
}

static void free_desc_ring(struct eth_driver *driver)
{
    struct imx6_eth_data *dev = imx6_eth_driver(driver);
    assert(dev);

    ps_dma_man_t *dma_man = &(driver->io_ops.dma_manager);
    assert(dma_man);

    if (dev->rx.descr) {
        dma_unpin_free(
            dma_man,
            (void *)dev->rx.descr,
            sizeof(struct descriptor) * dev->rx.cnt);
        dev->rx.descr = NULL;
    }

    if (dev->tx.descr) {
        dma_unpin_free(
            dma_man,
            (void *)dev->tx.descr,
            sizeof(struct descriptor) * dev->tx.cnt);
        dev->tx.descr = NULL;
    }

    ps_malloc_ops_t *malloc_ops = &(driver->io_ops.malloc_ops);
    assert(malloc_ops);

    if (dev->rx.cookies) {
        ps_free(
            malloc_ops,
            sizeof(void *) * dev->rx.cnt,
            dev->rx.cookies);
        dev->rx.cookies = NULL;
    }

    if (dev->tx.cookies) {
        ps_free(
            malloc_ops,
            sizeof(void *) * dev->tx.cnt,
            dev->tx.cookies);
        dev->tx.cookies = NULL;
    }

    if (dev->tx_lengths) {
        ps_free(
            malloc_ops,
            sizeof(void *) * dev->tx.cnt,
            dev->tx_lengths);
        dev->tx_lengths = NULL;
    }
}

static int setup_desc_ring(struct eth_driver *driver, ring_ctx_t *ring)
{
    struct imx6_eth_data *dev = imx6_eth_driver(driver);
    assert(dev);
    assert(ring);

    /* caller should have zeroed the ring context already.
     *   memset(ring, 0, sizeof(*ring));
     */

    size_t size = sizeof(struct descriptor) * ring->cnt;

    /* allocate uncached memory, function will also clean an invalidate cache
     * to for the area internally to save us from surprises
     */
    ps_dma_man_t *dma_man = &(driver->io_ops.dma_manager);
    assert(dma_man);
    dma_addr_t dma = dma_alloc_pin(
                         dma_man,
                         size,
                         0, // uncached
                         driver->dma_alignment);
    if (!dma.phys || !dma.virt) {
        ZF_LOGE("Faild to allocate %d bytes of DMA memory", size);
        return -1;
    }

    ZF_LOGI("dma_alloc_pin: %x -> %p, %d descriptors, %d bytes",
            dma.phys, dma.virt, ring->cnt, size);

    /* zero ring */
    memset(dma.virt, 0, size);

    ring->phys = dma.phys;
    ring->descr = dma.virt;

    assert(ring->cnt >= 2);
    ring->descr[ring->cnt - 1].stat = TXD_WRAP;
    /* Remaining needs to be 2 less than size as we cannot actually enqueue
     * size many descriptors, since then the head and tail pointers would be
     * equal, indicating empty.
     */
    ring->remain = ring->cnt - 2;

    ring->tail = 0;
    ring->head = 0;

    /* allocate and zero memory for cookies */
    ps_malloc_ops_t *malloc_ops = &(driver->io_ops.malloc_ops);
    assert(malloc_ops);
    int ret = ps_calloc(
                  malloc_ops,
                  ring->cnt,
                  sizeof(void *),
                  (void **) &ring->cookies);
    if (ret) {
        ZF_LOGE("Failed to malloc, code %d", ret);
        return -1;
    }

    /* Ensure operation propagate, so DMA is really set up. */
    __sync_synchronize();

    return 0;
}

static void complete_rx(struct eth_driver *driver)
{
    struct imx6_eth_data *dev = imx6_eth_driver(driver);
    assert(dev);

    void *cb_cookie = driver->cb_cookie;
    ethif_raw_rx_complete cb_complete = driver->i_cb.rx_complete;
    assert(cb_complete);

    ring_ctx_t *ring = &(dev->rx);
    unsigned int head = ring->head;

    /* Release all descriptors that have data. */
    while (head != ring->tail) {

        /* The NIC hardware can modify the descriptor any time, 'volatile'
         * prevents the compiler's optimizer from caching values and enforces
         * every access happen as stated in the code.
         */
        volatile struct descriptor *d = &(ring->descr[head]);

        /* If the slot is still marked as empty we are done. */
        if (d->stat & RXD_EMPTY) {
            assert(dev->enet);
            if (!enet_rx_enabled(dev->enet)) {
                enet_rx_enable(dev->enet);
            }
            break;
        }

        void *cookie = ring->cookies[head];
        /* Go to next buffer, handle roll-over. */
        if (++head == ring->cnt) {
            head = 0;
        }
        ring->head = head;

        /* There is a race condition here if add/remove is not synchronized. */
        ring->remain++;

        /* Tell the driver it can return the DMA buffer to the pool. */
        unsigned int len = d->len;
        cb_complete(cb_cookie, 1, &cookie, &len);
    }
}

static void complete_tx(struct eth_driver *driver)
{
    struct imx6_eth_data *dev = imx6_eth_driver(driver);
    assert(dev);

    void *cb_cookie = driver->cb_cookie;
    ethif_raw_tx_complete cb_complete = driver->i_cb.tx_complete;
    assert(cb_complete);

    unsigned int cnt_org;
    void *cookie;
    ring_ctx_t *ring = &(dev->tx);
    unsigned int head = ring->head;
    unsigned int cnt = 0;

    while (head != ring->tail) {

        if (0 == cnt) {
            cnt = dev->tx_lengths[head];
            if ((0 == cnt) || (cnt > dev->tx.cnt)) {
                /* We are not supposed to read 0 here. */
                ZF_LOGE("complete_tx with cnt=%u at head %u", cnt, head);
                return;
            }
            cnt_org = cnt;
            cookie = ring->cookies[head];
        }

        /* The NIC hardware can modify the descriptor any time, 'volatile'
         * prevents the compiler's optimizer from caching values and enforces
         * every access happens as stated in the code.
         */
        volatile struct descriptor *d = &(ring->descr[head]);

        /* If this buffer was not sent, we can't release any buffer. */
        if (d->stat & TXD_READY) {
            assert(dev->enet);
            if (!enet_tx_enabled(dev->enet)) {
                enet_tx_enable(dev->enet);
            }
            return;
        }

        /* Go to next buffer, handle roll-over. */
        if (++head == dev->tx.cnt) {
            head = 0;
        }

        if (0 == --cnt) {
            ring->head = head;
            /* race condition if add/remove is not synchronized. */
            ring->remain += cnt_org;
            /* give the buffer back */
            cb_complete(cb_cookie, cookie);
        }

    }

    /* The only reason to arrive here is when head equals tails. If cnt is not
     * zero, then there is some kind of overflow or data corruption. The number
     * of tx descriptors holding data can't exceed the space in the ring.
     */
    if (0 != cnt) {
        ZF_LOGE("head at %u reached tail, but cnt=%u", head, cnt);
        assert(0);
    }
}

static void print_state(struct eth_driver *driver)
{
    assert(driver);

    struct imx6_eth_data *dev = imx6_eth_driver(driver);
    struct enet *enet = dev->enet;
    assert(enet);

    enet_print_mib(enet);
}

static void handle_irq(struct eth_driver *driver, int irq)
{
    assert(driver);

    struct imx6_eth_data *dev = imx6_eth_driver(driver);
    assert(dev);
    struct enet *enet = dev->enet;
    assert(enet);

    uint32_t e = enet_clr_events(enet, IRQ_MASK);
    if (e & NETIRQ_TXF) {
        complete_tx(driver);
    }
    if (e & NETIRQ_RXF) {
        complete_rx(driver);
        fill_rx_bufs(driver);
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

    struct imx6_eth_data *dev = imx6_eth_driver(driver);
    assert(dev);

    // TODO: If the interrupts are still enabled, there could be race here. The
    //       caller must ensure this can't happen.
    complete_rx(driver);
    complete_tx(driver);
    fill_rx_bufs(driver);
}

static int raw_tx(struct eth_driver *driver, unsigned int num, uintptr_t *phys,
                  unsigned int *len, void *cookie)
{
    if (0 == num) {
        ZF_LOGW("raw_tx() called with num=0");
        return ETHIF_TX_ENQUEUED;
    }

    assert(driver);

    struct imx6_eth_data *dev = imx6_eth_driver(driver);
    assert(dev);

    ring_ctx_t *ring = &(dev->tx);

    /* Ensure we have room */
    if (ring->remain < num) {
        /* not enough room, try to complete some and check again */
        complete_tx(driver);
        unsigned int rem = ring->remain;
        if (rem < num) {
            ZF_LOGE("TX queue lacks space, has %d, need %d", rem, num);
            return ETHIF_TX_FAILED;
        }
    }

    __sync_synchronize();

    unsigned int tail = ring->tail;
    unsigned int tail_new = tail;

    unsigned int i = num;
    while (i-- > 0) {
        uint16_t stat = TXD_READY;
        if (0 == i) {
            stat |= TXD_ADDCRC | TXD_LAST;
        }

        unsigned int idx = tail_new;
        if (++tail_new == dev->tx.cnt) {
            tail_new = 0;
            stat |= TXD_WRAP;
        }
        update_ring_slot(ring, idx, *phys++, *len++, stat);
    }

    ring->cookies[tail] = cookie;
    dev->tx_lengths[tail] = num;
    ring->tail = tail_new;
    /* There is a race condition here if add/remove is not synchronized. */
    ring->remain -= num;

    __sync_synchronize();

    struct enet *enet = dev->enet;
    assert(enet);
    if (!enet_tx_enabled(enet)) {
        enet_tx_enable(enet);
    }

    return ETHIF_TX_ENQUEUED;
}

static uint64_t obtain_mac(const nic_config_t *nic_config,
                           ps_io_mapper_t *io_mapper)
{
    unsigned int enet_id = nic_config ? nic_config->id : 0;
    uint64_t cfg_mac = nic_config ? nic_config->mac : 0;
    unsigned int doForceMac = nic_config && (nic_config->flags & NIC_CONFIG_FORCE_MAC);

    if (doForceMac && (0 != cfg_mac)) {
        ZF_LOGI("config: overwriting default MAC");
        return cfg_mac;
    }

    struct ocotp *ocotp = ocotp_init(io_mapper);
    if (!ocotp) {
        ZF_LOGE("Failed to initialize OCOTP to read MAC");
    } else {
        uint64_t ocotp_mac = ocotp_get_mac(ocotp, enet_id);
        ocotp_free(ocotp, io_mapper);
        if (0 != ocotp_mac) {
            ZF_LOGI("taking MAC #%u from OCOTP", enet_id);
            return ocotp_mac;
        }

#ifdef CONFIG_PLAT_IMX6SX

        if (1 == enet_id) {
            ZF_LOGI("IMX6SX: no MAC for enet2 in OCOTP, use enet1 MAC + 1");
            ocotp_mac = ocotp_get_mac(ocotp, 0);
            if (0 != ocotp_mac) {
                /* The uint64_t is 0x0000<aa><bb><cc><dd><ee><ff> for the MAC
                 * aa:bb:cc:dd:ee:ff, where aa:bb:cc is the OUI and dd:ee:ff is
                 * and ID. Leave OUI as it is and increment the ID by one with
                 * roll over handling.
                 */
                uint32_t oui = ocotp_mac >> 24;
                uint32_t id = ocotp_mac & 0xffffff;
                return ((uint64_t)oui << 24) | ((id + 1) & 0xffffff);
            }

            ZF_LOGI("IMX6SX: no MAC for enet1 in OCOTP");
        }

#endif /* CONFIG_PLAT_IMX6SX */

    }

    /* no MAC from OCOTP, try using MAC config */
    if (0 != cfg_mac) {
        ZF_LOGI("no MAC in OCOTP, taking MAC from config");
        return cfg_mac;
    }

    ZF_LOGE("Failed to get MAC from OCOTP or config");
    return 0;
}

static struct raw_iface_funcs iface_fns = {
    .raw_handleIRQ = handle_irq,
    .print_state = print_state,
    .low_level_init = low_level_init,
    .raw_tx = raw_tx,
    .raw_poll = raw_poll,
    .get_mac = get_mac
};



static int init_device(struct eth_driver *driver, const nic_config_t *nic_config, void *mapped_peripheral)
{
    struct imx6_eth_data *dev = imx6_eth_driver(driver);
    assert(dev);

    int ret;


    uint64_t mac = obtain_mac(nic_config, &(driver->io_ops.io_mapper));
    if (0 == mac) {
        ZF_LOGE("Failed to obtain a MAC");
        return -1;
    }

    ZF_LOGI("using MAC: %02x:%02x:%02x:%02x:%02x:%02x",
            (uint8_t)(mac >> 40),
            (uint8_t)(mac >> 32),
            (uint8_t)(mac >> 24),
            (uint8_t)(mac >> 16),
            (uint8_t)(mac >> 8),
            (uint8_t)(mac));

    ret = setup_desc_ring(driver, &(dev->rx));
    if (ret) {
        ZF_LOGE("Failed to allocate rx_ring, code %d", ret);
        goto error;
    }

    ret = setup_desc_ring(driver, &(dev->tx));
    if (ret) {
        ZF_LOGE("Failed to allocate tx_ring, code %d", ret);
        goto error;
    }

    ps_malloc_ops_t *malloc_ops = &(driver->io_ops.malloc_ops);
    assert(malloc_ops);
    ret = ps_calloc(
              malloc_ops,
              dev->tx.cnt,
              sizeof(unsigned int),
              (void **)&dev->tx_lengths);
    if (ret) {
        ZF_LOGE("Failed to malloc, code %d", ret);
        goto error;
    }
    /* ring got allocated, need to free it on error */

    unsigned int enet_id = nic_config ? nic_config->id : 0;

    /* Initialise ethernet pins, also does a PHY reset */
    if (0 != enet_id) {
        ZF_LOGI("skipping IOMUX setup for ENET id=%d", enet_id);
    } else {
        ret = setup_iomux_enet(&(driver->io_ops));
        if (ret) {
            ZF_LOGE("Failed to setup IOMUX for ENET id=%d, code %d",
                    enet_id, ret);
            goto error;
        }
    }

    /* Initialise the RGMII interface, clears and masks all interrupts */
    dev->enet = enet_init(
                    mapped_peripheral,
                    dev->tx.phys,
                    dev->rx.phys,
                    BUF_SIZE,
                    mac,
                    &(driver->io_ops));
    if (!dev->enet) {
        ZF_LOGE("Failed to initialize RGMII");
        /* currently no way to properly clean up enet */
        assert(!"enet cannot be cleaned up");
        goto error;
    }

    /* Remove CRC (FCS) from ethernet frames when passing it to upper layers,
     * because the NIC hardware would discard frames with an invalid checksum
     * anyway by default. Usually, there not much practical gain in keeping it.
     */
    bool do_strip_crc = nic_config &&
                        (nic_config->flags & NIC_CONFIG_DROP_FRAME_CRC);
    ZF_LOGI("config: CRC stripping %s", do_strip_crc ? "ON" : "OFF");
    if (do_strip_crc) {
        enet_crc_strip_enable(dev->enet);
    } else {
        enet_crc_strip_disable(dev->enet);
    }

    /* Non-Promiscuous mode means that only traffic relevant for us is made
     * visible by the hardware, everything else is discarded automatically. We
     * will only see packets addressed to our MAC and broadcast/multicast
     * packets. This is usually all that is needed unless the upper layer
     * implements functionality beyond a "normal" application scope, e.g.
     * switching or monitoring.
     */
    bool do_promiscuous_mode = nic_config &&
                               (nic_config->flags & NIC_CONFIG_PROMISCUOUS_MODE);
    ZF_LOGI("config: promiscuous mode %s", do_promiscuous_mode ? "ON" : "OFF");
    if (do_promiscuous_mode) {
        enet_prom_enable(dev->enet);
    } else {
        enet_prom_disable(dev->enet);
    }

    /* Initialise the phy library */
    miiphy_init();
    /* Initialise the phy */
#if defined(CONFIG_PLAT_SABRE) || defined(CONFIG_PLAT_WANDQ)
    phy_micrel_init();
#elif defined(CONFIG_PLAT_NITROGEN6SX)
    phy_atheros_init();
#else
#error "unsupported board"
#endif

    /* Connect the phy to the ethernet controller */
    unsigned int phy_mask = 0xffffffff;
    if (nic_config && (0 != nic_config->phy_address)) {
        ZF_LOGI("config: using PHY at address %d", nic_config->phy_address);
        phy_mask = BIT(nic_config->phy_address);
    }
    /* ENET1 has an MDIO interface, for ENET2 we use callbacks */
    dev->phy = fec_init(
                   phy_mask,
                   (0 == enet_id) ? dev->enet : NULL,
                   nic_config);
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

    /* Start the controller, all interrupts are still masked here */
    enet_enable(dev->enet);

    /* This could also enable receiving, so the controller must be enabled. */
    fill_rx_bufs(driver);

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
    free_desc_ring(driver);
    return -1;
}


int ethif_imx6_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config) {

    if (config == NULL) {
        LOG_ERROR("Cannot get platform info; Passed in Config Pointer NULL");
        return -1;
    }

    struct arm_eth_plat_config *plat_config = (struct arm_eth_plat_config *)config;

    struct imx6_eth_data *eth_data = (struct imx6_eth_data *)calloc(1, sizeof(struct imx6_eth_data));
    if (eth_data == NULL) {
        LOG_ERROR("Failed to allocate eth data struct");
        return -1;
    }

    eth_data->tx.cnt = CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT;
    eth_data->rx.cnt = CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT;
    eth_driver->eth_data = eth_data;
    eth_driver->dma_alignment = DMA_ALIGN;
    eth_driver->i_fn = iface_fns;
    eth_driver->io_ops = io_ops;

    /* get a configuration if function is implemented */
    const nic_config_t *nic_config = NULL;
    if (get_nic_configuration) {
        ZF_LOGI("calling get_nic_configuration()");
        /* we can get NULL here, if somebody implements this function but does
         * not give us a config. It's a bit odd, but valid.
         */
        nic_config = get_nic_configuration();
        if (nic_config && nic_config->funcs.sync) {
            ZF_LOGI("Waiting for primary NIC to finish initialization");
            int ret = nic_config->funcs.sync();
            if (ret) {
                ZF_LOGE("pirmary NIC sync failed,  code %d", ret);
                return -ENODEV;
            }
            ZF_LOGI("primary NIC init done, run secondary NIC init");
        }
    } else {
        // Create a nic_config in order to passthrough prom_mode option
        nic_config_t *_nic_config = calloc(1, sizeof(*nic_config));
        _nic_config->flags = plat_config->prom_mode ? NIC_CONFIG_PROMISCUOUS_MODE : 0;
        nic_config = _nic_config;
    }


    int ret = init_device(eth_driver, nic_config, plat_config->buffer_addr);
    if (ret) {
        ZF_LOGE("Failed to initialise the Ethernet driver, code %d", ret);
        ret = -ENODEV;
        return -1;
    }

}


typedef struct {
    struct eth_driver eth_drv;
    struct imx6_eth_data eth_data;
    irq_id_t irq_id;
    void *mapped_peripheral;
} imx6_eth_driver_t;

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

    /* get a configuration if function is implemented */
    const nic_config_t *nic_config = NULL;
    if (get_nic_configuration) {
        ZF_LOGI("calling get_nic_configuration()");
        /* we can get NULL here, if somebody implements this function but does
         * not give us a config. It's a bit odd, but valid.
         */
        nic_config = get_nic_configuration();
        if (nic_config && nic_config->funcs.sync) {
            ZF_LOGI("Waiting for primary NIC to finish initialization");
            ret = nic_config->funcs.sync();
            if (ret) {
                ZF_LOGE("pirmary NIC sync failed,  code %d", ret);
                return -ENODEV;
            }
            ZF_LOGI("primary NIC init done, run secondary NIC init");
        }
    }

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


    driver->eth_drv.eth_data = &driver->eth_data; /* use simply extend the structure */
    driver->eth_drv.io_ops = *io_ops;
    driver->eth_drv.i_fn = iface_fns;

    driver->eth_drv.dma_alignment = DMA_ALIGN;
    driver->eth_data.tx.cnt = CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT;
    driver->eth_data.rx.cnt = CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT;

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

    ret = init_device(&driver->eth_drv, nic_config, driver->mapped_peripheral);
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
     * been tested are:
     *   - SABRE Lite (i.MX6Quad)
     *   - Nitrogen6_SoloX (i.MX6SoloX)
     *   - i.MX8MQ Evaluation Kit
     */
    "fsl,imx6q-fec",
    "fsl,imx6sx-fec",
    "fsl,imx8mq-fec",
    NULL
};

PS_DRIVER_MODULE_DEFINE(imx_fec, compatible_strings, ethif_imx_init_module);
