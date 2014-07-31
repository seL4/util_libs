/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef __DESC_RAW_H
#define __DESC_RAW_H

#include "descriptors.h"

/**
 * Allocate a DMA tx descriptor ring.
 * @param[in] dma_man   DMA manager to use to allocate dma.
 * @param[in] count     Number of tx descriptors in the ring.
 * @return The dma_addr_t of the created ring.
 */ 
typedef dma_addr_t (*create_tx_descs_t)(ps_dma_man_t *dma_man, int count);

/**
 * Allocate a DMA rx descriptor ring.
 * @param[in] dma_man   DMA manager to use to allocate dma.
 * @param[in] count     Number of tx descriptors in the ring. 
 * @return The location of the created ring.
 */
typedef dma_addr_t (*create_rx_descs_t)(ps_dma_man_t *dma_man, int count);

/**
 * Initalises rx descritor ring. Could include descriptors and device registers.
 * @param[in] driver    Ethernet driver to get the ring from.
 */
typedef void (*reset_rx_descs_t)(struct eth_driver *driver);

/**
 * Initalises tx descritor ring. Could include descriptors and device registers.
 * @param[in] driver    Ethernet driver to get the ring from.
 */
typedef void (*reset_tx_descs_t)(struct eth_driver *driver);

/**
 * Returns whether the specified tx desciptor is waiting for hardware to service it.
 * @param[in] buf_num   Which number desciptor to check.
 * @param[in] desc      The structure to find rx ring in.
 * @return 0 if descriptor is not ready to be used by hardware, non zero if it is.
 */ 
typedef int (*is_tx_desc_ready_t)(int buf_num, struct desc *desc);

/**
 * Returns if the rx descriptor is empty (not ready to recieve a packet from hardware).
 * @param[in] buf_num   Which number desciptor to check.
 * @param[in] desc      The structure to find rx ring in.
 * @return 0 if rx is not empty, non zero otherwise.
 */
typedef int (*is_rx_desc_empty_t)(int buf_num, struct desc *desc);

/**
 * Marks the specified desciptors as ready for the hardware to use.
 * @param[in] buf_num           Which number desciptor to start from.
 * @param[in] num               How many descriptors to mark
 * @driver ethernet driver this relates to.
 */
typedef void (*ready_tx_desc_t)(int buf_num, int num, struct eth_driver *driver);

/**
 * Marks the specified desciptor as ready for the hardware to use.
 * @param[in] buf_num       Which number desciptor to use.
 * @param[in] rx_desc_wrap  Flag to mark this desciptor as the wrap point for the ring 
 *                          TODO:this flag could & probably should be done by hardware
 * @param[in] driver        Ethernet driver this relates to.
 */
typedef void (*ready_rx_desc_t)(int buf_num, int rx_desc_wrap, struct eth_driver *driver);

/**
 * Assign a buffer to a tx descriptor.
 * @param[in] buf_num   Which number descriptor to use.
 * @param[in[ buf       The buffer to be assigned to the desciptor.
 * @param[in] len       The length of the buffer.
 * @param[in] tx_desc_wrap      Flag to mark this desciptor as the wrap point for the ring 
 *                              TODO:this flag could & probably should be done by hardware
 * @param[in] tx_last_section   Flag to mark if this is the last descriptor in a packet
 * @param[in] desc      The structure to find the tx ring in.
 */
typedef void (*set_tx_desc_buf_t)(int buf_num, dma_addr_t buf, int len,
                                  int tx_desc_wrap, int tx_last_section, struct desc *desc);

/**
 * Assign a buffer to a rx descriptor.
 * @param[in] buf_num   Which number descriptor to use.
 * @param[in] buf       The buffer to be assigned to the desciptor.
 * @param[in] len       The length of the buffer.
 * @param[in] desc      The structure to find the rx ring in.
 */
typedef void (*set_rx_desc_buf_t)(int buf_num, dma_addr_t buf, int len, struct desc *desc);

/**
 * Gets the length of the buffer the specified rx desc is pointing to.
 * @param[in] buf_num   Which number descriptor to use.
 * @param[in] desc      The struture to find the rx ring in.
 * @return              The len of the rx descriptor's buffer.
 */
typedef int (*get_rx_buf_len_t)(int buf_num, struct desc *desc);

/**
 * Checks if the specified descriptor contains any errors.
 * @param[in] buf_num   Which number descriptor to use.
 * @param[in] desc      The struture to find the rx ring in.
 * @return              Non zero if there are any errors in the specified rx descriptor.
 */
typedef int (*get_rx_desc_error_t)(int buf_num, struct desc *desc);

struct raw_desc_funcs {
    create_tx_descs_t create_tx_descs;
    create_rx_descs_t create_rx_descs;

    reset_tx_descs_t reset_tx_descs;
    reset_rx_descs_t reset_rx_descs;

    ready_tx_desc_t ready_tx_desc;
    ready_rx_desc_t ready_rx_desc;

    is_tx_desc_ready_t is_tx_desc_ready;
    is_rx_desc_empty_t is_rx_desc_empty;

    set_tx_desc_buf_t set_tx_desc_buf;
    set_rx_desc_buf_t set_rx_desc_buf;

    get_rx_buf_len_t get_rx_buf_len;
    get_rx_desc_error_t get_rx_desc_error;
};

#endif /* __DESC_RAW_H */
