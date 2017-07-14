/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
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

static const char* cpuid_get_implementer_str(uint32_t cpuid)
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

static const char* cpuid_get_arch_str(uint32_t cpuid)
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
    if ((CPUID_PART(cpuid) & 0xf00) == 0xC00) {
        printf("Cortex-A%d ", CPUID_PART(cpuid) & 0xff);
    } else if ((CPUID_PART(cpuid) & 0xf00) == 0xD00) {
        printf("Cortex-A5%d ", CPUID_PART(cpuid) & 0xff);
    } else {
        printf("Part: 0x%03x ", CPUID_PART(cpuid));
    }

    printf("r%dp%d", CPUID_MAJOR(cpuid), CPUID_MINOR(cpuid));
    printf("\n");
}

int get_cortex_a_part(void)
{
    uint32_t cpuid;
    cpuid = read_cpuid_id();
    if(CPUID_ARCH(cpuid) == CPUID_ARCH_CPUID && CPUID_IMPL(cpuid) == CPUID_IMPL_ARM){
        return CPUID_PART(cpuid) & 0xFF;
    }else{
        return -1;
    }
}
