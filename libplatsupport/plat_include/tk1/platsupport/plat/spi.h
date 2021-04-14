/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

enum spi_id {
    SPI0 = 0,
};

int
tegra_spi_init(enum spi_id id, volatile void *base, spi_chipselect_fn cs_func,
               mux_sys_t *mux_sys, clock_sys_t *clock_sys,
               spi_bus_t **ret_spi_bus);
