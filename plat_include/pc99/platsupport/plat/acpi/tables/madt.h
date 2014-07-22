/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __ACPI_H__
#error This file should not be included directly
#endif

#pragma pack(push,1)


#define ACPI_APIC_LOCAL            0x00
#define ACPI_APIC_IOAPIC           0x01
#define ACPI_APIC_ISO              0x02
#define ACPI_APIC_NMI              0x03
#define ACPI_APIC_LOCAL_NMI        0x04
#define ACPI_APIC_LOCAL_AO         0x05
#define ACPI_APIC_IOSAPIC          0x06
#define ACPI_APIC_SAPIC            0x07
#define ACPI_APIC_PINT_SRC         0x08
#define ACPI_APIC_LOCAL_X2APIC     0x09
#define ACPI_APIC_LOCAL_X2APIC_NMI 0x0A
#define ACPI_APIC_GIC              0x0B
#define ACPI_APIC_GICD             0x0C

/* interrupt control structure header */
typedef struct acpi_madt_ics_hdr {
    uint8_t type;
    uint8_t length;
} acpi_madt_ics_hdr_t;

/* Processor local APIC (0)*/
typedef struct acpi_madt_local_apic {
    acpi_madt_ics_hdr_t header;
    uint8_t             processor_id;
    uint8_t             apic_id;
    uint32_t            flags;
} acpi_madt_local_apic_t;

/* IO APIC structre (1)*/
typedef struct acpi_madt_ioapic {
    acpi_madt_ics_hdr_t header;
    uint8_t             ioapic_id;
    uint8_t             res; /* 0 */
    uint32_t             address;
    uint32_t            gs_interrupt_base;
} acpi_madt_ioapic_t;

/* Interrupt Source Override structure (2)*/
typedef struct acpi_madt_override {
    acpi_madt_ics_hdr_t header;
    uint8_t             bus;
    uint8_t             src;
    uint32_t            gs_interrupt;
    uint16_t            mps_inti_flags;
} acpi_madt_override_t;

/* NMI source structure (3)*/
typedef struct acpi_madt_nmi {
    acpi_madt_ics_hdr_t header;
    uint16_t            flags;
    uint32_t            gs_interrupt;
} acpi_madt_nmi_t;

/* Local APIC NMI structure (4)*/
typedef struct acpi_madt_locnmi {
    acpi_madt_ics_hdr_t header;
    uint8_t             processor_id;
    uint16_t            flags;
    uint8_t             local_apic_lint;
} acpi_madt_locnmi_t;

/* Local APIC address override structure (5) */
typedef struct acpi_madt_locoverride {
    acpi_madt_ics_hdr_t header;
    uint8_t             res[2];
    uint64_t             local_apic_address;
} acpi_madt_locoverride_t;

/* IO SAPIC structure (6)*/
typedef struct acpi_madt_sapic {
    acpi_madt_ics_hdr_t header;
    uint8_t             ioapic_id;
    uint8_t             res;
    uint32_t            gs_interrupt_base;
    uint64_t             iosapic_address;
} acpi_madt_sapic_t;

/* Processor Local SAPIC structure (7)*/
typedef struct acpi_madt_locsapic {
    acpi_madt_ics_hdr_t header;
    uint8_t             processor_id;
    uint8_t             locsapic_id;
    uint8_t             locsapic_eid;
    uint8_t             res[3];
    uint32_t            flags;
    uint32_t            processor_uid;
    /* \0 terminated string */
    char                processor_uid_str[];
} acpi_madt_locsapic_t;

/* Platform interrupt source (8)*/
typedef struct acpi_madt_platformis {
    acpi_madt_ics_hdr_t header;
    uint16_t            flags;
    uint8_t             int_type;
    uint8_t             processor_id;
    uint8_t             processor_eid;
    uint8_t             iosapic_vector;
    uint32_t            gs_interrupt;
    uint32_t            platform_int_src_flags;
} acpi_madt_platformis_t;

/* Processor local x2APIC structure (9)*/
typedef struct acpi_madt_locx2apic {
    acpi_madt_ics_hdr_t header;
    uint8_t             res[2]; /* 0 */
    uint32_t            local_x2apic_id;
    uint32_t            flags;
    uint32_t            processor_uid;
} acpi_madt_locx2apic;

/* Local x2 APIC NMI structure (A) */
typedef struct acpi_madt_locx2apicnmi {
    acpi_madt_ics_hdr_t header;
    uint16_t            flags;
    uint32_t            processor_uid;
    uint8_t             local_x2apic_lint;
    uint8_t             res[3]; /* 0 */
} acpi_madt_locx2apicnmi_t;

/* Processor Local GIC structure (B) */
typedef struct acpi_madt_gic {
    acpi_madt_ics_hdr_t header;
    uint8_t             res[2]; /* 0 */
    uint32_t            gic_id;
    uint32_t            processor_uid;
    uint32_t            flags;
    uint32_t            parking_prot_version;
    uint64_t             parked_address;
    uint64_t             gic_address;
} acpi_madt_gic_t;

/* GIC Distributor structure (C) */
typedef struct acpi_madt_gicdist {
    acpi_madt_ics_hdr_t header;
    uint8_t             res1[2];
    uint32_t            gic_id;
    uint64_t             gicdist_address;
    uint32_t            system_vector_base;
    uint8_t             res2[4];
} acpi_madt_gicdist_t;




/* MADT structure */
typedef struct acpi_madt {
    acpi_header_t header;
    uint32_t       local_int_crt_address;
    uint32_t      flags;
    /* list of APICS */
//    acpi_madt_ics_hdr_t ics;
} acpi_madt_t;

#pragma pack(pop)


/********************************
 **** MADT sub table helpers ****
 ********************************/

/* Retrieve the header of the first entry */
static inline acpi_madt_ics_hdr_t*
acpi_madt_first_ics(acpi_madt_t* tbl)
{
    return (acpi_madt_ics_hdr_t*)(tbl + 1);
}

/* Retrieve the next DMAR sub header */
static inline acpi_madt_ics_hdr_t*
acpi_madt_next_ics(acpi_madt_t* tbl,
                   acpi_madt_ics_hdr_t* hdr)
{
    void* next = (uint8_t*)hdr + hdr->length;
    void* end  = (uint8_t*)tbl + tbl->header.length;
    if (next < end) {
        return (acpi_madt_ics_hdr_t*)next;
    } else {
        return NULL;
    }
}

/* Retrieve the DMAR sub header located at the specified index */
static inline acpi_madt_ics_hdr_t*
acpi_madt_ics_at(acpi_madt_t* tbl, int index)
{
    acpi_madt_ics_hdr_t* next = acpi_madt_first_ics(tbl);
    while (next != NULL && index-- > 0) {
        next = acpi_madt_next_ics(tbl, next);
    }
    return next;
}

/* Retrieve the next DMAR sub header of the specified type */
static inline acpi_madt_ics_hdr_t*
acpi_madt_next_ics_type(acpi_madt_t* tbl,
        acpi_madt_ics_hdr_t* hdr, int type)
{
    do {
        hdr = acpi_madt_next_ics(tbl, hdr);
        if (hdr == NULL) {
            return NULL;
        }
        if (hdr->type == type) {
            return hdr;
        }
    } while (1);
}

/* Retrieve the first DMAR sub header of the specified type */
static inline acpi_madt_ics_hdr_t*
acpi_madt_first_ics_type(acpi_madt_t* tbl, int type)
{
    acpi_madt_ics_hdr_t* hdr = acpi_madt_first_ics(tbl);
    if (hdr == NULL) {
        return NULL;
    }

    if (hdr->type != type) {
        return acpi_madt_next_ics_type(tbl, hdr, type);
    } else {
        return hdr;
    }
}


