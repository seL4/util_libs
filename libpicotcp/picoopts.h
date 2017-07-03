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

/* PicoTCP - Definition file to add support for modules */
#pragma once

#define SEL4

#define PICO_SUPPORT_ETH
#define PICO_SUPPORT_IPV4
#define PICO_SUPPORT_UDP
#define PICO_SUPPORT_PING
#define PICO_SUPPORT_ICMP4
#define PICO_SUPPORT_TCP
#define PICO_SOCKET_TCP
#define PICO_SUPPORT_TAP
#define PICO_SUPPORT_MOCK
#define DPICO_SUPPORT_DNS_CLIENT

#define DEBUG_ROUTE
#define DEBUG_ARP
