/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "browser.h"

#ifdef CONFIG_LIB_SEL4_ACPI_DEBUG

#include "acpi.h"
#include "regions.h"
#include "printer.h"

#include <stdio.h>
#include <string.h>

static int _browse_tables(void *table, size_t offset)
{
    /*
     * NOTE: offset added to address JUST BEFORE moving
     * to the next table
     */
    void *vector[26] = {NULL};

    printf("reading table at %p\n", table);

    do {

        // print table and set links
        printf("\n");
        region_type_t table_type = acpi_sig_id(table);
        char input;

        switch (table_type) {
        case ACPI_RSDP: {
            printf("Root System Descriptor table Pointer\n");
            acpi_print_table(table);
            acpi_rsdp_t *rsdp = (acpi_rsdp_t *)table;
            vector[0] = (void *)rsdp->rsdt_address;
            vector[1] = (void *)(uintptr_t)rsdp->xsdt_address;
            printf("\n");
            printf("a - RSDT at %p\n", vector[0]);
            printf("b - XSDT at %p\n", vector[1]);
            break;
        }
        case ACPI_RSDT: {
            printf("Root System Descriptor Table\n");
            acpi_print_table(table);
            acpi_rsdt_t *rsdt = (acpi_rsdt_t *)table;
            printf("\n");
            int i = 0;
            uint32_t *entry = acpi_rsdt_first(rsdt);
            while (entry != NULL) {
                char sig[5];
                sig[4] = '\0';
                memcpy(sig, (char *)(*entry) + offset, 4);
                vector[i] = (void *)(*entry);
                printf("%c - %s table at %p\n", i + 'a', sig, vector[i]);
                i++;
                entry = acpi_rsdt_next(rsdt, entry);
            }
            break;
        }
        case ACPI_XSDT: {
            printf("eXtended root System Descriptor Table\n");
            acpi_print_table(table);
            acpi_xsdt_t *xsdt = (acpi_xsdt_t *)table;
            printf("\n");
            int i = 0;
            uint64_t *entry = acpi_xsdt_first(xsdt);
            while (entry != NULL) {
                char sig[5];
                sig[4] = '\0';
                char *_e = (char *)(uintptr_t)(*entry);
                memcpy(sig, _e, 4);
                vector[i] = _e;
                printf("%c - %s table at %p\n", i + 'a', sig, vector[i]);
                i++;
                entry = acpi_xsdt_next(xsdt, entry);
            }
            break;
        }
        case ACPI_FADT: {
            printf("Fixed ACPI Description Table\n");
            acpi_print_table(table);
            acpi_fadt_t *fadt = (acpi_fadt_t *)table;
            vector[0] = (void *)fadt->facs_address;
            vector[1] = (void *)fadt->dsdt_address;
            vector[2] = (void *)(uintptr_t)fadt->x_facs_address;
            vector[3] = (void *)(uintptr_t)fadt->x_dsdt_address;
            printf("\n");
            printf("a -  FACS at %p\n", vector[0]);
            printf("b -  DSDT at %p\n", vector[1]);
            printf("c - XFACS at %p\n", vector[2]);
            printf("d - XDSDT at %p\n", vector[3]);
            break;
        }
        case ACPI_FACS: {
            printf("Firmware ACPI Constrol Structure\n");
            acpi_print_table(table);
            acpi_facs_t *facs = (acpi_facs_t *)table;
            vector[0] = (void *)facs->firmware_walking_vector;
            vector[3] = (void *)(uintptr_t)facs->x_firmware_walking_vector;
            printf("\n");
            printf("a - Firware Walking vector at %p\n", vector[0]);
            printf("b - X Firware walking vector at %p\n", vector[1]);
            break;
        }
        case ACPI_MCFG:
            printf("PCI Memory mapped ConFiGuration\n");
            acpi_print_table(table);
            break;
        case ACPI_DSDT:
            printf("Differentiated System Description Table\n");
            acpi_print_table(table);
            break;
        case ACPI_SSDT:
            printf("Secondary System Description Table\n");
            acpi_print_table(table);
            break;
        case ACPI_SPMI:
            printf("Server Platform Management Interface table\n");
            acpi_print_table(table);
            break;
        case ACPI_HPET:
            printf("High Precision Event Timer table\n");
            acpi_print_table(table);
            break;
        case ACPI_SPCR:
            printf("Serial Port Console Redirection table\n");
            acpi_print_table(table);
            break;
        case ACPI_BOOT:
            printf("Boot flags table\n");
            acpi_print_table(table);
            break;
        case ACPI_DMAR:
            printf("DMA Remapping table\n");
            acpi_print_table(table);
            break;
        case ACPI_ASF :
            printf("Alert Standard Format table\n");
            acpi_print_table(table);
            break;
        case ACPI_HEST:
            printf("Hardware Error Source Table\n");
            acpi_print_table(table);
            break;
        case ACPI_ERST:
            printf("Error Record Serialisation Table\n");
            acpi_print_table(table);
            break;
        case ACPI_BERT:
            printf("Boot Error Record Table\n");
            acpi_print_table(table);
            break;
        case ACPI_EINJ:
            printf("Error INjection Table\n");
            acpi_print_table(table);
            break;
        case ACPI_MADT:
            printf("Multiple Apic Descriptor table\n");
            acpi_print_table(table);
            break;
        case ACPI_ASPT:
            printf("ASPT -- Unknown table\n");
            acpi_print_table(table);
            break;

        default:
            printf("Unknown Table\n");
            acpi_print_table(table);
        }

        // collect input and act
        do {
            printf(">>");
            input = getchar();

            if (input >= 'a' && input <= 'z') {
                // input steps into another table
                if (vector[input - 'a'] != NULL) {
                    void *next_tbl;
                    next_tbl = vector[input - 'a'] + offset;
                    if (_browse_tables(next_tbl, offset)) {
                        return 1 /* quit */;
                    } else {
                        input = 'p';
                    }
                }
            }
            // other options
            switch (input) {
            case 'p':
                break;
            case 'q':
                return 1;
            case '\\':
                return 0;
            case '?':
            default:
                printf("\n"
                       "p - print table\n"
                       "q - quit\n"
                       "\\ - go up 1 table level\n"
                       "[a-z] Traverse tables\n");
                break;
            }
        } while (input != 'p');
    } while (1);
}

void acpi_browse_tables(const acpi_rsdp_t *rsdp, size_t offset)
{
    _browse_tables((void *)rsdp, offset);
}

static int _browse_regions(const RegionList_t *rlist, int parent)
{
    /*
     * NOTE: We don't use offsets here because we are following
     * the absolute address listed in the region list instead of
     * the link addresses contained within the tables.
     */

    while (1) {
        /* print table at region "parent" */
        if (parent != NOPARENT) {
            acpi_print_table(rlist->regions[parent].start);
        } else {
            printf("Root tables\n");
        }

        /* print children */
        int children[MAX_REGIONS];
        int child_count = 0;
        int i;
        for (i = 0; i < rlist->region_count; i++) {
            const Region_t *r = rlist->regions + i;
            if (r->parent == parent) {
                const char *sig = acpi_sig_str(r->type);
                char key = child_count + 'a';
                printf("%c - %s at %p\n", key, sig, r->start);
                children[child_count++] = i;
            }
        }

        do {
            printf(">>");
            char input = getchar();
            printf("%c\n\n", input);

            unsigned char key = input - 'a';
            if (key < child_count) {
                // input steps into another table
                if (_browse_regions(rlist, children[key])) {
                    return 1 /* quit */;
                } else {
                    break; /* restart loop, print tables */
                }
            }

            // other options
            switch (input) {
            case 'p':
                break; /* restart loop, print tables */
            case 'q':
                return 1; /* exit */
            case '\\':
                return 0; /* up one level */
            case '?':
            default:
                printf("\n"
                       "p - print table\n"
                       "q - quit\n"
                       "\\ - go up 1 table level\n"
                       "[a-%c] Traverse tables\n",
                       'a' + child_count);
                break;
            }
        } while (1);
    }
    while (1);
}

void acpi_browse_regions(const RegionList_t *rlist)
{
    _browse_regions(rlist, NOPARENT);
}

#endif
