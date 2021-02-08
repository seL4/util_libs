/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/io.h>
#include <platsupport/gpio.h>

typedef struct gpio_chain gpio_chain_t;

/**
 * Initialise a GPIO chain structure.
 *
 * @param malloc_ops    Initialised malloc_ops structure.
 * @param ret_chain     Pointer to a (gpio_chain_t *) that will be filled in.
 * @return              0 on success, otherwise an error code
 */
int gpio_chain_init(ps_malloc_ops_t *malloc_ops, gpio_chain_t **ret_chain);

/**
 * Destroy a initialised GPIO chain.
 *
 * @param malloc_ops    Initialised malloc_ops structure.
 * @param chain         Initialised GPIO chain that will be destroyed.
 * @return              0 on success, otherwise an error code
 */
int gpio_chain_destroy(ps_malloc_ops_t *malloc_ops, gpio_chain_t *chain);

/**
 * Add a GPIO pin to a GPIO chain. Note that the lifetime of the gpio_t
 * pointer passed in must be the same as the lifetime of the gpio chain that
 * holds the pointer.
 *
 * @param chain     Initialised GPIO chain to have a GPIO pin added to it.
 * @param gpio      Initialised gpio_t structure which represents a chain.
 * @return          0 on success, otherwise an error code
 */
int gpio_chain_add(gpio_chain_t *chain, gpio_t *gpio);

/**
 * Remove a GPIO pin from a GPIO chain.
 *
 * @param chain     Initialised GPIO chain to have a GPIO removed from it.
 * @param gpio      The target GPIO pin to remove from the chain.
 * @return          0 on success, otherwise an error code
 */
int gpio_chain_remove(gpio_chain_t *chain, gpio_t *gpio);

/**
 * Read from a daisy chain of GPIO pins.
 *
 * All pins in the chain must be configured for input.  The buffer supplied for
 * output will only be touched as far as "len" bits.
 *
 * @param chain     An initialised GPIO chain.
 * @param data      A buffer in which the driver should place the data it reads.
 * @param len       The number of chained GPIO pins to read from.
 * @return          The number of bits of data read. Or an error code
 *                  on error.
 */
int gpio_chain_read(gpio_chain_t *chain, char *data, int len);

/**
 * Write to a daisy chain of GPIO pins.
 *
 * All pins in the chian must be configured for output. The "len" argument will
 * determine how many chained pins the driver will attempt to write to.
 *
 * @param chain     An initialized GPIO chain.
 * @param data      A buffer from which the driver will draw the data to be
 *                  written out.
 * @param len       The number of bits to write out from the buffer to chained
 *                  GPIO pins.
 * @return          The number of bits of data written out. Or an error code
 */
int gpio_chain_write(gpio_chain_t *chain, char *data, int len);
