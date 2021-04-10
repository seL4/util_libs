/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2020, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <stdint.h>
#include <platsupport/io.h>

#define NETIRQ_BABR     (1UL << 30) /* Babbling Receive Error          */
#define NETIRQ_BABT     (1UL << 29) /* Babbling Transmit Error         */
#define NETIRQ_GRA      (1UL << 28) /* Graceful Stop Complete          */
#define NETIRQ_TXF      (1UL << 27) /* Transmit Frame Interrupt        */
#define NETIRQ_TXB      (1UL << 26) /* Transmit Buffer Interrupt       */
#define NETIRQ_RXF      (1UL << 25) /* Receive Frame Interrupt         */
#define NETIRQ_RXB      (1UL << 24) /* Receive Buffer Interrupt        */
#define NETIRQ_MII      (1UL << 23) /* MII Interrupt                   */
#define NETIRQ_EBERR    (1UL << 22) /* Ethernet bus error              */
#define NETIRQ_LC       (1UL << 21) /* Late Collision                  */
#define NETIRQ_RL       (1UL << 20) /* Collision Retry Limit           */
#define NETIRQ_UN       (1UL << 19) /* Transmit FIFO Underrun          */
#define NETIRQ_PLR      (1UL << 18) /* Payload Receive Error           */
#define NETIRQ_WAKEUP   (1UL << 17) /* Node Wakeup Request Indication  */
#define NETIRQ_TS_AVAIL (1UL << 16) /* Transmit Timestamp Available    */
#define NETIRQ_TS_TIMER (1UL << 15) /* Timestamp Timer                 */

struct enet;

struct desc_data {
    uint32_t tx_phys;
    uint32_t rx_phys;
    uint32_t rx_bufsize;
};

struct enet *enet_init(struct desc_data desc_data, ps_io_ops_t *io_ops);

/* Debug */
void enet_dump_regs(struct enet *enet);
void enet_clear_mib(struct enet *enet);
void enet_print_mib(struct enet *enet);
void enet_print_state(struct enet *enet);

/* Read and write to the phy over the mdio interface */
int enet_mdio_read(struct enet *enet, uint16_t phy, uint16_t reg);
int enet_mdio_write(struct enet *enet, uint16_t phy, uint16_t reg, uint16_t data);

void enet_enable(struct enet *enet);
void enet_disable(struct enet *enet);
int enet_enabled(struct enet *enet);

void enet_set_mac(struct enet *enet, unsigned char *mac);
void enet_get_mac(struct enet *enet, unsigned char *mac);

void enet_set_speed(struct enet *enet, int speed, int full_duplex);

/* Clears interrupt events and returns the original value - before the clear */
uint32_t enet_clr_events(struct enet *enet, uint32_t clr_bits);
/* Sets the event mask */
void enet_enable_events(struct enet *enet, uint32_t mask_bits);
/* Returns the value of ievents */
uint32_t enet_get_events(struct enet *enet);

void enet_tx_enable(struct enet *enet);
int enet_tx_enabled(struct enet *enet);

void enet_rx_enable(struct enet *enet);
int enet_rx_enabled(struct enet *enet);

void enet_set_mdcclk(struct enet *enet, uint32_t fout);
uint32_t enet_get_mdcclk(struct enet *imx_eth);

void enet_prom_enable(struct enet  *enet);
void enet_prom_disable(struct enet *enet);
