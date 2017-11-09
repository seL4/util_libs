/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#pragma once

/** Sets a pin to either GPIO or SFIO (special function I/O) mode.
 *
 * Normally this functionality is provided by a Mux driver and not by a GPIO
 * driver, but on the TK1, the GPIO controller actually controls this
 * functionality. So the Mux driver calls on this function.
 */
int
gpio_set_pad_mode(gpio_sys_t *gpio_sys,
                  enum gpio_pin gpio, enum gpio_pad_mode mode, enum gpio_dir dir);
