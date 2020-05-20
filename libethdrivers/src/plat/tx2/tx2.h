/*
 * Copyright 2020, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
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
