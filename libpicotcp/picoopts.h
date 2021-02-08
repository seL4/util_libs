/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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
#define PICO_SUPPORT_DNS_CLIENT

#define DEBUG_ROUTE
#define DEBUG_ARP
