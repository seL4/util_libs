/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _PLATSUPPORT_PLAT_MUX_H
#define _PLATSUPPORT_PLAT_MUX_H

#include <platsupport/gpio.h>
#include <platsupport/plat/gpio.h>

#define MUX_PADDR_BASE 0x70000000


typedef struct mux_sys mux_sys_t;

enum mux_feature {
    NMUX_FEATURES
};

enum gpio_features_indexes {
   CAN1_INTn = 0,
   CAN1_CS,
   CAN2_CS,
};

struct gpio_feature_data {
    int pin_number;
    enum gpio_int_enb int_enb;
    enum gpio_int_type int_type;
    enum gpio_mode mode;
    bool default_value;
};


#define GMACFG_ADDR_OFFSET 0x0900


int
tegra_mux_init(volatile void* gpio,volatile void* pinmux_misc,volatile void* pinmux_aux, mux_sys_t* mux);
int
tegra_gpio_sys_init(mux_sys_t* mux_sys, gpio_sys_t* gpio_sys);

#endif /* _PLATSUPPORT_PLAT_MUX_H */
