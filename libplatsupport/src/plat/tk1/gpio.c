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
#include <stddef.h>

#include <utils/arith.h>
#include <utils/fence.h>
#include <utils/attribute.h>
#include <platsupport/gpio.h>

#include "mux_gpio_priv.h"
#include "../../services.h"

/** @file GPIO driver for the TK1.
 *
 * Contains routines for manipulating GPIO pins on the TK1.
 *
 *  PREREQUISITES:
 * This driver currently assumes that the MMIO registers it accesses are mapped
 * as strongly ordered and uncached. The driver makes no attempts whatsoever at
 * managing the write buffer or managing ordering of reads and writes.
 */

#define GPIO_CONTROLLER1_ADDR_OFFSET 0
#define GPIO_CONTROLLER2_ADDR_OFFSET 0x100
#define GPIO_CONTROLLER3_ADDR_OFFSET 0x200
#define GPIO_CONTROLLER4_ADDR_OFFSET 0x300
#define GPIO_CONTROLLER5_ADDR_OFFSET 0x400
#define GPIO_CONTROLLER6_ADDR_OFFSET 0x500
#define GPIO_CONTROLLER7_ADDR_OFFSET 0x600
#define GPIO_CONTROLLER8_ADDR_OFFSET 0x700

#define PORT_NBITS                  (8)
#define REG_VALID_BITS_MASK         (0xFF)
#define CNFREG_VALID_BITS_MASK      (0xFFFF)

enum tk1_gpio_bank_regs {
    GPIO_CNF = 0,
    GPIO_OE,
    GPIO_OUT,
    GPIO_IN,
    GPIO_INT_STA,
    GPIO_INT_ENB,
    GPIO_INT_LVL,
    GPIO_INT_CLR,
    GPIO_MSK_CNF,
    GPIO_MSK_OE,
    GPIO_MSK_OUT,
    /* 0xB is invalid - no register exists there */
    GPIO_REG_INVALID = 0xB,
    GPIO_MSK_INT_STA,
    GPIO_MSK_INT_ENB,
    GPIO_MSK_INT_LVL,

    TEGRA_GPIO_REGS
};

#define GPIO_PORT1    0x0
#define GPIO_PORT2    0x4
#define GPIO_PORT3    0x8
#define GPIO_PORT4    0xC

#define TEGRA_GPIO_PORTS   4
#define TEGRA_GPIO_BANKS   8

#define GPIO_BANK(x)		((x) >> 5)
#define GPIO_PORT(x)		(((x) >> 3) & 0x3)
#define GPIO_BIT(x)		((x) & 0x7)

#define GPIO_INT_LVL_MASK		0x010101
#define GPIO_INT_LVL_EDGE_RISING	0x000101
#define GPIO_INT_LVL_EDGE_FALLING	0x000100
#define GPIO_INT_LVL_EDGE_BOTH		0x010100
#define GPIO_INT_LVL_LEVEL_HIGH		0x000001
#define GPIO_INT_LVL_LEVEL_LOW		0x000000

/* Used for debug output */
static uint32_t tegra_gpio_controller[] = { GPIO_CONTROLLER1_ADDR_OFFSET,
                                     GPIO_CONTROLLER2_ADDR_OFFSET,
                                     GPIO_CONTROLLER3_ADDR_OFFSET,
                                     GPIO_CONTROLLER4_ADDR_OFFSET,
                                     GPIO_CONTROLLER5_ADDR_OFFSET,
                                     GPIO_CONTROLLER6_ADDR_OFFSET,
                                     GPIO_CONTROLLER7_ADDR_OFFSET,
                                     GPIO_CONTROLLER8_ADDR_OFFSET
                                   };

static uint32_t tegra_gpio_port[] = { GPIO_PORT1,
                               GPIO_PORT2,
                               GPIO_PORT3,
                               GPIO_PORT4
                             };

typedef struct tk1_gpio_ {
    volatile uint32_t ports[TEGRA_GPIO_PORTS];
} tk1_gpio_ports_t;
static_assert(sizeof(tk1_gpio_ports_t) == 16, "Ports struct should be 16 bytes");

typedef struct tk1_gpio_bank_ {
    tk1_gpio_ports_t    regs[TEGRA_GPIO_REGS];
    PAD_STRUCT_BETWEEN(0x0,
                       0x100,
                       tk1_gpio_ports_t[TEGRA_GPIO_REGS]);
} tk1_gpio_bank_t;
static_assert(sizeof(tk1_gpio_bank_t) == 256, "Banks should be 256 bytes each");

typedef struct tk1_gpio_regs_ {
    tk1_gpio_bank_t     bank[TEGRA_GPIO_BANKS];
} tk1_gpio_regs_t;
static_assert(sizeof(tk1_gpio_regs_t) == 256*8, "There are only 8 banks");

static inline tk1_gpio_regs_t *
tk1_gpio_get_priv(gpio_sys_t *sys)
{
    ZF_LOGF_IF(sys == NULL, "Invalid controller handle!");
    return (tk1_gpio_regs_t *)sys->priv;
}

static inline tk1_gpio_regs_t *
tk1_gpio_pin_get_priv(gpio_t *pin)
{
    ZF_LOGF_IF(pin == NULL, "Invalid pin handle!");
    return tk1_gpio_get_priv(pin->gpio_sys);
}

static inline tk1_gpio_bank_t *
tk1_gpio_get_bank_by_pin(gpio_sys_t *sys, int pin)
{
    return &tk1_gpio_get_priv(sys)->bank[GPIO_BANK(pin)];
}

static inline volatile uint32_t *
get_controller_register(gpio_sys_t *gpio_sys,
                        enum gpio_pin gpio_num,
                        enum tk1_gpio_bank_regs gpio_register)
{
    ZF_LOGF_IF(gpio_register == GPIO_REG_INVALID, "Invalid register index!");
    ZF_LOGF_IF(gpio_register < 0 || gpio_register > GPIO_MSK_INT_LVL,
               "Invalid register index!");

    return &tk1_gpio_get_bank_by_pin(gpio_sys, gpio_num)->regs[gpio_register]
           .ports[GPIO_PORT(gpio_num)];
}

/** For debugging.
 *
 * The lock bit determines whether or not it's possible to reconfigure the
 * GPIO pins. This function does a scan and prints all those pins that have
 * their lock bits set.
 */
UNUSED static void
tk1_gpio_debug_print_locks(gpio_sys_t *gs)
{
    volatile uint32_t   *v_cnf;
    int                 total = 0;

    for (int i = 0; i <= GPIO_PFF7; i++) {
        v_cnf = get_controller_register(gs, i, GPIO_CNF);

        if (*v_cnf & BIT(GPIO_BIT(i))) {
            total++;
            printf("GPIO: Pin %d has its lock set: CNF lock: regval %x.\n",
                   i, *v_cnf);
        }
    }
    printf("GPIO: lock scan: %d pins have their locks set.\n", total);
}

static int
gpio_set_direction(gpio_sys_t *gpio_sys,
              enum gpio_pin gpio, enum gpio_dir mode)
{
    uint32_t            val, cnf_val;
    volatile uint32_t   *cnf_vaddr = get_controller_register(gpio_sys,
                                                             gpio, GPIO_CNF);;
    volatile uint32_t   *reg_vaddr = get_controller_register(gpio_sys,
                                                             gpio, GPIO_OE);

    val = *reg_vaddr & REG_VALID_BITS_MASK;
    cnf_val = *cnf_vaddr & CNFREG_VALID_BITS_MASK;

    /* There are lock bits in the CNF register (TK1 SoC manual, sec 8.11.1).
     * These lock bits enable and disable access to the CNF and OE registers.
     *
     * The CNF register has the bit that sets the pins to be either GPIO or
     * SFIO mode.
     * The OE register has the bit that sets the pins to be input or output
     * mode.
     *
     * Unfortunately, if the lock bit is set therefore, we will be unable to
     * actually change the configuration of the pin, because the lock bits can
     * only be set up once during boot, and they are sticky after that:
     *
     * Section 8.11.1:
     *  "Lock bits are used to control the access to the CNF and OE registers.
     *  When set, no one can write to the CNF and OE bits. They can be
     *  programmed ONLY during Boot and get reset by chip reset only."
     *
     * So we must first test to see if the pin is already configured the way the
     * user wants it. If it is, then we don't have to do anything.
     *
     * If the bit is NOT already configured to act the way the user is
     * requesting, then we have to check the "lock" bit. If the lock bit is
     * UNSET, then we can reconfigure the pin.
     *
     * If the "lock" bit is SET, then we cannot reconfigure the pin and we
     * must return an error to the user.
     */
    switch (mode) {
    case GPIO_DIR_OUT_DEFAULT_HIGH:
    case GPIO_DIR_OUT_DEFAULT_LOW:
        if (val & BIT(GPIO_BIT(gpio))) {
            /* If it's already how we want it, return success immediately. */
            return 0;
        }
        if (cnf_val & BIT(GPIO_BIT(gpio) + PORT_NBITS)) {
            /* If it's not how the user wants it already, and the lock bit is
             * set, then we can't fulfill the user's request: return error
             * immediately.
             */
            ZF_LOGW("Bit %d: Unable to setup as output pin. Pin is locked.",
                    gpio);
            return -1;
        }

        val |= BIT(GPIO_BIT(gpio));
        break;

    case GPIO_DIR_IN:
        if ((val & BIT(GPIO_BIT(gpio))) == 0) {
            return 0;
        }
        if (cnf_val & BIT(GPIO_BIT(gpio) + PORT_NBITS)) {
            ZF_LOGW("Bit %d: Unable to setup as input pin. Pin is locked.",
                    gpio);
            return -1;
        }

        val &= ~(BIT(GPIO_BIT(gpio)));
        break;

    default:
        ZF_LOGF("gpio: %d, invalid mode %d.", gpio, mode);
    }

    *reg_vaddr = val;
    assert(*reg_vaddr == val);
    return 0;
}

static void
gpio_set_interrupt_type(gpio_sys_t *gpio_sys,
                        enum gpio_pin gpio, enum gpio_dir dir)
{
    uint32_t            val, lvl_type = 0;
    volatile uint32_t   *reg_vaddr = get_controller_register(gpio_sys,
                                                             gpio, GPIO_INT_LVL);

    val = *reg_vaddr & REG_VALID_BITS_MASK;
    switch (dir) {
    case GPIO_DIR_IRQ_RISE:
        lvl_type = GPIO_INT_LVL_EDGE_RISING;
        break;
    case GPIO_DIR_IRQ_FALL:
        lvl_type = GPIO_INT_LVL_EDGE_FALLING;
        break;
    case GPIO_DIR_IRQ_EDGE:
        lvl_type = GPIO_INT_LVL_EDGE_BOTH;
        break;
    case GPIO_DIR_IRQ_HIGH:
        lvl_type = GPIO_INT_LVL_LEVEL_HIGH;
        break;
    case GPIO_DIR_IRQ_LOW:
        lvl_type = GPIO_INT_LVL_LEVEL_LOW;
        break;
    default:
       ZF_LOGF("GPIO: %d, invalid pin direction/interrupt %d", gpio, dir);
    }

    val &= ~(GPIO_INT_LVL_MASK << GPIO_BIT(gpio));
    val |= lvl_type << GPIO_BIT(gpio);
    *reg_vaddr = val;
    assert(*reg_vaddr == val);
}

static void
gpio_interrupt_enable(gpio_sys_t *gpio_sys,
                      enum gpio_pin gpio, bool enable)
{
    uint32_t            val;
    volatile uint32_t   *reg_vaddr = get_controller_register(gpio_sys,
                                                             gpio, GPIO_INT_ENB);

    val = *reg_vaddr & REG_VALID_BITS_MASK;
    if (enable) {
        val |= BIT(GPIO_BIT(gpio));
    } else {
        val &= ~(BIT(GPIO_BIT(gpio)));
    }
    *reg_vaddr = val;
    assert(*reg_vaddr == val);
}

static void
gpio_set_level(gpio_sys_t *gpio_sys, enum gpio_pin gpio, int level)
{
    uint32_t            val;
    volatile uint32_t   *reg_vaddr = get_controller_register(gpio_sys,
                                                             gpio, GPIO_OUT);

    ZF_LOGV("Offset: 0x%x (vaddr %x), controller offset: 0x%x, port offset: 0x%x, "
            "GPIO_IN: 0x%x, gpio: %d, bank: %d, port: %d, gpio_bit: %d",
            tegra_gpio_controller[GPIO_BANK(gpio)]
                + tegra_gpio_port[GPIO_PORT(gpio)]
                + GPIO_OUT,
            reg_vaddr,
            tegra_gpio_controller[GPIO_BANK(gpio)],
            tegra_gpio_port[GPIO_PORT(gpio)],
            GPIO_OUT, gpio, GPIO_BANK(gpio), GPIO_PORT(gpio), GPIO_BIT(gpio));

    val = *reg_vaddr & REG_VALID_BITS_MASK;
    if (level) {
        val |= BIT(GPIO_BIT(gpio));
    } else {
        val &= ~(BIT(GPIO_BIT(gpio)));
    }

    *reg_vaddr = val;
    assert(*reg_vaddr == val);
}

static bool
gpio_get_input(gpio_sys_t *gpio_sys, enum gpio_pin gpio)
{
   uint32_t             val;
   volatile uint32_t    *reg_vaddr = get_controller_register(gpio_sys,
                                                             gpio, GPIO_IN);

    ZF_LOGV("Offset: 0x%x, controller offset: 0x%x, port offset: 0x%x, "
            "GPIO_IN: 0x%x, gpio: %d, bank: %d, port: %d, gpio_bit: %d",
                tegra_gpio_controller[GPIO_BANK(gpio)]
                    + tegra_gpio_port[GPIO_PORT(gpio)]
                    + GPIO_IN,
                tegra_gpio_controller[GPIO_BANK(gpio)],
                tegra_gpio_port[GPIO_PORT(gpio)],
                GPIO_OUT, gpio, GPIO_BANK(gpio), GPIO_PORT(gpio), GPIO_BIT(gpio));


    val = *reg_vaddr & REG_VALID_BITS_MASK;
    ZF_LOGV("GPIO %d bit value: 0x%x\n", gpio, val);
    return !!(val & BIT(GPIO_BIT(gpio)));

}

static void
gpio_int_clear(gpio_sys_t *gpio_sys, enum gpio_pin gpio)
{
   uint32_t             val;
   volatile uint32_t    *reg_vaddr = get_controller_register(gpio_sys,
                                                             gpio, GPIO_INT_CLR);

    ZF_LOGV("offset: 0x%x, controller offset: 0x%x, port offset: 0x%x, "
            "GPIO_IN: 0x%x, gpio: %d, bank: %d, port: %d, gpio_bit: %d",
                tegra_gpio_controller[GPIO_BANK(gpio)]
                    + tegra_gpio_port[GPIO_PORT(gpio)]
                    + GPIO_INT_CLR,
                tegra_gpio_controller[GPIO_BANK(gpio)],
                tegra_gpio_port[GPIO_PORT(gpio)],
                GPIO_OUT, gpio, GPIO_BANK(gpio), GPIO_PORT(gpio), GPIO_BIT(gpio));

    val = *reg_vaddr & REG_VALID_BITS_MASK;
    val |= BIT(GPIO_BIT(gpio));
    *reg_vaddr = val;
}

static bool
gpio_check_pending(gpio_sys_t *gpio_sys, enum gpio_pin gpio)
{
    uint32_t            val;
    volatile uint32_t   *reg_vaddr = get_controller_register(gpio_sys,
                                                             gpio, GPIO_INT_STA);

    val = *reg_vaddr & REG_VALID_BITS_MASK;
    return !!(val & BIT(GPIO_BIT(gpio)));
}

static int tegra_pending_status(gpio_t* gpio, int clear)
{
    int pending;

    if (gpio == NULL) {
        return -ENOSYS;
    }
    if (gpio->gpio_sys == NULL) {
        return -ENOSYS;
    }

    pending = gpio_check_pending(gpio->gpio_sys, gpio->id);

    if (clear) {
        gpio_int_clear(gpio->gpio_sys, gpio->id);
    }

    return pending;
}

static void
tegra_gpio_set_next(gpio_t *self, gpio_t *next)
{
    self->next = next;
}

static int
tegra_gpio_read(gpio_t* gpio, char* data, int len)
{
    /* Assuming 8 bits per byte. This is TK1-specific code, so this is fine.
     */
    gpio_t *curr_gpio = gpio;
    const int nbytes = DIV_ROUND_UP(len, 8);

    for (int i = 0; i < nbytes; i++) {
        const int nbits = ((len - i * 8) / 8) ? 8 : (len % 8);

        for (int j = 0; j < nbits; j++, curr_gpio = curr_gpio->next) {
            int val;

            if (curr_gpio == NULL) {
                ZF_LOGE("Called to write out %d bits, but on bit %d, gpio link "
                        "was NULL.",
                        len, (i * 8 + j));

                return -EINVAL;
            }

            val = gpio_get_input(gpio->gpio_sys, curr_gpio->id);
            val <<= j;
            data[i] &= ~BIT(j);
            data[i] |= val;
        }
    }

    return len;
}

static int
tegra_gpio_write(gpio_t* gpio, const char* data, int len)
{
    gpio_t *curr_gpio = gpio;
    const int nbytes = DIV_ROUND_UP(len, 8);

    for (int i = 0; i < nbytes ; i++) {
        const int nbits = ((len - i * 8) / 8) ? 8 : (len % 8);

        for (int j = 0; j < nbits; j++, curr_gpio = curr_gpio->next) {
            if (curr_gpio == NULL) {
                ZF_LOGE("Called to write out %d bits, but on bit %d, gpio link "
                        "was NULL.",
                        len, (i * 8 + j));

                return -EINVAL;
            }

            gpio_set_level(gpio->gpio_sys, curr_gpio->id,
                           !!(data[i] & BIT(j)));
        }
    }

    return len;
}

static int tegra_gpio_init(gpio_sys_t *gpio_sys, int id, enum gpio_dir dir, gpio_t* gpio)
{
    int error;

    ZF_LOGV("Configuring GPIO pin %d", id);

    gpio->id = id;
    gpio->gpio_sys = gpio_sys;
    gpio->next = NULL;

    gpio->set_next = &tegra_gpio_set_next;

    error = gpio_set_pad_mode(gpio_sys, id, GPIO_MODE, dir);
    if (error != 0) {
        return error;
    }

    if (dir == GPIO_DIR_IN
        || dir == GPIO_DIR_OUT_DEFAULT_HIGH || dir == GPIO_DIR_OUT_DEFAULT_LOW) {
        error = gpio_set_direction(gpio_sys, id, dir);
        if (error != 0) {
            return error;
        }
    } else {
        gpio_set_interrupt_type(gpio_sys, id, dir);
    }

    /* Default to disabled IRQ state. The caller should have to call
     * gpio_int_enable() explicitly to enable the IRQ signal for the pin, if the
     * pin is configured to be an IRQ pin.
     *
     * The caller should also explicitly set the default output value if the pin
     * is an input/output pin.
     */
    gpio_interrupt_enable(gpio_sys, id, 0);
    return 0;
}

static int
tegra_gpio_int_enable_disable(gpio_t *gpio, bool enable)
{
    gpio_interrupt_enable(gpio->gpio_sys, gpio->id, enable);
    return 0;
}

int
gpio_init(volatile void *vaddr, gpio_sys_t *gpio_sys)
{
    if (gpio_sys == NULL) {
        return -ENOSYS;
    }
    if (vaddr == NULL) {
        return -ENOSYS;
    }

    ZF_LOGV("vaddr: %p", vaddr);

    gpio_sys->read = &tegra_gpio_read;
    gpio_sys->write = &tegra_gpio_write;
    gpio_sys->pending_status = &tegra_pending_status;
    gpio_sys->irq_enable_disable = &tegra_gpio_int_enable_disable;
    gpio_sys->init = &tegra_gpio_init;

    gpio_sys->priv = (void *)vaddr;
    return 0;
}

int gpio_sys_init(ps_io_ops_t *io_ops, gpio_sys_t* gpio_sys)
{
    if (io_ops == NULL) {
        return -ENOSYS;
    }
    if (gpio_sys == NULL) {
        return -ENOSYS;
    }

    MAP_IF_NULL(io_ops, TK1_GPIO, gpio_sys->priv);
    if (gpio_sys->priv == NULL) {
        ZF_LOGE("Failed to map TK1 GPIO frame.");
        return -1;
    }

    return gpio_init(gpio_sys->priv, gpio_sys);
}

int
gpio_set_pad_mode(gpio_sys_t *gpio_sys,
                  enum gpio_pin gpio, enum gpio_pad_mode mode, enum gpio_dir dir)
{
    uint32_t            val, out_val;
    volatile uint32_t   *reg_vaddr = get_controller_register(gpio_sys,
                                                             gpio, GPIO_CNF);
    volatile uint32_t   *out_reg_vaddr = get_controller_register(gpio_sys,
                                                             gpio, GPIO_OUT);

    ZF_LOGV("Offset: 0x%x, controller offset: 0x%x, port offset: 0x%x, "
            "GPIO_CNF: 0x%x, gpio: %d, bank: %d, port: %d, gpio_bit: %d"
            "Mode %d.\n",
            tegra_gpio_controller[GPIO_BANK(gpio)]
                + tegra_gpio_port[GPIO_PORT(gpio)]
                + GPIO_CNF,
            tegra_gpio_controller[GPIO_BANK(gpio)],
            tegra_gpio_port[GPIO_PORT(gpio)],
            GPIO_CNF, gpio, GPIO_BANK(gpio), GPIO_PORT(gpio), GPIO_BIT(gpio),
            mode);

    /* See the comment in gpio_set_direction() above: ability to change the
     * mode of a pin from SFIO to GPIO mode requires us to change bits in the
     * CNF register. But the CNF register is locked off if the "lock" bit is
     * set.
     *
     * So we must first test the lock bit to know if we can fulfill the user's
     * request at all. If we can't, we return early with an error.
     */

    val = *reg_vaddr & CNFREG_VALID_BITS_MASK;
    switch (mode) {
    case GPIO_MODE:
        /* We need to support a transition to GPIO mode while ensuring that
         * the initialization preserves a certain output value (DEFAULT_HIGH vs
         * DEFAULT_LOW).
         */
        out_val = *out_reg_vaddr & REG_VALID_BITS_MASK;
        switch (dir) {
        case GPIO_DIR_OUT_DEFAULT_HIGH:
            out_val |= BIT(GPIO_BIT(gpio));
            break;
        case GPIO_DIR_OUT_DEFAULT_LOW:
            out_val &= ~(BIT(GPIO_BIT(gpio)));
            break;
        default:
            break;
        }
        *out_reg_vaddr = out_val;

        if (val & BIT(GPIO_BIT(gpio))) {
            /* If it's already setup the way the user requested, just return
             * success early.
             */
            return 0;
        }
        if (val & BIT(GPIO_BIT(gpio) + PORT_NBITS)) {
            /* If it's not setup the way the user wants, but the lock bit is
             * also set, then we cannot reconfigure the pin. Return error.
             */
            ZF_LOGW("Pin %d: Failed to set to GPIO mode. Pin is locked.", gpio);
            return -1;
        }

        val |= BIT(GPIO_BIT(gpio));
        break;

    case SFIO_MODE:
        if ((val & BIT(GPIO_BIT(gpio))) == 0) {
            return 0;
        }
        if (val & BIT(GPIO_BIT(gpio) + PORT_NBITS)) {
            ZF_LOGW("Pin %d: Failed to set to GPIO mode. Pin is locked.", gpio);
            return -1;
        }

        val &= ~(BIT(GPIO_BIT(gpio)));
        break;

    default:
        ZF_LOGF("GPIO: pin %d, invalid mode %d", gpio, mode);
    }

    ZF_LOGV("Reg value: 0x%x", val);
    *reg_vaddr = val;

    assert(*reg_vaddr == val);
    return 0;
}
