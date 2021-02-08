/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stddef.h>
#include <stdio.h>
#include <utils/attribute.h>
#include <utils/xml.h>

#define PRINT(s) (print == NULL ? printf("%s", (s)) : print(arg, "%s", (s)))

int
utils_put_xml_escape(const char *string,
                     int (*print)(void *arg, const char *format, ...) FORMAT(printf, 2, 3),
                     void *arg)
{

    int ret = 0;

    while (*string != '\0') {

        switch (*string) {

            case '"':
                ret += PRINT("&quot;");
                break;

            case '\'':
                ret += PRINT("&apos;");
                break;

            case '<':
                ret += PRINT("&lt;");
                break;

            case '>':
                ret += PRINT("&gt;");
                break;

            case '&':
                ret += PRINT("&amp;");
                break;

            default:
                if (print == NULL) {
                    putchar(*string);
                    ret++;
                } else {
                    ret += print(arg, "%c", *string);
                }
        }

        string++;
    }

    return ret;
}
