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
/* Xi (Ma) Chen
 * Fri 22 Nov 2013 04:10:13 EST */

#ifndef __LIB_PCI_SUPPORT_LIBRARY_PCI_CONFIG_H__
#define __LIB_PCI_SUPPORT_LIBRARY_PCI_CONFIG_H__

#include <pci/helper.h>
#include <stdbool.h>
#include <inttypes.h>

// ref: http://www.acm.uiuc.edu/sigops/roll_your_own/7.c.0.html
//      PCI System Architecture, rev 4

#define PCI_CONFIG_HEADER_SIZE_BYTES PCI_STD_HEADER_SIZEOF

/* Detailed base address information about a device. */
struct libpci_device_iocfg {
    /* PCI_BASE_ADDRESS_MEM address or
       PCI_BASE_ADDRESS_IO address */
    uint32_t base_addr[6];
    /* PCI_BASE_ADDRESS_SPACE_IO or
       PCI_BASE_ADDRESS_SPACE_MEMORY */
    uint8_t base_addr_space[6];
    /* PCI_BASE_ADDRESS_MEM_TYPE_32 or
       PCI_BASE_ADDRESS_MEM_TYPE_64 */
    uint8_t base_addr_type[6];
    /* PCI_BASE_ADDRESS_MEM_PREFETCH */
    uint8_t base_addr_prefetchable[6];
    /* size */
    uint32_t base_addr_size_mask[6];
    uint32_t base_addr_size[6];
    /* raw addr */
    uint32_t base_addr_raw[6];
    /* Is this BAR the higher word of a 64-bit address? If true, then this BAR is partial
       and should not be directly processed in any way. */
    bool base_addr_64H[6];
};

typedef struct libpci_device_iocfg libpci_device_iocfg_t;

/* Get the size of a PCI config space element. */
static inline int libpci_device_cfg_sizeof(int offset) {
    switch (offset) {
        case PCI_VENDOR_ID: return 2;
        case PCI_DEVICE_ID: return 2;

        case PCI_COMMAND: return 2;
        case PCI_STATUS: return 2;

        case PCI_CLASS_REVISION: return 1;
        case PCI_CLASS_PROG: return 1;
        case PCI_CLASS_DEVICE: return 2;

        case PCI_CACHE_LINE_SIZE: return 1;
        case PCI_LATENCY_TIMER: return 1;
        case PCI_HEADER_TYPE: return 1;
        case PCI_BIST: return 1;

        case PCI_BASE_ADDRESS_0:
        case PCI_BASE_ADDRESS_1:
        case PCI_BASE_ADDRESS_2:
        case PCI_BASE_ADDRESS_3:
        case PCI_BASE_ADDRESS_4:
        case PCI_BASE_ADDRESS_5:
            return 4;

        case PCI_CARDBUS_CIS: return 4;
        case PCI_SUBSYSTEM_VENDOR_ID: return 2;
        case PCI_SUBSYSTEM_ID: return 2;
        case PCI_ROM_ADDRESS: return 4;

        case PCI_INTERRUPT_LINE: return 1;
        case PCI_INTERRUPT_PIN: return 1;
        case PCI_MIN_GNT: return 1;
        case PCI_MAX_LAT: return 1;
    }
    return 0;
}

/* Get the base address at a given index. Will automatically handle split 64-bit addreses. */
static inline uint64_t libpci_device_iocfg_get_baseaddr(libpci_device_iocfg_t *cfg, int index) {
    assert(cfg && index >= 0 && index < 6);
    if (cfg->base_addr_type[index] != PCI_BASE_ADDRESS_MEM_TYPE_64)
           return (uint64_t) cfg->base_addr[index];
    /* 64-bit mode BARs must have a word after it. */
    assert(index < 5);
    /* And the word before it better be set to 64L mode. */
    assert(cfg->base_addr_64H[index + 1]);
    return ((uint64_t) cfg->base_addr[index]) | (((uint64_t) cfg->base_addr[index + 1]) << 32);
}

/* Get the 32-bit base address at given index. Will automatically handle split 64-bit addresses,
 * and cast them (with an assert check that the upper 32-bits are zero). */
static inline uint32_t libpci_device_iocfg_get_baseaddr32(libpci_device_iocfg_t *cfg, int index) {
    uint64_t baddr = libpci_device_iocfg_get_baseaddr(cfg, index);
    assert((baddr & 0xFFFFFFFFUL) == baddr);
    if ((baddr & 0xFFFFFFFFUL) != baddr) {
        printf("WARNING: get_baseaddr32 called for 64-bit address. Address will be truncated.\n");
        printf("         This will most likely lead to problems.\n");
        assert(!"WARNING. Zap this assert to ignore.");
    }
    return (uint32_t)(baddr & 0xFFFFFFFFUL);
}

/* Returns true if the given device has at least one IO port base addr associated,
 * false otherwise. */
static inline bool libpci_device_iocfg_uses_iomem(libpci_device_iocfg_t *cfg) {
   assert(cfg);
   for (int i = 0; i < 6; i++) {
        if (cfg->base_addr[i] == 0 || cfg->base_addr_64H[i]) continue;
        return true;
   }
   return false;
}

/* Print out detailed info about a device's base addresses. */
static inline void libpci_device_iocfg_debug_print(libpci_device_iocfg_t *cfg, bool compact) {
    for(int i = 0; i < 6; i++) {
        if (compact) {
            /* Display in compact space mode, shoving as much information as possible in a few
             * lines. This is similar to how the Linux kernel PCI debug displays in dmesg. */
            if (cfg->base_addr[i] == 0 || cfg->base_addr_64H[i]) continue;
            if (cfg->base_addr_space[i] == PCI_BASE_ADDRESS_SPACE_IO) {
                printf("    BAR%d : [ io 0x%"PRIx64" sz 0x%x szmask 0x%x ]\n", i,
                       libpci_device_iocfg_get_baseaddr(cfg, i),
                       cfg->base_addr_size[i],
                       cfg->base_addr_size_mask[i]);
            } else {
                printf("    BAR%d : [ mem 0x%"PRIx64" sz 0x%x szmask 0x%x %s %s ]\n", i,
                       libpci_device_iocfg_get_baseaddr(cfg, i),
                       cfg->base_addr_size[i],
                       cfg->base_addr_size_mask[i],
                       cfg->base_addr_type[i] == PCI_BASE_ADDRESS_MEM_TYPE_64 ? "64bit" : "",
                       cfg->base_addr_prefetchable[i] ? "prefetch" : "");
            }
        } else {
            /* Very verbose and space wasting debug output. */
            printf("    BASE_ADDR[%d] ----\n", i);
            if (cfg->base_addr[i] == 0 || cfg->base_addr_64H[i]) continue;
            printf("        base_addr_space[%d]: 0x%x [%s]\n", i, cfg->base_addr_space[i],
                   cfg->base_addr_space[i] ? "PCI_BASE_ADDRESS_SPACE_IO" :
                                                 "PCI_BASE_ADDRESS_SPACE_MEMORY");
            printf("        base_addr_type[%d]: 0x%x [ ", i, cfg->base_addr_type[i]);
            if (cfg->base_addr_type[i] == PCI_BASE_ADDRESS_MEM_TYPE_32) printf("32bit ");
            if (cfg->base_addr_type[i] == PCI_BASE_ADDRESS_MEM_TYPE_64) printf("64bit ");
            if (cfg->base_addr_type[i] == PCI_BASE_ADDRESS_MEM_TYPE_1M) printf("<1M ");
            printf("]\n");
            printf("        base_addr_prefetchable[%d]: %s\n", i, cfg->base_addr_prefetchable[i]
                   ? "yes" : "no");
            printf("        base_addr[%d]: 0x%"PRIx64"\n", i, libpci_device_iocfg_get_baseaddr(cfg, i));
            printf("        base_addr_size_mask[%d]: 0x%x\n", i, cfg->base_addr_size_mask[i]);
        }
    }
}

#endif /* __LIB_PCI_SUPPORT_LIBRARY_PCI_CONFIG_H__ */
