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

#include <unistd.h>

typedef ssize_t mux_feature_t;
typedef struct mux_sys mux_sys_t;

#include <utils/arith.h>

typedef enum mux_gpio_dir {
    MUX_DIR_NOT_A_GPIO = 0,
    MUX_DIR_GPIO_IN = BIT(0),
    MUX_DIR_GPIO_OUT = BIT(1),
    MUX_DIR_GPIO_BOTH = (MUX_DIR_GPIO_IN | MUX_DIR_GPIO_OUT)
} mux_gpio_dir_t;

struct mux_sys {
    int (*feature_enable)(mux_sys_t* mux, mux_feature_t, enum mux_gpio_dir);
    int (*feature_disable)(mux_sys_t* mux, mux_feature_t);
    void *(*get_mux_vaddr)(mux_sys_t *mux);
    void *priv;
};

#include <platsupport/io.h>

static inline int mux_sys_valid(const mux_sys_t* mux_sys)
{
    return mux_sys && mux_sys->priv;
}

/**
 * Returns the vaddr of the mux controller so a driver can directly
 * access the MUX controller and bypass the muxc.
 */
static inline void * mux_sys_get_vaddr(mux_sys_t *mux) {
    return (mux && mux->get_mux_vaddr) ? mux->get_mux_vaddr(mux) : NULL;
}

/**
 * Initialise (IO)MUX sub systems
 * @param[in]  io_ops  collection of IO operations for the subsystem to use.
 * @param[out] mux     On success, this will be filled with the appropriate
 *                     subsystem data.
 * @param[in]  dependencies     As an edge case, if this driver depends on some
 *                              other set of drivers, you can pass in instances
 *                              of those dependencies as a pointer or array of
 *                              pointers here.
 * @return             0 on success.
 */
int mux_sys_init(ps_io_ops_t* io_ops, void *dependencies, mux_sys_t* mux);

/**
 * Enable a SoC feature via the IO MUX
 * @param[in] mux           A handle to the mux system
 * @param[in] mux_feature   A SoC specific feature to enable.
 * @param[in] mux_gpio_dir  If the signal being muxed out on the pin is a
 *                          a GPIO signal, this might be required by your
 *                          platform. For example, if your mux controller has
 *                          input and output buffers, you would want to ensure
 *                          that the output buffer on the pin is enabled if you
 *                          plan to use the pin as a GPIO output.
 *
 *                          If the signal you're muxing out on the pin is not
 *                          a GPIO controller, then you should use
 *                          MUX_DIR_NOT_A_GPIO.
 * @return                0 on success
 */
static inline int mux_feature_enable(mux_sys_t* mux, mux_feature_t mux_feature,
                                     enum mux_gpio_dir mux_gpio_dir)
{
    if (mux->feature_enable) {
        return mux->feature_enable(mux, mux_feature, mux_gpio_dir);
    } else {
        return -ENOSYS;
    }
}

/**
 * Explicitly ensure that a certain controller's signals are not being driven
 * out.
 *
 * This is useful if you need to ensure that no signals are output by the
 * controller while it is being set up. In such a case you'd feature_disable()
 * the signals, then initialize the controller, then feature_enable() the
 * controller again.
 *
 * @param[in] mux         A handle to the mux system
 * @param[in] mux_feature A SoC specific feature to enable.
 * @return                0 on success
 */
static inline int mux_feature_disable(mux_sys_t* mux, mux_feature_t mux_feature)
{
    if (mux->feature_disable) {
        return mux->feature_disable(mux, mux_feature);
    } else {
        return -ENOSYS;
    }
}
