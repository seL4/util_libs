/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* Tool for removing metadata from a CPIO archive.
 *
 * The motivation behind this is to work towards idempotent builds. Part of the
 * seL4 build system forms a CPIO archive of ELF files from the host file
 * system. This archive inadvertently includes information like the i-node
 * numbers and modified times of these files. This information is irrelevant at
 * runtime, but causes the resulting image to not be binary identical between
 * otherwise identical builds.
 *
 * The code that follows strips or replaces the following fields from CPIO file
 * entries:
 *  - i-node number
 *  - UID
 *  - GID
 *  - modified time
 */

/* We deliberately use seL4's CPIO library rather than libarchive or similar so
 * we have the same interpretation of CPIO files as seL4. This isn't strictly
 * essential, but it's nice for testing the robustness of this library.
 */
#include <cpio/cpio.h>

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* Find the pointer to a CPIO entry header from a pointer to the entry's data.
 * This essentially reverses the transformation in cpio_get_entry.
 */
static void *get_header(void *data, const char *filename) {
    assert((uintptr_t)data % CPIO_ALIGNMENT == 0);
    uintptr_t p = (uintptr_t)data - strlen(filename) - 1
        - sizeof(struct cpio_header);
    return (void*)(p - (p % CPIO_ALIGNMENT));
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n"
                        " Strip meta data from a CPIO file\n", argv[0]);
        return -1;
    }

    FILE *archive = NULL;
    void *p = NULL;
    long len = 0;
    
    archive = fopen(argv[1], "r+");
    if (archive == NULL) {
        perror("failed to open archive");
        goto fail;
    }

    /* Determine the size of the archive, as we'll need to mmap the whole
     * thing.
     */
    if (fseek(archive, 0, SEEK_END) != 0) {
        perror("failed to seek archive");
        goto fail;
    }
    len = ftell(archive);
    if (len == -1) {
        perror("failed to read size of archive");
        goto fail;
    }
    if (fseek(archive, 0, SEEK_SET) != 0) {
        perror("failed to return to beginning of archive");
        goto fail;
    }

    /* Mmap the file so we can operate on it with libcpio. */
    p = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fileno(archive), 0);
    if (p == MAP_FAILED) {
        perror("failed to mmap archive");
        p = NULL;
        goto fail;
    }

    struct cpio_info info = { .file_count = 0 };
    int err = cpio_info(p, &info);
    if (err != 0) {
        fprintf(stderr, "failed to read CPIO info\n");
        goto fail;
    }

    for (unsigned int i = 0; i < info.file_count; i++) {

        /* Use libcpio to look up the entry. */
        unsigned long size;
        const char *filename;
        void *data = cpio_get_entry(p, i, &filename, &size);
        if (data == NULL) {
            fprintf(stderr, "failed to locate entry %u\n", i);
            goto fail;
        }

        /* Reverse the data pointer to a header pointer. */
        struct cpio_header *header = get_header(data, filename);
        assert((uintptr_t)header % CPIO_ALIGNMENT == 0);

        /* Synthesise an i-node number. This just needs to be distinct within
         * the archive. I-node numbers <=10 are reserved on certain file
         * systems.
        */
        unsigned int inode = 11 + i;
        snprintf(header->c_ino, sizeof(header->c_ino), "%08x", inode);

        /* Set the file owned by 'root'. */
        memset(header->c_uid, 0, sizeof(header->c_uid));
        memset(header->c_gid, 0, sizeof(header->c_gid));

        /* Blank the modified time. */
        memset(header->c_mtime, 0, sizeof(header->c_mtime));
    }

    munmap(p, len);
    fclose(archive);

    return 0;

fail:
    if (p != NULL)        munmap(p, len);
    if (archive != NULL)  fclose(archive);
    return -1;
}

