/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "../../chardev.h"

#define KEYBOARD_PS2_IRQ 1
#define PS2_IOPORT_CONTROL 0x64
#define PS2_IOPORT_DATA 0x60

/* ref: http://www.computer-engineering.org/ps2keyboard/ */
#define PS2_CMD_DISABLE_MOUSE_INTERFACE         0xA7
#define PS2_CMD_ENABLE_MOUSE_INTERFACE          0xA8
#define PS2_CMD_CONTROLLER_SELF_TEST            0xAA
#define PS2_CMD_KEYBOARD_INTERFACE_TEST         0xAB
#define PS2_CMD_DISABLE_KEYBOARD_INTERFACE      0xAD
#define PS2_CMD_ENABLE_KEYBOARD_INTERFACE       0xAE
#define PS2_READ_CMD_BYTE                       0x20
#define PS2_WRITE_CMD_BYTE                      0x60
#define PS2_CONTROLLER_SELF_TEST_OK             0x55

#define KEYBOARD_RESET                  0xFF
#define KEYBOARD_RESEND                 0xFE
#define KEYBOARD_ACK                    0xFA
#define KEYBOARD_ERROR                  0xFC
#define KEYBOARD_DISABLE_SCAN           0xF5
#define KEYBOARD_ENABLE_SCAN            0xF4
#define KEYBOARD_SET_SCANCODE_MODE      0xF0
#define KEYBOARD_ECHO                   0xEE
#define KEYBOARD_SET_LEDS               0xED
#define KEYBOARD_BAT_SUCCESSFUL         0xAA

#define KEYBOARD_PS2_STATE_NORMAL 0x1
#define KEYBOARD_PS2_STATE_IGNORE 0x2
#define KEYBOARD_PS2_STATE_EXTENDED_MODE 0x4
#define KEYBOARD_PS2_STATE_RELEASE_KEY 0x8
#define KEYBOARD_PS2_EVENTCODE_RELEASE 0xF0
#define KEYBOARD_PS2_EVENTCODE_EXTENDED 0xE0
#define KEYBOARD_PS2_EVENTCODE_EXTENDED_PAUSE 0xE1

typedef struct keyboard_key_event {
    int16_t vkey;
    bool pressed;
} keyboard_key_event_t;

/* Internal state structure which stores the book-keeping to convert from PS2 keycodes to
   vkey press / release events.

   This only generated press / release events given in virtual key codes; it does NOT keep the
   key state machine needed to generate a stream of typed bytes from these events. This is
   done by keyboard_vkey.c/h.
*/
struct keyboard_state {
    ps_io_ops_t ops;
    int state;
    int num_ignore;

    /* Callback function which gets called when there is a keyboard key event. */
    void (*handle_event_callback)(keyboard_key_event_t ev, void *cookie);
};

/* ---------------------------------------------------------------------------------------------- */

/* Initialise keyboard driver state.
   The handle_event_callback parameter is optional, and may be set to NULL. Events are be
   returned by keyboard_poll_ps2_keyevents().
*/
int keyboard_init(struct keyboard_state *state, const ps_io_ops_t* ops,
                  void (*handle_event_callback)(keyboard_key_event_t ev, void *cookie));

void keyboard_set_scanmode(struct keyboard_state *state, uint8_t mode);

void keyboard_set_led(struct keyboard_state *state, char scroll_lock, char num_lock, char caps_lock);

int keyboard_reset(struct keyboard_state *state);

/* This may be called in a loop on every IRQ, until no more character events reported.
   Note that this polling will NOT call the handle_event_callback on key events. */
keyboard_key_event_t keyboard_poll_ps2_keyevent(struct keyboard_state *state);

/* This may be called on every IRQ, for driver to read any key events. This function continuously
   calls keyboard_poll_ps2_keyevent in a loop and invokes handle_event_callback for every key
   event detected. */
void keyboard_poll_ps2_keyevents(struct keyboard_state *state, void *cookie);

