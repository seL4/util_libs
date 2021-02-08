/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <platsupport/io.h>
#include <utils/util.h>

#include "uboot/tx2_configs.h"

#define TX_IRQ BIT(0)
#define RX_IRQ BIT(1)

void eqos_dma_disable_rxirq(struct tx2_eth_data *dev);

void eqos_dma_enable_rxirq(struct tx2_eth_data *dev);

void eqos_dma_disable_txirq(struct tx2_eth_data *dev);

void eqos_dma_enable_txirq(struct tx2_eth_data *dev);

void eqos_stop(struct tx2_eth_data *dev);

int eqos_start(struct tx2_eth_data *dev);

int eqos_send(struct tx2_eth_data *dev, void *packet, int length);

int eqos_handle_irq(struct tx2_eth_data *dev, int irq);

int eqos_recv(struct tx2_eth_data *dev, uintptr_t packetp);

void *tx2_initialise(uintptr_t base_addr, ps_io_ops_t *io_ops);

void eqos_set_rx_tail_pointer(struct tx2_eth_data *dev);
