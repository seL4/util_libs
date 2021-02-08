/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

/* this is a dumping ground */
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#ifndef RESOURCE
#define RESOURCE(mapper, id) ps_io_map(mapper,  (uintptr_t) id##_PADDR, id##_SIZE, 0, PS_MEM_NORMAL)
#define UNRESOURCE(mapper, id, addr) ps_io_unmap(mapper, addr, id##_SIZE)
#endif

#define __aligned(x) __attribute__((aligned(x)))
#define unlikely(x) __builtin_expect(!!(x), 0)

#define __always_inline inline __attribute__((always_inline))
#define  noinline   __attribute__((noinline))

#define __deprecated    __attribute__((deprecated))
#define __packed    __attribute__((packed))
#define __weak      __attribute__((weak))
#define __alias(symbol) __attribute__((alias(#symbol)))
#define __must_check        __attribute__((warn_unused_result))

#define MAX_PKT_SIZE    1536

#define BITS_PER_LONG 32

#define ENOTSUPP    524 /* Operation is not supported */

void udelay(unsigned long us);

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base);

#ifdef CONFIG_PHYS_64BIT
typedef unsigned long long phys_addr_t;
typedef unsigned long long phys_size_t;
#else
/* DMA addresses are 32-bits wide */
typedef unsigned long phys_addr_t;
typedef unsigned long phys_size_t;
#endif

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned int  uint;
typedef unsigned char uchar;

typedef u64 __u64;
typedef u32 __u32;
typedef u16 __u16;
typedef u8  __u8;

// typedef u8  bool;

#define __bitwise /*__attribute__((bitwise))*/
#define __force /* __attribute__((force)) */

typedef s64 __bitwise __le64;
typedef s32 __bitwise __le32;
typedef s16 __bitwise __le16;
typedef s8  __bitwise __le8;

typedef s64 __bitwise __be64;
typedef s32 __bitwise __be32;
typedef s16 __bitwise __be16;
typedef s8  __bitwise __be8;

typedef unsigned __bitwise  gfp_t;

#define gpio_init()
#define WATCHDOG_RESET()

typedef struct bd_info {
    unsigned long   bi_memstart;    /* start of DRAM memory */
    phys_size_t bi_memsize; /* size  of DRAM memory in bytes */
    unsigned long   bi_flashstart;  /* start of FLASH memory */
    unsigned long   bi_flashsize;   /* size  of FLASH memory */
    unsigned long   bi_flashoffset; /* reserved area for startup monitor */
    unsigned long   bi_sramstart;   /* start of SRAM memory */
    unsigned long   bi_sramsize;    /* size  of SRAM memory */
#ifdef CONFIG_AVR32
    unsigned char   bi_phy_id[4];   /* PHY address for ATAG_ETHERNET */
    unsigned long   bi_board_number;/* ATAG_BOARDINFO */
#endif
#ifdef CONFIG_ARM
    unsigned long   bi_arm_freq; /* arm frequency */
    unsigned long   bi_dsp_freq; /* dsp core frequency */
    unsigned long   bi_ddr_freq; /* ddr frequency */
#endif
#if defined(CONFIG_5xx) || defined(CONFIG_8xx) || defined(CONFIG_MPC8260) \
    || defined(CONFIG_E500) || defined(CONFIG_MPC86xx)
    unsigned long   bi_immr_base;   /* base of IMMR register */
#endif
#if defined(CONFIG_MPC5xxx) || defined(CONFIG_M68K)
    unsigned long   bi_mbar_base;   /* base of internal registers */
#endif
#if defined(CONFIG_MPC83xx)
    unsigned long   bi_immrbar;
#endif
    unsigned long   bi_bootflags;   /* boot / reboot flag (Unused) */
    unsigned long   bi_ip_addr; /* IP Address */
    unsigned char   bi_enetaddr[6]; /* OLD: see README.enetaddr */
    unsigned short  bi_ethspeed;    /* Ethernet speed in Mbps */
    unsigned long   bi_intfreq; /* Internal Freq, in MHz */
    unsigned long   bi_busfreq; /* Bus Freq, in MHz */
#if defined(CONFIG_CPM2)
    unsigned long   bi_cpmfreq; /* CPM_CLK Freq, in MHz */
    unsigned long   bi_brgfreq; /* BRG_CLK Freq, in MHz */
    unsigned long   bi_sccfreq; /* SCC_CLK Freq, in MHz */
    unsigned long   bi_vco;     /* VCO Out from PLL, in MHz */
#endif
#if defined(CONFIG_MPC512X)
    unsigned long   bi_ipsfreq; /* IPS Bus Freq, in MHz */
#endif /* CONFIG_MPC512X */
#if defined(CONFIG_MPC5xxx) || defined(CONFIG_M68K)
    unsigned long   bi_ipbfreq; /* IPB Bus Freq, in MHz */
    unsigned long   bi_pcifreq; /* PCI Bus Freq, in MHz */
#endif
#if defined(CONFIG_EXTRA_CLOCK)
    unsigned long bi_inpfreq;   /* input Freq in MHz */
    unsigned long bi_vcofreq;   /* vco Freq in MHz */
    unsigned long bi_flbfreq;   /* Flexbus Freq in MHz */
#endif
#if defined(CONFIG_405)   || \
        defined(CONFIG_405GP) || \
        defined(CONFIG_405EP) || \
        defined(CONFIG_405EZ) || \
        defined(CONFIG_405EX) || \
        defined(CONFIG_440)
    unsigned char   bi_s_version[4];    /* Version of this structure */
    unsigned char   bi_r_version[32];   /* Version of the ROM (AMCC) */
    unsigned int    bi_procfreq;    /* CPU (Internal) Freq, in Hz */
    unsigned int    bi_plb_busfreq; /* PLB Bus speed, in Hz */
    unsigned int    bi_pci_busfreq; /* PCI Bus speed, in Hz */
    unsigned char   bi_pci_enetaddr[6]; /* PCI Ethernet MAC address */
#endif

#ifdef CONFIG_HAS_ETH1
    unsigned char   bi_enet1addr[6];    /* OLD: see README.enetaddr */
#endif
#ifdef CONFIG_HAS_ETH2
    unsigned char   bi_enet2addr[6];    /* OLD: see README.enetaddr */
#endif
#ifdef CONFIG_HAS_ETH3
    unsigned char   bi_enet3addr[6];    /* OLD: see README.enetaddr */
#endif
#ifdef CONFIG_HAS_ETH4
    unsigned char   bi_enet4addr[6];    /* OLD: see README.enetaddr */
#endif
#ifdef CONFIG_HAS_ETH5
    unsigned char   bi_enet5addr[6];    /* OLD: see README.enetaddr */
#endif

#if defined(CONFIG_405GP) || defined(CONFIG_405EP) || \
        defined(CONFIG_405EZ) || defined(CONFIG_440GX) || \
        defined(CONFIG_440EP) || defined(CONFIG_440GR) || \
        defined(CONFIG_440EPX) || defined(CONFIG_440GRX) || \
        defined(CONFIG_460EX) || defined(CONFIG_460GT)
    unsigned int    bi_opbfreq;     /* OPB clock in Hz */
    int     bi_iic_fast[2];     /* Use fast i2c mode */
#endif
#if defined(CONFIG_4xx)
#if defined(CONFIG_440GX) || \
        defined(CONFIG_460EX) || defined(CONFIG_460GT)
    int     bi_phynum[4];           /* Determines phy mapping */
    int     bi_phymode[4];          /* Determines phy mode */
#elif defined(CONFIG_405EP) || defined(CONFIG_405EX) || defined(CONFIG_440)
    int     bi_phynum[2];           /* Determines phy mapping */
    int     bi_phymode[2];          /* Determines phy mode */
#else
    int     bi_phynum[1];           /* Determines phy mapping */
    int     bi_phymode[1];          /* Determines phy mode */
#endif
#endif /* defined(CONFIG_4xx) */
    ulong           bi_arch_number; /* unique id for this board */
    ulong           bi_boot_params; /* where this board expects params */
#ifdef CONFIG_NR_DRAM_BANKS
    struct {            /* RAM configuration */
        phys_addr_t start;
        phys_size_t size;
    } bi_dram[CONFIG_NR_DRAM_BANKS];
#endif /* CONFIG_NR_DRAM_BANKS */
} bd_t;
