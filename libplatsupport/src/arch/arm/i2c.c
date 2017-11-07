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

#include <platsupport/i2c.h>
#include <utils/util.h>
#include <assert.h>

/**
 * Register based I2C device
 * We use copyin/out in order to automatically fix endianess and
 * also because a write operation must be a contiguous stream of register
 * address then data.
 */
#define ABS(x) ( ((x) < 0)? -x : x )

#define BUFFER_SIZE 128

/* Copy from data into buf while fixing endianess */
static void
_fill_data(char* buf, const char* data, enum kvfmt fmt, int count)
{
    int bytes = ABS(fmt);
    int i, j;
    for (j = 0; j < count; j++) {
        for (i = 0; i < bytes; i++) {
            int idx = (fmt > 0) ? i : (bytes - 1) - i;
            buf[idx] = data[i];
        }
        data += bytes;
        buf += bytes;
    }
}

/* Copy reg into buf with required endianess */
static int
_fill_reg(char* buf, uint64_t reg, enum kvfmt fmt)
{
    int i;
    int bytes = ABS(fmt);
    for (i = 0; i < bytes; i++, reg >>= 8) {
        int idx = (fmt > 0) ? i : (bytes - 1) - i;
        buf[idx] = reg & 0xff;
    }
    return i;
}

/* Read no more than count registers into data with correct endianess */
static int
_do_kvread(i2c_slave_t* i2c_slave, uint64_t reg, void* data, int count)
{
    int abytes, dbytes, bytes;
    char d[BUFFER_SIZE];
    assert(i2c_slave);
    abytes = ABS(i2c_slave->address_fmt);
    dbytes = ABS(i2c_slave->data_fmt);
    assert(abytes < BUFFER_SIZE && dbytes < BUFFER_SIZE);
    /* Limit the amount of data to read to fit our buffer */
    if (count * dbytes > BUFFER_SIZE) {
        count = BUFFER_SIZE / dbytes;
    }
    /* Send the register address */
    ZF_LOGD("Seek register 0x%02llx", reg);
    _fill_reg(d, reg, i2c_slave->address_fmt);
    bytes = i2c_slave_write(i2c_slave, d, abytes, NULL, NULL);
    if (bytes != abytes) {
        ZF_LOGD("Bus error");
        return -1;
    }
    /* Receive the reply */
    ZF_LOGD("Read register %d", dbytes * count);
    bytes = i2c_slave_read(i2c_slave, d, dbytes * count, NULL, NULL);
    if (bytes < 0) {
        ZF_LOGD("read error");
        return bytes;
    }
    if (bytes != dbytes * count) {
        ZF_LOGD("short read %d/%d", bytes, dbytes * count);
    }
    /* Fix endianess */
    count = bytes / dbytes;
    _fill_data(data, d, i2c_slave->data_fmt, count);
    return count;
}

/* Write no more than count registers from data */
static int
_do_kvwrite(i2c_slave_t* i2c_slave, uint64_t reg, const void* data, int count)
{
    int abytes, dbytes, bytes;
    char d[BUFFER_SIZE];
    assert(i2c_slave);
    abytes = ABS(i2c_slave->address_fmt);
    dbytes = ABS(i2c_slave->data_fmt);
    assert(abytes < BUFFER_SIZE && dbytes < BUFFER_SIZE);
    /* Limit the amount of data to fit our buffer */
    if (count * dbytes + abytes > BUFFER_SIZE) {
        count = (BUFFER_SIZE - abytes) / dbytes;
    }
    /* Set up the register address */
    ZF_LOGD("Seek register 0x%02llx", reg);
    _fill_reg(d, reg, i2c_slave->address_fmt);
    /* Load up the data */
    _fill_data(d + abytes, data, i2c_slave->data_fmt, count);
    /* Send the request */
    bytes = i2c_slave_write(i2c_slave, d, abytes + count * dbytes, NULL, NULL);
    if (bytes <= 0) {
        ZF_LOGD("Bus error (%d)", bytes);
        return bytes;
    }
    count = (bytes - abytes) / dbytes;
    return count;
}

int
i2c_kvslave_init(i2c_bus_t* i2c_bus, int address,
                 enum kvfmt afmt, enum kvfmt dfmt,
                 i2c_slave_t* i2c_slave)
{
    assert(i2c_slave);
    i2c_slave->bus = i2c_bus;
    i2c_slave->address = address;
    i2c_slave->data_fmt = dfmt;
    i2c_slave->address_fmt = afmt;
    return 0;
}

/* Loop until count registers have been read or an error occurs */
int
i2c_kvslave_read(i2c_slave_t* i2c_slave, uint64_t reg, void* vdata, int count)
{
    int dbytes = ABS(i2c_slave->data_fmt);
    char* data = (char*)vdata;
    int this = -1;
    int remain = count;
    /* For large reads, copyin/out requires that they be split reads */
    while (remain > 0) {
        this = _do_kvread(i2c_slave, reg, data, remain);
        if (this <= 0) {
            break;
        }
        data += dbytes * this;
        reg += dbytes * this;
        remain -= this;
    }
    return count - remain;
}

/* Loop until count registers have been written or an error occurs */
int
i2c_kvslave_write(i2c_slave_t* i2c_slave, uint64_t reg, const void* vdata, int count)
{
    int dbytes = ABS(i2c_slave->data_fmt);
    char* data = (char*)vdata;
    int this = 0;
    int remain = count;
    /* For large reads, copyin/out requires that they be split reads */
    while (remain > 0) {
        this = _do_kvwrite(i2c_slave, reg, data, remain);
        if (this <= 0) {
            break;
        }
        data += dbytes * this;
        remain -= this;
        reg += this;
    }
    return count - remain;
}

int
i2c_slave_read(i2c_slave_t* i2c_slave, void* data, int size, i2c_callback_fn cb, void* token)
{
    return i2c_mread(i2c_slave->bus, i2c_slave->address, data, size, cb, token);
}

int
i2c_slave_write(i2c_slave_t* i2c_slave, const void* data, int size, i2c_callback_fn cb, void* token)
{
    return i2c_mwrite(i2c_slave->bus, i2c_slave->address, data, size, cb, token);
}

int
i2c_slave_init(i2c_bus_t* i2c_bus, int address, i2c_slave_t* i2c_slave)
{
    return i2c_kvslave_init(i2c_bus, address, STREAM, STREAM, i2c_slave);
}

int
i2c_scan(i2c_bus_t* i2c_bus, int start, int* addr, int naddr)
{
    /* TODO:
     * This should use the I2C Device-ID command. Currently it just attempts to
     * read from each possible addressable ID on the bus.
     *
     * Ideally, it should still try each addressable ID in sequence, but it
     * should first send out the Device-ID command, then follow it up with the
     * current addressable ID, and repeat this in a loop.
     *
     * The Device-ID command was introduced in 2007, and is supported by devices
     * that support the I2C protocol from version 3 upwards.
     */
    int ret;
    int i;
    int count;
    char dummy[10];
    for (count = 0, i = start & ~0x1; i < 0x100 && count < naddr; i += 2) {
        ret = i2c_mread(i2c_bus, i, &dummy, 10, NULL, NULL);
        if (ret == 10) {
            *addr++ = i;
            count++;
        } else if (ret < 0) {
        } else {
            ZF_LOGE("Invalid response");
        }
    }
    return count;
}
