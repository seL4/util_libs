/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _PLATSUPPORT_SPI_H_
#define _PLATSUPPORT_SPI_H_

#include <platsupport/io.h>

typedef struct spi_bus spi_bus_t;

#include <platsupport/plat/spi.h>

typedef void (*spi_callback_fn)(spi_bus_t* spi_bus, int status, void* token);


/**
 * Initialise an SPI bus
 * @param[in]  id      The id of the SPI bus to initialise
 * @param[in]  io_ops  A structure providing operations which this device
 *                     may perform for intialisation.
 * @param[out] spi_bus A handle to the spi bus driver for future calls
 * @return             0 on success
 */
int spi_init(enum spi_id id, ps_io_ops_t* io_ops, spi_bus_t** spi_bus);

/**
 * Set the speed of the SPI bus
 * @param[in] spi_bus  A handle to an SPI bus
 * @param[in] bps      The speed to set in bits per second.
 * @return             The actual speed set
 */
long spi_set_speed(spi_bus_t* spi_bus, long bps);

/**
 * Allow the driver to handle an incoming IRQ
 * @param[in] dev The SPI bus that triggered the IRQ
 */
void spi_handle_irq(spi_bus_t* dev);


/*
 * Write and read data to and from the SPI bus.
 * txdata will be sent from byte position 0 until txcnt bytes have been sent.
 * rxdata will be read once txcnt bytes have been sent and until rxcnt bytes have
 * been read.
 * @param[in] spi_bus  A handle to an SPI bus
 * @param[in] txdata   A reference to the data to be transmitted
 * @param[in] txcnt    The number of bytes to transmit
 * @param[in] rxdata   A reference to the location where data should be read into.
 *                     The buffer should be large enough to store TX echo data as
 *                     well as the data to be received. ie, if txcnt is 2, then
 *                     rxdata[0:1] will contain data returned by the TX cycle and
 *                     rxdata[2:] will contain the received data.
 * @param[in] rxcnt    The number of bytes to receive. This count only begins after
 *                     the required amount of data has been transmitted.
 * @param[in] cb       A callback function to call when all data has been read.
 *                     If the provided callback function is NULL, the call will block
 *                     until the transfer is complete
 * @param[in] token    A token to pass, unmodified, to the callback function.
 */
int spi_xfer(spi_bus_t* spi_bus, const void* txdata, size_t txcnt,
             void* rxdata, size_t rxcnt, spi_callback_fn cb, void* token);

#endif /* _PLATSUPPORT_SPI_H_ */

