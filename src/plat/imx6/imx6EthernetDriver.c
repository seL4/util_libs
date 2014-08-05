/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include "../../descriptors.h"
#include <ethdrivers/lwip_iface.h>
#include "imx6EthernetDriver.h"
#include "../../dma_buffers.h"
#include "unimplemented.h"
#include "uboot/common.h"
#include "uboot/fec_mxc.h"
#include "io.h"
#include "../../raw_iface.h"
#include <netif/etharp.h>
#include <lwip/stats.h>
#include "uboot/miiphy.h"
#include "uboot/micrel.h"
#include "debug.h"
#include "ocotp_ctrl.h"
#include <assert.h>
#include <string.h>
#include "enet.h"
#include <utils/util.h>

#define DEFAULT_MAC "\x00\x19\xb8\x00\xf0\xa3"

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

/* We chose the total number of spare DMA buffers to be 2 * RX ring size + TX ring size
 * This means we can be processing a full set of received packets, transmitting as
 * many packets as possible, and still have buffers to receive more packets. In reality
 * I made this constant up so it's probably bad */
#define NUM_DMA_BUFS (CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT * 2 + \
                        CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT)
int setup_iomux_enet(ps_io_ops_t *io_ops);

#define INTERRUPT_ENET 150
#define INTERRUPT_ENET_TIMER 151
static const int net_irqs[] = {
        INTERRUPT_ENET,
        INTERRUPT_ENET_TIMER,
        0
};

struct imx6_eth_data {
    struct enet * enet;
    ps_io_ops_t io_ops;
    int irq_enabled;
};

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

// Driver specific implementations of function in raw_iface.h and raw_descriptors.h
struct raw_iface_funcs* setup_raw_iface_funcs();
struct raw_desc_funcs* setup_raw_desc_funcs();
void  imx6_low_level_init(struct netif *netif);
void imx6_start_tx_logic(struct netif *netif);
void imx6_start_rx_logic(struct netif *netif);
void imx6_raw_handleIRQ(struct netif *netif, int irq);
const int* imx6_raw_enableIRQ(struct eth_driver *driver, int *nirqs);
dma_addr_t imx6_create_tx_descs(ps_dma_man_t *dma_man, int count);
dma_addr_t imx6_create_rx_descs(ps_dma_man_t *dman_man, int count);
void imx6_reset_tx_descs(struct eth_driver *driver);
void imx6_reset_rx_descs(struct eth_driver *driver);
int imx6_is_tx_desc_ready(int buf_num, struct desc *desc);
int imx6_is_rx_desc_empty(int buf_num, struct desc *desc);
void imx6_set_tx_desc_buf(int buf_num, dma_addr_t buf, int len, int tx_desc_wrap, int tx_last_section, struct desc *desc);
void imx6_ready_tx_desc(int buf_num, int num, struct eth_driver *driver);
void imx6_set_rx_desc_buf(int buf_num, dma_addr_t buf, int len, struct desc *desc);
void imx6_ready_rx_desc(int buf_num, int rx_desc_wrap, struct eth_driver *driver);
int imx6_get_rx_buf_len(int buf_num, struct desc *desc);
int imx6_get_rx_desc_error(int buf_num, struct desc *desc);

struct imx6_eth_data*
eth_driver_get_eth_data(struct eth_driver *eth_driver) {
    return (struct imx6_eth_data*)eth_driver->eth_data;
}

struct eth_driver*
netif_get_eth_driver(struct netif *netif) {
    return (struct eth_driver*)netif->state;
}

struct imx6_eth_data*
netif_get_eth_data(struct netif *netif) {
    return eth_driver_get_eth_data(netif_get_eth_driver(netif));
}

struct eth_driver*
ethif_imx6_init(int dev_id, ps_io_ops_t interface)
{
    struct eth_driver *driver = NULL;
    struct desc* desc = NULL;
    
    struct ocotp *ocotp = NULL;
    int err;
    struct enet * enet;
    struct imx6_eth_data *eth_data = NULL;
    uint8_t mac[6];
    (void)dev_id;

    driver = (struct eth_driver*)malloc(sizeof(struct eth_driver));
    if (driver == NULL) {
        LOG_ERROR("Failed to allocate eth driver struct");
        goto error;
    }
    driver->r_fn = setup_raw_iface_funcs();
    if (!driver->r_fn) {
        goto error;
    }
    driver->d_fn = setup_raw_desc_funcs(); 
    if (!driver->d_fn) {
        goto error;
    }

    eth_data = (struct imx6_eth_data*)malloc(sizeof(struct imx6_eth_data));
    if (eth_data == NULL) {
        LOG_ERROR("Failed to allocate eth data struct");
        goto error;
    }

    desc = desc_init(&interface.dma_manager, CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT, 
                    CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT, NUM_DMA_BUFS, MAX_PKT_SIZE, driver);
    if (!desc) {
        LOG_ERROR("Failed to initialize / allocate DMA descriptors");
        goto error;
    }

    /* initialise the eFuse controller so we can get a MAC address */
    ocotp = ocotp_init(&interface.io_mapper);
    if (!ocotp) {
        LOG_INFO("Failed to initialize ocotp");
    }
    /* Initialise ethernet pins */
    //gpio_init();
    err = setup_iomux_enet(&interface);
    if (err) {
        LOG_INFO("Failed to setup iomux enet");
    }
    /* Initialise the phy library */
    miiphy_init();
    /* Initialise the phy */
    phy_micrel_init();
    /* Initialise the RGMII interface */ 
    enet = enet_init(desc, &interface);
    assert(enet);

    /* Fetch and set the MAC address */
    if (ocotp == NULL || ocotp_get_mac(ocotp, mac)) {
        memcpy(mac, DEFAULT_MAC, 6);
    }
    enet_set_mac(enet, mac);

    /* Connect the phy to the ethernet controller */
    if (fec_init(CONFIG_FEC_MXC_PHYMASK, enet)) {
        return NULL;
    }

    /* Start the controller */
    enet_enable(enet);

    /* Update book keeping */
    eth_data->irq_enabled = 0;
    eth_data->enet = enet;
    driver->io_ops = interface;
    driver->eth_data = eth_data;
    driver->desc = desc;
    
    /* done */
    return driver;
error:
    if (ocotp) {
        ocotp_free(ocotp, &interface.io_mapper);
    }
    if (desc) {
        desc_free(desc);
    }
    if (eth_data) {
        free(eth_data);
    }
    if (driver) {
        free(driver);
    }
    return NULL;
}

struct raw_iface_funcs*
setup_raw_iface_funcs()
{
    struct raw_iface_funcs* r_fn = malloc(sizeof(struct raw_iface_funcs));
    if (!r_fn) {
        return NULL;
    }
    r_fn->low_level_init = imx6_low_level_init;
    r_fn->start_tx_logic = imx6_start_tx_logic;
    r_fn->start_rx_logic = imx6_start_rx_logic;
    r_fn->raw_handleIRQ = imx6_raw_handleIRQ; 
    r_fn->raw_enableIRQ = imx6_raw_enableIRQ;
    // Missing rawscattertx, print_state
    return r_fn;
}

struct raw_desc_funcs*
setup_raw_desc_funcs()
{
    struct raw_desc_funcs *d_fn = malloc(sizeof(struct raw_desc_funcs));
    if (!d_fn) {
        return NULL;
    }
    d_fn->create_tx_descs = imx6_create_tx_descs;
    d_fn->create_rx_descs = imx6_create_rx_descs;
    d_fn->reset_tx_descs = imx6_reset_tx_descs;
    d_fn->reset_rx_descs = imx6_reset_rx_descs;
    d_fn->ready_tx_desc = imx6_ready_tx_desc;
    d_fn->ready_rx_desc = imx6_ready_rx_desc;
    d_fn->is_tx_desc_ready = imx6_is_tx_desc_ready;
    d_fn->is_rx_desc_empty = imx6_is_rx_desc_empty;
    d_fn->set_tx_desc_buf = imx6_set_tx_desc_buf;
    d_fn->set_rx_desc_buf = imx6_set_rx_desc_buf;
    d_fn->get_rx_buf_len = imx6_get_rx_buf_len;
    d_fn->get_rx_desc_error = imx6_get_rx_desc_error;
    return d_fn;
}

void imx6_ethernet_get_mac(struct netif* netif, uint8_t* hwaddr){
    enet_get_mac(netif_get_eth_data(netif)->enet, hwaddr);
}

void 
imx6_low_level_init(struct netif *netif) 
{
#if LWIP_NETIF_HOSTNAME
    netif -> hostname = "imx6_sabrelite";
#endif

    netif->name[0] = 'K';
    netif->name[1] = 'Z';

    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    imx6_ethernet_get_mac(netif, netif->hwaddr); 
    netif->mtu = MAX_PKT_SIZE;
}

void 
imx6_start_tx_logic(struct netif *netif)
{
    struct imx6_eth_data* eth_data = netif_get_eth_data(netif);
    enet_tx_enable(eth_data->enet);
}

void 
imx6_start_rx_logic(struct netif *netif)
{
    struct imx6_eth_data* eth_data = netif_get_eth_data(netif);
    enet_rx_enable(eth_data->enet);
}

void
imx6_raw_handleIRQ(struct netif *netif, int irq)
{
    struct imx6_eth_data *eth_data = netif_get_eth_data(netif);
    struct eth_driver *driver = netif_get_eth_driver(netif);
    struct enet* enet = eth_data->enet; 
    uint32_t e;
    while((e = enet_clr_events(enet, NETIRQ_RXF | NETIRQ_TXF | NETIRQ_EBERR))){
        if(e & NETIRQ_TXF){
            int enabled, complete;
            e &= ~NETIRQ_TXF;
            enabled = enet_tx_enabled(enet);
            complete = desc_txcomplete(driver->desc, driver->d_fn);
            if(!enabled && !complete){
                enet_tx_enable(enet);
            }
        }
        if(e & NETIRQ_RXF){
            e &= ~NETIRQ_RXF;
            while(ethif_input(netif));
        }
        if(e & NETIRQ_EBERR){
            printf("Error: System bus/uDMA\n");
            //ethif_print_state(netif_get_eth_driver(netif));
            assert(0);
            while(1);
        }
        if(e){
            printf("Unhandled irqs 0x%x\n", e);
        }
    }
}

const int*
imx6_raw_enableIRQ(struct eth_driver *driver, int *nirqs)
{
    struct imx6_eth_data *eth_data = eth_driver_get_eth_data(driver);
    struct enet* enet = eth_data->enet;
    assert(enet); 
    int i;
    enet_enable_events(enet, 0);
    enet_clr_events(enet, ~(NETIRQ_RXF | NETIRQ_TXF | NETIRQ_EBERR));
    enet_enable_events(enet, NETIRQ_RXF | NETIRQ_TXF | NETIRQ_EBERR);
    eth_data->irq_enabled = 1;
    /* Count the IRQS. This is complex for backwards compatibility 
     * The IRQ list may be NULL terminated, else we can just take the size.
     */
    for(i = 0; i < sizeof(net_irqs)/sizeof(*net_irqs) && net_irqs[i] != 0; i++);
    *nirqs = i;
    return net_irqs;
}

dma_addr_t
imx6_create_tx_descs(ps_dma_man_t *dma_man, int count)
{
    return dma_alloc_pin(dma_man, sizeof(struct descriptor) * count, 0);
}

dma_addr_t
imx6_create_rx_descs(ps_dma_man_t *dma_man, int count)
{
    return dma_alloc_pin(dma_man, sizeof(struct descriptor) * count, 0);
}

void
imx6_reset_tx_descs(struct eth_driver *driver)
{
    int i;
    struct desc *desc = driver->desc;
    struct descriptor *d = desc->tx.ring.virt;
    for (i = 0; i < desc->tx.count; i++) {
        d[i].stat = 0;
        d[i].phys = 0;
        d[i].len = 0;
    }
    d[desc->tx.count - 1].stat |= TXD_WRAP;
    __sync_synchronize();
    ps_dma_cache_clean(&desc->dma_man, desc->tx.ring.virt, sizeof(*d) * desc->tx.count);
}

// TODO: remove buffer handling from here
void
imx6_reset_rx_descs(struct eth_driver *driver)
{
    int i;
    struct desc *desc = driver->desc;
    struct descriptor *d = desc->rx.ring.virt;
    for(i = 0; i < desc->rx.count; i++) {
        desc->rx.buf[i] = alloc_dma_buf(desc);
        assert(desc->rx.buf[i].phys);
        d[i].phys = (uint32_t)desc->rx.buf[i].phys;
        d[i].stat = RXD_EMPTY;
        d[i].len = 0;
    }
    d[desc->rx.count - 1].stat |= RXD_WRAP;
    __sync_synchronize();
    ps_dma_cache_clean(&desc->dma_man, desc->rx.ring.virt, sizeof(*d) * desc->rx.count);
}

int 
imx6_is_tx_desc_ready(int buf_num, struct desc *desc) 
{
    struct descriptor *d = desc->tx.ring.virt;
    return d[buf_num].stat & TXD_READY;
}

int
imx6_is_rx_desc_empty(int buf_num, struct desc *desc)
{
    struct descriptor *d = desc->rx.ring.virt;
    return d[buf_num].stat & RXD_EMPTY; 
}

void
imx6_set_tx_desc_buf(int buf_num, dma_addr_t buf, int len, int tx_desc_wrap, int tx_last_section, struct desc *desc)
{
    struct descriptor *d = desc->tx.ring.virt;
    int stat = 0;
    d[buf_num].len = len;
    d[buf_num].phys = (uint32_t)buf.phys;
    if (tx_desc_wrap) stat = stat | TXD_WRAP;
    if (tx_last_section) stat = stat | TXD_ADDCRC | TXD_LAST;
    d[buf_num].stat = stat;
}

// Might need to abstract higher for scatter buffers
void
imx6_ready_tx_desc(int buf_num, int num, struct eth_driver *driver)
{
    struct descriptor *d = driver->desc->tx.ring.virt;
    int i;
    /* Synchronize updates to buffers before signaling ready */
    __sync_synchronize();
    /* Set buffers in reverse order */
    for (i = 0; i < num; i++) {
        int j = (buf_num + (num - 1 - i)) % driver->desc->tx.count;
        d[j].stat |= TXD_READY;
    }
    /* Make sure updates are observable */
    __sync_synchronize();
}

void
imx6_set_rx_desc_buf(int buf_num, dma_addr_t buf, int len, struct desc *desc)
{
    struct descriptor *d = desc->rx.ring.virt;
    d[buf_num].len = len;
    d[buf_num].phys = buf.phys;
}

void
imx6_ready_rx_desc(int buf_num, int rx_desc_wrap, struct eth_driver *driver)
{
    struct descriptor *d = driver->desc->rx.ring.virt;
    int stat = RXD_EMPTY;
    __sync_synchronize();
    if (rx_desc_wrap) stat = stat | RXD_WRAP;
    d[buf_num].stat = stat;
    __sync_synchronize();
}

int
imx6_get_rx_buf_len(int buf_num, struct desc *desc)
{
    struct descriptor *d = desc->rx.ring.virt;
    return d[buf_num].len;
}

int
imx6_get_rx_desc_error(int buf_num, struct desc *desc)
{
    struct descriptor *d = desc->rx.ring.virt;
    return d[buf_num].stat & RXD_ERROR;
}

struct desc_data
desc_get_ringdata(struct desc *desc)
{
    struct desc_data d;
    d.tx_phys = (uint32_t)desc->tx.ring.phys;
    d.tx_bufsize = desc->buf_size;
    d.rx_phys = (uint32_t)desc->rx.ring.phys;
    d.rx_bufsize = desc->buf_size;
    return d;
} 
