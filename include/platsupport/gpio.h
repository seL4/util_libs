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

#include <platsupport/io.h>
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
 * Clear a GPIO pin
 * @param[in] a handle to a GPIO
 * @return    0 on success
 */
static inline int gpio_clr(gpio_t* gpio){
    char data;
    assert(gpio);
    assert(gpio->gpio_sys);
    data = 0;
    return (gpio->gpio_sys->write(gpio, &data, 1) != 1);
}


/**
 * Return the state of a GPIO pin
 * @param[in] a handle to a GPIO
 * @return    the value of the pin, -1 on failure
 */
static inline int gpio_get(gpio_t* gpio){
    char data;
    int ret;
    assert(gpio);
    assert(gpio->gpio_sys);
    ret = gpio->gpio_sys->read(gpio, &data, 1);
    if(ret == 1){
        return data;
    }else{
        return -1;
    }
}


/**
 * Set a GPIO pin
 * @param[in] a handle to a GPIO
 * @return    0 on success
 */
static inline int gpio_set(gpio_t* gpio){
    char data;
    assert(gpio);
    assert(gpio->gpio_sys);
    data = 0xff;
    return (gpio->gpio_sys->write(gpio, &data, 1) != 1);
}

/**
 * Acquire a handle to a GPIO pin
 * @param[in]  gpio_sys  a handle to an initialised GPIO subsystem\
 * @param[in]  id        A pin identifier obtained from the macro
 *                       GPIOID(port, pin)
 * @param[in]  dir       The direction of the pin
 * @param[out] gpio      a GPIO handle to initialise 
 * @return               0 on success
 */

static inline int gpio_new(gpio_sys_t* gpio_sys, int id, enum gpio_dir dir, gpio_t* gpio) {
    assert(gpio);
    return gpio_sys->init(gpio_sys, id, dir, gpio);
}
#endif /* _PLATSUPPORT_GPIO_H_ */

