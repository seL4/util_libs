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

#include <stdint.h>

#include <utils/util.h>
#include <platsupport/io.h>

typedef struct i2c_bus i2c_bus_t;
#include <platsupport/plat/i2c.h>

/* For bit banged API */
#include <platsupport/gpio.h>

/***********
 *** BUS ***
 ***********/

/* Make sure to remember to SHL these constants by 1 when passing them
 * in as arguments to `i2c_slave_init()`, etc.
 */
/* General call must be sent out as a WRITE transaction. */
#define I2C_ADDR_GENERAL_CALL           0
/* Start byte must be sent out as a READ transaction. */
#define I2C_ADDR_BITBANG_START_BYTE     I2C_ADDR_GENERAL_CALL,
#define I2C_ADDR_CBUS                   1
#define I2C_ADDR_HISPEED_MASTER         0x4
/* Device-ID must be sent out first as a WRITE, then as a READ. */
#define I2C_ADDR_DEVICE_ID              0x3C
#define I2C_ADDR_10BIT                  0x38,

/* Use this macro to check whether or not an i2cstat value is an error value */
#define I2CSTAT_IS_ERROR(i2cstat)       ((i2cstat) < 0)
/* Use this macro to check whether or not an i2cstat value is NOT an error value */
#define I2CSTAT_OK(i2cstat)             (((i2cstat) == I2CSTAT_COMPLETE) || ((i2cstat) == I2CSTAT_LASTBYTE))
/* Use this to check whether or not an i2c transfer has been fully completed successfully */
#define I2CSTAT_COMPLETE(i2cstat)       ((i2cstat) == I2CSTAT_COMPLETE)

enum i2c_stat {
/// Transfer completed successfully
    I2CSTAT_COMPLETE = 0,
/// The last byte is about to be transfered. Call read/write if you wish to extend it.
    I2CSTAT_LASTBYTE,
/// Transfer was truncated or cancelled by the remote
    I2CSTAT_INCOMPLETE = INT_MIN,
/// A transfer error occurred
    I2CSTAT_ERROR,
/// The transfer was aborted by the user
    I2CSTAT_CANCELLED,
/// The slave sent NACK instead of ACK.
    I2CSTAT_NACK,
/// The slave lost its arbitration bid on the bus and the xfer must be retried.
    I2CSTAT_ARBITRATION_LOST
};

enum i2c_mode {
/// Idle
    I2CMODE_IDLE,
/// Receive mode
    I2CMODE_RX,
/// Transmit mode
    I2CMODE_TX
};

enum i2c_slave_address_size {
    I2C_SLAVE_ADDR_7BIT,
    I2C_SLAVE_ADDR_10BIT,
};

enum i2c_slave_speed {
    I2C_SLAVE_SPEED_STANDARD,
    I2C_SLAVE_SPEED_FAST,
    I2C_SLAVE_SPEED_FASTPLUS,
    I2C_SLAVE_SPEED_HIGHSPEED
};

/** Values for i2c_slave_t::i2c_opts.
 *
 * Pass these values to i2c_slave_init()'s "i2c_opts" argument.
 */
/* If a slave device doesn't send ACK after each byte, set this flag in its
 * slave data structure.
 */
#define I2C_SLAVE_OPTS_DEVICE_DOES_NOT_ACK     BIT(0)
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

/* I2C addresses in the ranges 0x0-0x7 are reserved.
 * I2C addresses in the ranges 0x78-0x7F are also reserved.
 */
#define I2C_VALID_ADDR_MIN      (0x8)
#define I2C_VALID_ADDR_MAX      (0x78)

static inline bool
i2c_is_valid_address(uint8_t id)
{
    return id >= I2C_VALID_ADDR_MIN && id < I2C_VALID_ADDR_MAX;
}

static inline int
i2c_extract_address(uint8_t id_with_rw_bit)
{
    return (id_with_rw_bit >> 1) & 0x7F;
}

/**
 * This callback is called when the I2C bus is addressed as a slave. The application
 * should respond by calling i2c_read or i2c_write with an appropriate transfer buffer
 * depending on the reported i2c_mode.
 */
typedef void (*i2c_aas_callback_fn)(i2c_bus_t* bus, enum i2c_mode, void* token);
/**
 * This callback is called when a transfer needs attention.
 * It will be called if the transfer is interrupted, just before the last byte is
 * transferred, and also when the transfer has completed. The catalyse for the call
 * to the callback is reported in i2c_stat.
 * If the I2C buffer for the transfer is not contiguous, the I2CSTAT_LASTBYTE status
 * can be used to replace the active transfer buffer and continue the transfer. To
 * achieve this, simply call i2c_read or i2c_write with a new buffer when the
 * I2CSTAT_LASTBYTE status callback is received. NOTE that in this case, the last byte
 * of the original buffer will not take part in the transfer. The reason for this
 * strange operation is due to the ACK policy of I2C transfers.
 */
typedef void (*i2c_callback_fn)(i2c_bus_t* bus, enum i2c_stat, size_t size, void* token);

/** I2C slave controller handle structure.
 *
 * You should initialize one of these using i2c_slave_init() or
 * i2c_kvslave_init() for each slave controller you intend to communicate with
 * on the bus.
 *
 * Then, use i2c_slave_read() and i2c_slave_write() or i2c_kvslave_read() and
 * i2c_kvslave_write() calls on this handle structure to communicate with the
 * desired slave controller.
 */
typedef struct i2c_slave i2c_slave_t;
struct i2c_slave {
    int (*slave_read)(i2c_slave_t* i2c_slave, void* data, size_t size,
                      bool end_with_repeat_start,
                      i2c_callback_fn cb, void* token);
    int (*slave_write)(i2c_slave_t* i2c_slave, const void* data, size_t size,
                       bool end_with_repeat_start,
                       i2c_callback_fn cb, void* token);

    i2c_bus_t* bus;
    int        address;
    enum i2c_slave_address_size address_size;
    enum i2c_slave_speed        max_speed;
    enum kvfmt data_fmt;
    enum kvfmt address_fmt;
    /* This "i2c_opts" member is not meant to be managed directly by the caller,
     * but it is set internally by the driver based on the "i2c_opts" argument
     * passed to "i2c_slave_init()".
     *
     * Please see the documentation for i2c_slave_init() to understand this
     * member.
     */
    uint32_t   i2c_opts;
};

/** I2C master controller handle structure.
 *
 * You should initialize one of these for each controller you intend to use in
 * master mode on the bus.
 */
struct i2c_bus {
    /* Master mode functions: To be used when the controller is in master
     * mode.
     *
     * To read and write from/to a target slave in master mode, please see the
     * i2c_slave_t related API functions further down below.
     *
     * You must initialize an i2c_slave_t structure for each slave using the
     * i2c_slave_init() or i2c_kvslave_init() API functions if you will be
     * communicating with slave devices.
     *
     * If you will be using the controller in slave mode, you should register
     * a slave mode event handler to receive notifications of slave mode events.
     */
    int (*slave_init)(i2c_bus_t* i2c_bus, int address,
                      enum i2c_slave_address_size address_size,
                      enum i2c_slave_speed max_speed,
                      uint32_t i2c_opts,
                      i2c_slave_t* i2c_slave);

    long (*set_speed)(i2c_bus_t* bus, enum i2c_slave_speed speed);
    /* Bus clear operation, for when the slave holds SDA line low. */
    int (*master_stop)(i2c_bus_t* bus);
    void (*set_hsmode_master_address)(i2c_bus_t* bus, int addr);
    /* Slave mode functions: To be used when the controller is in slave mode. */
    int (*read)(i2c_bus_t* bus, void* buf, size_t size, bool end_with_repeat_start,
                i2c_callback_fn cb, void* token);
    int (*write)(i2c_bus_t* bus, const void* buf, size_t size, bool end_with_repeat_start,
                 i2c_callback_fn cb, void* token);
    void (*register_slave_event_handler)(i2c_bus_t *bus,
                                         i2c_aas_callback_fn cb, void *token);

    int (*set_self_slave_address)(i2c_bus_t *bus, int addr);
    enum i2c_mode (*probe_aas)(i2c_bus_t* bus);

    void (*handle_irq)(i2c_bus_t* bus);

    i2c_callback_fn cb;
    void* token;
    i2c_aas_callback_fn aas_cb;
    void* aas_token;
    void* priv;
};

struct i2c_bb {
    gpio_id_t scl;
    gpio_id_t sda;
    int speed;
    gpio_sys_t* gpio_sys;
};

/**
 * Initialise an I2C bus
 * @param[in]  id      The id of the I2C bus to initialise
 * @param[in]  io_ops  A structure providing operations which this device
 *                     may perform for intialisation.
 * @param[out] i2c_bus A handle to the i2c bus driver for future calls
 * @return             0 on success
 */
int i2c_init(enum i2c_id id, ps_io_ops_t* io_ops, i2c_bus_t* i2c_bus);

/**
 * Initialise a bit-banged I2C bus
 * @param[in]  gpio_sys  A handle to a gpio subsystem. This handle must be valid while the bus is in use
 * @param[in]  scl       The GPIO ID of the SCL pin
 * @param[in]  sda       The GPIO ID of the SDA pin
 * @param[out] i2c_bb    A bit-banged i2c structure to populate. This caller is responsible for managing the memory
 *                       for this structure. The memory must be valid while the bus is in use.
 * @param[out] i2c_bus   A generic I2C bus structure to populate
 * @return               0 on success
 */
int i2c_bb_init(gpio_sys_t* gpio_sys, gpio_id_t scl, gpio_id_t sda, struct i2c_bb* i2c_bb, struct i2c_bus* i2c_bus);

/** Initialise an I2C slave device for "stream" reading and writing.
 *
 * This is the standard mode of use.
 * There are functions also provided below for treating I2C slaves as key-value
 * stores.
 *
 * @param[in]  i2c_bus   A handle to the I2C bus that the device resides on
 * @param[in]  address   The chip address of the slave device. The RW bit of
 *                       the address should be set to 0.
 * @param[in]  address_size The I2C hardware address bitlength: 7 or 10 bits.
 * @param[in]  max_speed    The maximum speed usable with the target slave
 *                          device. Device speed is one of: Standard, FM, FM+
 *                          and HS.
 * @param[out] i2c_slave A handle to an i2c slave device structure to initialise
 * @return               0 on success
 *
 * Values for i2c_slave_t::i2c_opts are #defined below.
 *
 * Pass these values to i2c_slave_init()'s "i2c_opts" argument.
 */
/* If a slave device doesn't send ACK after each byte, set this flag in its
 * slave data structure.
 */
#define I2C_SLAVE_OPTS_DEVICE_DOES_NOT_ACK     BIT(0)

int i2c_slave_init(i2c_bus_t* i2c_bus, int address,
                   enum i2c_slave_address_size address_size,
                   enum i2c_slave_speed max_speed,
                   uint32_t i2c_opts,
                   i2c_slave_t* i2c_slave);

/** Initialize an I2C slave device for key-value reading and writing.
 *
 * @param[in]  bus     The I2C bus that this device resides on
 * @param[in]  address The chip address of the slave device. The RW bit of
 *                     the address should be set to 0.
 * @param[in]  address_size The I2C hardware address bitlength: 7 or 10 bits.
 * @param[in]  max_speed    The maximum speed usable with the target slave
 *                          device. Device speed is one of: Standard, FM, FM+
 *                          and HS.
 * @param[in]  asize   The address size in bytes
 *                     Positive values indicate LITTLE_ENDIAN while
 *                     negative values indicate BIG_ENDIAN.
 * @param[in]  dsize   The data word size in bytes
 *                     Positive values indicate LITTLE_ENDIAN while
 *                     negative values indicate BIG_ENDIAN.
 * @param[out] i2c_slave A slave device structure to initialise
 * @return             0 on success
 */
int i2c_kvslave_init(i2c_bus_t* i2c_bus, int address,
                        enum i2c_slave_address_size address_size,
                        enum i2c_slave_speed max_speed,
                        enum kvfmt asize, enum kvfmt dsize,
                        i2c_slave_t* i2c_slave);

/** Initialize an I2C slave device for GPIO bit-banged reading and writing.
 *
 * Initializes metadata to treat a set of GPIO pins as if they are addressig
 * an I2C slave.
 *
 * @param[in]  bus     The I2C bus that this device resides on
 * @param[in]  address The chip address of the slave device. The RW bit of
 *                     the address should be set to 0.
 * @param[in]  address_size The I2C hardware address bitlength: 7 or 10 bits.
 * @param[in]  max_speed    The maximum speed usable with the target slave
 *                          device. Device speed is one of: Standard, FM, FM+
 *                          and HS.
 * @param[out] i2c_slave A slave device structure to initialise
 * @return             0 on success
 */
int i2c_bb_slave_init(i2c_bus_t* i2c_bus, int address,
                      enum i2c_slave_address_size address_size,
                      enum i2c_slave_speed max_speed,
                      i2c_slave_t* i2c_slave);

/**
 * Set the speed of the I2C bus
 * @param[in] i2c_bus  A handle to an I2C bus
 * @param[in] speed    One of the values in i2c_slave_speed: std, fast, fast+,
 *                     HS.
 * @return             The actual speed set
 */
static inline long i2c_set_speed(i2c_bus_t* i2c_bus, enum i2c_slave_speed speed)
{
    ZF_LOGF_IF(!i2c_bus, "Handle to I2C controller not supplied!");
    ZF_LOGF_IF(!i2c_bus->set_speed, "Unimplemented!");
    return i2c_bus->set_speed(i2c_bus, speed);
}

/**
 * Signal an IRQ event to an I2C bus.
 * @param[in] i2c_bus The I2C bus that triggered the IRQ
 */
static inline void i2c_handle_irq(i2c_bus_t* i2c_bus)
{
    ZF_LOGF_IF(!i2c_bus, "Handle to I2C controller not supplied!");
    ZF_LOGF_IF(!i2c_bus->handle_irq, "Unimplemented!");
    i2c_bus->handle_irq(i2c_bus);
}

/** Registers an event handler for slave event notifications.
 *
 * Such notifications are usually a mode change from read to write, or from
 * write to read when the remote master decides to request such a change.
 *
 * See i2c_aas_callback_fb above.
 * @param[in] bus       Initialized I2C bus controller handle.
 * @param[in] cb        Callback which will be called when there is an I2C event
 *                      to notify the client application of.
 * @param[in] token     Client-specific data that the driver returns unchanged.
 */
static inline void
i2c_register_slave_event_handler(i2c_bus_t *bus,
                                 i2c_aas_callback_fn cb, void *token)
{
    ZF_LOGF_IF(!bus, "Handle to I2C controller not supplied!");
    ZF_LOGF_IF(!bus->register_slave_event_handler, "Unimplemented!");
    bus->register_slave_event_handler(bus, cb, token);
}

/** Assign slave mode address to the I2C controller.
 *
 * Assign an address to the I2C controller which it should respond to when
 * acting as a slave.
 *
 * This function should probably use the I2C General Call procedure, and then
 * program the new I2C address using some platform-specific configuration,
 * perhaps via firmware or GPIO.
 *
 * @param[in] i2c_bus   A handle to an i2c bus driver
 * @param[in] addr      The address to assign to this bus. The RW bit of
 *                      the address should be set to 0.
 * @return              0 on success
 */
static inline int
i2c_set_self_slave_address(i2c_bus_t *bus, int addr)
{
    ZF_LOGF_IF(!bus, "Handle to I2C controller not supplied!");
    ZF_LOGF_IF(!bus->set_self_slave_address, "Unimplemented!");
    ZF_LOGF_IF(!i2c_is_valid_address(i2c_extract_address(addr)), "I2C address "
               "input is invalid!");

    return bus->set_self_slave_address(bus, addr);
}

/**
 * Set the I2C controller's 3-bit master address for high-speed mode. If you
 * set the speed of the bus to high-speed mode, you should also call this
 * function to set the HS-master address of the I2C controller.
 *
 * @param[in] i2c_bus   A handle to an i2c bus driver
 * @param[in] addr      The address to assign to this bus. The RW bit of
 *                      the address should be set to 0.
 * @param[in] aas_cb    A callback function to call when the slave is addressed
 * @param[in] aas_token A token to pass, unmodified to the provided callback function
 */
static inline void
i2c_set_hsmode_master_address(i2c_bus_t* i2c_bus, int addr)
{
    ZF_LOGF_IF(!i2c_bus, "Handle to I2C controller not supplied!");
    ZF_LOGF_IF(!i2c_bus->set_hsmode_master_address, "Unimplemented!");
    return i2c_bus->set_hsmode_master_address(i2c_bus, addr);
}

/*** Slave mode ***/

/**
 * While operating as a slave, read from a remote I2C master
 * @param[in] i2c_bus A handle to an i2c bus driver
 * @param[in] data    A address to store the recieved data
 * @param[in] size    The number of bytes to read
 * @param[in] cb      A callback to call when the transfer has finished.
 * @param[in] token   A token to pass unmodified to the registered callback
 * @return            The number of bytes read
 */
static inline int i2c_read(i2c_bus_t* i2c_bus, void* data, size_t size,
                           bool end_with_repeat_start,
                           i2c_callback_fn cb, void* token)
{
    ZF_LOGF_IF(!i2c_bus, "Handle to I2C controller not supplied!");
    ZF_LOGF_IF(!i2c_bus->read, "Unimplemented!");
    return i2c_bus->read(i2c_bus, data, size, end_with_repeat_start, cb, token);
}

/**
 * While operating as a slave, write to a remote I2C master
 * @param[in] i2c_bus A handle to an i2c bus driver
 * @param[in] data    The address of the data to send
 * @param[in] size    The number of bytes to send
 * @param[in] cb      A callback to call when the transfer has finished.
 * @param[in] token   A token to pass unmodified to the registered callback
 * @return            The number of bytes sent
 */
static inline int i2c_write(i2c_bus_t* i2c_bus, const void* data, size_t size,
                            bool end_with_repeat_start,
                            i2c_callback_fn cb, void* token)
{
    ZF_LOGF_IF(!i2c_bus, "Handle to I2C controller not supplied!");
    ZF_LOGF_IF(!i2c_bus->write, "Unimplemented!");
    return i2c_bus->write(i2c_bus, data, size, end_with_repeat_start, cb, token);
}

/**
 * Determine if the I2C bus received the assigned slave address
 * This function should only be used in polling mode and should be considered to
 * be edge triggered (mode is only reported at the beginning of a slave transfer).
 * @param[in] i2c_bus  A handle to the bus to probe
 * @return             The mode in which the bus was addressed, or I2CMODE_IDLE if
 *                     the bus was not addressed in the last byte received.
 */
static inline enum i2c_mode i2c_probe_aas(i2c_bus_t* i2c_bus)
{
    ZF_LOGF_IF(!i2c_bus, "Handle to I2C controller not supplied!");
    ZF_LOGF_IF(!i2c_bus->probe_aas, "Unimplmented!");
    return i2c_bus->probe_aas(i2c_bus);
}

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



/*****************************
 *** Slave controller APIs ***
 *****************************/

/**** Key-Value device ****/

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
 * Read from a streaming slave device
 * @param[in] i2c_slave  A handle to the I2C slave device to read from
 * @param[in] data       A address to read the data to
 * @param[in] size       The number of bytes to read
 * @param[in] cb         The callback which is called when write is complete
 * @param[in] token      The token that the callback returns
 * @return               The actual number of registers read
 */
static inline int
i2c_slave_read(i2c_slave_t* i2c_slave, void* data, size_t size,
               bool end_with_repeat_start,
               i2c_callback_fn cb, void* token)
{
    ZF_LOGF_IF(!i2c_slave, "Handle to I2C slave info not supplied!");
    ZF_LOGF_IF(!i2c_slave->bus, "I2C slave's parent bus not filled out!");
    ZF_LOGF_IF(!i2c_slave->slave_read, "Unimplemented!");
    return i2c_slave->slave_read(i2c_slave, data, size, end_with_repeat_start,
                                 cb, token);
}

/**
 * Write to a streaming slave device
 * @param[in] i2c_slave  A handle to the I2C slave device to write to
 * @param[in] data       The address of the data to be written
 * @param[in] size       The number of bytes to write
 * @param[in] cb         The callback which is called when write is complete
 * @param[in] token      The token that the callback returns
 * @return               The actual number of registers written
 */
static inline int
i2c_slave_write(i2c_slave_t* i2c_slave, const void* data, int size,
                bool end_with_repeat_start,
                i2c_callback_fn cb, void* token)
{
    ZF_LOGF_IF(!i2c_slave, "Handle to I2C slave info not supplied!");
    ZF_LOGF_IF(!i2c_slave->bus, "I2C slave's parent bus not filled out!");
    ZF_LOGF_IF(!i2c_slave->slave_write, "Unimplemented!");
    return i2c_slave->slave_write(i2c_slave, data, size, end_with_repeat_start,
                                  cb, token);
}

