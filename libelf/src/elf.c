/* @LICENSE(UNSW_OZPLB) */

/*
 * Australian Public Licence B (OZPLB)
 *
 * Version 1-0
 *
 * Copyright (c) 2004 University of New South Wales
 *
 * All rights reserved.
 *
 * Developed by: Operating Systems and Distributed Systems Group (DiSy)
 *               University of New South Wales
 *               http://www.disy.cse.unsw.edu.au
 *
 * Permission is granted by University of New South Wales, free of charge, to
 * any person obtaining a copy of this software and any associated
 * documentation files (the "Software") to deal with the Software without
 * restriction, including (without limitation) the rights to use, copy,
 * modify, adapt, merge, publish, distribute, communicate to the public,
 * sublicense, and/or sell, lend or rent out copies of the Software, and
 * to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimers.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimers in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of University of New South Wales, nor the names of its
 *       contributors, may be used to endorse or promote products derived
 *       from this Software without specific prior written permission.
 *
 * EXCEPT AS EXPRESSLY STATED IN THIS LICENCE AND TO THE FULL EXTENT
 * PERMITTED BY APPLICABLE LAW, THE SOFTWARE IS PROVIDED "AS-IS", AND
 * NATIONAL ICT AUSTRALIA AND ITS CONTRIBUTORS MAKE NO REPRESENTATIONS,
 * WARRANTIES OR CONDITIONS OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO ANY REPRESENTATIONS, WARRANTIES OR CONDITIONS
 * REGARDING THE CONTENTS OR ACCURACY OF THE SOFTWARE, OR OF TITLE,
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT,
 * THE ABSENCE OF LATENT OR OTHER DEFECTS, OR THE PRESENCE OR ABSENCE OF
 * ERRORS, WHETHER OR NOT DISCOVERABLE.
 *
 * TO THE FULL EXTENT PERMITTED BY APPLICABLE LAW, IN NO EVENT SHALL
 * NATIONAL ICT AUSTRALIA OR ITS CONTRIBUTORS BE LIABLE ON ANY LEGAL
 * THEORY (INCLUDING, WITHOUT LIMITATION, IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHERWISE) FOR ANY CLAIM, LOSS, DAMAGES OR OTHER
 * LIABILITY, INCLUDING (WITHOUT LIMITATION) LOSS OF PRODUCTION OR
 * OPERATION TIME, LOSS, DAMAGE OR CORRUPTION OF DATA OR RECORDS; OR LOSS
 * OF ANTICIPATED SAVINGS, OPPORTUNITY, REVENUE, PROFIT OR GOODWILL, OR
 * OTHER ECONOMIC LOSS; OR ANY SPECIAL, INCIDENTAL, INDIRECT,
 * CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES, ARISING OUT OF OR IN
 * CONNECTION WITH THIS LICENCE, THE SOFTWARE OR THE USE OF OR OTHER
 * DEALINGS WITH THE SOFTWARE, EVEN IF NATIONAL ICT AUSTRALIA OR ITS
 * CONTRIBUTORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH CLAIM, LOSS,
 * DAMAGES OR OTHER LIABILITY.
 *
 * If applicable legislation implies representations, warranties, or
 * conditions, or imposes obligations or liability on University of New South
 * Wales or one of its contributors in respect of the Software that
 * cannot be wholly or partly excluded, restricted or modified, the
 * liability of University of New South Wales or the contributor is limited, to
 * the full extent permitted by the applicable legislation, at its
 * option, to:
 * a.  in the case of goods, any one or more of the following:
 * i.  the replacement of the goods or the supply of equivalent goods;
 * ii.  the repair of the goods;
 * iii. the payment of the cost of replacing the goods or of acquiring
 *  equivalent goods;
 * iv.  the payment of the cost of having the goods repaired; or
 * b.  in the case of services:
 * i.  the supplying of the services again; or
 * ii.  the payment of the cost of having the services supplied again.
 *
 * The construction, validity and performance of this licence is governed
 * by the laws in force in New South Wales, Australia.
 */
#include <elf/elf.h>
#include <elf/elf32.h>
#include <elf/elf64.h>
#include <string.h>
#include <stdio.h>

/* ELF header functions */
int
elf_newFile(void *file, size_t size, elf_t *res)
{
    return elf_newFile_maybe_unsafe(file, size, true, true, res);
}

int
elf_newFile_maybe_unsafe(void *file, size_t size, bool check_pht, bool check_st, elf_t *res)
{
    elf_t new_file = {
        .elfFile = file,
        .elfSize = size
    };

    int status = elf_checkFile(&new_file);
    if (status < 0) {
        return status;
    }

    if (check_pht) {
        status = elf_checkProgramHeaderTable(&new_file);
        if (status < 0) {
            return status;
        }
    }

    if (check_st) {
        status = elf_checkSectionTable(&new_file);
        if (status < 0) {
            return status;
        }
    }

    if (res) {
        *res = new_file;
    }

    return status;
}

/*
 * Checks that elfFile points to a valid elf file. Returns 0 if the elf
 * file is valid, < 0 if invalid.
 */
int
elf_checkFile(elf_t *elfFile)
{
    int res = elf32_checkFile(elfFile);
    if (res == 0) {
        return 0;
    }

    res = elf64_checkFile(elfFile);
    if (res == 0) {
        return 0;
    }

    return -1;
}

int
elf_checkProgramHeaderTable(elf_t *elfFile)
{
    if (elf_isElf32(elfFile)) {
        return elf32_checkProgramHeaderTable(elfFile);
    } else {
        return elf64_checkProgramHeaderTable(elfFile);
    }
}

int
elf_checkSectionTable(elf_t *elfFile)
{
    if (elf_isElf32(elfFile)) {
        return elf32_checkSectionTable(elfFile);
    } else {
        return elf64_checkSectionTable(elfFile);
    }
}

uint64_t
elf_getEntryPoint(elf_t *elfFile)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getEntryPoint(elfFile);
    } else {
        return elf64_getEntryPoint(elfFile);
    }
}

uint16_t
elf_getNumProgramHeaders(elf_t *elfFile)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getNumProgramHeaders(elfFile);
    } else {
        return elf64_getNumProgramHeaders(elfFile);
    }
}

unsigned
elf_getNumSections(elf_t *elfFile)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getNumSections(elfFile);
    } else {
        return elf64_getNumSections(elfFile);
    }
}

char *
elf_getStringTable(elf_t *elfFile, int string_segment)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getStringTable(elfFile, string_segment);
    } else {
        return elf64_getStringTable(elfFile, string_segment);
    }
}

char *
elf_getSectionStringTable(elf_t *elfFile)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getSectionStringTable(elfFile);
    } else {
        return elf64_getSectionStringTable(elfFile);
    }
}


/* Section header functions */
void *
elf_getSection(elf_t *elfFile, int i)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getSection(elfFile, i);
    } else {
        return elf64_getSection(elfFile, i);
    }
}

void *
elf_getSectionNamed(elf_t *elfFile, const char *_str, int *i)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getSectionNamed(elfFile, _str, i);
    } else {
        return elf64_getSectionNamed(elfFile, _str, i);
    }
}

char *
elf_getSectionName(elf_t *elfFile, int i)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getSectionName(elfFile, i);
    } else {
        return elf64_getSectionName(elfFile, i);
    }
}

uint32_t
elf_getSectionType(elf_t *elfFile, int i)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getSectionType(elfFile, i);
    } else {
        return elf64_getSectionType(elfFile, i);
    }
}

uint32_t
elf_getSectionFlags(elf_t *elfFile, int i)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getSectionFlags(elfFile, i);
    } else {
        return elf64_getSectionFlags(elfFile, i);
    }
}

uint64_t
elf_getSectionAddr(elf_t *elfFile, int i)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getSectionAddr(elfFile, i);
    } else {
        return elf64_getSectionAddr(elfFile, i);
    }
}

uint64_t
elf_getSectionSize(elf_t *elfFile, int i)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getSectionSize(elfFile, i);
    } else {
        return elf64_getSectionSize(elfFile, i);
    }
}


/* Program headers function */
void *
elf_getProgramSegment(elf_t *elf, uint16_t ph)
{
    if (elf_isElf32(elf)) {
        return elf32_getProgramSegment(elf, ph);
    } else {
        return elf64_getProgramSegment(elf, ph);
    }
}

uint32_t
elf_getProgramHeaderType(elf_t *elfFile, uint16_t ph)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getProgramHeaderType(elfFile, ph);
    } else {
        return elf64_getProgramHeaderType(elfFile, ph);
    }
}

uint64_t
elf_getProgramHeaderOffset(elf_t *elfFile, uint16_t ph)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getProgramHeaderOffset(elfFile, ph);
    } else {
        return elf64_getProgramHeaderOffset(elfFile, ph);
    }
}

uint64_t
elf_getProgramHeaderVaddr(elf_t *elfFile, uint16_t ph)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getProgramHeaderVaddr(elfFile, ph);
    } else {
        return elf64_getProgramHeaderVaddr(elfFile, ph);
    }
}

uint64_t
elf_getProgramHeaderPaddr(elf_t *elfFile, uint16_t ph)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getProgramHeaderPaddr(elfFile, ph);
    } else {
        return elf64_getProgramHeaderPaddr(elfFile, ph);
    }
}

uint64_t
elf_getProgramHeaderFileSize(elf_t *elfFile, uint16_t ph)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getProgramHeaderFileSize(elfFile, ph);
    } else {
        return elf64_getProgramHeaderFileSize(elfFile, ph);
    }
}

uint64_t
elf_getProgramHeaderMemorySize(elf_t *elfFile, uint16_t ph)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getProgramHeaderMemorySize(elfFile, ph);
    } else {
        return elf64_getProgramHeaderMemorySize(elfFile, ph);
    }
}

uint32_t
elf_getProgramHeaderFlags(elf_t *elfFile, uint16_t ph)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getProgramHeaderFlags(elfFile, ph);
    } else {
        return elf64_getProgramHeaderFlags(elfFile, ph);
    }
}

uint32_t
elf_getProgramHeaderAlign(elf_t *elfFile, uint16_t ph)
{
    if (elf_isElf32(elfFile)) {
        return elf32_getProgramHeaderAlign(elfFile, ph);
    } else {
        return elf64_getProgramHeaderAlign(elfFile, ph);
    }
}


/* Utility functions */
int
elf_getMemoryBounds(elf_t *elfFile, int phys, uint64_t *min, uint64_t *max)
{
    uint64_t mem_min = UINT64_MAX;
    uint64_t mem_max = 0;
    int i;

    for(i = 0; i < elf_getNumProgramHeaders(elfFile); i++) {
        uint64_t sect_min, sect_max;

        if (elf_getProgramHeaderMemorySize(elfFile, i) == 0) {
            continue;
        }

        if (phys) {
            sect_min = elf_getProgramHeaderPaddr(elfFile, i);
        } else {
            sect_min = elf_getProgramHeaderVaddr(elfFile, i);
        }

        sect_max = sect_min + elf_getProgramHeaderMemorySize(elfFile, i);

        if (sect_max > mem_max) {
            mem_max = sect_max;
        }
        if (sect_min < mem_min) {
            mem_min = sect_min;
        }
    }
    *min = mem_min;
    *max = mem_max;

    return 1;
}

int
elf_vaddrInProgramHeader(elf_t *elfFile, uint16_t ph, uint64_t vaddr)
{
    uint64_t min = elf_getProgramHeaderVaddr(elfFile, ph);
    uint64_t max = min + elf_getProgramHeaderMemorySize(elfFile, ph);
    if (vaddr >= min && vaddr < max) {
        return 1;
    } else {
        return 0;
    }
}

uint64_t
elf_vtopProgramHeader(elf_t *elfFile, uint16_t ph, uint64_t vaddr)
{
    uint64_t ph_phys = elf_getProgramHeaderPaddr(elfFile, ph);
    uint64_t ph_virt = elf_getProgramHeaderVaddr(elfFile, ph);
    uint64_t paddr;

    paddr = vaddr - ph_virt + ph_phys;

    return paddr;
}

int
elf_loadFile(elf_t *elf, int phys)
{
    int i;

    for(i = 0; i < elf_getNumProgramHeaders(elf); i++) {
        /* Load that section */
        uint64_t dest, src;
        size_t len;
        if (phys) {
            dest = elf_getProgramHeaderPaddr(elf, i);
        } else {
            dest = elf_getProgramHeaderVaddr(elf, i);
        }
        len = elf_getProgramHeaderFileSize(elf, i);
        src = (uint64_t) (uintptr_t) elf->elfFile + elf_getProgramHeaderOffset(elf, i);
        memcpy((void *) (uintptr_t) dest, (void *) (uintptr_t) src, len);
        dest += len;
        memset((void *) (uintptr_t) dest, 0, elf_getProgramHeaderMemorySize(elf, i) - len);
    }

    return 1;
}
