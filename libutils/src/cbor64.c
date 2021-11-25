/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <utils/cbor64.h>

/* Generate the initial byte indicating the type of the following data */
int cbor64_initial_byte(base64_t *streamer, cbor64_mt_t type, uint8_t data)
{
    return base64_putbyte(streamer, (type << 5) | (data & MASK(5)));
}

/* Send a break byte to terminate indefinite-length item */
int cbor64_send_break(base64_t *streamer)
{
    return cbor64_initial_byte(streamer, CBOR64_MT_BREAK, CBOR64_AI_INDEFINITE_LENGTH);
}

int cbor64_send_item(base64_t *streamer, cbor64_mt_t type, uint64_t number)
{
    uint8_t additional_info;
    size_t size;

    if (number < CBOR64_AI_INT_LITERAL_MAX) {
        /* Encode number in item byte */
        additional_info = number;
        size = 0;
    } else if (number < LLBIT(8)) {
        /* Encode number as uint8_t */
        additional_info = CBOR64_AI_UINT8_T;
        size = 8;
    } else if (number < LLBIT(16)) {
        /* Encode number as uint16_t */
        additional_info = CBOR64_AI_UINT16_T;
        size = 16;
    } else if (number < LLBIT(32)) {
        /* Encode number as uint32_t */
        additional_info = CBOR64_AI_UINT32_T;
        size = 32;
    } else {
        /* Encode number as uint64_t */
        additional_info = CBOR64_AI_UINT64_T;
        size = 64;
    }

    int err = cbor64_initial_byte(streamer, type, additional_info);
    if (err != 0) {
        return err;
    }

    while (size > 0) {
        size -= 8;
        err = base64_putbyte(streamer, (number >> size) & MASK(8));
        if (err != 0) {
            return err;
        }
    }

    return err;
}


/* Send a bytearray */
int cbor64_send_typed_bytes(base64_t *streamer, cbor64_mt_t type, unsigned char *buffer, size_t length)
{
    int err = cbor64_send_item(streamer, type, length);
    if (err != 0) {
        return err;
    }

    while (length > 0) {
        err = base64_putbyte(streamer, *buffer);
        if (err != 0) {
            return err;
        }

        length -= 1;
        buffer += 1;
    }

    return 0;
}

/* Send a special value in one or two bytes */
int cbor64_send_simple(base64_t *streamer, cbor64_simple_t value)
{
    if ((uint8_t)value < CBOR64_AI_SIMPLE_BYTE) {
        return cbor64_initial_byte(streamer, CBOR64_MT_SIMPLE, value);
    } else {
        int err = cbor64_initial_byte(streamer, CBOR64_MT_SIMPLE, CBOR64_AI_SIMPLE_BYTE);
        if (err == 0) {
            err = base64_putbyte(streamer, value);
        }
        return err;
    }
}

static inline int send_endian_bytes(base64_t *streamer, unsigned char *bytes, size_t length)
{
    for (unsigned int b = 0; b < length; b += 1) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        int err = base64_putbyte(streamer, bytes[length - (b + 1)]);
#else
        int err = base64_putbyte(streamer, bytes[b]);
#endif
        if (err != 0) {
            return err;
        }
    }

    return 0;
}

/* Send a single-precision float value */
int cbor64_float(base64_t *streamer, float number)
{
    int err = cbor64_initial_byte(streamer, CBOR64_MT_FLOAT, CBOR64_AI_FLOAT32_T);
    if (err == 0) {
        err = send_endian_bytes(streamer, (void *)&number, 4);
    }
    return err;
}

/* Send a double-precision float value */
int cbor64_double(base64_t *streamer, double number)
{
    int err = cbor64_initial_byte(streamer, CBOR64_MT_FLOAT, CBOR64_AI_FLOAT64_T);
    if (err == 0) {
        err = send_endian_bytes(streamer, (void *)&number, 8);
    }
    return err;
}

/* Find the index for a given string */
static size_t find_reference(cbor64_domain_t *domain, char *string)
{
    size_t index = 0;
    while (domain->strings[index] != NULL) {
        if (string == domain->strings[index] || strcmp(string, domain->strings[index]) == 0) {
            /* Found matching string */
            break;
        }
        index += 1;
    }

    return index;
}

/*
 * Emit a new string and update the array
 */
static int new_reference(base64_t *streamer, cbor64_domain_t *domain, size_t index)
{
    /*
     * If the string reference would be no less than the raw string
     * encoding, don't actually track the string in the references.
     */
    size_t length = strlen(domain->strings[index]);
    uint64_t next_ref = domain->emitted;

    bool is_referenced = true;
    if (next_ref < 24) {
        is_referenced = length >= 3;
    } else if (next_ref < BIT(8)) {
        is_referenced = length >= 4;
    } else if (next_ref < BIT(16)) {
        is_referenced = length >= 5;
    } else if (next_ref < LLBIT(32)) {
        is_referenced = length >= 7;
    } else {
        is_referenced = length >= 11;
    }

    if (is_referenced) {
        char *temp = domain->strings[next_ref];
        domain->strings[next_ref] = domain->strings[index];
        domain->strings[index] = temp;
        domain->emitted += 1;

        if (domain->shared_values) {
            return cbor64_tag(streamer, CBOR64_TAG_SHAREABLE);
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

typedef struct {
    bool emit_literal;
    int error;
} emit_reference_ret_t;

/*
 * Emit a reference to a string
 */
static emit_reference_ret_t emit_reference(base64_t *streamer, cbor64_domain_t *domain, char *string)
{
    size_t index = find_reference(domain, string);
    emit_reference_ret_t ret = {
        .emit_literal = false,
        .error = 0,
    };

    if (domain->strings[index] == NULL) {
        /* String not in index, just emit it its own namespace */
        if (!domain->shared_values) {
            ret.error = cbor64_tag(streamer, CBOR64_TAG_STRING_REF_DOMAIN);
        }
        ret.emit_literal = true;
    } else if (index < domain->emitted) {
        /* String already emitted, emit reference */
        if (domain->shared_values) {
            ret.error = cbor64_tag(streamer, CBOR64_TAG_SHARED_VALUE);
        } else {
            ret.error = cbor64_tag(streamer, CBOR64_TAG_STRING_REF);
        }
        if (ret.error == 0) {
            ret.error = cbor64_int(streamer, index);
        }
    } else {
        /* Create a new reference */
        ret.error = new_reference(streamer, domain, index);
        ret.emit_literal = true;
    }

    return ret;
}

/*
 * Emit a string reference
 */
int cbor64_string_ref(base64_t *streamer, cbor64_domain_t *domain, char *string)
{
    emit_reference_ret_t ret = emit_reference(streamer, domain, string);
    if (ret.error == 0 && ret.emit_literal) {
        ret.error = cbor64_string(streamer, string);
    }
    return ret.error;
}

/*
 * Emit a utf8 reference
 */
int cbor64_utf8_ref(base64_t *streamer, cbor64_domain_t *domain, char *string)
{
    emit_reference_ret_t ret = emit_reference(streamer, domain, string);
    if (ret.error == 0 && ret.emit_literal) {
        ret.error = cbor64_utf8(streamer, string);
    }
    return ret.error;
}
