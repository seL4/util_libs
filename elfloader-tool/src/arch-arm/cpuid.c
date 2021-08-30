/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <printf.h>
#include <cpuid.h>

#define CPUID_IMPL(cpuid)    (((cpuid) >> 24) &  0xff)
#define CPUID_MAJOR(cpuid)   (((cpuid) >> 20) &   0xf)
#define CPUID_VARIANT(cpuid) (((cpuid) >> 20) &   0xf)
#define CPUID_ARCH(cpuid)    (((cpuid) >> 16) &   0xf)
#define CPUID_PART(cpuid)    (((cpuid) >>  4) & 0xfff)
#define CPUID_MINOR(cpuid)   (((cpuid) >>  0) &   0xf)

#define CPUID_IMPL_ARM     'A'
#define CPUID_IMPL_DEC     'D'
#define CPUID_IMPL_QCOMM   'Q'
#define CPUID_IMPL_MARV    'V'
#define CPUID_IMPL_MOT     'M'
#define CPUID_IMPL_INTEL   'i'
#define CPUID_ARCH_ARMv4    0x1
#define CPUID_ARCH_ARMv4T   0x2
#define CPUID_ARCH_ARMv5    0x3
#define CPUID_ARCH_ARMv5T   0x4
#define CPUID_ARCH_ARMv5TE  0x5
#define CPUID_ARCH_ARMv5TEJ 0x6
#define CPUID_ARCH_ARMv6    0x7
#define CPUID_ARCH_CPUID    0xF

#if __has_attribute(optimize)
#define OPTIMIZE_CHANGE __attribute__((optimize(1)))
#else
#define OPTIMIZE_CHANGE __attribute__((optnone))
#endif

/*
 * At O2 the switch gets optimised into a table, (at least on GCC 7.4 and 8.2)
 * which isn't handled properly for position independent code (i.e. when booting on EFI).
 */
OPTIMIZE_CHANGE static const char *cpuid_get_implementer_str(uint32_t cpuid)
{
    switch (CPUID_IMPL(cpuid)) {
    case CPUID_IMPL_ARM:
        return "ARM Ltd.";
    case CPUID_IMPL_DEC:
        return "Digital Equipment Corp.";
    case CPUID_IMPL_QCOMM:
        return "Qualcomm Inc.";
    case CPUID_IMPL_MARV:
        return "Marvell Semiconductor Inc.";
    case CPUID_IMPL_MOT:
        return "Motorola, Freescale Semiconductor Inc.";
    case CPUID_IMPL_INTEL:
        return "Intel Corp.";
    default:
        return "<Reserved>";
    }
}

OPTIMIZE_CHANGE static const char *cpuid_get_arch_str(uint32_t cpuid)
{
    switch (CPUID_ARCH(cpuid)) {
    case CPUID_ARCH_ARMv4:
        return "ARMv4";
    case CPUID_ARCH_ARMv4T:
        return "ARMv4T";
    case CPUID_ARCH_ARMv5:
        return "ARMv5 (obsolete)";
    case CPUID_ARCH_ARMv5T:
        return "ARMv5T";
    case CPUID_ARCH_ARMv5TE:
        return "ARMv5TE";
    case CPUID_ARCH_ARMv5TEJ:
        return "ARMv5TEJ";
    case CPUID_ARCH_ARMv6:
        return "ARMv6";
    case CPUID_ARCH_CPUID:
        return "Defined by CPUID scheme";
    default:
        return "<Reserved>";
    }
}


void print_cpuid(void)
{
    uint32_t cpuid;
    cpuid = read_cpuid_id();
    printf("CPU: %s ", cpuid_get_implementer_str(cpuid));
    if (CPUID_ARCH(cpuid) != CPUID_ARCH_CPUID) {
        printf("%s ", cpuid_get_arch_str(cpuid));
    }
    /* References: https://en.wikipedia.org/wiki/Comparison_of_ARMv8-A_cores */
    switch (CPUID_PART(cpuid)) {
    case 0xC05:
        printf("Cortex-A5 ");
        break;
    case 0xC07:
        printf("Cortex-A7 ");
        break;
    case 0xC08:
        printf("Cortex-A8 ");
        break;
    case 0xC09:
        printf("Cortex-A9 ");
        break;
    case 0xC0D:
        printf("Cortex-A12 ");
        break;
    case 0xC0F:
        printf("Cortex-A15 ");
        break;
    case 0xC0E:
        printf("Cortex-A17 ");
        break;
    case 0xD01:
        printf("Cortex-A32 ");
        break;
    case 0xD02:
        printf("Cortex-A34 ");
        break;
    case 0xD03:
        printf("Cortex-A53 ");
        break;
    case 0xD04:
        printf("Cortex-A35 ");
        break;
    case 0xD05:
        printf("Cortex-A55 ");
        break;
    case 0xD06:
        printf("Cortex-A65 ");
        break;
    case 0xD07:
        printf("Cortex-A57 ");
        break;
    case 0xD08:
        printf("Cortex-A72 ");
        break;
    case 0xD09:
        printf("Cortex-A73 ");
        break;
    case 0xD0A:
        printf("Cortex-A75 ");
        break;
    case 0xD0B:
        printf("Cortex-A76 ");
        break;
    case 0xD0C:
        printf("Neoverse N1 ");
        break;
    case 0xD0D:
        printf("Cortex-A77 ");
        break;
    case 0xD0E:
        printf("Cortex-A76AE ");
        break;
    case 0xD40:
        printf("Neoverse V1 ");
        break;
    case 0xD41:
        printf("Cortex-A78 ");
        break;
    case 0xD42:
        printf("Cortex-A78AE ");
        break;
    case 0xD43:
        printf("Cortex-A65AE ");
        break;
    case 0xD44:
        printf("Cortex-X1 ");
        break;
    case 0xD46:
        printf("Cortex-A510 ");
        break;
    case 0xD47:
        printf("Cortex-A710 ");
        break;
    case 0xD48:
        printf("Cortex-X2 ");
        break;
    case 0xD49:
        printf("Neoverse N2 ");
        break;
    case 0xD4A:
        printf("Neoverse E1 ");
        break;
    case 0xD4B:
        printf("Cortex-78C ");
        break;
    default:
        printf("Part: 0x%03x ", CPUID_PART(cpuid));
    }

    printf("r%dp%d", CPUID_MAJOR(cpuid), CPUID_MINOR(cpuid));
    printf("\n");
}

int get_cortex_a_part(void)
{
    uint32_t cpuid;
    cpuid = read_cpuid_id();
    if (CPUID_ARCH(cpuid) == CPUID_ARCH_CPUID && CPUID_IMPL(cpuid) == CPUID_IMPL_ARM) {
        return CPUID_PART(cpuid) & 0xFF;
    } else {
        return -1;
    }
}
