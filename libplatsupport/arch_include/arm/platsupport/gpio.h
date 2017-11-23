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

struct gpio_sys;
typedef struct gpio_sys gpio_sys_t;
typedef int gpio_id_t;

#include <stdbool.h>
#include <utils/util.h>
#include <platsupport/io.h>

#include <platsupport/plat/gpio.h>

#define GPIOID(port, pin)             ((port) * 32 + (pin))
#define GPIOID_PORT(gpio)             ((gpio) / 32)
#define GPIOID_PIN(gpio)              ((gpio) % 32)

typedef struct gpio {
/// GPIO port identifier
    gpio_id_t id;
/// GPIO subsystem handle
    gpio_sys_t* gpio_sys;
/// Chain GPIO's to enable bulk reads/writes
    struct gpio* next;
} gpio_t;

enum gpio_dir {
/// Output direction
    GPIO_DIR_OUT,
/// Input direction
    GPIO_DIR_IN,

/// Input direction with IRQ on low logic level
    GPIO_DIR_IRQ_LOW,
/// Input direction with IRQ on high logic level
    GPIO_DIR_IRQ_HIGH,
/// Input direction with IRQ on falling edge
    GPIO_DIR_IRQ_FALL,
/// Input direction with IRQ on rising edge
    GPIO_DIR_IRQ_RISE,
/// Input direction with IRQ on both rising and falling edges
    GPIO_DIR_IRQ_EDGE
};

struct gpio_sys {
/// Initialise a GPIO pin
    int (*init)(gpio_sys_t* gpio_sys, gpio_id_t id, enum gpio_dir dir, gpio_t* gpio);
/// Write to a GPIO
    int (*write)(gpio_t* gpio, const char* data, int len);
/// Read from a GPIO
    int (*read)(gpio_t* gpio, char* data, int len);
/// Manipulate the status of a pending IRQ
    int (*pending_status)(gpio_t *gpio, int clear);
/// Enable and disable the IRQ signal from the pin
    int (*irq_enable_disable)(gpio_t *gpio, bool enable);
/// platform specific private data
    void* priv;
};

static inline bool
gpio_sys_valid(const gpio_sys_t *gpio_sys)
{
    return gpio_sys != NULL && gpio_sys->priv != NULL;
}

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
static inline int gpio_clr(gpio_t* gpio)
{
    char data;
    ZF_LOGF_IF(!gpio, "Handle to GPIO pin not supplied!");
    ZF_LOGF_IF(!gpio->gpio_sys, "GPIO pin's parent controller handle invalid!");
    data = 0;
    return (gpio->gpio_sys->write(gpio, &data, 1) != 1);
}

/**
 * Return the state of a GPIO pin
 * @param[in] a handle to a GPIO
 * @return    the value of the pin, -1 on failure
 */
static inline int gpio_get(gpio_t* gpio)
{
    char data;
    int ret;
    ZF_LOGF_IF(!gpio, "Handle to GPIO pin not supplied!");
    ZF_LOGF_IF(!gpio->gpio_sys, "GPIO pin's parent controller handle invalid!");
    ret = gpio->gpio_sys->read(gpio, &data, 1);
    if (ret == 1) {
        return data;
    } else {
        return -1;
    }
}

/**
 * Set a GPIO pin
 * @param[in] a handle to a GPIO
 * @return    0 on success
 */
static inline int gpio_set(gpio_t* gpio)
{
    char data;
    ZF_LOGF_IF(!gpio, "Handle to GPIO pin not supplied!");
    ZF_LOGF_IF(!gpio->gpio_sys, "GPIO pin's parent controller handle invalid!");
    data = 0xff;
    return (gpio->gpio_sys->write(gpio, &data, 1) != 1);
}

/**
 * Check if an IRQ is pending for this GPIO
 * @param[in] a handle to a GPIO
 * @return    -1 if the GPIO does not support IRQs
 *             0 if an IRQ is not pending
 *             1 if an IRQ is pending
 */
static inline int gpio_is_pending(gpio_t* gpio)
{
    ZF_LOGF_IF(!gpio, "Handle to GPIO pin not supplied!");
    ZF_LOGF_IF(!gpio->gpio_sys, "GPIO pin's parent controller handle invalid!");
    return gpio->gpio_sys->pending_status(gpio, 0);
}

/**
 * Clear pending IRQs for this GPIO
 * @param[in] a handle to a GPIO
 */
static inline void gpio_pending_clear(gpio_t* gpio)
{
    ZF_LOGF_IF(!gpio, "Handle to GPIO pin not supplied!");
    ZF_LOGF_IF(!gpio->gpio_sys, "GPIO pin's parent controller handle invalid!");
    gpio->gpio_sys->pending_status(gpio, 1);
}

/**
 * Enable the IRQ signal from the pin.
 * @param[in] gpio Handle to the pin to manipulate
 * @return 0 for success, nonzero on error.
 */
static inline int
gpio_irq_enable(gpio_t *gpio)
{
    ZF_LOGF_IF(!gpio, "Handle to GPIO pin not supplied!");
    ZF_LOGF_IF(!gpio->gpio_sys, "GPIO pin's parent controller handle invalid!");
    return gpio->gpio_sys->irq_enable_disable(gpio, true);
}

/**
 * Disable the IRQ signal from the pin.
 * @param[in] gpio Handle to the pin to manipulate
 * @return 0 for success, nonzero on error.
 */
static inline int
gpio_irq_disable(gpio_t *gpio)
{
    ZF_LOGF_IF(!gpio, "Handle to GPIO pin not supplied!");
    ZF_LOGF_IF(!gpio->gpio_sys, "GPIO pin's parent controller handle invalid!");
    return gpio->gpio_sys->irq_enable_disable(gpio, false);
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

static inline int gpio_new(gpio_sys_t* gpio_sys, gpio_id_t id, enum gpio_dir dir, gpio_t* gpio)
{
    ZF_LOGF_IF(!gpio_sys, "Handle to GPIO controller not supplied!");
    ZF_LOGF_IF(!gpio_sys->init, "Unimplemented!");
    ZF_LOGF_IF(!gpio, "Handle to output pin structure not supplied!");
    return gpio_sys->init(gpio_sys, id, dir, gpio);
}
