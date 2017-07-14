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

#include "printer.h"

#ifdef CONFIG_LIB_SEL4_ACPI_DEBUG

/************************
 **** Debug features ****
 ************************/
#include <acpi/acpi.h>
#include "acpi.h"

#include <stdio.h>
#include <string.h>

static char
pprint(char c)
{
    if (c < 0x32) {
        return '.';
    }
    if (c < 0x7F) {
        return c;
    }
    return '.';
}

void
colour_bf(int i, const char* ptr)
{
    if (i == 3 && ((*ptr) & 0xff) == 0xbf) {
        printf("\033[01;31m");
    }
}

void
colour_1a(int i, const char* ptr)
{
    (void)i;
    if (((*ptr) & 0xff) == 0x1a) {
        printf("\033[01;29m");
    }
}

void
colour_1d(int i, const char* ptr)
{
    (void)i;
    if (((*ptr) & 0xff) == 0x1d) {
        printf("\033[01;28m");
    }
}

static void
colour(int i, const char* ptr)
{
    colour_bf(i, ptr);
    colour_1a(i, ptr);
    colour_1d(i, ptr);
}

// print the raw table with "indent" printed before each column
void
acpi_print_table_raw(const void* start, int length)
{
    int i, j;
    const char *ptr = start;
    const char *end = start + length;

    while (1) {
        // store pointer to start of row for txt printing later
        const char* row_start = ptr;

        // Col1 : position
        printf("0x%p: ", row_start);
        // print hex values
        for (i =  0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                if (ptr == end) {
                    printf("   ");
                } else {
                    colour(j, ptr);
                    printf("%02x ", (*ptr++) & 0xff);
                    printf("\033[00m");
                }
            }
            printf("  ");
        }
        // print txt
        ptr = row_start;
        for (i =  0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                printf("%c", pprint(*ptr++));
                if (ptr == end) {
                    printf("\n");
                    return;
                }
            }
            printf(" ");
        }
        printf("\n");
    }
}

static void
print_path(acpi_dmar_dscope_t* dscope)
{
    int entries = acpi_dmar_dscope_path_length(dscope);
    acpi_device_path_t* path = acpi_dmar_path_first(dscope);
    printf("\t\t<<<Path>>>\n");
    while (entries-- > 0) {
        printf("\t\tPCI device number = 0x%02x, ", path->device & 0xff);
        printf("PCI function number = 0x%02x\n", path->function & 0xff);
        path++;
    }
}

static void
print_dscope(acpi_dmar_remap_hdr_t* head, acpi_dmar_dscope_t* dscope)
{
    while (dscope != NULL) {
        printf("\t<<Device Scope>>\n\t");
        switch (dscope->type) {
        case ACPI_DSCOPE_PCI_ENDPOINT:
            printf("PCI Endpoint Device\n");
            break;
        case ACPI_DSCOPE_PCI_BRIDGE:
            printf("PCI Sub-heirarchy\n");
            break;
        case ACPI_DSCOPE_IOAPIC:
            printf("IOAPIC 0x%02x\n", dscope->enum_id & 0xff);
            break;
        case ACPI_DSCOPE_HPET:
            printf("HPET timer 0x%02d\n", dscope->enum_id & 0xff);
            break;
        default:
            printf("Unknown device scope\n");
            acpi_print_table_raw(dscope, dscope->length);
        }

        if (ACPI_DSCOPE_VALID(dscope->type)) {
            printf("\tStart bus number 0x%02x\n",
                   dscope->start_bus_number & 0xff);
        }

        print_path(dscope);

        dscope = acpi_dmar_next_dscope(head, dscope);
    }
}

static void
print_dmar(acpi_dmar_hdr_t* dmar)
{
    acpi_print_table_raw(dmar, dmar->header.length);

    acpi_dmar_remap_hdr_t* sub = acpi_dmar_first_remap(dmar);
    while (sub != NULL) {
        switch (sub->type) {
        case ACPI_DMAR_DRHD_TYPE:
            printf("\n<DMA Remapping Hardware Unit Definition>\n");
            acpi_dmar_drhd_t* drhd = (acpi_dmar_drhd_t*)sub;
            printf("Flags 0x%02x\n", drhd->flags & 0xff);
            printf("Segment number 0x%04x\n",
                   drhd->segment_number & 0xffff);
            printf("Base address 0x%016lx\n",
                   (unsigned long)drhd->register_address);
//            acpi_print_table_raw(drhd, drhd->header.length, "");
            print_dscope(sub, acpi_dmar_drhd_first_dscope(drhd));
            break;
        case ACPI_DMAR_RMRR_TYPE:
            printf("\n<Reserved Memory Region Reporting>\n");
            acpi_dmar_rmrr_t* rmrr = (acpi_dmar_rmrr_t*)sub;
            printf("Segment number 0x%04x\n",
                   rmrr->segment_number & 0xffff);
            printf("Memory range 0x%016lx -> 0x%016lx\n",
                   (unsigned long)rmrr->base_address,
                   (unsigned long)rmrr->limit_address);
//            acpi_print_table_raw(rmrr, rmrr->header.length, "");
            print_dscope(sub, acpi_dmar_rmrr_first_dscope(rmrr));
            break;
        case ACPI_DMAR_ATSR_TYPE:
            printf("\n<root port ATS capbility Reporting>\n");
            acpi_dmar_atsr_t* atsr = (acpi_dmar_atsr_t*)sub;
            printf("Flags 0x%02x\n", atsr->flags & 0xff);
            printf("Segment number 0x%04x\n",
                   atsr->segment_number & 0xffff);
//            acpi_print_table_raw(atsr, atsr->header.length, "");
            print_dscope(sub, acpi_dmar_atsr_first_dscope(atsr));
            break;
        case ACPI_DMAR_RHSA_TYPE:
            printf("\n<Remapping Hardware Affinity>\n");
            acpi_dmar_rhsa_t* rhsa = (acpi_dmar_rhsa_t*)sub;
            printf("Base address 0x%016lx\n",
                   (unsigned long)rhsa->base_address);
            printf("Proximity domain 0x%08x\n",
                   rhsa->proximity_domain);
//            acpi_print_table_raw(rhsa, sizeof(*rhsa), "");
            break;
        default:
            printf("\n<Unknown remapping structure>\n");
            acpi_print_table_raw(sub, sizeof(*sub));
            break;
        }
        sub = acpi_dmar_next_remap(dmar, sub);
    }
}

static void
print_mcfg_desc(acpi_mcfg_desc_t* mcfg_desc)
{
    printf("<PCI Device Description %p>\n", mcfg_desc);
    printf("Address: 0x%016lx\n", (unsigned long)mcfg_desc->address);
    printf("Segment 0x%04x\n", mcfg_desc->segment);
    printf("Bus 0x%02x - 0x%02x\n", mcfg_desc->bus_end,
           mcfg_desc->bus_start);
}

static void
print_mcfg(acpi_mcfg_t* mcfg)
{
    acpi_print_table_raw(mcfg, mcfg->header.length);
    acpi_mcfg_desc_t* cur = acpi_mcfg_desc_first(mcfg);
    while (cur != NULL) {
        print_mcfg_desc(cur);
        cur = acpi_mcfg_desc_next(mcfg, cur);
    }
}

static void
print_rsdt(acpi_rsdt_t* rsdt)
{
    acpi_print_table_raw(rsdt, rsdt->header.length);
    printf("Child tables:\n");
    uint32_t* next = acpi_rsdt_first(rsdt);
    int entries = acpi_rsdt_entry_count(rsdt);
    int i = 0;
    while (next != NULL) {
        printf("%d/%d -> %p\n", i++, entries, (void*)*next);
        next = acpi_rsdt_next(rsdt, next);
    }
    printf("\n");
}

static void
print_xsdt(acpi_xsdt_t* xsdt)
{
    acpi_print_table_raw(xsdt, xsdt->header.length);
    printf("Child tables:\n");
    uint64_t* next = acpi_xsdt_first(xsdt);
    int entries = acpi_xsdt_entry_count(xsdt);
    int i = 0;
    while (next != NULL) {
        printf("%d/%d -> %p\n", i++, entries,
               (void*)(uintptr_t)*next);
        next = acpi_xsdt_next(xsdt, next);
    }
    printf("\n");
}

static void
print_rsdp(acpi_rsdp_t* rsdp)
{
    acpi_print_table_raw(rsdp, rsdp->length);
    printf("RSDT->%p\n", (void*)rsdp->rsdt_address);
    printf("XSDT->%p\n", (void*)(uintptr_t)rsdp->xsdt_address);
    printf("\n");
}

static void
print_madt(acpi_madt_t* madt)
{
    acpi_print_table_raw(madt, madt->header.length);
    acpi_madt_ics_hdr_t* entry = acpi_madt_first_ics(madt);
    while (entry != NULL) {
        char* txt;
        switch (entry->type) {
        case ACPI_APIC_LOCAL:
            txt = "LOCAL";
            break;
        case ACPI_APIC_ISO:
            txt = "ISO";
            break;
        case ACPI_APIC_NMI:
            txt = "NMI";
            break;
        case ACPI_APIC_LOCAL_NMI:
            txt = "LOCAL NMI";
            break;
        case ACPI_APIC_LOCAL_AO:
            txt = "LOCAL AO";
            break;
        case ACPI_APIC_SAPIC:
            txt = "SAPIC";
            break;
        case ACPI_APIC_PINT_SRC:
            txt = "PINT SRC";
            break;
        case ACPI_APIC_LOCAL_X2APIC:
            txt = "LOCAL X2APIC";
            break;
        case ACPI_APIC_LOCAL_X2APIC_NMI:
            txt = "LOCAL X2APIC_NMI";
            break;
        case ACPI_APIC_GIC:
            txt = "GIC";
            break;
        case ACPI_APIC_GICD:
            txt = "GICD";
            break;
        case ACPI_APIC_IOSAPIC:
            txt = "IOSAPIC";
            break;
        case ACPI_APIC_IOAPIC:
            txt = "IOAPIC";
            break;
        default:
            txt = "Unknown";
            break;
        }
        printf("<<%s>>\n", txt);
        acpi_print_table_raw(entry, entry->length);
        entry = acpi_madt_next_ics(madt, entry);
    }
}

void
acpi_print_table(const void* start)
{
    // for now, just find the length of the table and print
    // in raw format
    int len = acpi_table_length(start);
    if (len > 0) {
        if (ACPI_TABLE_TEST(start, DMAR)) {
            print_dmar((acpi_dmar_hdr_t*)start);
        } else if (ACPI_TABLE_TEST(start, MCFG)) {
            print_mcfg((acpi_mcfg_t*)start);
        } else if (ACPI_TABLE_TEST(start, RSDT)) {
            print_rsdt((acpi_rsdt_t*)start);
        } else if (ACPI_TABLE_TEST(start, XSDT)) {
            print_xsdt((acpi_xsdt_t*)start);
        } else if (ACPI_TABLE_TEST(start, RSDP)) {
            print_rsdp((acpi_rsdp_t*)start);
        } else if (ACPI_TABLE_TEST(start, MADT)) {
            print_madt((acpi_madt_t*)start);
        } else {
            acpi_print_table_raw(start, len);
        }
    }
}

void
acpi_print_regions(const RegionList_t* rl)
{
    printf("\n");
    printf("index | Signiture  | Address                | "
           "Adjusted address       | parent index\n");
    int i;
    for (i = 0; i < 83; i++) {
        printf("-");
    }
    printf("\n");

    for (i = 0; i < rl->region_count; i++) {
        const Region_t* r = rl->regions + i;
        const char* sig = acpi_sig_str(r->type);
        int sig_len = strlen(sig);
        printf("  %2d  | ", i);
        printf("\"%s\"",  acpi_sig_str(r->type));
        while (sig_len++ < 9) {
            printf(" ");
        }
        printf("| ");

        printf("%p->%p | ", r->start, r->start + r->size);
        printf("%p->", r->start - rl->offset);
        printf("%p | ", r->start + r->size - rl->offset);
        printf("%3d\n", r->parent);
    }
}

#endif
