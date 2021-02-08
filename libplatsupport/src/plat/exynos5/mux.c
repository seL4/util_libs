/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "../../mach/exynos/mux.h"
#include <platsupport/gpio.h>
#include <platsupport/plat/gpio.h>

/* I2C */
static struct mux_feature_data i2c0_data[] = {
    { .port = GPB3, .pin = 1, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV1X)},
    { .port = GPB3, .pin = 0, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV1X)},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data i2c1_data[] = {
    { .port = GPB3, .pin = 3, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV1X)},
    { .port = GPB3, .pin = 2, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV1X)},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data i2c2_data[] = {
    { .port = GPA0, .pin = 7, .value = MUXVALUE_CPD(3, PUD_PULLUP, DRV1X)},
    { .port = GPA0, .pin = 6, .value = MUXVALUE_CPD(3, PUD_PULLUP, DRV1X)},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data i2c3_data[] = {
    { .port = GPA1, .pin = 3, .value = MUXVALUE_CPD(3, PUD_PULLUP, DRV1X)},
    { .port = GPA1, .pin = 2, .value = MUXVALUE_CPD(3, PUD_PULLUP, DRV1X)},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data i2c4_data[] = {
    { .port = GPA2, .pin = 1, .value = MUXVALUE_CPD(3, PUD_PULLUP, DRV1X)},
    { .port = GPA2, .pin = 0, .value = MUXVALUE_CPD(3, PUD_PULLUP, DRV1X)},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data i2c5_data[] = {
    { .port = GPA2, .pin = 3, .value = MUXVALUE_CPD(3, PUD_PULLUP, DRV1X)},
    { .port = GPA2, .pin = 2, .value = MUXVALUE_CPD(3, PUD_PULLUP, DRV1X)},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data i2c6_data[] = {
    { .port = GPB1, .pin = 4, .value = MUXVALUE_CPD(4, PUD_PULLUP, DRV1X)},
    { .port = GPB1, .pin = 3, .value = MUXVALUE_CPD(4, PUD_PULLUP, DRV1X)},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data i2c7_data[] = {
    { .port = GPB2, .pin = 3, .value = MUXVALUE_CPD(3, PUD_PULLUP, DRV1X)},
    { .port = GPB2, .pin = 2, .value = MUXVALUE_CPD(3, PUD_PULLUP, DRV1X)},
    { .port = GPIOPORT_NONE }
};
/* UART */
#define UART_MUX_VAL 0x2
static struct mux_feature_data uart0_data[] = {
    { .port = GPA0, .pin = 0, .value = UART_MUX_VAL},
    { .port = GPA0, .pin = 1, .value = UART_MUX_VAL},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data uart0_flow_data[] = {
    { .port = GPA0, .pin = 2, .value = UART_MUX_VAL},
    { .port = GPA0, .pin = 3, .value = UART_MUX_VAL},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data uart1_data[] = {
    { .port = GPD0, .pin = 0, .value = UART_MUX_VAL},
    { .port = GPD0, .pin = 1, .value = UART_MUX_VAL},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data uart1_flow_data[] = {
    { .port = GPD0, .pin = 2, .value = UART_MUX_VAL},
    { .port = GPD0, .pin = 3, .value = UART_MUX_VAL},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data uart2_data[] = {
    { .port = GPA1, .pin = 0, .value = UART_MUX_VAL},
    { .port = GPA1, .pin = 1, .value = UART_MUX_VAL},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data uart2_flow_data[] = {
    { .port = GPA1, .pin = 2, .value = UART_MUX_VAL},
    { .port = GPA1, .pin = 3, .value = UART_MUX_VAL},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data uart3_data[] = {
    { .port = GPA1, .pin = 4, .value = UART_MUX_VAL},
    { .port = GPA1, .pin = 5, .value = UART_MUX_VAL},
    { .port = GPIOPORT_NONE }
};

/* SPI */
static struct mux_feature_data spi0_data[] = {
    { .port = GPA2, .pin = 0, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPA2, .pin = 1, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPA2, .pin = 2, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPA2, .pin = 3, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data spi1_data[] = {
    { .port = GPA2, .pin = 4, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPA2, .pin = 5, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPA2, .pin = 6, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPA2, .pin = 7, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data spi2_data[] = {
    { .port = GPB1, .pin = 1, .value = MUXVALUE_CPD(5, PUD_PULLUP, DRV3X)},
    { .port = GPB1, .pin = 2, .value = MUXVALUE_CPD(5, PUD_PULLUP, DRV3X)},
    { .port = GPB1, .pin = 3, .value = MUXVALUE_CPD(5, PUD_PULLUP, DRV3X)},
    { .port = GPB1, .pin = 4, .value = MUXVALUE_CPD(5, PUD_PULLUP, DRV3X)},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data spi0_isp_data[] = {
    { .port = GPF1, .pin = 0, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPF1, .pin = 1, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPF1, .pin = 2, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPF1, .pin = 3, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data spi1_isp_data[] = {
    { .port = GPF1, .pin = 4, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPF1, .pin = 5, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPF1, .pin = 6, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPF1, .pin = 7, .value = MUXVALUE_CPD(2, PUD_PULLUP, DRV3X)},
    { .port = GPIOPORT_NONE }
};

struct mux_feature_data* feature_data[] = {
    [MUX_I2C0]       = i2c0_data,
    [MUX_I2C1]       = i2c1_data,
    [MUX_I2C2]       = i2c2_data,
    [MUX_I2C3]       = i2c3_data,
    [MUX_I2C4]       = i2c4_data,
    [MUX_I2C5]       = i2c5_data,
    [MUX_I2C6]       = i2c6_data,
    [MUX_I2C7]       = i2c7_data,
    [MUX_UART0]      = uart0_data,
    [MUX_UART0_FLOW] = uart0_flow_data,
    [MUX_UART1]      = uart1_data,
    [MUX_UART1_FLOW] = uart1_flow_data,
    [MUX_UART2]      = uart2_data,
    [MUX_UART2_FLOW] = uart2_flow_data,
    [MUX_UART3]      = uart3_data,
    [MUX_SPI0]       = spi0_data,
    [MUX_SPI1]       = spi1_data,
    [MUX_SPI2]       = spi2_data,
    [MUX_SPI0_ISP]   = spi0_isp_data,
    [MUX_SPI1_ISP]   = spi1_isp_data
};
