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

#ifndef _PLATSUPPORT_SPI_H_
#define _PLATSUPPORT_SPI_H_

#include <platsupport/io.h>
#include <platsupport/clock.h>
#include <platsupport/gpio.h>

typedef struct spi_bus spi_bus_t;
typedef struct spi_slave_config spi_slave_config_t;

struct spi_slave_config {
    /// Device operation speed
    freq_t   speed_hz;
    /// Slave selection signal delay in microseconds
    uint32_t nss_udelay;
    /// Feedback/propagation delay in clock phases
    uint32_t fb_delay;
};

enum spi_cs_state {
    SPI_CS_ASSERT,
    SPI_CS_RELAX
};

/**
 * Function pointer to override chip select function.
 *
 * It is sometimes necessary to override the chip select functionality such
 * as when GPIO pins are used for chipselect.  This function pointer is passed
 * in when a SPI driver is initialised.  If NULL is passed then the driver's
 * default chip select behavior will occur.
 * @param  config  A pointer to the slave struct
 * @param  state   The release or assert cs state
 */
typedef void (*spi_chipselect_fn)(const spi_slave_config_t* cfg, int state);

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

/**
 * Configure the SPI bus to meet the slave device's requirement
 * @param[in] spi_bus  A handle to an SPI bus
 * @param[in] cfg      Slave configuration
 */
void spi_prepare_transfer(spi_bus_t* spi_bus, const spi_slave_config_t* cfg);

/**
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
