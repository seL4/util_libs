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
#pragma once

enum spi_id {
    SPI0 = 0,
};

int
tegra_spi_init(enum spi_id id, volatile void* base, spi_chipselect_fn cs_func,
               mux_sys_t* mux_sys, clock_sys_t* clock_sys,
               spi_bus_t** ret_spi_bus);
