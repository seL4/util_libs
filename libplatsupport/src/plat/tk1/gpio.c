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

#include <platsupport/gpio.h>

#define GPIO_CONTROLLER1_ADDR_OFFSET 0
#define GPIO_CONTROLLER2_ADDR_OFFSET 0x100
#define GPIO_CONTROLLER3_ADDR_OFFSET 0x200
#define GPIO_CONTROLLER4_ADDR_OFFSET 0x300
#define GPIO_CONTROLLER5_ADDR_OFFSET 0x400
#define GPIO_CONTROLLER6_ADDR_OFFSET 0x500
#define GPIO_CONTROLLER7_ADDR_OFFSET 0x600
#define GPIO_CONTROLLER8_ADDR_OFFSET 0x700

#define GPIO_CNF      0x0
#define GPIO_OE       0x10
#define GPIO_OUT      0x20
#define GPIO_IN       0x30
#define GPIO_INT_STA  0x40
#define GPIO_INT_ENB  0x50
#define GPIO_INT_LVL  0x60
#define GPIO_INT_CLR  0x70

#define GPIO_PORT1    0x0
#define GPIO_PORT2    0x4
#define GPIO_PORT3    0x8
#define GPIO_PORT4    0xC

#define TEGRA_GPIO_PORTS   4
#define TEGRA_GPIO_BANKS   8

#define GPIO_BANK(x)		((x) >> 5)
#define GPIO_PORT(x)		(((x) >> 3) & 0x3)
#define GPIO_FULLPORT(x)	((x) >> 3)
#define GPIO_BIT(x)		((x) & 0x7)


#define GPIO_INT_LVL_MASK		0x010101
#define GPIO_INT_LVL_EDGE_RISING	0x000101
#define GPIO_INT_LVL_EDGE_FALLING	0x000100
#define GPIO_INT_LVL_EDGE_BOTH		0x010100
#define GPIO_INT_LVL_LEVEL_HIGH		0x000001
#define GPIO_INT_LVL_LEVEL_LOW		0x000000

uint32_t tegra_gpio_controller[] = { GPIO_CONTROLLER1_ADDR_OFFSET,
                                     GPIO_CONTROLLER2_ADDR_OFFSET,
                                     GPIO_CONTROLLER3_ADDR_OFFSET,
                                     GPIO_CONTROLLER4_ADDR_OFFSET,
                                     GPIO_CONTROLLER5_ADDR_OFFSET,
                                     GPIO_CONTROLLER6_ADDR_OFFSET,
                                     GPIO_CONTROLLER7_ADDR_OFFSET,
                                     GPIO_CONTROLLER8_ADDR_OFFSET
                                   };

uint32_t tegra_gpio_port[] = { GPIO_PORT1,
                               GPIO_PORT2,
                               GPIO_PORT3,
                               GPIO_PORT4
                             };

static struct gpio_feature_data can1_intn = {
    .pin_number = GPIO_PS2, .int_enb = GPIO_INT_ENABLE, .int_type = GPIO_INT_LOW_LVL, .mode = GPIO_MODE_INPUT, .default_value = 0
};

static struct gpio_feature_data can1_cs = {
    .pin_number = GPIO_PS6, .int_enb = GPIO_INT_DISABLE, .int_type = GPIO_INT_LOW_LVL, .mode = GPIO_MODE_OUTPUT, .default_value = 1
};

static struct gpio_feature_data can2_cs = {
    .pin_number = GPIO_PT0, .int_enb = GPIO_INT_DISABLE, .int_type = GPIO_INT_LOW_LVL, .mode = GPIO_MODE_OUTPUT, .default_value = 1
};

static struct gpio_feature_data usb_vbus_en1 = {
    .pin_number = GPIO_PN5, .int_enb = GPIO_INT_DISABLE, .int_type = GPIO_INT_LOW_LVL, .mode = GPIO_MODE_OUTPUT, .default_value = 1
};

/* The SPI CSn signal for the MPU is connected to the TK1's GPIO4 pin.
 *
 * Please find these values recorded in the daughterboard's "pins.ods" mapping
 * spreadsheet.
 */
static struct gpio_feature_data feat_gpio_ps3 = {
    .pin_number = GPIO_PS3,
    .int_enb = GPIO_INT_DISABLE,
    .int_type = GPIO_INT_LOW_LVL,
    .mode = GPIO_MODE_OUTPUT,
    .default_value = 1
};

/* The SPI CSn signal for the barometer is connected to the TK1's GPIO5 pin. */
static struct gpio_feature_data feat_gpio_pr0 = {
    .pin_number = GPIO_PR0,
    .int_enb = GPIO_INT_DISABLE,
    .int_type = GPIO_INT_LOW_LVL,
    .mode = GPIO_MODE_OUTPUT,
    .default_value = 1
};

/* The SPI CSn signal for the accelerometer is connected to the TK1's GPIO6 pin. */
static struct gpio_feature_data feat_gpio_pr6 = {
    .pin_number = GPIO_PR6,
    .int_enb = GPIO_INT_DISABLE,
    .int_type = GPIO_INT_LOW_LVL,
    .mode = GPIO_MODE_OUTPUT,
    .default_value = 1
};

/* The SPI CSn signal for the gyroscope is connected to the TK1's GPIO7 pin. */
static struct gpio_feature_data feat_gpio_ps4 = {
    .pin_number = GPIO_PS4,
    .int_enb = GPIO_INT_DISABLE,
    .int_type = GPIO_INT_LOW_LVL,
    .mode = GPIO_MODE_OUTPUT,
    .default_value = 1
};


struct gpio_feature_data* gpio_features[] = {
    [CAN1_INTn] = &can1_intn,
    [CAN1_CS] = &can1_cs,
    [CAN2_CS] = &can2_cs,
    [USB_VBUS_EN1] = &usb_vbus_en1,
    [FEAT_GPIO_PS3n] = &feat_gpio_ps3,
    [FEAT_GPIO_PR0n] = &feat_gpio_pr0,
    [FEAT_GPIO_PR6n] = &feat_gpio_pr6,
    [FEAT_GPIO_PS4n] = &feat_gpio_ps4
};

static int tegra_pending_status(gpio_t* gpio, int clear)
{
    int pending;
    pending = gpio_check_pending(gpio->gpio_sys, gpio->id);

    if (clear) {
        gpio_int_clear(gpio->gpio_sys, gpio->id);
    }

    return pending;
}

static int tegra_gpio_init(gpio_sys_t *gpio_sys, int id, enum gpio_dir dir, gpio_t* gpio)
{
    ZF_LOGV("Configuring GPIO pin %d\n", id);

    gpio->gpio_sys = gpio_sys;
    gpio->next = NULL;

    struct gpio_feature_data* gpio_config = gpio_features[id];

    enum gpio_pin gpio_number = gpio_config->pin_number;
    gpio->id = gpio_number;
    gpio_set_interrupt_type(gpio_sys, gpio_number, gpio_config->int_type);
    gpio_set_mode(gpio_sys, gpio_number, gpio_config->mode);
    gpio_interrupt_enable(gpio_sys, gpio_number, gpio_config->int_enb);

    if(gpio_config->mode == GPIO_MODE_OUTPUT)
    {
        gpio_set_level(gpio_sys, gpio_number, gpio_config->default_value);
    }

    return 0;
}

static int tegra_gpio_read(gpio_t* gpio, char* data, int len)
{
    *data = gpio_get_input(gpio->gpio_sys, gpio->id);
    return 1;
}

static int tegra_gpio_write(gpio_t* gpio, const char* data, int len)
{
    if(NULL != data) {
       int level = *data;
       gpio_set_level(gpio->gpio_sys, gpio->id, level);
       return 1;
    }

    return 0;
}

static inline volatile void* get_controller_register(gpio_sys_t *gpio_sys, enum gpio_pin gpio_num, uint32_t gpio_register)
{
     return (volatile void*)((uintptr_t)gpio_sys->priv + tegra_gpio_controller[GPIO_BANK(gpio_num)] + tegra_gpio_port[GPIO_PORT(gpio_num)] + gpio_register);
}

void gpio_init(volatile void *vaddr, mux_sys_t *mux_sys, gpio_sys_t *gpio_sys)
{
    ZF_LOGV("%s, vaddr: %p\n", __func__,vaddr);
    gpio_sys->read = &tegra_gpio_read;
    gpio_sys->write = &tegra_gpio_write;
    gpio_sys->pending_status = &tegra_pending_status;
    gpio_sys->init = &tegra_gpio_init;
    gpio_sys->priv = (void*)vaddr;
    /* TODO: mux_sys is not yet used by this GPIO driver, so currently
     * we do not save this parameter */
}

int gpio_sys_init(ps_io_ops_t *io_ops, gpio_sys_t* gpio_sys)
{
    if (NULL == io_ops || NULL == gpio_sys) {
        ZF_LOGE("Invalid io_ops or gpio_sys");
        return 1;
    } else {
        mux_sys_t *mux = malloc(sizeof(*mux));
        if (!mux) {
            ZF_LOGE("Failed to allocate mux_sys_t");
            return 1;
        }
        if (!mux_sys_init(io_ops, mux)) {
            ZF_LOGE("Failed to initialize mux");
            free(mux);
            return 1;
        }
        volatile void *gpio_vaddr = ps_io_map(&io_ops->io_mapper, GPIO_PADDR_BASE, PAGE_SIZE_4K, 0, PS_MEM_NORMAL);
        if (!gpio_vaddr) {
            ZF_LOGE("Failed to map GPIO page");
            free(mux);
            return 1;
        }
        gpio_init(gpio_vaddr, mux, gpio_sys);
        return 0;
    }
}

void gpio_set_pad_mode(gpio_sys_t *gpio_sys, enum gpio_pin gpio, enum gpio_pad_mode mode)
{
    ZF_LOGV("%s, offset: 0x%x, controller offset: 0x%x, port offset: 0x%x, GPIO_CNF: 0x%x, gpio: %d, bank: %d, port: %d, gpio_bit: %d\n",
                __func__, tegra_gpio_controller[GPIO_BANK(gpio)] + tegra_gpio_port[GPIO_PORT(gpio)] + GPIO_CNF,
                 tegra_gpio_controller[GPIO_BANK(gpio)],
                  tegra_gpio_port[GPIO_PORT(gpio)],
                  GPIO_CNF, gpio, GPIO_BANK(gpio), GPIO_PORT(gpio), GPIO_BIT(gpio));
    ZF_LOGV("%s, mode: %d\n", __func__, mode);

    volatile void *reg_vaddr = get_controller_register(gpio_sys, gpio, GPIO_CNF);

    uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;

    switch (mode) {
        case GPIO_MODE:
            reg |= BIT(GPIO_BIT(gpio));
            break;
        case SFIO_MODE:
            reg &= ~(BIT(GPIO_BIT(gpio)));
            break;
        default:
            ZF_LOGF("%s gpio: %d,error: %d\n", __func__, gpio, mode);
    }

    ZF_LOGV("%s, reg: 0x%x\n", __func__, reg);
    *(uint32_t*)reg_vaddr = reg;
}

void gpio_interrupt_enable(gpio_sys_t *gpio_sys, enum gpio_pin gpio, enum gpio_int_enb setting)
{
    volatile void *reg_vaddr = get_controller_register(gpio_sys, gpio, GPIO_INT_ENB);

    uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;

    switch (setting) {
        case GPIO_INT_ENABLE:
            reg |= BIT(GPIO_BIT(gpio));
            break;
        case GPIO_INT_DISABLE:
            reg &= ~(BIT(GPIO_BIT(gpio)));
            break;
        default:
            ZF_LOGF("%s gpio: %d,error: %d\n", __func__, gpio, setting);
    }

    *(volatile uint32_t*)reg_vaddr = reg;
}

void gpio_set_interrupt_type(gpio_sys_t *gpio_sys, enum gpio_pin gpio, enum gpio_int_type type)
{
    uint32_t lvl_type = 0;
    volatile void *reg_vaddr = get_controller_register(gpio_sys, gpio, GPIO_INT_LVL);

    uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;

    switch (type) {
        case GPIO_INT_RISING_EDGE:
           lvl_type = GPIO_INT_LVL_EDGE_RISING;
           break;
        case GPIO_INT_FALLING_EDGE:
           lvl_type = GPIO_INT_LVL_EDGE_FALLING;
           break;
        case GPIO_INT_BOTH_EDGE:
           lvl_type = GPIO_INT_LVL_EDGE_BOTH;
           break;
        case GPIO_INT_HIGH_LVL:
           lvl_type = GPIO_INT_LVL_LEVEL_HIGH;
           break;
        case GPIO_INT_LOW_LVL:
           lvl_type = GPIO_INT_LVL_LEVEL_LOW;
           break;
        default:
           ZF_LOGF("%s gpio: %d,error: %d\n", __func__, gpio, type);
     }

     reg &= ~(GPIO_INT_LVL_MASK << GPIO_BIT(gpio));

     reg |= lvl_type << GPIO_BIT(gpio);

     *(uint32_t*)reg_vaddr = reg;
}

void gpio_set_mode(gpio_sys_t *gpio_sys, enum gpio_pin gpio, enum gpio_mode mode)
{
    volatile void *reg_vaddr = get_controller_register(gpio_sys, gpio, GPIO_OE);

    uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;

    switch (mode) {
        case GPIO_MODE_OUTPUT:
            reg |= BIT(GPIO_BIT(gpio));
            break;
        case GPIO_MODE_INPUT:
            reg &= ~(BIT(GPIO_BIT(gpio)));
            break;
        default:
            ZF_LOGF("%s gpio: %d,error: %d\n", __func__, gpio, mode);
    }

    *(uint32_t*)reg_vaddr = reg;
}

void gpio_set_level(gpio_sys_t *gpio_sys, enum gpio_pin gpio, int level)
{

    ZF_LOGV("%s, offset: 0x%x, controller offset: 0x%x, port offset: 0x%x, GPIO_IN: 0x%x, gpio: %d, bank: %d, port: %d, gpio_bit: %d\n",
                __func__, tegra_gpio_controller[GPIO_BANK(gpio)] + tegra_gpio_port[GPIO_PORT(gpio)] + GPIO_OUT,
                 tegra_gpio_controller[GPIO_BANK(gpio)],
                 tegra_gpio_port[GPIO_PORT(gpio)],
                  GPIO_OUT, gpio, GPIO_BANK(gpio), GPIO_PORT(gpio), GPIO_BIT(gpio));
    volatile void *reg_vaddr = get_controller_register(gpio_sys, gpio, GPIO_OUT);

    uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;
    ZF_LOGV("%s, level: %d\n", __func__, level);
    if (level) {
        reg |= BIT(GPIO_BIT(gpio));
    } else {
        reg &= ~(BIT(GPIO_BIT(gpio)));
    }

    *(uint32_t*)reg_vaddr = reg ;
}

bool gpio_get_input(gpio_sys_t *gpio_sys, enum gpio_pin gpio)
{

   ZF_LOGV("%s, offset: 0x%x, controller offset: 0x%x, port offset: 0x%x, GPIO_IN: 0x%x, gpio: %d, bank: %d, port: %d, gpio_bit: %d\n",
                __func__, tegra_gpio_controller[GPIO_BANK(gpio)] + tegra_gpio_port[GPIO_PORT(gpio)] + GPIO_IN,
                 tegra_gpio_controller[GPIO_BANK(gpio)],
                 tegra_gpio_port[GPIO_PORT(gpio)],
                  GPIO_OUT, gpio, GPIO_BANK(gpio), GPIO_PORT(gpio), GPIO_BIT(gpio));

   volatile void *reg_vaddr = get_controller_register(gpio_sys, gpio, GPIO_IN);

   uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;

   ZF_LOGV("reg: 0x%x\n", reg);

   return !!(reg & BIT(GPIO_BIT(gpio)));

}


void gpio_int_clear(gpio_sys_t *gpio_sys, enum gpio_pin gpio)
{

   ZF_LOGV("%s, offset: 0x%x, controller offset: 0x%x, port offset: 0x%x, GPIO_IN: 0x%x, gpio: %d, bank: %d, port: %d, gpio_bit: %d\n", 
                __func__, tegra_gpio_controller[GPIO_BANK(gpio)] + tegra_gpio_port[GPIO_PORT(gpio)] + GPIO_INT_CLR,
                 tegra_gpio_controller[GPIO_BANK(gpio)],
                  tegra_gpio_port[GPIO_PORT(gpio)],
                  GPIO_OUT, gpio, GPIO_BANK(gpio), GPIO_PORT(gpio), GPIO_BIT(gpio));

   volatile void *reg_vaddr = get_controller_register(gpio_sys, gpio, GPIO_INT_CLR);

   uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;

   reg |= BIT(GPIO_BIT(gpio));

   ZF_LOGV("reg: 0x%x\n", reg);

   *(uint32_t*)reg_vaddr = reg;
}

bool gpio_check_pending(gpio_sys_t *gpio_sys, enum gpio_pin gpio)
{
    volatile void *reg_vaddr = get_controller_register(gpio_sys, gpio, GPIO_INT_STA);

    uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;

    return !!(reg & BIT(GPIO_BIT(gpio)));
}
