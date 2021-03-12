/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <printf.h>
#include <types.h>
#include <vargs.h>
#include <elfloader_common.h>

/*
 * Maximum space needed to print an integer in any base.
 *
 * We set this to log_2(2**64) + 1 + safety margin ~= 80.
 */
#define MAX_INT_BUFF_SIZE  80

/*
 * Function to process a simple character. "payload" may point
 * to arbitrary state needed by the "write_char" function.
 */
typedef void write_char_fn(
    void *payload,
    int c);

/* Write a NUL-terminated string to the given 'write_char' function. */
static void write_string(
    write_char_fn write_char,
    void *payload,
    const char *str)
{
    int i;
    for (i = 0; str[i] != 0; i++) {
        write_char(payload, str[i]);
    }
}

/*
 * Write the given unsigned number "n" to the given write_char function.
 *
 * We only support bases up to 16.
 */
static void write_num(
    write_char_fn write_char,
    void *payload,
    int base,
    unsigned long n)
{
    static const char hex[] = "0123456789abcdef";
    char buff[MAX_INT_BUFF_SIZE];
    int k = MAX_INT_BUFF_SIZE - 1;

    /* Special case for "0". */
    if (n == 0) {
        write_string(write_char, payload, "0");
        return;
    }

    /* NUL-terminate. */
    buff[k--] = 0;

    /* Generate the number. */
    while (n > 0) {
        buff[k] = hex[n % base];
        n /= base;
        k--;
    }

    /* Print the number. */
    write_string(write_char, payload, &buff[k + 1]);
}

/*
 * Print a printf-style string to the given write_char function.
 */
static void vxprintf(
    write_char_fn write_char,
    void *payload,
    const char *format,
    va_list args)
{
    int d, i;
    char c, *s;
    unsigned long p, ul;
    int escape_mode = 0;

    /* Iterate over the format list. */
    for (i = 0; format[i] != 0; i++) {
        /* Handle simple characters. */
        if (!escape_mode && format[i] != '%') {
            write_char(payload, format[i]);
            continue;
        }

        /* Handle the percent escape character. */
        if (format[i] == '%') {
            if (!escape_mode) {
                /* Entering escape mode. */
                escape_mode = 1;
            } else {
                /* Already in escape mode; print a percent. */
                write_char(payload, format[i]);
                escape_mode = 0;
            }
            continue;
        }

        /* Handle the modifier. */
        switch (format[i]) {
        /* Ignore printf modifiers we don't support. */
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '-':
        case '.':
            break;

        /* String. */
        case 's':
            s = va_arg(args, char *);
            write_string(write_char, payload, s);
            escape_mode = 0;
            break;

        /* Pointers. */
        case 'p':
            p = va_arg(args, unsigned long);
            write_num(write_char, payload, 16, p);
            escape_mode = 0;
            break;

        /* Hex number. */
        case 'x':
            d = va_arg(args, int);
            write_num(write_char, payload, 16, d);
            escape_mode = 0;
            break;

        /* Decimal number. */
        case 'd':
        case 'u':
            d = va_arg(args, int);
            write_num(write_char, payload, 10, d);
            escape_mode = 0;
            break;

        /* Character. */
        case 'c':
            c = va_arg(args, int);
            write_char(payload, c);
            escape_mode = 0;
            break;

        /* Long number. */
        case 'l':
            switch (format[++i]) {
            case 'u':
                ul = va_arg(args, unsigned long);
                write_num(write_char, payload, 10, ul);
                break;

            case 'x':
                ul = va_arg(args, unsigned long);
                write_num(write_char, payload, 16, ul);
                break;

            default:
                write_char(payload, '?');
            }
            escape_mode = 0;
            break;

        /* Unknown. */
        default:
            write_char(payload, '?');
            escape_mode = 0;
            break;
        }
    }
}

/*
 * Simple printf/puts implementation.
 */

static void arch_write_char(
    void *num_chars_printed_ptr,
    int c)
{
    int *num_chars_printed = (int *)num_chars_printed_ptr;

    /* For now, console output goes into a UART on every platform eventually
     * and we write a '\r' (CR) before every '\n' (LF) unconditinally. If there
     * will even be a console that works differently, we can still add a
     * configuration flag that allows disabling this feature.
     */
    if (c == '\n') {
        /* TODO: There is no "(*num_chars_printed)++;" here, as the CR char has
         *       never been counted by any platform specific implementations in
         *       the past. For now the behavior is kept, but it seem quite
         *       wrong to hide this. Check if any code depends really on this
         *       and consider counting the CR char also.
         */
        plat_console_putchar('\r');
    }

    (*num_chars_printed)++;
    plat_console_putchar(c);
}

int printf(
    const char *format,
    ...)
{
    int n = 0;
    va_list args;
    va_start(args, format);
    vxprintf(arch_write_char, &n, format, args);
    va_end(args);
    return n;
}

int puts(
    const char *str)
{
    int n = 0;
    write_string(arch_write_char, &n, str);
    arch_write_char(&n, '\n');
    return n;
}

/*
 * Simple sprintf implementation.
 */

struct sprintf_payload {
    char *buff;
    int n;
};

static void sprintf_write_char(
    void *payload,
    int c)
{
    struct sprintf_payload *p = (struct sprintf_payload *)payload;
    p->buff[p->n] = c;
    p->n++;
}

int sprintf(
    char *buff,
    const char *format,
    ...)
{
    struct sprintf_payload p = {buff, 0};
    va_list args;
    va_start(args, format);
    vxprintf(sprintf_write_char, &p, format, args);
    va_end(args);
    return p.n;
}
