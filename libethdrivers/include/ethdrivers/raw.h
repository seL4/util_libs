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

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <platsupport/io.h>

struct eth_driver;

#define ETHIF_TX_ENQUEUED 0
#define ETHIF_TX_FAILED -1
#define ETHIF_TX_COMPLETE 1

/**
 * Transmit a packet.
 *
 * @param driver    Pointer to ethernet driver
 * @param num       Number of memory regions making up the packet
 * @param phys      Array of length 'num' detailing physical addresses
 *                  of each memory region
 * @param len       Array of length 'num' detailing the length of each
 *                  memory region
 * @param cookie    Cookie to be passed to receive complete function
 *
 * @return          ETHIF_TX_ENQUEUED if buffer request is enqueued, ethif_raw_tx_complete
                    will be called when completed. ETHIF_TX_COMPLETE if transmit completed
                    inline and buffer can be freed. ETHIF_TX_FAILED if transmit could not
                    be done
 */
typedef int (*ethif_raw_tx)(struct eth_driver *driver, unsigned int num, uintptr_t *phys, unsigned int *len, void *cookie);

/**
 * Handle an IRQ event
 *
 * @param driver    Pointer to ethernet driver
 * @param irq       If driver has multiple IRQs this is a driver
 *                  specific encoding of which one
 */
typedef void (*ethif_raw_handleIRQ_t)(struct eth_driver *driver, int irq);

/**
 * Get low level device from the driver
 *
 * @param driver    Pointer to ethernet driver
 * @param mac       Pointer to 6 byte array to fill in HW mac address
 * @param mtu       Pointer to int that should be filled in with devices MTU
 */
typedef void (*ethif_low_level_init_t)(struct eth_driver *driver, uint8_t *mac, int *mtu);

/* Debug method for printing internal driver state */
typedef void (*ethif_print_state_t)(struct eth_driver* driver);

/**
 * Request the driver to poll for any changes. This can
 * be used if you want to run without interrupts
 *
 * @param driver    Pointer to ethernet driver
 */
typedef void (*ethif_raw_poll)(struct eth_driver *driver);

/**
 * Function called by the driver to allocate receive buffers.
 * Must respect the dma_alignment specified by the driver in
 * the eth_driver struct
 *
 * @param cb_cookie     Cookie given in eth_driver struct
 * @param buf_size      Size of buffer to allocate
 * @param cookie        Pointer to location to store a buffer
 *                      specific cookie that will be passed
 *                      to the ethif_raw_rx_complete function
 *
 * @return              Physical address of buffer, or 0 on error
 */
typedef uintptr_t (*ethif_raw_allocate_rx_buf)(void *cb_cookie, size_t buf_size, void **cookie);

/**
 * Function called by the driver upon successful RX
 *
 * @param cb_cookie     Cookie given in the eth_driver struct
 * @param num_bufs      Number of buffers that were used in the receive
 * @param cookies       Array of size 'num_bufs' containing all the
 *                      cookies as given by ethif_raw_allocate_rx_buf
 *                      This array will be freed upon completion of the callback
 * @param lens          Array of size 'num_bufs' containing how much
 *                      data was placed in each receive buffer
 *                      This array will be freed upon completion of the callback
 */
typedef void (*ethif_raw_rx_complete)(void *cb_cookie, unsigned int num_bufs, void **cookies, unsigned int *lens);

/**
 * Function called by the driver upon successful TX
 *
 * @param cb_cookie     Cookie given in eth_driver struct
 * @param cookie        Buffer specific cookie passed to ethif_raw_tx
 */
typedef void (*ethif_raw_tx_complete)(void *cb_cookie, void *cookie);

/**
 * Defining of generic function for initializing an ethernet
 * driver. Takes an allocated and partially filled out
 * eth_driver struct that it will finish filling out.
 *
 * @param driver        Partially filled out eth_driver struct. Expects
                        i_cb and cb_cookie to already be filled out
 * @param io_ops        Interface containing OS specific functions
 * @param config        Pointer to driver specific config struct. The
 *                      caller is responsible for freeing this
 *
 * @return              0 on success
 */
typedef int (*ethif_driver_init)(struct eth_driver *driver, ps_io_ops_t io_ops, void *config);

/**
 * Get the device MAC address
 * 
 * @param driver    Pointer to ethernet driver
 * @param mac       Pointer to 6 byte array to be filled in device MAC address
 */
typedef void (*ethif_get_mac)(struct eth_driver *driver, uint8_t *mac);

/* Structure defining the set of functions an ethernet driver
 * must implement and expose */
struct raw_iface_funcs {
    ethif_raw_tx raw_tx;
    ethif_raw_handleIRQ_t raw_handleIRQ;
    ethif_raw_poll        raw_poll;
    ethif_print_state_t print_state;
    ethif_low_level_init_t low_level_init;
    ethif_get_mac get_mac;
};

/* Structure defining the set of functions an ethernet driver
 * expects to be given to it for handling memory allocation
 * and receive/transmit completions */
struct raw_iface_callbacks {
    ethif_raw_tx_complete tx_complete;
    ethif_raw_rx_complete rx_complete;
    ethif_raw_allocate_rx_buf allocate_rx_buf;
};

/* Structure to hold the interface for an ethernet driver */
struct eth_driver {
    void* eth_data;
    struct raw_iface_funcs i_fn;
    struct raw_iface_callbacks i_cb;
    void *cb_cookie;
    ps_io_ops_t io_ops;
    int dma_alignment;
};

struct dma_buf_cookie {
    void* vbuf;
    void* pbuf;
};

/* Structure for i.MX6 and Zynq7000 Drivers
 * TODO: Move to ARCH Specific Folder
 */
struct arm_eth_plat_config {
    void* buffer_addr;
    uint8_t prom_mode;
};

