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


/* Alert Standard Format table */
typedef struct acpi_asf_hdr {
    acpi_header_t header;
    /* first item in "linked list" of records */
    // acpi_asf_rec_hdr_t records;
} acpi_asf_hdr_t;

typedef struct acpi_asf_rec_hdr_t {
    uint8_t  type;
    uint8_t  res[1];
    uint16_t length;
} acpi_asf_rec_hdr_t;

#define ACPI_ASF_INFO 0x00
#define ACPI_ASF_ALRT 0x10
#define ACPI_ASF_RCTL 0x02
#define ACPI_ASF_RMCP 0x03
#define ACPI_ASF_ADDR 0x04

#define ACPI_ASF_LAST (1<<7)
#define ACPI_ASF_TYPE(x) ((x) & ~(ACPI_ASF_LAST))
#define ACPI_ASF_IS_LAST_REC(x) (((x)->type) & ACPI_ASF_LAST)
typedef struct acpi_asf_info {
    acpi_asf_rec_hdr_t header;
    uint8_t            min_wdt_rst_val;
    uint8_t            min_interpoll_wait;
    uint16_t           sys_id;
    uint32_t           iana_id;
    uint8_t            res[4]; /* 0 */
} acpi_asf_info_t;

typedef struct acpi_asf_dev_array {
    uint8_t dev_address;
    uint8_t cmd;
    uint8_t mask;
    uint8_t compare_value;
    uint8_t event_sensor_type;
    uint8_t event_type;
    uint8_t event_offset;
    uint8_t event_source_type;
    uint8_t event_severity;
    uint8_t sensor_number;
    uint8_t entity;
    uint8_t entity_instance;
} acpi_asf_dev_array_t;

typedef struct acpi_asf_alrt {
    acpi_asf_rec_hdr_t   header;
    uint8_t              res[2]; /* 0 */
    uint8_t              n_alerts; /* 1-8 */
    uint8_t              element_length;
//    acpi_asf_dev_array_t device;
} acpi_asf_alrt_t;

typedef struct acpi_asf_ctl_array {
    uint8_t function;
    uint8_t device_address;
    uint8_t command;
    uint8_t data;
} acpi_asf_ctl_array_t;

typedef struct acpi_asf_rctl {
    acpi_asf_rec_hdr_t header;
    uint8_t            n_controls;
    uint8_t            element_length;
//    acpi_asf_ctl_array_t device;
} acpi_asf_rctl_t;

typedef struct acpi_asf_rmcp {
    acpi_asf_rec_hdr_t header;
    uint8_t            rc_caps[7];
    uint8_t            completion_code;
    uint32_t           iana_id;
    uint8_t            special_cmd;
    uint16_t           special_cmd_param;
    uint16_t           boot_options;
    uint16_t           oem_param;
} acpi_asf_rmcp_t;


typedef struct acpi_asf_addr_smb {
    uint8_t smb;
} acpi_asf_addr_smb_t;

typedef struct acpi_asf_addr {
    acpi_asf_rec_hdr_t header;
    uint8_t            seeprom_address;
    uint8_t            num_devices;
    // acpi_asf_addr_smbus_t smb;
} acpi_asf_addr_t;


#pragma pack(pop)

/****************************
 **** ASF record helpers ****
 ****************************/

static inline acpi_asf_rec_hdr_t*
acpi_asf_first_rec(acpi_asf_hdr_t* h)
{
    return (acpi_asf_rec_hdr_t*)((char*)h + sizeof(*h));
}

static inline acpi_asf_rec_hdr_t*
acpi_asf_next_rec(acpi_asf_hdr_t* h, acpi_asf_rec_hdr_t* cur)
{
    (void)h;
    if (ACPI_ASF_IS_LAST_REC(cur)) {
        return NULL;
    }
    return (acpi_asf_rec_hdr_t*)((char*)cur + cur->length);
}

/**********************
 **** ALRT helpers ****
 ***********************/

static inline acpi_asf_dev_array_t*
acpi_asf_alrt_first_dev(acpi_asf_alrt_t* h)
{
    return (acpi_asf_dev_array_t*)((char*)h + sizeof(*h));
}

static inline acpi_asf_dev_array_t*
acpi_asf_alrt_next_dev(acpi_asf_alrt_t* h, acpi_asf_dev_array_t* cur)
{
    char* next = (char*)cur + h->element_length;
    char* end  = (char*)h + h->header.length;
    if (next < end) {
        return (acpi_asf_dev_array_t*)next;
    } else {
        return NULL;
    }
}


/**********************
 **** RCTL helpers ****
 ***********************/

static inline acpi_asf_ctl_array_t*
acpi_asf_rctl_first_ctl(acpi_asf_rctl_t* h)
{
    return (acpi_asf_ctl_array_t*)((char*)h + sizeof(*h));
}

static inline acpi_asf_ctl_array_t*
acpi_asf_rtcl_next_ctl(acpi_asf_rctl_t* h, acpi_asf_ctl_array_t* cur)
{
    char* next = (char*)cur + h->element_length;
    char* end  = (char*)h + h->header.length;
    if (next < end) {
        return (acpi_asf_ctl_array_t*)next;
    } else

    {
        return NULL;
    }
}


/**********************
 **** ADDR helpers ****
 ***********************/

static inline acpi_asf_addr_smb_t*
acpi_asf_addr_smb_first(acpi_asf_addr_t* h)
{
    return (acpi_asf_addr_smb_t*)((char*)h + sizeof(*h));
}

static inline acpi_asf_addr_smb_t*
acpi_asf_addr_smb_next(acpi_asf_addr_t* h, acpi_asf_addr_smb_t* cur)
{
    char* next = (char*)(cur + 1);
    char* end  = (char*)h + h->header.length;
    if (next < end) {
        return (acpi_asf_addr_smb_t*)next;
    } else

    {
        return NULL;
    }
}



