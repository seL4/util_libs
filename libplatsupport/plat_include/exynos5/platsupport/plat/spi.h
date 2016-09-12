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
#define EXYNOS_SPI0_IRQ        88
#define EXYNOS_SPI1_IRQ        89
#define EXYNOS_SPI2_IRQ        90
#define EXYNOS_SPI0_ISP_IRQ    91
#define EXYNOS_SPI1_ISP_IRQ    92

/* Physical addresses */
#define EXYNOS_SPI0_PADDR      0x12D20000
#define EXYNOS_SPI1_PADDR      0x12D30000
#define EXYNOS_SPI2_PADDR      0x12D40000
#define EXYNOS_SPI0_ISP_PADDR  0x131A0000
#define EXYNOS_SPI1_ISP_PADDR  0x131B0000

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
    SPI0_ISP,
    SPI1_ISP,
    NSPI,

    SPI3 = SPI0_ISP,
    SPI4 = SPI1_ISP
};

int exynos_spi_init(enum spi_id id, void* base,
                    mux_sys_t* mux_sys, clock_sys_t* clock_sys,
                    spi_bus_t** spi_bus);


#endif /* _PLATSUPPORT_PLAT_SPI_H_ */
