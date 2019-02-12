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
#include <inttypes.h>
#include <string.h>

/* ELF header functions */
int
elf32_checkFile(elf_t *elf)
{
    if (elf->elfSize < sizeof(Elf32_Ehdr)) {
        return -1; /* file smaller than ELF header */
    }

    Elf32_Ehdr *header = elf->elfFile;
    if (header->e_ident[EI_MAG0] != ELFMAG0 ||
            header->e_ident[EI_MAG1] != ELFMAG1 ||
            header->e_ident[EI_MAG2] != ELFMAG2 ||
            header->e_ident[EI_MAG3] != ELFMAG3) {
        return -1; /* not an ELF file */
    }

    if (header->e_ident[EI_CLASS] != ELFCLASS32) {
        return -1; /* not a 32-bit ELF */
    }

    if (header->e_phentsize != sizeof(Elf32_Phdr)) {
        return -1; /* unexpected program header size */
    }

    if (header->e_shentsize != sizeof(Elf32_Shdr)) {
        return -1; /* unexpected section header size */
    }

    if (header->e_shstrndx >= header->e_shnum) {
        return -1; /* invalid section header string table section */
    }

    elf->elfClass = header->e_ident[EI_CLASS];
    return 0; /* elf header looks OK */
}

int
elf32_checkProgramHeaderTable(elf_t *elf)
{
    Elf32_Ehdr *header = elf->elfFile;
    size_t ph_end = header->e_phoff + header->e_phentsize * header->e_phnum;
    if (elf->elfSize < ph_end || ph_end < header->e_phoff) {
        return -1; /* invalid program header table */
    }

    return 0;
}

int
elf32_checkSectionTable(elf_t *elf)
{
    Elf32_Ehdr *header = elf->elfFile;
    size_t sh_end = header->e_shoff + header->e_shentsize * header->e_shnum;
    if (elf->elfSize < sh_end || sh_end < header->e_shoff) {
        return -1; /* invalid section header table */
    }

    return 0;
}

char *
elf32_getStringTable(elf_t *elf, int string_segment)
{
    char *string_table = elf32_getSection(elf, string_segment);
    if (string_table == NULL) {
        return NULL; /* no such section */
    }

    if (elf32_getSectionType(elf, string_segment) != SHT_STRTAB) {
        return NULL; /* not a string table */
    }

    uint32_t size = elf32_getSectionSize(elf, string_segment);
    if (string_table[size - 1] != 0) {
        return NULL; /* string table is not null-terminated */
    }

    return string_table;
}

char *
elf32_getSectionStringTable(elf_t *elf)
{
    uint16_t index = elf32_getSectionStringTableIndex(elf);
    return elf32_getStringTable(elf, index);
}


/* Section header functions */
void *
elf32_getSection(elf_t *elf, int i)
{
    if (i == 0 || i >= elf32_getNumSections(elf)) {
        return NULL; /* no such section */
    }

    size_t section_offset = elf32_getSectionOffset(elf, i);
    size_t section_size = elf32_getSectionSize(elf, i);
    if (section_size == 0) {
        return NULL; /* section is empty */
    }

    size_t section_end = section_offset + section_size;
    /* possible wraparound - check that section end is not before section start */
    if (section_end > elf->elfSize || section_end < section_offset) {
        return NULL;
    }

    return elf->elfFile + section_offset;
}

void *
elf32_getSectionNamed(elf_t *elfFile, const char *str, int *id)
{
    int numSections = elf32_getNumSections(elfFile);
    for (int i = 0; i < numSections; i++) {
        if (strcmp(str, elf32_getSectionName(elfFile, i)) == 0) {
            if (id != NULL) {
                *id = i;
            }
            return elf32_getSection(elfFile, i);
        }
    }
    return NULL;
}

char *
elf32_getSectionName(elf_t *elf, int i)
{
    uint16_t str_table_idx = elf32_getSectionStringTableIndex(elf);
    char *str_table = elf32_getStringTable(elf, str_table_idx);
    uint32_t offset = elf32_getSectionNameOffset(elf, i);
    uint32_t size = elf32_getSectionSize(elf, str_table_idx);

    if (str_table == NULL || offset > size) {
        return "<corrupted>";
    }

    return str_table + offset;
}

void *
elf32_getProgramSegment(elf_t *elf, uint16_t ph)
{
    Elf32_Phdr p = elf32_getProgramHeaderTable(elf)[ph];
    size_t segment_end = p.p_offset + p.p_filesz;
    /* possible wraparound - check that segment end does is not before start */
    if (elf->elfSize < segment_end || segment_end < p.p_offset) {
        return NULL;
    }

    return elf->elfFile + p.p_offset;
}
