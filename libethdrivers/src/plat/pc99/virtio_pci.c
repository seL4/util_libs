/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#include <ethdrivers/virtio_pci.h>
#include <assert.h>
#include <ethdrivers/helpers.h>
#include <ethdrivers/virtio/virtio_config.h>
#include <ethdrivers/virtio/virtio_pci.h>
#include <ethdrivers/virtio/virtio_ring.h>
#include <ethdrivers/virtio/virtio_net.h>
#include <string.h>

/* Mask of features we will use */
#define FEATURES_REQUIRED (BIT(VIRTIO_NET_F_MAC))

#define BUF_SIZE 2048
#define DMA_ALIGN 16

#define RX_QUEUE 0
#define TX_QUEUE 1

typedef struct virtio_dev {
    void *mmio_base;
    uint16_t io_base;
    ps_io_port_ops_t ioops;
    /* R/T Descriptor Head represents the beginning of the block of
     * descriptors that are currently in use */
    unsigned int tdh;
    unsigned int rdh;
    /* R/T Descriptor Tail represents the next free slot to add
     * a descriptor */
    unsigned int tdt;
    unsigned int rdt;
    /* R/T Used Head represents the index in the used ring that
     * we last observed */
    uint16_t tuh;
    uint16_t ruh;
    /* descriptor rings */
    uintptr_t rx_ring_phys;
    struct vring rx_ring;
    unsigned int rx_size;
    unsigned int rx_remain;
    void **rx_cookies;
    uintptr_t tx_ring_phys;
    struct vring tx_ring;
    unsigned int tx_size;
    unsigned int tx_remain;
    void **tx_cookies;
    unsigned int *tx_lengths;
    /* preallocated header. Since we do not actually use any features
     * in the header we put the same one before every send/receive packet */
    uintptr_t virtio_net_hdr_phys;
} virtio_dev_t;

static uint8_t read_reg8(virtio_dev_t *dev, uint16_t port) {
    uint32_t val;
    ps_io_port_in(&dev->ioops, dev->io_base + port, 1, &val);
    return (uint8_t)val;
}

static uint16_t read_reg16(virtio_dev_t *dev, uint16_t port) {
    uint32_t val;
    ps_io_port_in(&dev->ioops, dev->io_base + port, 2, &val);
    return (uint16_t)val;
}

static uint32_t read_reg32(virtio_dev_t *dev, uint16_t port) {
    uint32_t val;
    ps_io_port_in(&dev->ioops, dev->io_base + port, 4, &val);
    return val;
}

static void write_reg8(virtio_dev_t *dev, uint16_t port, uint8_t val) {
    ps_io_port_out(&dev->ioops, dev->io_base + port, 1, val);
}

static void write_reg16(virtio_dev_t *dev, uint16_t port, uint16_t val) {
    ps_io_port_out(&dev->ioops, dev->io_base + port, 2, val);
}

static void write_reg32(virtio_dev_t *dev, uint16_t port, uint32_t val) {
    ps_io_port_out(&dev->ioops, dev->io_base + port, 4, val);
}

static void set_status(virtio_dev_t *dev, uint8_t status) {
    write_reg8(dev, VIRTIO_PCI_STATUS, status);
}

static uint8_t get_status(virtio_dev_t *dev) {
    return read_reg8(dev, VIRTIO_PCI_STATUS);
}

static void add_status(virtio_dev_t *dev, uint8_t status) {
    write_reg8(dev, VIRTIO_PCI_STATUS, get_status(dev) | status);
}

static uint32_t get_features(virtio_dev_t *dev) {
    return read_reg32(dev, VIRTIO_PCI_HOST_FEATURES);
}

static void set_features(virtio_dev_t *dev, uint32_t features) {
    write_reg32(dev, VIRTIO_PCI_GUEST_FEATURES, features);
}

static void free_desc_ring(virtio_dev_t *dev, ps_dma_man_t *dma_man) {
    if (dev->rx_ring.desc) {
        dma_unpin_free(dma_man, (void*)dev->rx_ring.desc, vring_size(dev->rx_size, VIRTIO_PCI_VRING_ALIGN));
        dev->rx_ring.desc = NULL;
    }
    if (dev->tx_ring.desc) {
        dma_unpin_free(dma_man, (void*)dev->tx_ring.desc, vring_size(dev->tx_size, VIRTIO_PCI_VRING_ALIGN));
        dev->tx_ring.desc = NULL;
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

static int initialize_desc_ring(virtio_dev_t *dev, ps_dma_man_t *dma_man) {
    dma_addr_t rx_ring = dma_alloc_pin(dma_man, vring_size(dev->rx_size, VIRTIO_PCI_VRING_ALIGN), 1, VIRTIO_PCI_VRING_ALIGN);
    if (!rx_ring.phys) {
        LOG_ERROR("Failed to allocate rx_ring");
        return -1;
    }
    memset(rx_ring.virt, 0, vring_size(dev->rx_size, VIRTIO_PCI_VRING_ALIGN));
    vring_init(&dev->rx_ring, dev->rx_size, rx_ring.virt, VIRTIO_PCI_VRING_ALIGN);
    dev->rx_ring_phys = rx_ring.phys;
    dma_addr_t tx_ring = dma_alloc_pin(dma_man, vring_size(dev->tx_size, VIRTIO_PCI_VRING_ALIGN), 1, VIRTIO_PCI_VRING_ALIGN);
    if (!tx_ring.phys) {
        LOG_ERROR("Failed to allocate tx_ring");
        free_desc_ring(dev, dma_man);
        return -1;
    }
    memset(tx_ring.virt, 0, vring_size(dev->tx_size, VIRTIO_PCI_VRING_ALIGN));
    vring_init(&dev->tx_ring, dev->tx_size, tx_ring.virt, VIRTIO_PCI_VRING_ALIGN);
    dev->tx_ring_phys = tx_ring.phys;
    dev->rx_cookies = malloc(sizeof(void*) * dev->rx_size);
    dev->tx_cookies = malloc(sizeof(void*) * dev->tx_size);
    dev->tx_lengths = malloc(sizeof(unsigned int) * dev->tx_size);
    if (!dev->rx_cookies || !dev->tx_cookies || !dev->tx_lengths) {
        LOG_ERROR("Failed to malloc");
        free_desc_ring(dev, dma_man);
        return -1;
    }
    /* Remaining needs to be 2 less than size as we cannot actually enqueue size many descriptors,
     * since then the head and tail pointers would be equal, indicating empty. */
    dev->rx_remain = dev->rx_size - 2;
    dev->tx_remain = dev->tx_size - 2;

    dev->tdh = dev->tdt = 0;
    dev->rdh = dev->rdt = 0;
    dev->tuh = dev->ruh = 0;

    return 0;
}

static int initialize(virtio_dev_t *dev, ps_dma_man_t *dma_man) {
    int err;
    /* perform a reset */
    set_status(dev, 0);
    /* acknowledge to the host that we found it */
    add_status(dev, VIRTIO_CONFIG_S_ACKNOWLEDGE);
    /* read device features */
    uint32_t features;
    features = get_features(dev);
    if ( (features & FEATURES_REQUIRED) != FEATURES_REQUIRED) {
        LOG_ERROR("Required features 0x%x, have 0x%x", (unsigned int)FEATURES_REQUIRED, features);
        return -1;
    }
    features &= FEATURES_REQUIRED;
    /* write the features we will use */
    set_features(dev, features);
    /* determine the queue size */
    write_reg16(dev, VIRTIO_PCI_QUEUE_SEL, RX_QUEUE);
    dev->rx_size = read_reg16(dev, VIRTIO_PCI_QUEUE_NUM);
    write_reg16(dev, VIRTIO_PCI_QUEUE_SEL, TX_QUEUE);
    dev->tx_size = read_reg16(dev, VIRTIO_PCI_QUEUE_NUM);
    /* create the rings */
    err = initialize_desc_ring(dev, dma_man);
    if (err) {
        return -1;
    }
    /* write the virtqueue locations */
    write_reg16(dev, VIRTIO_PCI_QUEUE_SEL, RX_QUEUE);
    write_reg32(dev, VIRTIO_PCI_QUEUE_PFN, ((uintptr_t)dev->rx_ring_phys) / 4096);
    write_reg16(dev, VIRTIO_PCI_QUEUE_SEL, TX_QUEUE);
    write_reg32(dev, VIRTIO_PCI_QUEUE_PFN, ((uintptr_t)dev->tx_ring_phys) / 4096);
    /* tell the driver everything is okay */
    add_status(dev, VIRTIO_CONFIG_S_DRIVER_OK);
    return 0;
}

static void get_mac(virtio_dev_t *dev, uint8_t *mac) {
    int i;
    for (i = 0; i < 6; i++) {
        mac[i] = read_reg8(dev, 0x14 + i);
    }
}

static void low_level_init(struct eth_driver *driver, uint8_t *mac, int *mtu) {
    virtio_dev_t *dev = (virtio_dev_t*)driver->eth_data;
    get_mac(dev, mac);
    *mtu = 1500;
}


static void print_state(struct eth_driver *eth_driver) {
}

static void complete_tx(struct eth_driver *driver) {
    virtio_dev_t *dev = (virtio_dev_t*)driver->eth_data;
    while (dev->tuh != dev->tx_ring.used->idx) {
        uint16_t ring = dev->tuh % dev->tx_size;
        unsigned int UNUSED desc = dev->tx_ring.used->ring[ring].id;
        assert(desc == dev->tdh);
        void *cookie = dev->tx_cookies[dev->tdh];
        /* add 1 to the length we stored to account for the extra descriptor
         * we used for the virtio header */
        unsigned int used = dev->tx_lengths[dev->tdh] + 1;
        dev->tx_remain += used;
        dev->tdh = (dev->tdh + used) % dev->tx_size;
        dev->tuh++;
        /* give the buffer back */
        driver->i_cb.tx_complete(driver->cb_cookie, cookie);
    }
}

static void fill_rx_bufs(struct eth_driver *driver) {
    virtio_dev_t *dev = (virtio_dev_t*)driver->eth_data;
    /* we need 2 free as we enqueue in pairs. One descriptor to hold the
     * virtio header, another one for the actual buffer */
    while (dev->rx_remain >= 2) {
        /* request a buffer */
        void *cookie;
        uintptr_t phys = driver->i_cb.allocate_rx_buf(driver->cb_cookie, BUF_SIZE, &cookie);
        if (!phys) {
            break;
        }
        unsigned int next_rdt = (dev->rdt + 1) % dev->rx_size;
        dev->rx_ring.desc[dev->rdt] = (struct vring_desc) {
            .addr = dev->virtio_net_hdr_phys,
            .len = sizeof(struct virtio_net_hdr),
            .flags = VRING_DESC_F_NEXT | VRING_DESC_F_WRITE,
            .next = next_rdt
        };
        dev->rx_cookies[dev->rdt] = cookie;
        dev->rx_ring.desc[next_rdt] = (struct vring_desc) {
            .addr = phys,
            .len = BUF_SIZE,
            .flags = VRING_DESC_F_WRITE,
            .next = 0
        };
        dev->rx_ring.avail->ring[dev->rx_ring.avail->idx % dev->rx_size] = dev->rdt;
        asm volatile("sfence" ::: "memory");
        dev->rx_ring.avail->idx++;
        asm volatile("sfence" ::: "memory");
        write_reg16(dev, VIRTIO_PCI_QUEUE_NOTIFY, RX_QUEUE);
        dev->rdt = (dev->rdt + 2) % dev->rx_size;
        dev->rx_remain-=2;
    }
}

static void complete_rx(struct eth_driver *driver) {
    virtio_dev_t *dev = (virtio_dev_t*)driver->eth_data;
    while (dev->ruh != dev->rx_ring.used->idx) {
        uint16_t ring = dev->ruh % dev->rx_size;
        unsigned int UNUSED desc = dev->rx_ring.used->ring[ring].id;
        assert(desc == dev->rdh);
        void *cookie = dev->rx_cookies[dev->rdh];
        /* subtract off length of the virtio header we received */
        unsigned int len = dev->rx_ring.used->ring[ring].len - sizeof(struct virtio_net_hdr);
        /* update rdh. remember we actually had two descriptors, one
         * is the header that we threw away, the other being the actual data */
        dev->rdh = (dev->rdh + 2) % dev->rx_size;
        dev->rx_remain += 2;
        dev->ruh++;
        /* Give the buffers back */
        driver->i_cb.rx_complete(driver->cb_cookie, 1, &cookie, &len);
    }
}

static int raw_tx(struct eth_driver *driver, unsigned int num, uintptr_t *phys, unsigned int *len, void *cookie) {
    virtio_dev_t *dev = (virtio_dev_t*)driver->eth_data;
    /* we need to num + 1 free descriptors. The + 1 is for the virtio header */
    if (dev->tx_remain < num + 1) {
        complete_tx(driver);
        if (dev->tx_remain < num + 1) {
            return ETHIF_TX_FAILED;
        }
    }
    /* install the header */
    dev->tx_ring.desc[dev->tdt] = (struct vring_desc) {
        .addr = dev->virtio_net_hdr_phys,
        .len = sizeof(struct virtio_net_hdr),
        .flags = VRING_DESC_F_NEXT,
        .next = (dev->tdt + 1) % dev->tx_size
    };
    /* now all the buffers */
    unsigned int i;
    for (i = 0; i < num; i++) {
        unsigned int desc = (dev->tdt + i + 1) % dev->tx_size;
        unsigned int next_desc = (desc + 1) % dev->tx_size;
        dev->tx_ring.desc[desc] = (struct vring_desc) {
            .addr = phys[i],
            .len = len[i],
            .flags = (i + 1 == num ? 0 : VRING_DESC_F_NEXT),
            .next = next_desc
        };
    }
    dev->tx_ring.avail->ring[dev->tx_ring.avail->idx% dev->tx_size] = dev->tdt;
    dev->tx_cookies[dev->tdt] = cookie;
    dev->tx_lengths[dev->tdt] = num;
    /* ensure update to descriptors visible before updating the index */
    asm volatile("mfence" ::: "memory");
    dev->tdt = (dev->tdt + num + 1) % dev->tx_size;
    dev->tx_remain -= (num + 1);
    dev->tx_ring.avail->idx++;
    /* ensure index update visible before notifying */
    asm volatile("mfence" ::: "memory");
    write_reg16(dev, VIRTIO_PCI_QUEUE_NOTIFY, TX_QUEUE);
    return ETHIF_TX_ENQUEUED;
}

static void raw_poll(struct eth_driver *driver) {
    complete_tx(driver);
    complete_rx(driver);
    fill_rx_bufs(driver);
}

static void handle_irq(struct eth_driver *driver, int irq) {
    virtio_dev_t *dev = (virtio_dev_t*)driver->eth_data;
    /* read and throw away the ISR state. This will perform the ack */
    read_reg8(dev, VIRTIO_PCI_ISR);
    raw_poll(driver);
}
static struct raw_iface_funcs iface_fns = {
    .raw_handleIRQ = handle_irq,
    .print_state = print_state,
    .low_level_init = low_level_init,
    .raw_tx = raw_tx,
    .raw_poll = raw_poll
};

int ethif_virtio_pci_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config) {
    int err;
    ethif_virtio_pci_config_t *virtio_config = (ethif_virtio_pci_config_t*)config;
    virtio_dev_t *dev = (virtio_dev_t*)malloc(sizeof(*dev));
    if (!dev) {
        return -1;
    }

    dev->mmio_base = virtio_config->mmio_base;
    dev->io_base = virtio_config->io_base;
    dev->ioops = io_ops.io_port_ops;

    eth_driver->eth_data = dev;
    eth_driver->dma_alignment = 16;
    eth_driver->i_fn = iface_fns;

    err = initialize(dev, &io_ops.dma_manager);
    if (err) {
        goto error;
    }
    dma_addr_t packet = dma_alloc_pin(&io_ops.dma_manager, sizeof(struct virtio_net_hdr), 1, DMA_ALIGN);
    if (!packet.virt) {
        goto error;
    }
    memset(packet.virt, 0, sizeof(struct virtio_net_hdr));
    dev->virtio_net_hdr_phys = packet.phys;

    fill_rx_bufs(eth_driver);

    return 0;

error:
    set_status(dev, VIRTIO_CONFIG_S_FAILED);
    free_desc_ring(dev, &io_ops.dma_manager);
    free(dev);
    return -1;
}
