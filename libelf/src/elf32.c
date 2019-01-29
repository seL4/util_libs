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
#include <string.h>
#include <inttypes.h>

/* ELF header functions */
int
elf32_checkFile(struct Elf32_Header *file)
{
    if (file->e_ident[EI_MAG0] != ELFMAG0
        || file->e_ident[EI_MAG1] != ELFMAG1
        || file->e_ident[EI_MAG2] != ELFMAG2
        || file->e_ident[EI_MAG3] != ELFMAG3)
        return -1;  /* not an elf file */
    if (file->e_ident[EI_CLASS] != ELFCLASS32)
        return -2;  /* not 32-bit file */
    return 0;       /* elf file looks OK */
}

uint32_t
elf32_getEntryPoint (struct Elf32_Header *elfFile)
{
    return elfFile->e_entry;
}

/* Returns the number of program segments in this elf file. */
uint16_t
elf32_getNumProgramHeaders(struct Elf32_Header *elfFile)
{
    struct Elf32_Header *fileHdr = elfFile;
    return fileHdr->e_phnum;
}

/*
 * Returns the number of program segments in this elf file.
 */
unsigned
elf32_getNumSections(struct Elf32_Header *elfFile)
{
    return elfFile->e_shnum;
}

char *
elf32_getStringTable(struct Elf32_Header *elfFile)
{
    struct Elf32_Shdr *sections = elf32_getSectionTable(elfFile);
    return (char *)elfFile + sections[elfFile->e_shstrndx].sh_offset;
}

char *
elf32_getSegmentStringTable(struct Elf32_Header *elfFile)
{
    struct Elf32_Header *fileHdr = (struct Elf32_Header *) elfFile;
    if (fileHdr->e_shstrndx == 0) {
        return NULL;
    } else {
        return elf32_getStringTable(elfFile);
    }
}


/* Section header functions */
void *
elf32_getSection(struct Elf32_Header *elfFile, int i)
{
    struct Elf32_Shdr *sections = elf32_getSectionTable(elfFile);
    return (char *)elfFile + sections[i].sh_offset;
}

void *
elf32_getSectionNamed(struct Elf32_Header *elfFile, const char *str, int *id)
{
    int numSections = elf32_getNumSections(elfFile);
    int i;
    for (i = 0; i < numSections; i++) {
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
elf32_getSectionName(struct Elf32_Header *elfFile, int i)
{
    struct Elf32_Shdr *sections = elf32_getSectionTable(elfFile);
    char *str_table = elf32_getSegmentStringTable(elfFile);
    if (str_table == NULL) {
        return "<corrupted>";
    } else {
        return str_table + sections[i].sh_name;
    }
}

uint32_t
elf32_getSectionAddr(struct Elf32_Header *elfFile, int i)
{
    struct Elf32_Shdr *sections = elf32_getSectionTable(elfFile);
    return sections[i].sh_addr;
}

uint32_t
elf32_getSectionSize(struct Elf32_Header *elfFile, int i)
{
    struct Elf32_Shdr *sections = elf32_getSectionTable(elfFile);
    return sections[i].sh_size;
}
