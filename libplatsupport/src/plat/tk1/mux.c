/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <utils/util.h>

#include <platsupport/plat/mux.h>


#ifdef GPIO_DEBUG
#define DGPIO(...) printf("GPIO: " __VA_ARGS__)
#else
#define DGPIO(...) do{}while(0)
#endif


static int
tegra_gpio_init(gpio_sys_t* gpio_sys, int id, enum gpio_dir dir, gpio_t* gpio);

static int
tegra_gpio_read(gpio_t* gpio, char* data, int len);

static int
tegra_gpio_write(gpio_t* gpio, const char* data, int len);


static struct gpio_feature_data can1_intn = {
    .pin_number = GPIO_PS2, .int_enb = GPIO_INT_ENABLE, .int_type = GPIO_INT_LOW_LVL, .mode = GPIO_MODE_INPUT, .default_value = 0
};

static struct gpio_feature_data can1_cs = {
    .pin_number = GPIO_PS6, .int_enb = GPIO_INT_DISABLE, .int_type = GPIO_INT_LOW_LVL, .mode = GPIO_MODE_OUTPUT, .default_value = 1 
};

static struct gpio_feature_data can2_cs = {
    .pin_number = GPIO_PT0, .int_enb = GPIO_INT_DISABLE, .int_type = GPIO_INT_LOW_LVL, .mode = GPIO_MODE_OUTPUT, .default_value = 1 
};


struct gpio_feature_data* gpio_features[] = {
    [CAN1_INTn] = &can1_intn, 
    [CAN1_CS] = &can1_cs,
    [CAN2_CS] = &can2_cs
}; 

static int
tegra_pending_status(gpio_t* gpio, int clear)
{
    int pending;
    pending = gpio_check_pending(gpio->id);

    if(clear){
        gpio_int_clear(gpio->id);
    } 

    return pending;
}

int
tegra_gpio_sys_init(mux_sys_t* mux_sys, gpio_sys_t* gpio_sys)
{
    if (NULL == mux_sys || NULL == gpio_sys) {
        return -1; 
    } else {
        gpio_sys->priv = mux_sys;
        gpio_sys->read = &tegra_gpio_read;
        gpio_sys->write = &tegra_gpio_write;
        gpio_sys->pending_status = &tegra_pending_status;
        gpio_sys->init = &tegra_gpio_init;
        return 0;
    }
}

static int
tegra_gpio_init(gpio_sys_t* gpio_sys, int id, enum gpio_dir dir, gpio_t* gpio)
{
    DGPIO("Configuring GPIO pin %d\n", id);

    gpio->gpio_sys = gpio_sys;
    gpio->next = NULL;
 
    struct gpio_feature_data* gpio_config = gpio_features[id];

    enum gpio_pin gpio_number = gpio_config->pin_number;
    gpio->id = gpio_number; 
    gpio_set_interrupt_type(gpio_number, gpio_config->int_type);
    gpio_set_mode(gpio_number, gpio_config->mode);
    gpio_interrupt_enable(gpio_number, gpio_config->int_enb);

    if(gpio_config->mode == GPIO_MODE_OUTPUT)
    {
        gpio_set_level(gpio_number, gpio_config->default_value);
    }

    return 0; 
}

/********** pinmux *********************/

struct pmux_pingrp_desc {
	uint8_t funcs[4];
};


volatile static void* _mux_ctrl[2];


static int tegra_mux_init_common(mux_sys_t* mux)
{
    return 0;
}

int
tegra_mux_init(volatile void* gpio,volatile void* pinmux_misc,volatile void* pinmux_aux, mux_sys_t* mux)
{
    if(pinmux_misc) { 
        _mux_ctrl[0] = pinmux_misc;
    }
    if(pinmux_aux) {
        _mux_ctrl[1] = pinmux_aux; 
    }
   
    gpio_init(gpio);   
 
    return tegra_mux_init_common(mux); 
}

static int
tegra_gpio_read(gpio_t* gpio, char* data, int len)
{
    *data = gpio_get_input(gpio->id);
    return 1;      
}

static int
tegra_gpio_write(gpio_t* gpio, const char* data, int len)
{
    if(NULL != data) {
       int level = *data;
       gpio_set_level(gpio->id, level);
       return 1; 
    }

    return 0; 
}
