/*
 * Copyright 2019, DornerWorks
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <platsupport/io.h>
#include <satadrivers/virtio/virtio_blk.h>

struct disk_driver;

#define VIRTIO_BLK_XFER_FAILED   (-1)
#define VIRTIO_BLK_XFER_COMPLETE 1

/**
 * Transmit a packet.
 *
 * @param driver    Pointer to disk driver
 * @param num       Number of memory regions making up the packet
 * @param phys      Array of length 'num' detailing physical addresses
 *                  of each memory region
 * @param len       Array of length 'num' detailing the length of each
 *                  memory region
 * @param cookie    Cookie to be passed to receive complete function
 *
 * @return          DISKIF_TX_ENQUEUED if buffer request is enqueued, diskif_raw_tx_complete
 *                  will be called when completed. DISKIF_TX_COMPLETE if transmit completed
 *                  inline and buffer can be freed. DISKIF_TX_FAILED if transmit could not
 *                  be done
 */
typedef int (*diskif_raw_xfer)(struct disk_driver *driver, uint8_t direction, uint64_t sector, uint32_t len,
                               uintptr_t guest_buf_phys);

/**
 * Handle an IRQ event
 *
 * @param driver    Pointer to disk driver
 * @param irq       If driver has multiple IRQs this is a driver
 *                  specific encoding of which one
 */
typedef void (*diskif_raw_handleIRQ_t)(struct disk_driver *driver, int irq);

/**
 * Get low level device from the driver
 *
 * @param driver    Pointer to disk drivers
 */
typedef void (*diskif_low_level_init_t)(struct disk_driver *driver, struct virtio_blk_config *cfg);

/* Debug method for printing internal driver state */
typedef void (*diskif_print_state_t)(struct disk_driver *driver);

/**
 * Request the driver to poll for any changes. This can
 * be used if you want to run without interrupts
 *
 * @param driver    Pointer to disk driver
 */
typedef void (*diskif_raw_poll)(struct disk_driver *driver);

/**
 * Defining of generic function for initializing a disk
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
typedef int (*diskif_driver_init)(struct disk_driver *driver, ps_io_ops_t io_ops, void *config);

/* Structure defining the set of functions a disk driver
 * must implement and expose */
typedef struct raw_diskiface_funcs {
    diskif_raw_xfer         raw_xfer;
    diskif_raw_handleIRQ_t  raw_handleIRQ;
    diskif_raw_poll         raw_poll;
    diskif_print_state_t    print_state;
    diskif_low_level_init_t low_level_init;
} raw_diskiface_funcs_t;

/* Structure to hold the interface for a disk driver */
struct disk_driver {
    void *disk_data;
    raw_diskiface_funcs_t i_fn;
    void *cb_cookie;
    ps_io_ops_t io_ops;
    int dma_alignment;
};
