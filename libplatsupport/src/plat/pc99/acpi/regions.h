/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/plat/acpi/regions.h>

#include <assert.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_REGIONS 20
#define NOPARENT -1

typedef struct Region {
    region_type_t type;
    void *start;
    size_t size;
    int parent;
} Region_t;

typedef struct RegionList {
    Region_t regions[MAX_REGIONS];
    int region_count;
    /*
     * This offset is added to region start addresses when
     * traversing/updating links within regions.
     */
    size_t offset;
} RegionList_t;

#define REGIONLIST_INIT(_offset) (RegionList_t) \
    { \
         .region_count = 0, \
         .offset = _offset \
    }

/*
 * Join contiguous regions
 */
void
consolidate_regions(const RegionList_t *regions_in, RegionList_t *consolidated);

/*
 * Sort regions by start address
 */
void
sort_regions(RegionList_t *regions);

/*
 * Add a region to a region list
 * Returns the index that the region was placed at,
 * Returns -1 if the list is full
 */
int
add_region(RegionList_t *region_list, const Region_t r);

static inline
Region_t new_region(region_type_t type, void *start, size_t size, int parent)
{
    Region_t r;
    r.type = type;
    r.start = start;
    r.size = size;
    r.parent = parent;

    return r;
}

static inline int add_region_size(RegionList_t *region_list, region_type_t type,
                                  void *start, size_t size, int parent)
{
    return add_region(region_list, new_region(type, start, size, parent));
}

static inline int add_region_range(RegionList_t *region_list, region_type_t type,
                                   void *start, const void *end, int parent)
{
    return add_region(region_list, new_region(type, start, end - start, parent));
}
/*
 * remove the region at index "index" from the table
 * returns 0 on success, !0 if the index was invalid
 */
int
remove_region(RegionList_t *region_list, int index);

static inline int remove_region_last(RegionList_t *region_list)
{
    int index = region_list->region_count - 1;
    return remove_region(region_list, index);
}

/*
 * Find the smallest region that matches "type"
 * and is greater than or equal to "size"
 * Returns an index to the region in region_list or -1 on error.
 *
 */
int
find_space(const RegionList_t *region_list, size_t size,
           region_type_t type);

/*
 * Finds the next occurance of "type" in the region list. The
 * search starts at "start_index".
 */
int
find_region(const RegionList_t *rlist, int start_index,
            region_type_t type);

/*
 * Splits the region at index. A new region is created which
 * takes "size" bytes from the region at index.
 * Returns the index of the new region or -1 on error
 */
int
split_region(RegionList_t *region_list, int index, size_t size);

