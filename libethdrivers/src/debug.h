/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <stdint.h>
#include <utils/util.h>
//#define DBG_MAP
//#define DBG_PKT
//#define DBG_PRINT_PAYLOAD
//#define DBG_REG
//#define DBG_FEC
//#define DBG_BUF
//#define DBG_NET
//#define DBG_CLK

#ifdef DBG_MAP
#  define MAP_DEBUG(x) do{x;}while(0)
#else
#  define MAP_DEBUG(x) do{;}while(0)
#endif

#ifdef DBG_PKT
#  define PKT_DEBUG(x) do{x;}while(0)
#  ifdef DBG_PRINT_PAYLOAD
#    define DEBUG_PRINT_PAYLOAD(DEBUG_PRINT_PAYLOAD)
#  endif
#else
#  define PKT_DEBUG(x) do{;}while(0)
#endif

#ifdef DBG_FEC
#define FEC_DEBUG(x) do{x;}while(0)
#else
#define FEC_DEBUG(x) do{;}while(0)
#endif

#ifdef DBG_BUF
#define BUF_DEBUG(x) do{x;}while(0)
#else
#define BUF_DEBUG(x) do{;}while(0)
#endif

#ifdef DBG_REG
#define REG_DEBUG(x) do{x;}while(0)
#else
#define REG_DEBUG(x) do{;}while(0)
#endif

#ifdef DBG_NET
#define NET_DEBUG(x) do{x;}while(0)
#else
#define NET_DEBUG(x) do{;}while(0)
#endif

#ifdef DBG_CLK
#define CLK_DEBUG(x) do{x;} while(0)
#else
#define CLK_DEBUG(x) do{;} while(0)
#endif

#include <stdio.h>
#include <stdint.h>

#define COL_NET "\e[1;34m"
#define COL_IMP "\e[1;31m"
#define COL_FEC "\e[1;32m"
#define COL_RX  "\e[42;30m"
#define COL_TX  "\e[43;30m"
#define COL_ARP "\e[1;28m"
#define COL_PKT "\e[1;36m"
#define COL_DEF "\e[0;0m"

#define set_colour(x) printf(x);

#define cprintf(col, ...) do { \
        set_colour(col);       \
        ZF_LOGD(__VA_ARGS__);  \
        set_colour(COL_DEF);   \
        printf("\n");          \
    }while(0)

#define UNIMPLEMENTED() \
    do{\
        ZF_LOGF("unimplemented"); \
        while(1); \
    } while(0)

#ifdef DBG_PKT

static inline void print_mac(uint8_t* mac){
    int i;
    printf("%02x", *mac++);
    for(i = 0; i < 5; i++){
        printf(":%02x", *mac++);
    }
}

static inline void
print_type(uint8_t* p){
    uint32_t type = 0;
    int i;
    p += 6*2;
    for(i = 0; i < 2; i++){
        type = type << 8  | *p++;
    }
    switch(type){
    case 0x0806: printf("ARP"); break;
    case 0x0800: printf(" IP"); break;
    default: printf("UNKNOWN");
    }
    printf(" (0x%04x)", type);
}

static inline void print_val(uint8_t* p, int len){
    uint32_t val = 0;
    int i;
    for(i = 0; i < len; i++){
        val = val << 8 | *p++;
    }
    printf("%d", val);
}

static inline void print_packet(const char* col, void* packet, int length){
    cprintf(col, "packet 0x%x (%d bytes)", (uint32_t)packet, length);
    unsigned char* p = packet;
    printf(" dst MAC : ");  print_mac(p + 0);
    printf(" src MAC : ");  print_mac(p + 6); printf("\n");
    printf(" src port: ");  print_val(p +0x22, 2);
    printf(" dst port: ");  print_val(p +0x24, 2); printf("\n");
    printf(" type    : "); print_type(p + 0); printf("\n");
#if defined(DBG_PRINT_PAYLOAD)
    int i,j;
    for(i=0;i<length;i+=32){
        for(j=0;j<32 && length > i+j;j++){
            printf("%02x", *p++);
        }
        printf("\n");
    }
#endif
    printf("\n");
}
#endif

