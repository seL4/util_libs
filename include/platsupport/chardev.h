/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_CHARDEV_H__
#define __PLATSUPPORT_CHARDEV_H__

#include <platsupport/io.h>

struct ps_chardevice;
struct ps_clk;
typedef struct ps_chardevice ps_chardevice_t;

#include <platsupport/plat/serial.h>


enum chardev_status {
/// Transfer completed successfully
    CHARDEV_STAT_COMPLETE,
/// Transfer was truncated or cancelled by the remote
    CHARDEV_STAT_INCOMPLETE,
/// A transfer error occurred
    CHARDEV_STAT_ERROR,
/// The transfer was aborted by the user
    CHARDEV_STAT_CANCELLED,
};

typedef void (*chardev_callback_t)(ps_chardevice_t* device, enum chardev_status stat, size_t bytes_transfered, void* token);

struct chardev_xmit_descriptor {
    /// A function to call when the transfer is complete
    chardev_callback_t callback;
    /// A token to pass unmodified to callback
    void* token;
    /// The number of bytes transfered thus far
    size_t bytes_transfered;
    /// The total number of bytes to transfer
    size_t bytes_requested;
    /// The source or destination for the data
    void* data;
};

struct ps_chardevice {
    /* identifier for the device */
    enum chardev_id id;
    void* vaddr;
    /* Character operations for this device */
    ssize_t (*read)(ps_chardevice_t* device, void* data, size_t bytes, chardev_callback_t cb, void* token);
    ssize_t (*write)(ps_chardevice_t* device, const void* data, size_t bytes, chardev_callback_t cb, void* token);
    void (*handle_irq)(ps_chardevice_t* device);
    /* array of irqs associated with this device */
    const int *irqs;
    /// Transmit transfer data for use with IRQs
    struct chardev_xmit_descriptor read_descriptor;
    /// Receive transfer data for use with IRQs
    struct chardev_xmit_descriptor write_descriptor;
    /* Input clock for this device */
    struct ps_clk* clk;
    /* OS specific memory operations */
    ps_io_ops_t ioops;
};

/*
 * Initialiase a device
 * @param  id: the id of the character device
 * @param ops: a structure containing OS specific operations for memory access
 * @param dev: a character device structure to populate
 * @return   : NULL on error, otherwise returns the device structure pointer
 */
ps_chardevice_t* ps_cdev_init(enum chardev_id id,
                              const ps_io_ops_t* ops,
                              ps_chardevice_t* dev);

/*
 * Create a pseudo device: initialise with nop function pointers
 * @param o: a structure containing OS specific operations for memory access
 * @param d: a character device structure to populate
 * @return   : NULL on error, otherwise returns the device structure pointer
 */
ps_chardevice_t* ps_cdev_new(const ps_io_ops_t* o,
                             ps_chardevice_t* d);


/**
 * Send a character to the device. New lines will be automatically be chased
 * by a character return
 * @param[in] d    The character device to send a character to
 * @param[in] c    The character to send
 */
static inline void ps_cdev_putchar(ps_chardevice_t* d, int c)
{
    int ret;
    do {
        char data = c;
        ret = d->write(d, &data, 1, NULL, NULL);
    } while (ret < 1);
}

/**
 * Receive a character from the device
 * @param[in] d  The device to receive a character from
 * @return       The chracter received; negative values signal that an error occurred.
 */
static inline int ps_cdev_getchar(ps_chardevice_t* d)
{
    int ret;
    char data;
    ret = d->read(d, &data, 1, NULL, NULL);
    return (ret == 1) ? data : EOF;
}

/**
 * Read data from a device
 * @param[in]  d        The device to read data from
 * @param[out] data     The location to store the read data to
 * @param[in]  size     The number of bytes to read
 * @param[in]  callback Optional: A function to call when the requested number of
 *                      bytes have been read. The caller must periodically call
 *                      the IRQ handler to satisfy the request.
 * @param[in]  token    An anonymous pointer to pass, unmodified, to the provided
 *                      callback function.
 * @return              Returns the number of bytes read on succes, negative
 *                      values reprents an error. If a callback function is
 *                      provided, a return value of 0 will represent success
 *                      If no callback funtion is provided, this function will
 *                      read data from any internal fifos to meet the the request
 *                      and then return. It will not block until the requested
 *                      number of bytes are available.
 */
static inline ssize_t ps_cdev_read(ps_chardevice_t* d, void* data, size_t size,
                                   chardev_callback_t callback, void* token)
{
    return d->read(d, data, size, callback, token);
}

/**
 * Write data to a device
 * @param[in]  d        The device to write data to
 * @param[out] data     The location of the data to be written
 * @param[in]  size     The number of bytes to write
 * @param[in]  callback Optional: A function to call when the requested number of
 *                      bytes have been written. The caller must periodically call
 *                      the IRQ handler to satisfy the request.
 * @param[in]  token    An anonymous pointer to pass, unmodified, to the provided
 *                      callback function.
 * @return              Returns the number of bytes written on succes, negative
 *                      values reprents an error. If a callback function is
 *                      provided, a return value of 0 will represent success
 *                      If no callback funtion is provided, this function will
 *                      write data to any internal fifos to meet the the request
 *                      and then return. It will not block until the requested
 *                      number of bytes have been written.
 */
static inline ssize_t ps_cdev_write(ps_chardevice_t* d, void* data, size_t size,
                                    chardev_callback_t callback, void* token)
{
    return d->write(d, data, size, callback, token);
}

/**
 * Pass control to the devices IRQ handler
 * @param[in] The device to pass control to
 * @param[in] The physical IRQ number that triggered the event
 */
static inline void ps_cdev_handle_irq(ps_chardevice_t* d, int irq UNUSED)
{
    d->handle_irq(d);
}

/**
 * Check if the given device emits the given IRQ
 * @param[in] d   The device to query
 * @param[in] irq An irq number
 * @return        non-zero if the device will produce the given IRQ
 */
static inline int ps_cdev_produces_irq(const ps_chardevice_t* d, int irq)
{
    int i;
    for (i = 0; d->irqs[i] != -1; i++) {
        if (d->irqs[i] == irq) {
            return 1;
        }
    }
    return 0;
}

#endif /* __PLATSUPPORT_CHARDEV_H__ */
