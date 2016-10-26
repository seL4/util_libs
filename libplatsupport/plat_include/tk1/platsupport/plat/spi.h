/*
 * Copyright 2016, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */
#pragma once

#define SPI0_PADDR 0x7000D400
#define SPI1_PADDR 0x7000D600
#define SPI2_PADDR 0x7000D800
#define SPI3_PADDR 0x7000DA00
#define SPI4_PADDR 0x7000DC00
#define SPI5_PADDR 0x7000DE00

int 
transfer_data_test(spi_bus_t* spi_bus);

enum spi_id {
    SPI0 = 0,
//    SPI1,
//    SPI2,
//    SPI0_ISP,
//    SPI1_ISP,
//    NSPI,

//    SPI3 = SPI0_ISP,
//    SPI4 = SPI1_ISP
};

enum spi_cs_state {
    SPI_CS_ASSERT,
    SPI_CS_RELAX
};

int 
tegra_spi_init(enum spi_id id, volatile void* base,
                mux_sys_t* mux_sys, clock_sys_t* clock_sys,
                spi_bus_t** ret_spi_bus);

void
spi_cs(spi_bus_t* spi_bus, enum spi_cs_state state);
