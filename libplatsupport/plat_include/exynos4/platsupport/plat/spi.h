/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* IRQS */
#define EXYNOS_SPI0_IRQ        98
#define EXYNOS_SPI1_IRQ        99
#define EXYNOS_SPI2_IRQ        100
#define EXYNOS_SPI0_ISP_IRQ    122
#define EXYNOS_SPI1_ISP_IRQ    127

/* Physical addresses */
#define EXYNOS_SPI0_PADDR      0x13920000
#define EXYNOS_SPI1_PADDR      0x13930000
#define EXYNOS_SPI2_PADDR      0x13940000
#define EXYNOS_SPI0_ISP_PADDR  0x121A0000
#define EXYNOS_SPI1_ISP_PADDR  0x121B0000

/* Sizes */
#define EXYNOS_SPIX_SIZE      0x1000
#define EXYNOS_SPI0_SIZE      EXYNOS_SPIX_SIZE
#define EXYNOS_SPI1_SIZE      EXYNOS_SPIX_SIZE
#define EXYNOS_SPI2_SIZE      EXYNOS_SPIX_SIZE
#define EXYNOS_SPI0_ISP_SIZE  EXYNOS_SPIX_SIZE
#define EXYNOS_SPI1_ISP_SIZE  EXYNOS_SPIX_SIZE

enum spi_id {
    SPI0,
    SPI1,
    SPI2,
    NSPI
};

int exynos_spi_init(enum spi_id id, void *base,
                    mux_sys_t *mux_sys, clock_sys_t *clock_sys,
                    spi_bus_t **spi_bus);

