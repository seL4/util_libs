/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <autoconf.h>
#include <ethdrivers/gen_config.h>
#include <lwip/gen_config.h>

#ifdef CONFIG_LIB_LWIP

#include <ethdrivers/lwip.h>
#include <ethdrivers/helpers.h>
#include <string.h>
#include <lwip/netif.h>
#include <netif/etharp.h>
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include "debug.h"

static void initialize_free_bufs(lwip_iface_t *iface)
{
    dma_addr_t *dma_bufs = NULL;
    dma_bufs = malloc(sizeof(dma_addr_t) * CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS);
    if (!dma_bufs) {
        goto error;
    }
    memset(dma_bufs, 0, sizeof(dma_addr_t) * CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS);
    iface->bufs = malloc(sizeof(dma_addr_t *) * CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS);
    if (!iface->bufs) {
        goto error;
    }
    for (int i = 0; i < CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS; i++) {
        dma_bufs[i] = dma_alloc_pin(&iface->dma_man, CONFIG_LIB_ETHDRIVER_PREALLOCATED_BUF_SIZE, 1,
                                    iface->driver.dma_alignment);
        if (!dma_bufs[i].phys) {
            goto error;
        }
        ps_dma_cache_clean_invalidate(&iface->dma_man, dma_bufs[i].virt, CONFIG_LIB_ETHDRIVER_PREALLOCATED_BUF_SIZE);
        iface->bufs[i] = &dma_bufs[i];
    }
    iface->num_free_bufs = CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS;
    return;
error:
    if (iface->bufs) {
        free(iface->bufs);
    }
    if (dma_bufs) {
        for (int i = 0; i < CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS; i++) {
            if (dma_bufs[i].virt) {
                dma_unpin_free(&iface->dma_man, dma_bufs[i].virt, CONFIG_LIB_ETHDRIVER_PREALLOCATED_BUF_SIZE);
            }
        }
        free(dma_bufs);
    }
    iface->bufs = NULL;
}

static uintptr_t lwip_allocate_rx_buf(void *iface, size_t buf_size, void **cookie)
{   
    ZF_LOGW("lwip.c in libethdrivers/src/plat called lwip_allocate_rx_buf()\n");
    
    lwip_iface_t *lwip_iface = (lwip_iface_t *)iface;
    if (buf_size > CONFIG_LIB_ETHDRIVER_PREALLOCATED_BUF_SIZE) {
        LOG_ERROR("Requested RX buffer of size %zu which can never be fullfilled by preallocated buffers of size %d", buf_size,
                  CONFIG_LIB_ETHDRIVER_PREALLOCATED_BUF_SIZE);
        return 0;
    }
    if (lwip_iface->num_free_bufs == 0) {
        if (!lwip_iface->bufs) {
            initialize_free_bufs(lwip_iface);
            if (!lwip_iface->bufs) {
                LOG_ERROR("Failed lazy initialization of preallocated free buffers");
                return 0;
            }
        } else {
            return 0;
        }
    }
    lwip_iface->num_free_bufs--;
    dma_addr_t *buf = lwip_iface->bufs[lwip_iface->num_free_bufs];
    ps_dma_cache_invalidate(&lwip_iface->dma_man, buf->virt, buf_size);
    *cookie = (void *)buf;
    return buf->phys;
}

static void lwip_tx_complete(void *iface, void *cookie)
{
    lwip_iface_t *lwip_iface = (lwip_iface_t *)iface;
    lwip_iface->bufs[lwip_iface->num_free_bufs] = cookie;
    lwip_iface->num_free_bufs++;
}

static void lwip_rx_complete(void *iface, unsigned int num_bufs, void **cookies, unsigned int *lens)
{
    struct pbuf *p;
    int len;
    lwip_iface_t *lwip_iface = (lwip_iface_t *)iface;
    int i;
    len = 0;
    for (i = 0; i < num_bufs; i++) {
        ps_dma_cache_invalidate(&lwip_iface->dma_man, ((dma_addr_t *)cookies[i])->virt, lens[i]);
        len += lens[i];
    }
#if ETH_PAD_SIZE
    len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif
    /* Get a buffer from the pool */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p == NULL) {
        for (i = 0; i < num_bufs; i++) {
            lwip_tx_complete(iface, cookies[i]);
        }
        return;
    }

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
    len -= ETH_PAD_SIZE;
#endif

    /* fill the pbuf chain */
    struct pbuf *q = p;
    unsigned int copied = 0;
    unsigned int buf = 0;
    unsigned int buf_done = 0;
    unsigned int pbuf_done = 0;
    while (copied < len) {
        unsigned int next = MIN(q->len - pbuf_done, lens[buf] - buf_done);
        memcpy(q->payload + pbuf_done, ((dma_addr_t *)cookies[buf])->virt + buf_done, next);
        buf_done += next;
        pbuf_done += next;
        copied += next;
        if (buf_done == lens[buf]) {
            buf++;
            buf_done = 0;
        }
        if (pbuf_done == q->len) {
            q = q->next;
            pbuf_done = 0;
        }
    }

//    PKT_DEBUG(printf("Receiving packet\n"));
//    PKT_DEBUG(print_packet(COL_RX, p->payload, len));

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif
    LINK_STATS_INC(link.recv);

    for (i = 0; i < num_bufs; i++) {
        lwip_tx_complete(iface, cookies[i]);
    }

    struct eth_hdr *ethhdr;
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
        if (lwip_iface->netif->input(p, lwip_iface->netif) != ERR_OK) {
            LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
            pbuf_free(p);
            p = NULL;
        }
        break;

    default:
        pbuf_free(p);
        break;
    }
}

static err_t ethif_link_output(struct netif *netif, struct pbuf *p)
{
    lwip_iface_t *iface = (lwip_iface_t *)netif->state;
    dma_addr_t buf;
    struct pbuf *q;
    int status;

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

    if (p->tot_len > CONFIG_LIB_ETHDRIVER_PREALLOCATED_BUF_SIZE) {
        return ERR_MEM;
    }
    if (iface->num_free_bufs == 0) {
        return ERR_MEM;
    }
    iface->num_free_bufs--;
    dma_addr_t *orig_buf = iface->bufs[iface->num_free_bufs];
    buf = *orig_buf;

    char *pkt_pos = (char *)buf.virt;
    for (q = p; q != NULL; q = q->next) {
        memcpy(pkt_pos, q->payload, q->len);
        pkt_pos += q->len;
    }
    ps_dma_cache_clean(&iface->dma_man, buf.virt, p->tot_len);
//    PKT_DEBUG(cprintf(COL_TX, "Sending packet"));
//    PKT_DEBUG(print_packet(COL_TX, (void*)buf.virt, p->tot_len));

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    unsigned int length = p->tot_len;
    status = iface->driver.i_fn.raw_tx(&iface->driver, 1, &buf.phys, &length, orig_buf);
    switch (status) {
    case ETHIF_TX_FAILED:
        lwip_tx_complete(iface, orig_buf);
        return ERR_WOULDBLOCK;
    case ETHIF_TX_COMPLETE:
        lwip_tx_complete(iface, orig_buf);
    case ETHIF_TX_ENQUEUED:
        break;
    }

    LINK_STATS_INC(link.xmit);

    return ERR_OK;
}

static uintptr_t lwip_pbuf_allocate_rx_buf(void *iface, size_t buf_size, void **cookie)
{
    lwip_iface_t *lwip_iface = (lwip_iface_t *)iface;
#if ETH_PAD_SIZE
    buf_size += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif
    /* add space for alignment */
    buf_size += lwip_iface->driver.dma_alignment;
    struct pbuf *p = pbuf_alloc(PBUF_RAW, buf_size, PBUF_RAM);
    if (!p) {
        return 0;
    }
    /* we cannot support chained pbufs when doing this */
    if (p->next) {
        pbuf_free(p);
        return 0;
    }
#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif
    uintptr_t new_payload = (uintptr_t)p->payload;
    /* round up to dma_alignment */
    new_payload = ROUND_UP(new_payload, lwip_iface->driver.dma_alignment);
    pbuf_header(p, -(new_payload - (uintptr_t)p->payload));
    uintptr_t phys = ps_dma_pin(&lwip_iface->dma_man, p->payload, buf_size);
    if (!phys) {
        pbuf_free(p);
        return 0;
    }
    ps_dma_cache_invalidate(&lwip_iface->dma_man, p->payload, buf_size);
    *cookie = p;
    return phys;
}

static void lwip_pbuf_tx_complete(void *iface, void *cookie)
{
    lwip_iface_t *lwip_iface = (lwip_iface_t *)iface;
    struct pbuf *p = (struct pbuf *)cookie;
    for (; p; p = p->next) {
        uintptr_t loc = (uintptr_t)p->payload;
        uintptr_t end = (uintptr_t)p->payload + p->len;
        while (loc < end) {
            uintptr_t next = ROUND_UP(loc + 1, PAGE_SIZE_4K);
            if (next > end) {
                next = end;
            }
            ps_dma_unpin(&lwip_iface->dma_man, (void *)loc, next - loc);
            loc = next;
        }
    }
    pbuf_free(cookie);
}

static void lwip_pbuf_rx_complete(void *iface, unsigned int num_bufs, void **cookies, unsigned int *lens)
{
    struct pbuf *p = NULL;
    int i;
    lwip_iface_t *lwip_iface = (lwip_iface_t *)iface;

    assert(num_bufs > 0);
    /* staple all the bufs together, do it in reverse order for efficiency
     * of traversing pbuf chains */
    for (i = num_bufs - 1; i >= 0; i--) {
        struct pbuf *q = (struct pbuf *)cookies[i];
        ps_dma_cache_invalidate(&lwip_iface->dma_man, q->payload, lens[i]);
        pbuf_realloc(q, lens[i]);
        if (p) {
            pbuf_cat(q, p);
        }
        p = q;
    }

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.recv);

    struct eth_hdr *ethhdr;
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
        if (lwip_iface->netif->input(p, lwip_iface->netif) != ERR_OK) {
            LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
            LOG_INFO("failed to input\n");
            pbuf_free(p);
        }
        break;

    default:
        pbuf_free(p);
        break;
    }
}

static err_t ethif_pbuf_link_output(struct netif *netif, struct pbuf *p)
{
    lwip_iface_t *iface = (lwip_iface_t *)netif->state;
    struct pbuf *q;
    int status;

    /* grab a reference to the pbuf */
    pbuf_ref(p);

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif
    int max_frames = 0;

    /* work out how many pieces this buffer could potentially take up */
    for (q = p; q; q = q->next) {
        uintptr_t base = PAGE_ALIGN_4K((uintptr_t)q->payload);
        uintptr_t top = PAGE_ALIGN_4K((uintptr_t)q->payload + q->len - 1);
        max_frames += ((top - base) / PAGE_SIZE_4K) + 1;
    }
    int num_frames = 0;
    unsigned int lengths[max_frames];
    uintptr_t phys[max_frames];
    for (q = p; q; q = q->next) {
        uintptr_t loc = (uintptr_t)q->payload;
        uintptr_t end = (uintptr_t)q->payload + q->len;
        while (loc < end) {
            uintptr_t next = ROUND_UP(loc + 1, PAGE_SIZE_4K);
            if (next > end) {
                next = end;
            }
            lengths[num_frames] = next - loc;
            phys[num_frames] = ps_dma_pin(&iface->dma_man, (void *)loc, lengths[num_frames]);
            ps_dma_cache_clean(&iface->dma_man, (void *)loc, lengths[num_frames]);
            assert(phys[num_frames]);
            num_frames++;
            loc = next;
        }
    }

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    status = iface->driver.i_fn.raw_tx(&iface->driver, num_frames, phys, lengths, p);
    switch (status) {
    case ETHIF_TX_FAILED:
        lwip_pbuf_tx_complete(iface, p);
        return ERR_WOULDBLOCK;
    case ETHIF_TX_COMPLETE:
        lwip_pbuf_tx_complete(iface, p);
    case ETHIF_TX_ENQUEUED:
        break;
    }

    LINK_STATS_INC(link.xmit);

    return ERR_OK;
}

static struct raw_iface_callbacks lwip_prealloc_callbacks = {
    .tx_complete = lwip_tx_complete,
    .rx_complete = lwip_rx_complete,
    .allocate_rx_buf = lwip_allocate_rx_buf
};

static struct raw_iface_callbacks lwip_pbuf_callbacks = {
    .tx_complete = lwip_pbuf_tx_complete,
    .rx_complete = lwip_pbuf_rx_complete,
    .allocate_rx_buf = lwip_pbuf_allocate_rx_buf
};

static err_t ethif_init(struct netif *netif)
{
    if (netif -> state == NULL) {
        return ERR_ARG;
    }

    lwip_iface_t *iface = (lwip_iface_t *)netif->state;
    int mtu;
    iface->driver.i_fn.low_level_init(&iface->driver, netif->hwaddr, &mtu);
    netif->mtu = mtu;

    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    netif->output = etharp_output;
    if (iface->bufs == NULL) {
        netif->linkoutput = ethif_pbuf_link_output;
    } else {
        netif->linkoutput = ethif_link_output;
    }

    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd,
                    LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

    netif -> flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP |
                     NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP;

    iface->netif = netif;
    return ERR_OK;
}

lwip_iface_t *ethif_new_lwip_driver_no_malloc(ps_io_ops_t io_ops, ps_dma_man_t *pbuf_dma, ethif_driver_init driver,
                                              void *driver_config, lwip_iface_t *iface)
{
    memset(iface, 0, sizeof(*iface));
    iface->driver.cb_cookie = iface;
    if (pbuf_dma) {
        iface->driver.i_cb = lwip_pbuf_callbacks;
        iface->dma_man = *pbuf_dma;
    } else {
        iface->driver.i_cb = lwip_prealloc_callbacks;
        iface->dma_man = io_ops.dma_manager;
    }
    int err;
    err = driver(&iface->driver, io_ops, driver_config);
    if (err) {
        goto error;
    }
    /* if the driver did not already cause it to happen, allocate the preallocated buffers */
    if (!pbuf_dma && !iface->bufs) {
        initialize_free_bufs(iface);
        if (iface->bufs == NULL) {
            LOG_ERROR("Fault preallocating bufs");
            goto error;
        }
    }
    iface->ethif_init = ethif_init;
    return iface;
error:
    return NULL;
}

lwip_iface_t *ethif_new_lwip_driver(ps_io_ops_t io_ops, ps_dma_man_t *pbuf_dma, ethif_driver_init driver,
                                    void *driver_config)
{
    lwip_iface_t *ret;
    lwip_iface_t *iface = malloc(sizeof(*iface));
    if (!iface) {
        LOG_ERROR("Failed to malloc");
        return NULL;
    }
    ret = ethif_new_lwip_driver_no_malloc(io_ops, pbuf_dma, driver, driver_config, iface);
    if (!ret) {
        free(iface);
    }
    return ret;
}

#endif /* CONFIG_LIB_LWIP */
