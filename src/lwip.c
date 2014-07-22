/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <ethdrivers/lwip_iface.h>
#include "raw_iface.h"
#include "descriptors.h"
#include <stdlib.h>
#include <string.h>
#include <lwip/netif.h>
#include <netif/etharp.h>
#include <lwip/stats.h>

#define MAX_PKT_SIZE    1536

#ifdef CONFIG_LIB_ETHDRIVER_ZERO_COPY_RX
/* LWiP provides a custom pbuf that you cannot actually store
 * any metadata or anything else in. So we define a custom one that has
 * useful stuff tacked onto the end of it */
typedef struct our_pbuf_custom {
    struct pbuf_custom pbuf;
    struct netif *netif;
    dma_addr_t dma_buf;
}our_pbuf_custom_t;

static void lwip_pbuf_custom_free(struct pbuf *p) {
    our_pbuf_custom_t *pbuf = (our_pbuf_custom_t*)p;
    struct eth_driver *driver = (struct eth_driver *)pbuf->netif->state;
    desc_rxfree(driver, pbuf->dma_buf);
    driver->r_fn->start_rx_logic(pbuf->netif);
    free(pbuf);
}
#endif

err_t ethif_link_output(struct netif *netif, struct pbuf *p);
int ethif_rawtx(struct netif *netif, dma_addr_t buf, size_t length);
int ethif_rawrx(struct netif *netif, rx_cb_t rxcb);
int ethif_rawscattertx(struct netif *netif, ethif_scatter_buf_t *buf, tx_complete_fn_t func, 
                       void *cookie);

static inline struct eth_driver*
netif_get_eth_driver(struct netif* netif) {
    return (struct eth_driver*)netif->state;
}

err_t
ethif_init(struct netif *netif) 
{
    if (netif -> state == NULL) {
        return ERR_ARG;
    }

    struct eth_driver* ethdriver = (struct eth_driver*) netif -> state; 
    ethdriver->r_fn->low_level_init(netif);

    netif->output = etharp_output;
    netif->linkoutput = ethif_link_output;

    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 
        LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

    netif -> flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | 
            NETIF_FLAG_LINK_UP;

    return ERR_OK;
}

static struct pbuf*
recieve_packet(struct netif *netif)
{
    struct pbuf *p = NULL;
#ifndef CONFIG_LIB_ETHDRIVER_ZERO_COPY_RX    
    struct pbuf *q = NULL;
#endif
    dma_addr_t buf;
    int res, len;
    struct eth_driver* ethdriver = (struct eth_driver*) netif -> state; 
    res = desc_rxget(ethdriver, &buf, &len); 
    if (res == 1) {
        assert(buf.virt);
#ifdef CONFIG_FEC_MXC_SWAP_PACKET
        // heh theres no frame_length defined anywhere... maybe remove this?
        swap_packet((uint32_t *)buf, frame_length);
#endif
#if ETH_PAD_SIZE
        len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif
        
#ifdef CONFIG_LIB_ETHDRIVER_ZERO_COPY_RX
        our_pbuf_custom_t *pbuf = (struct our_pbuf_custom*)malloc(sizeof(*pbuf));
        /* cannot tolerate failure here */
        assert(pbuf);
        pbuf->pbuf.custom_free_function = lwip_pbuf_custom_free;
        pbuf->dma_buf = buf;
        pbuf->netif = netif;
        /* Now allocate a chain of custom pbufs containing our payload */
        p = pbuf_alloced_custom(PBUF_RAW, len, PBUF_RAM, (struct pbuf_custom*)pbuf, buf.virt, MAX_PKT_SIZE);
#else
        /* Get a buffer from the pool */
        p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
        if (p == NULL) {
            printf("OOM for pbuf: packet dropped\n");
            // TODO: handle errors better
            desc_rxfree(ethdriver, buf);
            return NULL;
        }
#endif

#if ETH_PAD_SIZE
        pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

#ifndef CONFIG_LIB_ETHDRIVER_ZERO_COPY_RX
        /* fill the pbuf chain */
        uint8_t *frame_pos = (uint8_t*)buf.virt;
        for(q = p; q != NULL; q = q->next) {
            memcpy(q->payload, frame_pos, q->len);
            frame_pos += q->len;
        }
#endif
        //PKT_DEBUG(print_packet(COL_RX, buf, len));

#if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif
        LINK_STATS_INC(link.recv);

    } else {
        //PKT_DEBUG(printf("Packet error"));
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
    }

#ifndef CONFIG_LIB_ETHDRIVER_ZERO_COPY_RX
    if (res) {
        desc_rxfree(ethdriver, buf); 
    }
#endif
    ethdriver->r_fn->start_rx_logic(netif);
    return p;
}

int 
ethif_input(struct netif *netif)
{
    struct pbuf *p;
    struct eth_hdr *ethhdr;

    /* 
     * This is a good time to make sure that we have no buffers waiting to be 
     * transmitted. The problem occurs because there is a window during TX 
     * shutdown in which enabling the transmitter has no effect. Normally an
     * IRQ event would tell us to restart the TX logic but if this function is
     * being called, it is assumed that IRQs are not enabled.
     */
    struct eth_driver* ethdriver = (struct eth_driver*) netif -> state; 
    ethdriver->r_fn->start_tx_logic(netif);
 
    p = recieve_packet(netif);
    
    if (p == NULL) {
        return 0;
    }

    ethhdr = p->payload;
    
    switch (htons(ethhdr->type)) {
    /* IP or ARP packet? */
    case ETHTYPE_IP:
    case ETHTYPE_ARP:
#if PPPOE_SUPPORT
    /* PPPoE packet? */
    case ETHTYPE_PPPOEDISC:
    case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
    /* full packet send to tcpip_thread to process */
        if (netif->input(p, netif) != ERR_OK) { 
            LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
            pbuf_free(p);
            p = NULL;
        }
    break;

    default:
        pbuf_free(p);
        p = NULL;
    break;
    }
    return 1;
}

void
ethif_handleIRQ(struct netif *netif, int irq)
{
    struct eth_driver *ethdriver = netif_get_eth_driver(netif);
    // Just a wrapper for now - possibly generalise later
    ethdriver->r_fn->raw_handleIRQ(netif, irq);
}

const int*
ethif_enableIRQ(struct eth_driver *driver, int *nirqs)
{
    // Just a wrapper for now - possibly generalise later
    return driver->r_fn->raw_enableIRQ(driver, nirqs);
}

typedef struct tx_cookie {
    ps_io_ops_t io_ops;
    struct pbuf *pbuf;
    ethif_scatter_buf_t *buf;
} tx_cookie_t;

static void tx_complete_func(void *cookie) {
    struct tx_cookie *tx_cookie = (struct tx_cookie*)cookie;
    int i;
    for (i = 0; i < tx_cookie->buf->count; i++) {
        ps_dma_unpin(&tx_cookie->io_ops.dma_manager, tx_cookie->buf->bufs[i].buf.virt, 
                     tx_cookie->buf->bufs[i].len);
    }
    free(tx_cookie->buf);
    pbuf_free(tx_cookie->pbuf);
    free(tx_cookie);
}

err_t
ethif_link_output(struct netif *netif, struct pbuf *p)
{
    struct eth_driver *driver = netif_get_eth_driver(netif);
    dma_addr_t buf;
    int len;
    char* pkt_pos;
    struct pbuf *q;

    err_t ret;

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif
    int count = 0;

    /* first try and send all the parts of the packet user zero copy */
    for (q = p, count = 0; q != NULL; q = q->next) {
        count++;
    }
    while(!desc_txhasspace(driver->desc, count)) {
        desc_txcomplete(driver->desc, driver->d_fn);
    }
    ethif_scatter_buf_t *scatter_buf = (ethif_scatter_buf_t*)malloc(sizeof(*scatter_buf) + 
            sizeof(scatter_buf->bufs) * (count - 1));
    if (!scatter_buf) {
        return -1;
    }
    struct tx_cookie *cookie = malloc(sizeof(struct tx_cookie));
    if (!cookie) {
        free(scatter_buf);
        return -1;
    }
    cookie->io_ops = driver->io_ops;
    scatter_buf->count = count;
    int i;
    for (i = 0, q = p; q != NULL; q = q->next, i++) {
        scatter_buf->bufs[i].buf.virt = q->payload;
        scatter_buf->bufs[i].buf.phys = ps_dma_pin(&driver->io_ops.dma_manager, q->payload, q->len);
        if (!scatter_buf->bufs[i].buf.phys) {
            i--;
            goto zero_fail;
        }
        if ((scatter_buf->bufs[i].buf.phys % 16) != 0) {
            goto zero_fail;
        }
        scatter_buf->bufs[i].len = q->len;
    }
    pbuf_ref(p);
    cookie->pbuf = p;
    cookie->buf = scatter_buf;
    ret = ethif_rawscattertx(netif, scatter_buf, tx_complete_func, cookie);
    assert(!ret);
#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif
    //printf("Sent a scatter packet!!\n");
    return ret;
zero_fail:
    for (; i >= 0; i--) {
        ps_dma_unpin(&driver->io_ops.dma_manager, scatter_buf->bufs[i].buf.virt, 
                     scatter_buf->bufs[i].len);
    }
    free(scatter_buf);
    free(cookie);

    /* send the packet the regular way */
    len = desc_txget(driver->desc, &buf, driver->d_fn);
    if (len == 0) {
        printf("No TX descriptors available\n");
#if 1
        /* Wait for a descriptor to be available */
        while (len == 0) {
            len = desc_txget(driver->desc, &buf, driver->d_fn);
        }
#else
        /* Return an error */
        return -1;
#endif
        printf("Got TX descriptor... Continuing\n");
    }

    if ((p->tot_len > len) || (p->tot_len <= 0)) {
        printf("Payload (%d) too large\n", p->tot_len);
        return -1;
    }
    pkt_pos = (char*)buf.virt;
    for(q = p; q != NULL; q = q->next) {
        memcpy(pkt_pos, q->payload, q->len);
        pkt_pos += q->len;
    }
    //PKT_DEBUG(cprintf(COL_TX, "Sending packet"));
    //PKT_DEBUG(print_packet(COL_TX, (void*)buf.virt, p->tot_len));

    ret = ethif_rawtx(netif, buf, p->tot_len);

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.xmit);

    return ret;
}

int
ethif_rawtx(struct netif *netif, dma_addr_t buf, size_t length)
{
    struct eth_driver *driver = netif_get_eth_driver(netif);
    /* create a descriptor */
    if (desc_txput(driver, buf, length)) {
        return -1;
    } else {
        driver->r_fn->start_tx_logic(netif);
        return 0;
    }
}

int
ethif_rawscattertx(struct netif *netif, ethif_scatter_buf_t *buf, tx_complete_fn_t func, void *cookie)
{
    struct eth_driver *driver = netif_get_eth_driver(netif);
    /* create a descriptor */
    if (desc_txputmany(driver, buf, func, cookie)) {
        return -1;
    } else {
        driver->r_fn->start_tx_logic(netif);
        return 0;
    }
}

int 
ethif_rawrx(struct netif *netif, rx_cb_t rxcb)
{
    struct eth_driver *driver = netif_get_eth_driver(netif);
    dma_addr_t buf;
    int len;
    int res;

    res = desc_rxget(driver, &buf, &len);
   
    if (!res) return 0;
    //PKT_DEBUG(cprintf(COL_RX, "Receiving packet"));
    if (res == 1) {
        /* Get buffer address and size */
        uint8_t* data = (uint8_t*)buf.virt;

        /* Fill the buffer and pass it to upper layers */
#ifdef CONFIG_FEC_MXC_SWAP_PACKET
        swap_packet((uint32_t *)frame->data, frame_length);
#endif

        //PKT_DEBUG(printf("Receiving packet\n"));
        //PKT_DEBUG(print_packet(COL_RX, data, len));
        rxcb((uintptr_t)data, len);

#if ETH_PAD_SIZE
//      pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif
    
    } else {
        //FEC_DEBUG(printf("Frame error\n"));
    }

    /* We are done with the buf, return it */
    desc_rxfree(driver, buf);
    /* enable rx */
    driver->r_fn->start_rx_logic(netif);
    return 1;
}
