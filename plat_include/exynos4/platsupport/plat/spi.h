/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


#ifndef _PLATSUPPORT_PLAT_SPI_H_
#define _PLATSUPPORT_PLAT_SPI_H_

#ifndef _PLATSUPPORT_SPI_H_
#error This file should not be included directly
#endif

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

int exynos_spi_init(enum spi_id id, void* base,
                    mux_sys_t* mux_sys, clock_sys_t* clock_sys,
                    spi_bus_t** spi_bus);


#endif /* _PLATSUPPORT_PLAT_SPI_H_ */

