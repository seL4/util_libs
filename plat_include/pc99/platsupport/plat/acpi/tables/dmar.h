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

/* Generic entry header */
typedef struct acpi_dmar_remap_hdr {
    uint16_t type;
    uint16_t length;
} acpi_dmar_remap_hdr_t;

/* DMA remapping table "DMAR" */
typedef struct acpi_dmar_hdr {
    acpi_header_t      header;
    uint8_t            host_address_width;
    uint8_t            flags;
    uint8_t            res[10];
    /* first remapping structure in "linked list" */
//    acpi_dmar_remap_hdr_t sheader;
} acpi_dmar_hdr_t;



/*******************
 *** Entry types ***
 *******************/

#define ACPI_DMAR_DRHD_TYPE 0
#define ACPI_DMAR_RMRR_TYPE 1
#define ACPI_DMAR_ATSR_TYPE 2
#define ACPI_DMAR_RHSA_TYPE 3
#define ACPI_DMAR_TYPE_IS_VALID(x) ((x) < 4)





// device path structure
typedef struct acpi_device_path {
    uint8_t device;
    uint8_t function;
} acpi_device_path_t;

// device scope structure
typedef struct acpi_dmar_dscope {
    uint8_t            type;
    uint8_t            length;
    uint8_t            res[2];
    uint8_t            enum_id;
    uint8_t            start_bus_number;
    /*
     * We could access this field as an array, but for
     * consistancy, we will treat it as a linked list
     */
//    acpi_device_path_t path[];
} acpi_dmar_dscope_t;

/********************
 *** DScope types ***
 ********************/
#define ACPI_DSCOPE_PCI_ENDPOINT 0x01
#define ACPI_DSCOPE_PCI_BRIDGE   0x02
#define ACPI_DSCOPE_IOAPIC       0x03
#define ACPI_DSCOPE_HPET         0x04
#define ACPI_DSCOPE_VALID(x)     \
         ((uint8_t)( (x) - ACPI_DSCOPE_PCI_ENDPOINT ) < 4)


/******************************
 **** Sub tables of DMAR ******
 ******************************/

// Reserved Memory Region Report
typedef struct acpi_dmar_rmrr {
    acpi_dmar_remap_hdr_t header;
    uint8_t               res[2];
    uint16_t              segment_number;
    uint64_t               base_address;
    uint64_t               limit_address;
    /* first device scope in "linked list" */
//    acpi_dmar_dscope_t device_scope;
} acpi_dmar_rmrr_t;

// DMA Remapping Hardware unit Definition
typedef struct acpi_dmar_drhd {
    acpi_dmar_remap_hdr_t header;
    uint8_t               flags;
    uint8_t               res[1];
    uint16_t              segment_number;
    uint64_t               register_address;
    /* first device scope in "linked list" */
//    acpi_dmar_dscope_t device_scope;
} acpi_dmar_drhd_t;


// Root Port ATS Capability Reporting
typedef struct acpi_dmar_atsr {
    acpi_dmar_remap_hdr_t header;
    uint8_t               flags;
    uint8_t               res[1];
    uint16_t              segment_number;
    /* first device scope in "linked list" */
//    acpi_dmar_dscope_t device_scope;
} acpi_dmar_atsr_t;

// Remaping Hardware Static Affinity structure
typedef struct acpi_dmar_rhsa {
    acpi_dmar_remap_hdr_t header;
    uint8_t            res[4];
    uint64_t            base_address;
    uint32_t           proximity_domain;
} acpi_dmar_rhsa_t;

#pragma pack(pop)



/********************************
 **** DMAR sub table helpers ****
 ********************************/

/* Retrieve the header of the first entry */
static inline acpi_dmar_remap_hdr_t*
acpi_dmar_first_remap(acpi_dmar_hdr_t* tbl)
{
    return (acpi_dmar_remap_hdr_t*)(tbl + 1);
}

/* Retrieve the next DMAR sub header */
static inline acpi_dmar_remap_hdr_t*
acpi_dmar_next_remap(acpi_dmar_hdr_t* tbl,
        acpi_dmar_remap_hdr_t* hdr)
{
    void* next = (uint8_t*)hdr + hdr->length;
    void* end  = (uint8_t*)tbl + tbl->header.length;
    if (next < end) {
        return (acpi_dmar_remap_hdr_t*)next;
    } else {
        return NULL;
    }
}

/* Retrieve the DMAR sub header located at the specified index */
static inline acpi_dmar_remap_hdr_t*
acpi_dmar_remap_at(acpi_dmar_hdr_t* tbl, int index)
{
    acpi_dmar_remap_hdr_t* next = acpi_dmar_first_remap(tbl);
    while (next != NULL && index-- > 0) {
        next = acpi_dmar_next_remap(tbl, next);
    }
    return next;
}

/* Retrieve the next DMAR sub header of the specified type */
static inline acpi_dmar_remap_hdr_t*
acpi_dmar_next_remap_type(acpi_dmar_hdr_t* tbl,
        acpi_dmar_remap_hdr_t* hdr, int type)
{
    do {
        hdr = acpi_dmar_next_remap(tbl, hdr);
        if (hdr == NULL) {
            return NULL;
        }
        if (hdr->type == type) {
            return hdr;
        }
    } while (1);
}

/* Retrieve the first DMAR sub header of the specified type */
static inline acpi_dmar_remap_hdr_t*
acpi_dmar_first_remap_type(acpi_dmar_hdr_t* tbl, int type)
{
    acpi_dmar_remap_hdr_t* hdr = acpi_dmar_first_remap(tbl);
    if (hdr == NULL) {
        return NULL;
    }

    if (hdr->type != type) {
        return acpi_dmar_next_remap_type(tbl, hdr, type);
    } else {
        return hdr;
    }
}


/***********************************
 **** DMAR device scope helpers ****
 ***********************************/

/* Retrieve the first device scope for a DRHD table */
static inline acpi_dmar_dscope_t*
acpi_dmar_drhd_first_dscope(acpi_dmar_drhd_t* h)
{
    return (acpi_dmar_dscope_t*)(h + 1);
}

/* Retrieve the first device scope for an RMRR table */
static inline acpi_dmar_dscope_t*
acpi_dmar_rmrr_first_dscope(acpi_dmar_rmrr_t* h)
{
    return (acpi_dmar_dscope_t*)(h + 1);
}

/* Retrieve the first device scope for an ATSR table */
static inline acpi_dmar_dscope_t*
acpi_dmar_atsr_first_dscope(acpi_dmar_atsr_t* h)
{
    return (acpi_dmar_dscope_t*)(h + 1);
}

/* Retrieve the first device scope (table independant) */
static inline acpi_dmar_dscope_t*
acpi_dmar_first_dscope(acpi_dmar_remap_hdr_t* h)
{
    switch (h->type) {
    case ACPI_DMAR_DRHD_TYPE:
        return acpi_dmar_drhd_first_dscope((acpi_dmar_drhd_t*)h);
    case ACPI_DMAR_RMRR_TYPE:
        return acpi_dmar_rmrr_first_dscope((acpi_dmar_rmrr_t*)h);
    case ACPI_DMAR_ATSR_TYPE:
        return acpi_dmar_atsr_first_dscope((acpi_dmar_atsr_t*)h);
    case ACPI_DMAR_RHSA_TYPE:
        return NULL; /* RHSA has no device scope */
    default:
        return NULL;
    }
}




/* Retrieve the next device scope */
static inline acpi_dmar_dscope_t*
acpi_dmar_next_dscope(acpi_dmar_remap_hdr_t* sh, acpi_dmar_dscope_t* scope)
{
    void* next = (uint8_t*)scope + scope->length;
    void* end  = (uint8_t*)sh + sh->length;
    if (next < end) {
        return (acpi_dmar_dscope_t*)next;
    } else {
        return NULL;
    }
}

/* Retrieve the device scope at the given index */
static inline acpi_dmar_dscope_t*
acpi_dmar_dscope_at(acpi_dmar_remap_hdr_t* sh, int index)
{
    acpi_dmar_dscope_t* next = acpi_dmar_first_dscope(sh);
    while (next != NULL && index-- > 0) {
        next = acpi_dmar_next_dscope(sh, next);
    }
    return next;
}

static inline acpi_device_path_t*
acpi_dmar_path_first(acpi_dmar_dscope_t* dscope)
{
    return (acpi_device_path_t*)(dscope + 1);
}

static inline int
acpi_dmar_dscope_path_length(acpi_dmar_dscope_t* dscope)
{
    int path_bytes = dscope->length - sizeof(*dscope);
    return path_bytes / sizeof(acpi_device_path_t);
}


