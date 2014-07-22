/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __REGIONS_H__
#define __REGIONS_H__

/*
 * Table signitures
 */
#define ACPI_SIG_RSDP           "RSD PTR "  /* Root System Descriptor table Pointer */

#define ACPI_SIG_RSDT           "RSDT"      /* Root System Descriptor Table */
#define ACPI_SIG_XSDT           "XSDT"      /* eXtended System Descriptor Table */
#define ACPI_SIG_BERT           "BERT"      /* Boot Error Record Table */
#define ACPI_SIG_CPEP           "CPEP"      /* Corrected Platform Error Polling table */
#define ACPI_SIG_ECDT           "ECDT"      /* Embedded Controller Boot Resources Table */
#define ACPI_SIG_EINJ           "EINJ"      /* Error Injection table */
#define ACPI_SIG_ERST           "ERST"      /* Error Record Serialization Table */
#define ACPI_SIG_MADT           "APIC"      /* Multiple APIC Description Table */
#define ACPI_SIG_MSCT           "MSCT"      /* Maximum System Characteristics Table */
#define ACPI_SIG_SBST           "SBST"      /* Smart Battery Specification Table */
#define ACPI_SIG_SLIT           "SLIT"      /* System Locality Distance Information Table */
#define ACPI_SIG_SRAT           "SRAT"      /* System Resource Affinity Table */
#define ACPI_SIG_FADT           "FACP"      /* Fixed ACPI Description Table */
#define ACPI_SIG_FACS           "FACS"      /* Firmware ACPI Control Structure */
#define ACPI_SIG_DSDT           "DSDT"      /* Differentiated System Description Table */
#define ACPI_SIG_SSDT           "SSDT"      /* Secondary System Description Table */
#define ACPI_SIG_SPMI           "SPMI"      /* Server Platform Management Interface table */
#define ACPI_SIG_HPET           "HPET"      /* High Precision Event Timer */
#define ACPI_SIG_BOOT           "BOOT"      /* BOOT flags table structure */
#define ACPI_SIG_SPCR           "SPCR"      /* Serial Port Console Redirection */
#define ACPI_SIG_DMAR           "DMAR"      /* DMA remapping table */
#define ACPI_SIG_ASF            "ASF!"      /* Alert Standard Format table */
#define ACPI_SIG_HEST           "HEST"      /* Hardware Error Source Table */
#define ACPI_SIG_ERST           "ERST"      /* Error Record Serialisation Table */
#define ACPI_SIG_MCFG           "MCFG"      /* PCI Memory mapped ConFiGuration */


#define ACPI_SIG_ASPT           "ASPT"      /* Unknown table? */

#define ACPI_TABLE_TEST(addr, TABLE) \
        (strncmp(addr, ACPI_SIG_##TABLE, 4)==0)

typedef enum {
    ACPI_RSDP,
    ACPI_RSDT,
    ACPI_XSDT,
    ACPI_BERT,
    ACPI_CPEP,
    ACPI_ECDT,
    ACPI_EINJ,
    ACPI_ERST,
    ACPI_MADT,
    ACPI_MSCT,
    ACPI_SBST,
    ACPI_SLIT,
    ACPI_SRAT,
    ACPI_FADT,
    ACPI_FACS,
    ACPI_DSDT,
    ACPI_SSDT,
    ACPI_SPMI,
    ACPI_HPET,
    ACPI_BOOT,
    ACPI_SPCR,
    ACPI_DMAR,
    ACPI_ASF ,
    ACPI_HEST,
    ACPI_MCFG,
    ACPI_ASPT,
    /* This must come directly after valid types */
    ACPI_NTYPES,
    /* These are special and must come last */
    ACPI_UNKNOWN_TYPE, /* the region type is unknown */
    ACPI_CONSOLIDATED, /* multiple tables exist here */
    ACPI_AVAILABLE,    /* Memory range available for writing */
    ACPI_AVAILABLE_PTR /* ACPI_AVAILABLE and suitable for pointers */
} region_type_t;

/*
 * type ID <-> signiture conversions
 */
const char*
acpi_sig_str(region_type_t);

region_type_t
acpi_sig_id(const char* sig);

#endif /* __REGIONS_H__ */
