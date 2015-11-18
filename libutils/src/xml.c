/*
 * Copyright 2015, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
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
