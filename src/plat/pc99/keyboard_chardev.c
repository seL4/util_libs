/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "keyboard_chardev.h"
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include <assert.h>

static struct keyboard_state kb_state;
static keycode_state_t kc_state;

void
keyboard_cdev_handle_led_changed(void *cookie)
{
    /* Update LED states. */
    keyboard_set_led(&kb_state, kc_state.scroll_lock, kc_state.num_lock, kc_state.caps_lock);
    kc_state.led_state_changed = false;
}

static int
keyboard_getchar(struct ps_chardevice *device)
{
    keyboard_key_event_t ev = keyboard_poll_ps2_keyevent(&kb_state);
    return keycode_process_vkey_event_to_char(&kc_state, ev.vkey, ev.pressed, NULL);
}

static ssize_t
keyboard_write(ps_chardevice_t* d, const void* vdata, size_t count, chardev_callback_t rcb UNUSED,
               void* token UNUSED)
{
    /* Keyboard has no write support. */
    return 0;
}

static ssize_t
keyboard_read(ps_chardevice_t* d, void* vdata, size_t count, chardev_callback_t rcb UNUSED,
              void* token UNUSED)
{
    int ret;
    int i;
    char* data = (char*) vdata;
    for (i = 0; i < count; i++) {
        ret = keyboard_getchar(d);
        if (ret != EOF) {
            *data++ = ret;
        } else {
            return i;
        }
    }
    return count;
}

static void
keyboard_handle_irq(ps_chardevice_t* device UNUSED)
{
    /* No IRQ handling required here. */
}

int
keyboard_cdev_init(const struct dev_defn* defn, const ps_io_ops_t* ops, ps_chardevice_t* dev)
{
    memset(dev, 0, sizeof(*dev));

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void*) NULL; /* Save the IO port base number. */
    dev->read       = &keyboard_read;
    dev->write      = &keyboard_write;
    dev->handle_irq = &keyboard_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;

    /* Initialise keyboard drivers. */
    if (keyboard_init(&kb_state, ops, NULL)) {
        return -1;
    }

    /* Initialise keycode. */
    keycode_init(&kc_state, NULL, NULL,
                 keyboard_cdev_handle_led_changed);

    /* Set initial LED state. */
    keyboard_cdev_handle_led_changed(NULL);

    return 0;
}
