/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <autoconf.h>

#ifdef CONFIG_LIB_PICOTCP

#include <ethdrivers/pico_dev_eth.h>
#include <ethdrivers/helpers.h>
#include <string.h>
#include "debug.h"

#include <pico_stack.h>
#include <pico_device.h>

/* Toggle for async driver, which will use eth_dsr instead of eth_poll */
#define ASYNC_DRIVER 1

static int alloc_buf_pool(pico_device_eth *pico_iface){
    /* Take the next free buffer */
    if (pico_iface->next_free_buf == -1){
        LOG_ERROR("Out of preallocated eth buffers.");
        return -1;
    }
    
    int retval = pico_iface->next_free_buf; 
    pico_iface->next_free_buf = pico_iface->buf_pool[retval];
    return retval;
    
}

static void free_buf_pool(pico_device_eth *pico_iface, int buf_no){
    /* Return back into the buffer pool */
    if (buf_no < 0 || buf_no > CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS){
        LOG_ERROR("Attempted to return a buffer outside of the pool %d.", buf_no);
        return;
    }
    pico_iface->buf_pool[pico_iface->next_free_buf] = pico_iface->next_free_buf;
    pico_iface->next_free_buf = buf_no;
}

static void initialize_free_bufs(pico_device_eth *pico_iface){
    dma_addr_t *dma_bufs = NULL;
    dma_bufs = malloc(sizeof(dma_addr_t) * CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS);
    if(!dma_bufs){
        goto error;
    }
    memset(dma_bufs, 0, sizeof(dma_addr_t) * CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS);
    
    pico_iface->bufs = malloc(sizeof(dma_addr_t*) * CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS);
    if(!pico_iface->bufs){
        goto error;
    }

    /* Pin buffers */
    for (int i=0; i<CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS; i++){
        dma_bufs[i] = dma_alloc_pin(&pico_iface->dma_man, 
			CONFIG_LIB_ETHDRIVER_PREALLOCATED_BUF_SIZE, 1,  pico_iface->driver.dma_alignment);
        if(!dma_bufs[i].phys){
            goto error;
        }
        ps_dma_cache_clean_invalidate(&pico_iface->dma_man, dma_bufs[i].virt, 
			CONFIG_LIB_ETHDRIVER_PREALLOCATED_BUF_SIZE);
        pico_iface->bufs[i] = &dma_bufs[i]; 
    }
    
    pico_iface->num_free_bufs = CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS;
    
    /* Setup the buffer pool management */
    pico_iface->buf_pool = malloc(sizeof(int) * CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS);
    if (!pico_iface->buf_pool){
        goto error;
    }
    
    for (int i=0; i<CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS; i++){
        pico_iface->buf_pool[i] = i-1;
    }
    pico_iface->next_free_buf = CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS - 1;

    /* Rx queue */
    pico_iface->rx_count = 0;
    pico_iface->rx_lens = malloc(sizeof(int) * CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS);
    if(!pico_iface->rx_lens){
        goto error;
    }
    memset(pico_iface->rx_lens, 0, sizeof(int) * CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS);
    
    pico_iface->rx_queue = malloc(sizeof(int) * CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS);
    if(!pico_iface->rx_queue){
        goto error;
    }
    memset(pico_iface->rx_queue, 0, sizeof(int) * CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS);

    return;

error:
    if (pico_iface->bufs) {
        free(pico_iface->bufs);
    }
    if (dma_bufs) {
        for (int i = 0; i < CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS; i++) {
            if (dma_bufs[i].virt) {
                dma_unpin_free(&pico_iface->dma_man, dma_bufs[i].virt, 
					CONFIG_LIB_ETHDRIVER_PREALLOCATED_BUF_SIZE);
            }
        }
        free(dma_bufs);
    }

    if (pico_iface->buf_pool) {
        free(pico_iface->buf_pool);
    }
    pico_iface->next_free_buf = -1; 

    if (pico_iface->rx_lens) {
        free(pico_iface->rx_lens);
    }

    pico_iface->bufs = NULL;

}

static uintptr_t pico_allocate_rx_buf(void *iface, size_t buf_size, void **cookie){
    pico_device_eth *pico_iface = (pico_device_eth*)iface;

    if (buf_size > CONFIG_LIB_ETHDRIVER_PREALLOCATED_BUF_SIZE) {
        LOG_ERROR("Requested RX buffer of size %d which can never be fullfilled by preallocated buffers of size %d", 
                buf_size, CONFIG_LIB_ETHDRIVER_PREALLOCATED_BUF_SIZE);
        return 0;
    }

    if (!pico_iface->bufs) {
        initialize_free_bufs(pico_iface);
        if (!pico_iface->bufs) {
            LOG_ERROR("Failed lazy initialization of preallocated free buffers");
            return 0;
        }
    } else {
        return 0;
    }

    int buf_no = alloc_buf_pool(pico_iface);
    if (buf_no < 0){
        /* No buffers available */
        return 0;
    }

    dma_addr_t *buf = pico_iface->bufs[buf_no];
    ps_dma_cache_invalidate(&pico_iface->dma_man, buf->virt, buf_size);
    *cookie = buf_no;
    return buf->phys;
}

static void pico_tx_complete(void *iface, void *cookie) {
    pico_device_eth *pico_iface = (pico_device_eth*)iface;
    free_buf_pool(iface, (int)cookie);
}

static void pico_rx_complete(void *iface, unsigned int num_bufs, void **cookies, unsigned int *lens) {
    /* A buffer has been filled. Put it into the receive queue to be collected. */
    pico_device_eth *pico_iface = (pico_device_eth*)iface;
    
    if (num_bufs > 1){
        LOG_ERROR("RX buffer of size is smaller than MTU. Frame splitting unhandled.\n");
        /* Frame splitting is not handled. Warn and return bufs to pool. */
        for (int i=0; i<num_bufs; i++){
            free_buf_pool(pico_iface, cookies[i]);
        } 
    } else {
        int buf_no = cookies[0]; 
        /* Store the information about the rx bufs */
        pico_iface->rx_queue[pico_iface->rx_count] = buf_no;
        pico_iface->rx_lens[buf_no] = lens[0];
        pico_iface->rx_count += 1;
            
        if(ASYNC_DRIVER) {
        	pico_iface->pico_dev.__serving_interrupt = 1;
        }
    }
    //printf("RX complete, %d in queue!\n", ((pico_device_eth*)iface)->rx_count);
    
    return;
}

/* Pico TCP implementation */

static int pico_eth_send(struct pico_device *dev, void *input_buf, int len){

    dma_addr_t buf;
    int status;
    struct pico_device_eth *eth_device = (struct pico_device_eth *)dev;
    
    if (len > CONFIG_LIB_ETHDRIVER_PREALLOCATED_BUF_SIZE){
        return 0;
    }
    
    int buf_no = alloc_buf_pool(eth_device);
    if (buf_no < 0) {
        return 0;
    }
    
    dma_addr_t *orig_buf = eth_device->bufs[buf_no];
    buf = *orig_buf;
    memcpy(buf.virt, input_buf, len);
    ps_dma_cache_clean(&eth_device->dma_man, buf.virt, len);
    
    unsigned int length = len;
    status = eth_device->driver.i_fn.raw_tx(&eth_device->driver, 1, &buf.phys, &length, buf_no);

    switch(status) {
    case ETHIF_TX_FAILED:
        pico_tx_complete(dev, buf_no);
        LOG_ERROR("Failed tx\n");
        return 0; // Error for PICO
    case ETHIF_TX_COMPLETE:
        pico_tx_complete(dev, buf_no);
    case ETHIF_TX_ENQUEUED:
        break;
    }

    return length;

}

/* Async driver */
static int pico_eth_poll(struct pico_device *dev, int loop_score){
    struct pico_device_eth *eth_device = (struct pico_device_eth *)dev;
    while (loop_score > 0){
        if (eth_device->rx_count == 0){
            if (ASYNC_DRIVER){
                /* Also clear the serving_interrupt flag for async driver*/
                eth_device->pico_dev.__serving_interrupt = 0;
            }
            break;
        }

        /* Retrieve the data from the rx buffer */
        eth_device->rx_count -= 1;    
        int buf_no = eth_device->rx_queue[eth_device->rx_count];
        dma_addr_t *buf = eth_device->bufs[buf_no];

        int len = eth_device->rx_lens[buf_no];
        ps_dma_cache_invalidate(&eth_device->dma_man, buf->virt, len);
        pico_stack_recv(dev, buf->virt, len);
        
        free_buf_pool(eth_device, buf_no);
        loop_score--;
    }

    return loop_score;
}

static struct raw_iface_callbacks pico_prealloc_callbacks = {
    .tx_complete = pico_tx_complete,
    .rx_complete = pico_rx_complete,
    .allocate_rx_buf = pico_allocate_rx_buf
};

struct pico_device *pico_eth_create(char *name, 
    ethif_driver_init driver_init, void *driver_config, ps_io_ops_t io_ops){

    /* Create the pico device struct */
    struct pico_device_eth *eth_dev = malloc(sizeof(struct pico_device_eth));
    if (!eth_dev){
        LOG_ERROR("Failed to malloc pico eth device interface");
        return NULL;
    }

    memset(eth_dev, 0, sizeof(struct pico_device_eth));

    /* Set the dma manager up */
    eth_dev->driver.i_cb = pico_prealloc_callbacks;
    eth_dev->dma_man = io_ops.dma_manager;

    eth_dev->driver.cb_cookie = eth_dev;
    /* Initialize hardware */
    int err;
    err = driver_init(&(eth_dev->driver), io_ops, driver_config);
    if (err){ 
        return NULL;
    }
    
    /* Initialise buffers in case driver did not do so */
    if (!eth_dev->bufs){
        initialize_free_bufs(eth_dev);
    }

    /* Attach funciton pointers */
    eth_dev->pico_dev.send = pico_eth_send;
    if (ASYNC_DRIVER){
        /* Although the same function, .poll needs to be set to NULL for an async driver */
        eth_dev->pico_dev.poll = NULL;
        eth_dev->pico_dev.dsr = pico_eth_poll;
    } else {
        eth_dev->pico_dev.poll = pico_eth_poll;
    }
    
    /* Also do some low level init */
    int mtu;
    uint8_t mac[6] = {};

    eth_dev->driver.i_fn.low_level_init(&eth_dev->driver, mac, &mtu);

    /* Configure the mtu in picotcp */
    eth_dev->pico_dev.mtu = mtu;

    /* Register in picoTCP (equivalent to netif init in lwip)*/
    if (pico_device_init(&(eth_dev->pico_dev), name, mac) != 0){
        LOG_ERROR("Failed to initialize pico device");
        free(eth_dev);
        return NULL;
    }

    return (struct pico_device *)eth_dev;
}

#endif /* CONFIG_LIB_PICOTCP */

