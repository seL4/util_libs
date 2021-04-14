/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
/*
 * Implementation of an 80x25 EGA text mode. All the functions are named with 'serial' since
 * that is what platsupport expects. This was a quick hack for a project and includes lots of
 * logic that is rather inspecific to the underlying frame buffer. If someone adds another text
 * mode frambuffer (or a linear frame buffer with a font renderer) then this should be abstracted.
 * ideally some kind of virtual terminal code could be ported over to properly deal with escape characters
 * and command codes for moving cursors etc around. But this is a basic hack for 'The machine I
 * want to test on has a screen and no serial port'
 */

#include <autoconf.h>
#include <platsupport/gen_config.h>
#include <assert.h>
#include <string.h>
#include <platsupport/plat/serial.h>
#include "../../chardev.h"
#include <string.h>

/* Assumptions on the graphics mode and frame buffer location */
#define EGA_TEXT_FB_BASE 0xB8000
#define MODE_WIDTH 80
#define MODE_HEIGHT 25

/* How many lines to scroll by */
#define SCROLL_LINES 1

/* Hacky global state */
static volatile short *base_ptr = NULL;
static int cursor_x = 0;
static int cursor_y = 0;

static void scroll(void)
{
    /* number of chars we are dropping when we do the scroll */
    int clear_chars = SCROLL_LINES * MODE_WIDTH;
    /* number of chars we need to move to perform the scroll. This all the lines
     * minus however many we drop */
    int scroll_chars = MODE_WIDTH * MODE_HEIGHT - clear_chars;
    /* copy the lines up. we skip the same number of characters that we will clear, and move the
     * rest to the top. cannot use memcpy as the regions almost certainly overlap */
    memmove((void *)base_ptr, (void *)&base_ptr[clear_chars], scroll_chars * sizeof(*base_ptr));
    /* now zero out the bottom lines that we got rid of */
    memset((void *)&base_ptr[scroll_chars], 0, clear_chars * sizeof(*base_ptr));
    /* move the virtual cursor up */
    cursor_y -= SCROLL_LINES;
}

static int text_ega_getchar(ps_chardevice_t *d UNUSED)
{
    assert(!"EGA framebuffer does not implement getchar");
    return 0;
}

static int text_ega_putchar(ps_chardevice_t *d, int c)
{
    /* emulate various control characters */
    if (c == '\t') {
        text_ega_putchar(d, ' ');
        while (cursor_x % 4 != 0) {
            text_ega_putchar(d, ' ');
        }
    } else if (c == '\n') {
        cursor_y ++;
        /* assume a \r with a \n */
        cursor_x = 0;
    } else if (c == '\r') {
        cursor_x = 0;
    } else {
        /* 7<<8 constructs a nice neutral grey color. */
        base_ptr[cursor_y * MODE_WIDTH + cursor_x] = ((char)c) | (7 << 8);
        cursor_x++;
    }
    if (cursor_x >= MODE_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    while (cursor_y >= MODE_HEIGHT) {
        scroll();
    }
    return 0;
}

static ssize_t text_ega_write(ps_chardevice_t *d, const void *vdata, size_t count, chardev_callback_t rcb UNUSED,
                              void *token UNUSED)
{
    const char *data = (const char *)vdata;
    int i;
    for (i = 0; i < count; i++) {
        if (text_ega_putchar(d, *data++) < 0) {
            return i;
        }
    }
    return count;
}

static ssize_t text_ega_read(ps_chardevice_t *d, void *vdata, size_t count, chardev_callback_t rcb UNUSED,
                             void *token UNUSED)
{
    char *data;
    int ret;
    int i;
    data = (char *)vdata;
    for (i = 0; i < count; i++) {
        ret = text_ega_getchar(d);
        if (ret != EOF) {
            *data++ = ret;
        } else {
            return i;
        }
    }
    return count;
}

static void text_ega_handle_irq(ps_chardevice_t *d)
{
    /* TODO */
}

int text_ega_init(const struct dev_defn *defn, const ps_io_ops_t *ops, ps_chardevice_t *dev)
{
    /* handle an insane case where the serial might get repeatedly initialized. and
     * avoid clearing the entire screen if this is the case. This comes about due
     * to implementing device 'sharing' by just giving every process the device */
    int clear = !base_ptr;
    memset(dev, 0, sizeof(*dev));
    base_ptr = chardev_map(defn, ops);
    assert(base_ptr);
    /* clear the screen */
    if (clear) {
        memset((void *)base_ptr, 0, MODE_WIDTH * MODE_HEIGHT * sizeof(*base_ptr));
        cursor_x = 0;
        cursor_y = 0;
    }

    dev->id         = defn->id;
    dev->vaddr      = (void *)base_ptr;
    dev->read       = &text_ega_read;
    dev->write      = &text_ega_write;
    dev->handle_irq = &text_ega_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;

    return 0;
}
