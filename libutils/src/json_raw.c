/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <utils/json.h>

int json_print_begin(void) {
    return printf(" --- BEGIN JSON ---\n");
}

int json_print_end(void) {
    return printf(" --- END JSON ---\n");
}

int json_print_true(void) {
    return printf("true");
}

int json_print_false(void) {
    return printf("false");
}

int json_print_bool(bool v) {
    return v ? json_print_true() : json_print_false();
}

int json_print_null(void) {
    return printf("null");
}

int json_print_int(int v) {
    return printf("%d", v);
}

int json_print_uint(unsigned int v) {
    return printf("%u", v);
}

int json_print_pointer(void *v) {
    if (v == NULL) {
        return json_print_null();
    }
    return json_print_uint((uintptr_t)v);
}

int json_print_string(const char *s) {
    int printed = 0;
    printed += printf("\"");
    while (*s != '\0') {
        /* Assume we are reading a narrow string (i.e. that each character is 1
         * byte.
         */
        switch (*s) {

            case '"':
            case '\\':
            case '/':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                printed += printf("\\%c", *s);
                break;

            /* Directly printable ASCII characters: */
            case ' ':
            case '!':
            case '#' ... '.':
            case '0' ... '[':
            case ']' ... '~':
                printed += printf("%c", *s);
                break;

            default:
                printed += printf("\\u%04u", (unsigned int)(*s));
        }
        s++;
    }
    printed += printf("\"");
    return printed;
}

int json_print_array(void **array, size_t size, int (*printer)(void *item)) {
    int printed = 0;
    printf("[");
    printed++;
    for (unsigned int i = 0; i < size; i++) {
        printed += printer(array[i]);
        if (size - i > 1) {
            printf(",");
            printed++;
        }
    }
    printf("]");
    printed++;
    return printed;
}

int json_print_object(void *object, const char **keys, size_t keys_sz, int (*printer)(void *object, const char *key)) {
    int printed = 0;
    printed += printf("{");

    for (unsigned int i = 0; i < keys_sz; i++) {
        if (i != 0) {
            printed += printf(",");
        }

        printed += json_print_string(keys[i]);

        printed += printf(":");

        printed += printer(object, keys[i]);
    }
    printed += printf("}");
    return printed;
}

#ifdef JSON_TESTCASE
/* Unit tests follow. */

JSON_TESTCASE(print_true) {
    printf("[");
    json_print_true();
    printf("]");
    expected("[true]");
}

JSON_TESTCASE(print_false) {
    printf("[");
    json_print_false();
    printf("]");
    expected("[false]");
}

JSON_TESTCASE(print_0) {
    printf("[");
    json_print_int(0);
    printf("]");
    expected("[0]");
}

JSON_TESTCASE(print_positive) {
    printf("[");
    json_print_int(42);
    printf("]");
    expected("[42]");
}

JSON_TESTCASE(print_negative) {
    printf("[");
    json_print_int(-42);
    printf("]");
    expected("[-42]");
}

JSON_TESTCASE(null_pointer) {
    printf("[");
    json_print_pointer(NULL);
    printf("]");
    expected("[null]");
}

JSON_TESTCASE(numeric_pointer) {
    char buffer[100];
    printf("[");
    json_print_pointer((void*)0x12345678);
    printf("]");
    sprintf(buffer, "[%u]", 0x12345678);
    expected(buffer);
}

JSON_TESTCASE(hex_pointer) {
    char buffer[100];
    printf("[");
    json_print_pointer((void*)0xcafe1dea);
    printf("]");
    sprintf(buffer, "[%u]", 0xcafe1dea);
    expected(buffer);
}

JSON_TESTCASE(string) {
    char buffer[100];
    printf("[");
    json_print_string("hello world");
    printf("]");
    sprintf(buffer, "[\"%s\"]", "hello world");
    expected(buffer);
}

#endif /* JSON_TESTCASE */
