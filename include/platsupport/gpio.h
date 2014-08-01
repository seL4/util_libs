/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _PLATSUPPORT_GPIO_H_
#define _PLATSUPPORT_GPIO_H_

struct gpio_sys;
typedef struct gpio_sys gpio_sys_t;

#include <platsupport/plat/gpio.h>

#define GPIOID(port, pin)             ((port) * 32 + (pin))
#define GPIOID_PORT(gpio)             ((gpio) / 32)
#define GPIOID_PIN(gpio)              ((gpio) % 32)


typedef struct gpio {
/// GPIO port identifier
    int id;
/// GPIO subsystem handle
    gpio_sys_t* gpio_sys;
/// Chain GPIO's to enable bulk reads/writes
    struct gpio* next;
} gpio_t;


enum gpio_dir {
/// Output direction
   GPIO_DIR_OUT,
/// Input direction
   GPIO_DIR_IN
};

struct gpio_sys{
/// Initialise a GPIO pin
    int (*init)(gpio_sys_t* gpio_sys, int id, enum gpio_dir dir, gpio_t* gpio);
/// Configure a GPIO pin
    int (*config)(gpio_t* gpio, int param_list);
/// Write to a GPIO
    int (*write)(gpio_t* gpio, const char* data, int len);
/// Read from a GPIO
    int (*read)(gpio_t* gpio, char* data, int len);
/// platform specific private data
    void* priv;
};

/**
 * Initialise the GPIO subsystem and provide a handle for access
 * @param[in]  io_ops   io operations for device initialisation
 * @param[out] gpio_sys A gpio handle structure to initialise
 * @return              0 on success
 */
int gpio_sys_init(ps_io_ops_t* io_ops, gpio_sys_t* gpio_sys);

/**
 * Acquire a handle to a GPIO pin
 * @param[in]  gpio_sys  a handle to an initialised GPIO subsystem\
 * @param[in]  id        A pin identifier obtained from the macro
 *                       GPIOID(port, pin)
 * @param[in]  dir       The direction of the pin
 * @param[out] gpio      a GPIO handle to initialise 
 * @return               0 on success
 */
int gpio_new(gpio_sys_t* gpio_sys, int id, enum gpio_dir dir, gpio_t* gpio);

/**
 * Set a GPIO pin
 * @param[in] a handle to a GPIO
 * @return    0 on success
 */
int gpio_set(gpio_t* gpio);

/**
 * Clear a GPIO pin
 * @param[in] a handle to a GPIO
 * @return    0 on success
 */
int gpio_clr(gpio_t* gpio);

/**
 * Return the state of a GPIO pin
 * @param[in] a handle to a GPIO
 * @return    the value of the pin, -1 on failure
 */
int gpio_get(gpio_t* gpio);

#endif /* _PLATSUPPORT_GPIO_H_ */

