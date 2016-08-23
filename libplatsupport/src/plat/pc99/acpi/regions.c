/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <platsupport/plat/acpi/regions.h>
#include <platsupport/plat/acpi/acpi.h>

#include "regions.h"

#include <stdlib.h>
#include <string.h>

#undef DEBUG
#define DEBUG 0

#ifdef DEBUG
#  include <stdio.h>
#  define DPRINTF(lvl, ...) do{ if(lvl < DEBUG){printf(__VA_ARGS__);fflush(stdout);}}while(0)
#else
#  define DPRINTF() do{ /* nothing */ }while(0)
#  if DEBUG < 5
#    error NO DEBUG
#  endif
#endif

typedef struct {
    region_type_t t;
    const char* s;
} sig_map_t;

#define SIG_MAP_T(lbl) [ACPI_##lbl] = ACPI_SIG_##lbl
static const char* sig_map[] = {
    SIG_MAP_T(RSDP),
    SIG_MAP_T(RSDT),
    SIG_MAP_T(XSDT),
    SIG_MAP_T(BERT),
    SIG_MAP_T(CPEP),
    SIG_MAP_T(ECDT),
    SIG_MAP_T(EINJ),
    SIG_MAP_T(ERST),
    SIG_MAP_T(MADT),
    SIG_MAP_T(MSCT),
    SIG_MAP_T(SBST),
    SIG_MAP_T(SLIT),
    SIG_MAP_T(SRAT),
    SIG_MAP_T(FADT),
    SIG_MAP_T(FACS),
    SIG_MAP_T(DSDT),
    SIG_MAP_T(SSDT),
    SIG_MAP_T(SPMI),
    SIG_MAP_T(HPET),
    SIG_MAP_T(BOOT),
    SIG_MAP_T(SPCR),
    SIG_MAP_T(DMAR),
    SIG_MAP_T(ASF ),
    SIG_MAP_T(HEST),
    SIG_MAP_T(MCFG),
    SIG_MAP_T(ASPT)
};
#undef SIG_MAP_T

void qsort(void *base, size_t nmemb, size_t size,
           int(*compar)(const void *, const void *));

static int region_start_compar(const void* _r1, const void* _r2)
{
    Region_t* r1 = (Region_t*) _r1;
    Region_t* r2 = (Region_t*) _r2;

    // test start address
    if (r1->start < r2->start) {
        return 1;
    }
    if (r1->start > r2->start) {
        return -1;
    }

    // test size
    if (r1->size < r2->size) {
        return 1;
    }
    if (r1->size > r2->size) {
        return -1;
    }

    /* equality */
    return 0;
}

void
consolidate_regions(const RegionList_t* regions_in, RegionList_t* consolidated)
{
    int i, j;
    memcpy(consolidated, regions_in, sizeof(RegionList_t));
    sort_regions(consolidated);
    for (i = 0, j = 0; j < consolidated->region_count; i++, j++) {
        if (consolidated->regions[i].start + consolidated->regions[i].size ==
                consolidated->regions[j].start) {
            consolidated->regions[i].size += consolidated->regions[j].size;
            j++; /* incremented again by for loop */
        } else {
            consolidated[i] = consolidated[j];
        }
    }
}


void
sort_regions(RegionList_t* regions)
{
    qsort(regions->regions, regions->region_count,
          sizeof(regions->regions), &region_start_compar);
}

const char*
acpi_sig_str(region_type_t t)
{
    if (t < ACPI_NTYPES) {
        return sig_map[t];
    } else {
        return "<none>";
    }
}

region_type_t
acpi_sig_id(const char* sig)
{
    if (strncmp(sig_map[ACPI_RSDP], sig, 6) == 0) {
        return ACPI_RSDP;
    }

    region_type_t i;
    for (i = 0; i < ACPI_NTYPES; i++)
        if (strncmp(sig_map[i], sig, 4) == 0) {
            return i;
        }

    return ACPI_UNKNOWN_TYPE;
}

int
add_region(RegionList_t* region_list, const Region_t region)
{
    if (region_list->region_count < MAX_REGIONS - 1) {
        int next_index = region_list->region_count++;
        region_list->regions[next_index] = region;
        return next_index;
    } else {
        return -1;
    }
}

int
remove_region(RegionList_t* region_list, int index)
{
    if (index >= region_list->region_count) {
        return !0;
    }

    region_list->region_count--;
    while (index < region_list->region_count) {
        region_list->regions[index] = region_list->regions[index + 1];
        index++;
    }

    return 0;
}



int
find_space(const RegionList_t* rlist, size_t size,
           region_type_t type)
{

    int best_fit = -1;
    const Region_t* best = NULL;

    int i;
    for (i = 0; i < rlist->region_count; i++) {
        DPRINTF(1, "examining %d of %d\n", i, rlist->region_count);
        const Region_t* this = rlist->regions + i;
        if (this->type == type && this->size > size) {
            if (best_fit == -1 || this->size < best->size) {
                DPRINTF(1, "Best fit!\n");
                best_fit = i;
                best = this;
            }
        }
    }
    return best_fit;
}

int
split_region(RegionList_t* rlist, int index, size_t size)
{
    int ret;
    Region_t* r = &rlist->regions[index];

    /* check params */
    if (index >= rlist->region_count || index < 0) {
        return -1;
    }
    if (r->size < size) {
        return -1;
    }

    /* create new region */
    ret = add_region_size(rlist, r->type, r->start, size, NOPARENT);
    if (ret >= 0) {
        /* adjust old region */
        r->size -= size;
        r->start += size;
    } else {
        /* Error */
    }
    return ret;
}

int
find_region(const RegionList_t* rlist, int start_index,
            region_type_t type)
{

    for (; start_index < rlist->region_count; start_index++) {
        if (rlist->regions[start_index].type == type) {
            return start_index;
        }
    }
    return -1;
}


acpi_header_t *
acpi_find_region(acpi_t *acpi, region_type_t region)
{
    RegionList_t *regions = acpi->regions;
    int index = find_region(regions, 0, region);

    if (index == -1) {
        return NULL;
    }

    return (acpi_header_t *) regions->regions[index].start;
}
