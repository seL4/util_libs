/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
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

//#define GPIO_DEBUG
//#define GPIO_DEBUG_LEVEL

#ifdef GPIO_DEBUG
#define gpio_debug(fmt,args...) ZF_LOGE(fmt, ##args)
#else
#define gpio_debug(fmt,args...)
#endif

#ifdef GPIO_DEBUG_LEVEL
#define gpio_debug_level(fmt,args...) ZF_LOGE(fmt, ##args)
#else
#define gpio_debug_level(fmt,args...)
#endif

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

volatile static void *gpio_vaddr;

static inline volatile void* get_controller_register(enum gpio_pin gpio_num, uint32_t gpio_register)
{
     return (gpio_vaddr + tegra_gpio_controller[GPIO_BANK(gpio_num)] + tegra_gpio_port[GPIO_PORT(gpio_num)] + gpio_register);
}


void gpio_init(volatile void *vaddr)
{
    gpio_debug("%s, vaddr: %p\n", __func__,vaddr);
    gpio_vaddr = vaddr;
}

void gpio_set_pad_mode(enum gpio_pin gpio, enum gpio_pad_mode mode)
{
    gpio_debug("%s, offset: 0x%x, controller offset: 0x%x, port offset: 0x%x, GPIO_CNF: 0x%x, gpio: %d, bank: %d, port: %d, gpio_bit: %d\n",
                __func__, tegra_gpio_controller[GPIO_BANK(gpio)] + tegra_gpio_port[GPIO_PORT(gpio)] + GPIO_CNF,
                 tegra_gpio_controller[GPIO_BANK(gpio)],
                  tegra_gpio_port[GPIO_PORT(gpio)],
                  GPIO_CNF, gpio, GPIO_BANK(gpio), GPIO_PORT(gpio), GPIO_BIT(gpio));
    gpio_debug("%s, mode: %d\n", __func__, mode);

    volatile void *reg_vaddr = get_controller_register(gpio, GPIO_CNF);

    uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;

    switch (mode) {
        case GPIO_MODE:
            reg |= BIT(GPIO_BIT(gpio));
            break;
        case SFIO_MODE:
            reg &= ~(BIT(GPIO_BIT(gpio)));
            break;
        default:
            ZF_LOGE("%s gpio: %d,error: %d\n", __func__, gpio, mode);
    }

    gpio_debug("%s, reg: 0x%x\n", __func__, reg);
    *(uint32_t*)reg_vaddr = reg;
}

void gpio_interrupt_enable(enum gpio_pin gpio, enum gpio_int_enb setting)
{
    
    volatile void *reg_vaddr = get_controller_register(gpio, GPIO_INT_ENB);

    uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;

    switch (setting) {
        case GPIO_INT_ENABLE:
            reg |= BIT(GPIO_BIT(gpio));
            break;
        case GPIO_INT_DISABLE:
            reg &= ~(BIT(GPIO_BIT(gpio)));
            break;
        default: 
            ZF_LOGE("%s gpio: %d,error: %d\n", __func__, gpio, setting);
    }

    *(uint32_t*)reg_vaddr = reg;    
}

void gpio_set_interrupt_type(enum gpio_pin gpio, enum gpio_int_type type)
{
    uint32_t lvl_type; 
    volatile void *reg_vaddr = get_controller_register(gpio, GPIO_INT_LVL);

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
           ZF_LOGE("%s gpio: %d,error: %d\n", __func__, gpio, type);
     }

     reg &= ~(GPIO_INT_LVL_MASK << GPIO_BIT(gpio));

     reg |= lvl_type << GPIO_BIT(gpio);

     *(uint32_t*)reg_vaddr = reg;
}

void gpio_set_mode(enum gpio_pin gpio, enum gpio_mode mode)
{
    volatile void *reg_vaddr = get_controller_register(gpio, GPIO_OE);

    uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;

    switch (mode) {
        case GPIO_MODE_OUTPUT:
            reg |= BIT(GPIO_BIT(gpio));
            break;
        case GPIO_MODE_INPUT:
            reg &= ~(BIT(GPIO_BIT(gpio)));
            break;
        default:
            ZF_LOGE("%s gpio: %d,error: %d\n", __func__, gpio, mode);
    }

    *(uint32_t*)reg_vaddr = reg;
}

void gpio_set_level(enum gpio_pin gpio, int level)
{

    gpio_debug("%s, offset: 0x%x, controller offset: 0x%x, port offset: 0x%x, GPIO_IN: 0x%x, gpio: %d, bank: %d, port: %d, gpio_bit: %d\n",
                __func__, tegra_gpio_controller[GPIO_BANK(gpio)] + tegra_gpio_port[GPIO_PORT(gpio)] + GPIO_OUT,
                 tegra_gpio_controller[GPIO_BANK(gpio)],
                 tegra_gpio_port[GPIO_PORT(gpio)],
                  GPIO_OUT, gpio, GPIO_BANK(gpio), GPIO_PORT(gpio), GPIO_BIT(gpio));
    volatile void *reg_vaddr = get_controller_register(gpio, GPIO_OUT);

    uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;
    gpio_debug_level("%s, level: %d\n", __func__, level);
    if (level) {
        reg |= BIT(GPIO_BIT(gpio));
    } else {
        reg &= ~(BIT(GPIO_BIT(gpio)));
    }

    *(uint32_t*)reg_vaddr = reg ;
}

bool gpio_get_input(enum gpio_pin gpio)
{

   gpio_debug("%s, offset: 0x%x, controller offset: 0x%x, port offset: 0x%x, GPIO_IN: 0x%x, gpio: %d, bank: %d, port: %d, gpio_bit: %d\n",
                __func__, tegra_gpio_controller[GPIO_BANK(gpio)] + tegra_gpio_port[GPIO_PORT(gpio)] + GPIO_IN,
                 tegra_gpio_controller[GPIO_BANK(gpio)],
                 tegra_gpio_port[GPIO_PORT(gpio)],
                  GPIO_OUT, gpio, GPIO_BANK(gpio), GPIO_PORT(gpio), GPIO_BIT(gpio));

   volatile void *reg_vaddr = get_controller_register(gpio, GPIO_IN);

   uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;

   gpio_debug("reg: 0x%x\n", reg);

   return !!(reg & BIT(GPIO_BIT(gpio)));

}


void gpio_int_clear(enum gpio_pin gpio)
{

   gpio_debug("%s, offset: 0x%x, controller offset: 0x%x, port offset: 0x%x, GPIO_IN: 0x%x, gpio: %d, bank: %d, port: %d, gpio_bit: %d\n", 
                __func__, tegra_gpio_controller[GPIO_BANK(gpio)] + tegra_gpio_port[GPIO_PORT(gpio)] + GPIO_INT_CLR,
                 tegra_gpio_controller[GPIO_BANK(gpio)],   
                  tegra_gpio_port[GPIO_PORT(gpio)],  
                  GPIO_OUT, gpio, GPIO_BANK(gpio), GPIO_PORT(gpio), GPIO_BIT(gpio));

   volatile void *reg_vaddr = get_controller_register(gpio, GPIO_INT_CLR);

   uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;

   reg |= BIT(GPIO_BIT(gpio));

   gpio_debug("reg: 0x%x\n", reg); 

   *(uint32_t*)reg_vaddr = reg;
}

bool gpio_check_pending( enum gpio_pin gpio)
{
    volatile void *reg_vaddr = get_controller_register(gpio,GPIO_INT_STA);

    uint32_t reg = (*(uint32_t*)reg_vaddr) & 0xff;

    return !!(reg & BIT(GPIO_BIT(gpio)));
}
