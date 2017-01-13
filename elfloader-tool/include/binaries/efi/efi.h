/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _EFI_H_
#define _EFI_H_

#include <printf.h>
#include <types.h>

typedef struct {
    uint64_t signature;
    uint32_t revision;
    uint32_t headersize;
    uint32_t crc32;
    uint32_t reserved;
} efi_table_hdr_t;

typedef struct {
    efi_table_hdr_t hdr;
    uint64_t fw_vendor;
    uint32_t fw_revision;
    uint32_t padding_1;
    uint64_t con_in_handle;
    uint64_t con_in;
    uint64_t con_out_handle;
    uint64_t con_out;
    uint64_t stderr_handle;
    uint64_t stderr;
    uint64_t runtime;
    uint64_t boottime;
    uint32_t nr_tables;
    uint32_t padding_2;
    uint64_t tables;
} efi_system_table_t;

extern void *__application_handle;
extern efi_system_table_t *__efi_system_table;

#define EFI_SUCCESS                     0
#define EFI_LOAD_ERROR                  (1 | (1UL << ((BYTE_PER_WORD * 8) - 1)))
#define EFI_BUFFER_TOO_SMALL            (5 | (1UL << ((BYTE_PER_WORD * 8) - 1)))

/* EFI Memory types: */
#define EFI_RESERVED_TYPE                0
#define EFI_LOADER_CODE                  1
#define EFI_LOADER_DATA                  2
#define EFI_BOOT_SERVICES_CODE           3
#define EFI_BOOT_SERVICES_DATA           4
#define EFI_RUNTIME_SERVICES_CODE        5
#define EFI_RUNTIME_SERVICES_DATA        6
#define EFI_CONVENTIONAL_MEMORY          7
#define EFI_UNUSABLE_MEMORY              8
#define EFI_ACPI_RECLAIM_MEMORY          9
#define EFI_ACPI_MEMORY_NVS             10
#define EFI_MEMORY_MAPPED_IO            11
#define EFI_MEMORY_MAPPED_IO_PORT_SPACE 12
#define EFI_PAL_CODE                    13
#define EFI_PERSISTENT_MEMORY           14
#define EFI_MAX_MEMORY_TYPE             15

/* Attribute values: */
#define EFI_MEMORY_UC                   ((uint64_t)0x1ULL)                  /* uncached */
#define EFI_MEMORY_WC                   ((uint64_t)0x2ULL)                  /* write-coalescing */
#define EFI_MEMORY_WT                   ((uint64_t)0x4ULL)                  /* write-through */
#define EFI_MEMORY_WB                   ((uint64_t)0x8ULL)                  /* write-back */
#define EFI_MEMORY_UCE                  ((uint64_t)0x10ULL)                 /* uncached, exported */
#define EFI_MEMORY_WP                   ((uint64_t)0x1000ULL)               /* write-protect */
#define EFI_MEMORY_RP                   ((uint64_t)0x2000ULL)               /* read-protect */
#define EFI_MEMORY_XP                   ((uint64_t)0x4000ULL)               /* execute-protect */
#define EFI_MEMORY_MORE_RELIABLE        ((uint64_t)0x10000ULL)              /* higher reliability */
#define EFI_MEMORY_RO                   ((uint64_t)0x20000ULL)              /* read-only */
#define EFI_MEMORY_RUNTIME              ((uint64_t)0x8000000000000000ULL)   /* range requires runtime mapping */
#define EFI_MEMORY_DESCRIPTOR_VERSION   1

#define EFI_PAGE_BITS                   12
#define EFI_PAGE_SIZE                   (1UL << EFI_PAGE_BITS)

typedef struct {
    uint32_t type;
    uint32_t padding_1;
    uint64_t phys_addr;
    uint64_t virt_addr;
    uint64_t num_pages;
    uint64_t attribute;
} efi_memory_desc_t;

typedef struct {
    efi_table_hdr_t hdr;
    uintptr_t padding_1[4];
    unsigned long (*get_memory_map)(unsigned long *, void *, unsigned long *, unsigned long *, uint32_t *);
    unsigned long (*allocate_pool)(int, unsigned long, void **);
    unsigned long (*free_pool)(void *);
    uintptr_t padding_2[19];
    unsigned long (*exit_boot_services)(void *, unsigned long);
    uintptr_t padding_3[17];
} efi_boot_services_t;

typedef struct {
    uintptr_t padding_1;
    unsigned long (*output_string)(void *, void *);
    uintptr_t padding_2;
} efi_simple_text_output_protocol_t;

efi_boot_services_t *get_efi_boot_services(void);
efi_simple_text_output_protocol_t *get_efi_con_out(void);

void efi_early_init(uintptr_t application_handle, uintptr_t efi_system_table);
unsigned long efi_exit_boot_services(void);
int efi_fputc(int c, FILE *stream);

#endif /* _EFI_H_ */
