/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _PLATSUPPORT_I2C_H_
#define _PLATSUPPORT_I2C_H_

#include <platsupport/io.h>
#include <platsupport/plat/i2c.h>

/***********
 *** BUS ***
 ***********/

typedef struct i2c_bus i2c_bus_t;
/*** Master mode ***/

/**
 * Initialise an I2C bus
 * @param[in]  id      The id of the I2C bus to initialise
 * @param[in]  io_ops  A structure providing operations which this device
 *                     may perform for intialisation.
 * @param[out] i2c_bus A handle to the i2c bus driver for future calls
 * @return             0 on success
 */ 
int i2c_init(enum i2c_id id, ps_io_ops_t* io_ops, i2c_bus_t** i2c_bus);

/**
 * Signal an IRQ even to an I2C bus.
 * @param[in] dev The I2C bus that triggered the IRQ
 */
void i2c_handle_irq(i2c_bus_t* dev);

/**
 * Scan a bus for devices
 * @param[in]  i2c_bus The I2C bus to scan
 * @param[in]  start   The address at which to begin the scan.
 *                     The RW bit of the address should be set to 0.
 * @param[out] addr    On success, and when the value passed is not NULL,
 *                     This 
 * @param[in]  naddr   The maximum number of addresses to return. This
 *                     should be equal to the number of elements in the addr
 *                     array.
 * @return             -1 on failure, otherwise, returns the number of addresses
 *                     that were entered into the addr array.
 *                     If the return value equals @ref(naddr), one should 
 *                     repeat the call with @ref(start) adjusted appropraitly.
 */
int i2c_scan(i2c_bus_t* i2c_bus, int start, int* addr, int naddr);

/**
 * Read from a remote I2C slave
 * This function is not generally called directly. Instead, one should
 * initialise a slave device structure and perform the appropriate slave
 * device calls.
 * @param[in] i2c_bus A handle to the i2c bus driver
 * @param[in] addr    The slave device address. The RW bit of the address
 *                    should be set to 0.
 * @param[in] data    A address to store the recieved data
 * @param[in] size    The number of bytes to read
 * @return            The number of bytes read
 */ 
int i2c_mread(i2c_bus_t* i2c_bus, int addr, void* data, int size);

/**
 * Write to a remote I2C slave
 * This function is not generally called directly. Instead, one should
 * initialise a slave device structure and perform the appropriate slave
 * device calls.
 * @param[in] i2c_bus A handle to an i2c bus driver
 * @param[in] addr    The slave device address. The RW bit of the address
 *                    should be set to 0.
 * @param[in] data    The address of the data to send
 * @param[in] size    The number of bytes to send
 * @return            The number of bytes sent
 */ 
int i2c_mwrite(i2c_bus_t* i2c_bus, int addr, const void* data, int size);

/*** Slave mode ***/

/**
 * Set the chip address of the bus for slave mode
 * @param[in] i2c_bus  A handle to an i2c bus driver
 * @param[in] address  The address to assign to this bus. The RW bit of 
 *                     the address should be set to 0.
 * @return             0 on success
 */
int i2c_set_address(i2c_bus_t* i2c_bus, int address);

/**
 * Read from a remote I2C master
 * @param[in] i2c_bus A handle to an i2c bus driver
 * @param[in] data    A address to store the recieved data
 * @param[in] size    The number of bytes to read
 * @return            The number of bytes read
 */ 
int i2c_read(i2c_bus_t* i2c_bus, void* data, int len);

/**
 * Write to a remote I2C master
 * @param[in] i2c_bus A handle to an i2c bus driver
 * @param[in] data    The address of the data to send
 * @param[in] size    The number of bytes to send
 * @return            The number of bytes sent
 */ 
int i2c_write(i2c_bus_t* i2c_bus, const void* data, int size);


/*********************
 *** Remote device ***
 *********************/

enum kvfmt {
    BIG64    = -8,
    BIG32    = -4,
    BIG16    = -2,
    BIG8     = -1,
    STREAM   =  0,
    LITTLE8  = +1,
    LITTLE16 = +2,
    LITTLE32 = +4,
    LITTLE64 = +8
};

typedef struct i2c_slave i2c_slave_t;

struct i2c_slave {
    i2c_bus_t* bus;
    int        address;
    enum kvfmt data_fmt;
    enum kvfmt address_fmt;
};


/**** Key-Value device ****/

/**
 * Initilaise an register map I2C slave device
 * @param[in]  bus     The I2C bus that this device resides on
 * @param[in]  address The chip address of the slave device. The RW bit of
 *                     the address should be set to 0.
 * @param[in]  asize   The address size in bytes
 *                     Positive values indicate LITTLE_ENDIAN while
 *                     negative values indicate BIG_ENDIAN.
 * @param[in]  dsize   The data word size in bytes
 *                     Positive values indicate LITTLE_ENDIAN while
 *                     negative values indicate BIG_ENDIAN.
 * @param[out] dev     A slave device structure to initialise
 * @return             0 on success
 */
int i2c_kvslave_init(i2c_bus_t* i2c_bus, int address, 
                     enum kvfmt asize, enum kvfmt dsize, 
                     i2c_slave_t* i2c_slave);

/**
 * Read registers from a register map I2C slave device
 * The endianess, word size and address size will be considered for the
 * transfer. Note that this function accepts the number of registers as
 * an argument rather than the number of bytes. Note also that the data
 * argument has no type, but is expected to have a type that matches the
 * word size (ie, int8_t[], int16_t[], int32_t[] or int64_t[])
 * @param[in] i2c_slave  A handle to a I2C slave device to read from
 * @param[in] start      The register address to begin reading from
 * @param[in] data       The address at which to store the read data
 * @param[in] nregs      The number of registers to read
 * @return               The actual number of bytes read
 */
int i2c_kvslave_read(i2c_slave_t* i2c_slave, uint64_t start, void* data, int nregs);

/**
 * Write registers to a register map I2C slave device
 * The endianess, word size and address size will be considered for the
 * transfer. Note that this function accepts the number of registers as
 * an argument rather than the number of bytes. Note also that the data
 * argument has no type, but is expected to have a type that matches the
 * word size (ie, int8_t[], int16_t[], int32_t[] or int64_t[])
 * @param[in] i2c_slave  A handle to a I2C slave device to read from
 * @param[in] start      The register address to begin reading from
 * @param[in] data       An address whcih provides the data to be written
 * @param[in] nregs      The number of registers to write
 * @return               The actual number of bytes read
 */
int i2c_kvslave_write(i2c_slave_t* i2c_slave, uint64_t start, const void* data, int nregs);


/**** Streaming device ****/

/**
 * Initilaise a streaming I2C slave device
 * @param[in]  i2c_bus   A handle to the I2C bus that the device resides on 
 * @param[in]  address   The chip address of the slave device. The RW bit of
 *                       the address should be set to 0.
 * @param[out] i2c_slave A handle to an i2c slave device structure to initialise
 * @return               0 on success
 */
int i2c_slave_init(i2c_bus_t* i2c_bus, int address, i2c_slave_t* i2c_slave);


/**
 * Read from a streaming slave device
 * @param[in] i2c_slave  A handle to the I2C slave device to read from
 * @param[in] data       A address to read the data to
 * @param[in] size       The number of bytes to read
 * @return               The actual number of registers read
 */
int i2c_slave_read(i2c_slave_t* i2c_slave, void* data, int size);

/**
 * Write to a streaming slave device
 * @param[in] i2c_slave  A handle to the I2C slave device to write to
 * @param[in] data       The address of the data to be written
 * @param[in] size       The number of bytes to write
 * @return               The actual number of registers written
 */
int i2c_slave_write(i2c_slave_t* i2c_slave, const void* data, int size);

#endif /* _PLATSUPPORT_I2C_H_ */
