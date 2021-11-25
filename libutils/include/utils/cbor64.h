/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <utils/arith.h>
#include <utils/base64.h>

/*
 * Streaming base64 CBOR encoder
 *
 * This implementation is intended to allow structured data to be
 * streamed out via a serial connection in a manner that minimises the
 * number of actual bytes that must be written to the output.
 *
 * Data is streamed to an output as base64 encoded CBOR which can then
 * be extracted from a serial log and decoded offline.
 */

typedef struct {
    base64_t streamer;
} cbor64_t;

/* Major types of CBOR items */
typedef enum {
    CBOR64_MT_UNSIGNED_INT = 0,
    CBOR64_MT_NEGATIVE_INT = 1,
    CBOR64_MT_BYTE_STRING = 2,
    CBOR64_MT_UTF8_STRING = 3,
    CBOR64_MT_ARRAY = 4,
    CBOR64_MT_MAP = 5,
    CBOR64_MT_TAG = 6,
    CBOR64_MT_FLOAT = 7,
    CBOR64_MT_SIMPLE = 7,
    CBOR64_MT_BREAK = 7,
} cbor64_mt_t;

/* Additional information identifiers */
typedef enum {
    /* Values below 24 are integer literals */
    CBOR64_AI_INT_LITERAL_MAX = 24,
    /* Numeric value sizes */
    CBOR64_AI_UINT8_T = 24,
    CBOR64_AI_UINT16_T = 25,
    CBOR64_AI_UINT32_T = 26,
    CBOR64_AI_UINT64_T = 27,
    /* Simple value indicated in next bytes */
    CBOR64_AI_SIMPLE_BYTE = 24,
    /* Float sizes */
    CBOR64_AI_FLOAT16_T = 25, /* IEEE 754 Half-precision */
    CBOR64_AI_FLOAT32_T = 26, /* IEEE 754 Single-precision */
    CBOR64_AI_FLOAT64_T = 27, /* IEEE 754 Double-precision */
    /* Array/map length specifier */
    CBOR64_AI_INDEFINITE_LENGTH = 31,
} cbor64_ai_t;

/* Simple values */
typedef enum {
    /* Boolean */
    CBOR64_SIMPLE_FALSE = 20,
    CBOR64_SIMPLE_TRUE = 21,
    /* Null */
    CBOR64_SIMPLE_NULL = 22,
    /* Undefined */
    CBOR64_SIMPLE_UNDEFINED = 23,
} cbor64_simple_t;

/* tags */
typedef enum {
    /* Semantic descriptors */

    /* Date & time (encoded as UTF-8 string) */
    CBOR64_TAG_DATETIME_UTF8 = 0,
    /* Date & time encoded relative to an epoch */
    CBOR64_TAG_DATETIME_EPOCH = 1,
    /* Big integers (encoded as bytes) */
    CBOR64_TAG_POSITIVE_BIGNUM = 2,
    CBOR64_TAG_NEGATIVE_BIGNUM = 3,
    /* Decimal fraction (encoded as array 2 integers (mantissa, base 10 scale)) */
    CBOR64_TAG_DECIMAL_FRACTION = 4,
    /* Big float (encoded as array 2 integers (mantissa, base 2 scale)) */
    CBOR64_TAG_BIG_FLOAT = 4,

    /* Encoding hints */

    /* Encode byte string children as base64url */
    CBOR64_TAG_ENCODE_BASE64URL = 21,
    /* Encode byte string children as base64 */
    CBOR64_TAG_ENCODE_BASE64 = 22,
    /* Encode byte string children as base16 */
    CBOR64_TAG_ENCODE_BASE16 = 23,
    /* Byte string encodes CBOR item */
    CBOR64_TAG_ENCODE_CBOR = 24,

    /* UTF-8 String descriptors */

    /* String is a URI */
    CBOR64_TAG_UTF8_URI = 32,
    /* String is a base64url */
    CBOR64_TAG_UTF8_BASE64URL = 33,
    /* String is a base64 */
    CBOR64_TAG_UTF8_BASE64 = 34,
    /* String is a PCRE/ECMA262 regular expression */
    CBOR64_TAG_UTF8_RE = 35,
    /* String MIME message */
    CBOR64_TAG_UTF8_MIME = 36,

    /* Shared values */

    /* A value that may later be referenced */
    CBOR64_TAG_SHAREABLE = 28,
    /* A reference to a previously shared value */
    CBOR64_TAG_SHARED_VALUE = 29,

    /* String referneces */

    /* A reference to a previously tagged string */
    CBOR64_TAG_STRING_REF = 25,
    /* A domain containing string references */
    CBOR64_TAG_STRING_REF_DOMAIN = 256,

    /* Self-described CBOR (magic bytes) */
    CBOR64_TAG_SELF_DESCRIBED = 55799,
} cbor64_tag_t;

/*
 * Inline implementation
 * =====================
 */

/* Generate the initial byte indicating the type of the following data */
int cbor64_initial_byte(base64_t *streamer, cbor64_mt_t type, uint8_t data);

/* Send a break byte to terminate indefinite-length item */
int cbor64_send_break(base64_t *streamer);

/* This sends a numeric item to the streamer using big-endian encoding */
int cbor64_send_item(base64_t *streamer, cbor64_mt_t type, uint64_t number);

/* Send a type array of bytes (UTF8 or bytes) */
int cbor64_send_typed_bytes(base64_t *streamer, cbor64_mt_t type, unsigned char *buffer, size_t length);

/* Send a simple value in one or two bytes */
int cbor64_send_simple(base64_t *streamer, cbor64_simple_t value);

/*
 * External API
 * ============
 */


/*
 * Send a tag for the following item
 *
 * A tag is a single item describing the next item in the stream. It
 * can denote some particular semantic meaning for the subsequent item
 * or that the item is to be encoded in some particular manner when
 * translated to JSON (see cbor64_tag_t).
 */
static inline int cbor64_tag(base64_t *streamer, cbor64_tag_t tag)
{
    return cbor64_send_item(streamer, CBOR64_MT_TAG, tag);
}

/*
 * Simple types
 * ------------
 */

/* Send a boolean value */
static inline int cbor64_bool(base64_t *streamer, int boolean)
{
    uint8_t value = CBOR64_SIMPLE_FALSE;
    if (boolean) {
        value = CBOR64_SIMPLE_TRUE;
    }
    return cbor64_send_simple(streamer, value);
}

/* Send a null */
static inline int cbor64_null(base64_t *streamer)
{
    return cbor64_send_simple(streamer, CBOR64_SIMPLE_NULL);
}

/* Send an undefined */
static inline int cbor64_undefined(base64_t *streamer)
{
    return cbor64_send_simple(streamer, CBOR64_SIMPLE_UNDEFINED);
}

/*
 * Integer types
 * -------------
 */

/* Send an unsigned integer value */
static inline int cbor64_uint(base64_t *streamer, uint64_t number)
{
    return cbor64_send_item(streamer, CBOR64_MT_UNSIGNED_INT, number);
}

/* Send a signed integer value */
static inline int cbor64_int(base64_t *streamer, int64_t number)
{
    cbor64_mt_t type = CBOR64_MT_UNSIGNED_INT;
    if (number < 0) {
        type = CBOR64_MT_NEGATIVE_INT;
        number = (-1) - number;
    }

    return cbor64_send_item(streamer, type, number);
}

/*
 * IEEE 754 Float types
 * --------------------
 */

/* Send a single-precision float value */
int cbor64_float(base64_t *streamer, float number);

/* Send a double-precision float value */
int cbor64_double(base64_t *streamer, double number);

/*
 * Byte arrays
 * -----------
 *
 * The following functions describe 3 kinds of byte array:
 *  - Raw bytes (bytes)
 *  - C strings that are not guaranteed to be UTF8 (string)
 *  - UTF-8 C strings (utf8)
 *
 * Each has a function that will stream a single array along with its
 * size which can be used directly. Additionally, a series of 'chunks'
 * can be sent without the need to know the number of chunks. A series
 * of chunks must start with a call to 'cbor64_<kind>_chunks_start' and
 * finish with a call to 'cbor64_<kind>_chunks_start' with only calls to
 * the corresponding 'cbor64_<kind>' in-between.
 *
 * For example:
 *
 *     cbor64_utf8_chunks_start(streamer);
 *     cbor64_utf8(streamer, "Hello,");
 *     cbor64_utf8(streamer, "world!");
 *     cbor64_utf8_chunks_end(streamer);
 */

/* send an array of bytes */
static inline int cbor64_bytes(base64_t *streamer, unsigned char *buffer, size_t length)
{
    return cbor64_send_typed_bytes(streamer, CBOR64_MT_BYTE_STRING, buffer, length);
}

/* Start chunked bytes */
static inline int cbor64_byte_chunks_start(base64_t *streamer)
{
    return cbor64_send_item(streamer, CBOR64_MT_BYTE_STRING, CBOR64_AI_INDEFINITE_LENGTH);
}

/* End chunked string */
static inline int cbor64_byte_chunks_end(base64_t *streamer)
{
    return cbor64_send_break(streamer);
}

/* Send a non-UTF-8 string */
static inline int cbor64_string(base64_t *streamer, char *text)
{
    return cbor64_bytes(streamer, (unsigned char *) text, strlen(text));
}

/* Start chunked string */
static inline int cbor64_string_chunks_start(base64_t *streamer)
{
    return cbor64_send_item(streamer, CBOR64_MT_BYTE_STRING, CBOR64_AI_INDEFINITE_LENGTH);
}

/* End chunked string */
static inline int cbor64_string_chunks_end(base64_t *streamer)
{
    return cbor64_send_break(streamer);
}

/* Send a UTF-8 string */
static inline int cbor64_utf8(base64_t *streamer, char *text)
{
    return cbor64_send_typed_bytes(streamer, CBOR64_MT_UTF8_STRING, (unsigned char *) text, strlen(text));
}

/* Start chunked UTF-8 string */
static inline int cbor64_utf8_chunks_start(base64_t *streamer)
{
    return cbor64_send_item(streamer, CBOR64_MT_UTF8_STRING, CBOR64_AI_INDEFINITE_LENGTH);
}

/* End chunked UTF-8 string */
static inline int cbor64_utf8_chunks_end(base64_t *streamer)
{
    return cbor64_send_break(streamer);
}

/*
 * Arrays
 * ------
 *
 * Arrays are a series of items. An array of known length need only
 * start with a call to 'cbor64_array_length'.
 *
 *     cbor64_array_length(streamer, 2);
 *     cbor64_uint(streamer, 12);
 *     cbor64_uint(streamer, 28);
 *
 * If the length is unknown, the array can be started with
 * 'cbor64_array_start' and completed with a call to 'cbor64_array_end'.
 *
 *     cbor64_array_start(streamer);
 *     cbor64_uint(streamer, 15);
 *     cbor64_uint(streamer, 10538);
 *     cbor64_array_end(streamer);
 */

/* Start an array of unknown length */
static inline int cbor64_array_start(base64_t *streamer)
{
    return cbor64_initial_byte(streamer, CBOR64_MT_ARRAY, CBOR64_AI_INDEFINITE_LENGTH);
}

/* End an array of unknown length */
static inline int cbor64_array_end(base64_t *streamer)
{
    return cbor64_send_break(streamer);
}

/* Start an array of known length */
static inline int cbor64_array_length(base64_t *streamer, uint64_t length)
{
    return cbor64_send_item(streamer, CBOR64_MT_ARRAY, length);
}

/*
 * Maps
 * ----
 *
 * Maps are a series of key-value pairs. The keys may be of any type.
 *
 * A map of known length need only start with a call to
 * 'cbor64_map_length'.
 *
 *     cbor64_map_length(streamer, 2);
 *     cbor64_utf8(streamer, "x");
 *     cbor64_uint(streamer, 48);
 *     cbor64_utf8(streamer, "y");
 *     cbor64_uint(streamer, 97);
 *
 * If the length is unknown, the map can be started with
 * 'cbor64_map_start' and completed with a call to 'cbor64_map_end'.
 *
 *     cbor64_map_start(streamer);
 *     cbor64_utf8(streamer, "x");
 *     cbor64_uint(streamer, 48);
 *     cbor64_utf8(streamer, "y");
 *     cbor64_uint(streamer, 97);
 *     cbor64_map_end(streamer);
 */

/* Start a map of unknown length */
static inline int cbor64_map_start(base64_t *streamer)
{
    return cbor64_initial_byte(streamer, CBOR64_MT_MAP, CBOR64_AI_INDEFINITE_LENGTH);
}

/* End a map of unknown length */
static inline int cbor64_map_end(base64_t *streamer)
{
    return cbor64_send_break(streamer);
}

/* Start a map of known length */
static inline int cbor64_map_length(base64_t *streamer, uint64_t length)
{
    return cbor64_send_item(streamer, CBOR64_MT_MAP, length);
}

/*
 * String reference domains
 * ========================
 *
 * String reference domains allow reduced encoding of strings by only
 * emitting each encoded string once and then using tagged numeric
 * references to previous occurrences of strings.
 *
 * The current implementation is suboptimal but avoid allocation by
 * using a static allocation of the strings used.
 *
 * Within a string reference domain, all strings must be emitted using
 * 'cbor64_string_ref' or 'cbor64_utf8_ref' emitter. To emit a sized
 * byte array or data containing strings not in the domain, you can
 * create a new null domain that contains no references.
 *
 * Using shared values
 * -------------------
 *
 * If the tooling used does not support string reference domains but
 * does support shared values, this can be used to implement similar
 * semantics, however only one domain using shared values can exist in a
 * dataset.
 */

/* Tracks the strings which have already been emitted and their index. */
typedef struct {
    char **strings;
    /* Use shared values rather than string references */
    bool shared_values;
    uint64_t emitted;
} cbor64_domain_t;

/* Start a new domain with no inner string references */
static inline int cbor64_null_domain(base64_t *streamer)
{
    return cbor64_tag(streamer, CBOR64_TAG_STRING_REF_DOMAIN);
}

/*
 * Create a new string reference domain
 *
 * The provided array of strings must not be used again within this
 * domain in a nested fashion.
 *
 * The array of strings must be terminated with a NULL.
 */
static inline int cbor64_string_ref_domain(base64_t *streamer, char **strings, cbor64_domain_t *domain)
{
    domain->strings = strings;
    domain->emitted = 0;
    domain->shared_values = false;

    return cbor64_tag(streamer, CBOR64_TAG_STRING_REF_DOMAIN);
}

/*
 * Create a new shared value domain
 *
 * There must be no more than one shared value domain in an output.
 *
 * The provided array of strings must not be used again within this
 * domain in a nested fashion.
 *
 * The array of strings must be terminated with a NULL.
 */
static inline void cbor64_shared_value_domain(char **strings, cbor64_domain_t *domain)
{
    domain->strings = strings;
    domain->emitted = 0;
    domain->shared_values = true;
}

/*
 * Emit a string reference
 */
int cbor64_string_ref(base64_t *streamer, cbor64_domain_t *domain, char *string);

/*
 * Emit a utf8 reference
 */
int cbor64_utf8_ref(base64_t *streamer, cbor64_domain_t *domain, char *string);
