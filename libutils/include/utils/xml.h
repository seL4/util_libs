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

/* Basic XML-related functionality. If this ever grows beyond simple helpers, we should switch to
 * something more full-featured like libxml2.
 */

#pragma once

#include <utils/attribute.h>

/**
 * Print a string, escaping characters that have special meanings in XML.
 * @param[in] string The string to print.
 * @param[in] print  A printf analogue to use. Will default to printf if this is NULL.
 * @param[in] arg    A parameter to pass as the first argument to the print function. Ignored if
 *                   the print parameter is NULL.
 * @return The number of characters printed.
 */
int utils_put_xml_escape(const char *string,
                         int (*print)(void *arg, const char *format, ...) FORMAT(printf, 2, 3),
                         void *arg) NONNULL(1);
