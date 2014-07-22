/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <lwip/netif.h>
#include <stdint.h>

struct eth_driver;

/**
 * Recieves a packe then handles it.
 * 
 * @param netif the lwip network interface structure for this ethernetif
 * @return 0 if no more packets, 1 otherwise.
 */
int 
ethif_input(struct netif *netif);

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
ethif_init(struct netif *netif);

/*
 * Handle an IRQ for this network device. This function will call 
 * ethif_input if appropriate and returns when no interrupts are pending.
 *
 * @param netif the lwip network interface structure for this ethernet
 *        interface.
 * @param irq the irq number that was triggered.
 */
void
ethif_handleIRQ(struct netif* netif, int irq);

/*
 * Enables irqs but does not provide a handling thread. Returns an array of
 * IRQs managed by this driver.
 * @param[in]  eth_driver  The ethernet driver for which IRQs should be enabled.
 * @param[out]      nirqs  On return, "nirqs" will be filled with the number
 *                         of IRQs managed by this device.
 * @return                 An array of "nirqs" elements that each represent an
 *                         IRQ number.
 */
const int* 
ethif_enableIRQ(struct eth_driver* driver, int* nirqs);


