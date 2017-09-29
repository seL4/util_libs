/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define VK_MAX_ENUM 0xFF
#define PS2_MAX_KEYCODES_BASIC 0xFF

#define KEYBOARD_KEY_DEBUG false

typedef struct keycode_info {
    int16_t ch;
    int16_t uppercase;
    int16_t ctrlchar;
} keycode_info_t;

typedef struct keycode_state {
    bool keystate[VK_MAX_ENUM];

    bool scroll_lock;
    bool num_lock;
    bool caps_lock;
    bool led_state_changed;

    /* Optional callback, called when a vkey has been pressed or released. */
    void (*handle_keyevent_callback)(int16_t vkey, bool pressed, void *cookie);
    /* Optional callback, called when a character has been typed. */
    void (*handle_chartyped_callback)(int c, void *cookie);
    /* Optional callback, called when num/scroll/caps lock LED state has changed. */
    void (*handle_led_state_changed_callback)(void *cookie);

} keycode_state_t;

/* ref:
        http://nehe.gamedev.net/article/msdn_virtualkey_codes/15009/
        http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx
*/
enum virtual_keycode_enum {
    VK_LBUTTON = 0x1 ,  /* Left mouse button */
    VK_RBUTTON = 0x2 ,  /* Right mouse button */
    VK_CANCEL = 0x3 ,  /* Control-break processing */
    VK_MBUTTON = 0x4 ,  /* Middle mouse button (three-button mouse) */
    VK_XBUTTON1 = 0x5 ,  /*  X1 mouse button */
    VK_XBUTTON2 = 0x6 ,  /*  X2 mouse button */
    VK_BACK = 0x8 ,  /* BACKSPACE key */
    VK_TAB = 0x9 ,  /* TAB key */
    VK_CLEAR = 0x0C,  /* CLEAR key */
    VK_RETURN = 0x0D,  /* ENTER key */
    VK_CONTROL = 0x11,  /* CTRL key */
    VK_SHIFT = 0x10,  /* SHIFT key */
    VK_MENU = 0x12,  /* ALT key */
    VK_PAUSE = 0x13,  /* PAUSE key */
    VK_CAPITAL = 0x14,  /* CAPS LOCK key */
    VK_KANA = 0x15,  /* IME Kana mode */
    VK_HANGUEL = 0x15,  /* IME Hanguel mode (maintained for compatibility; use VK_HANGUL) */
    VK_HANGUL = 0x15,  /* IME Hangul mode */
    VK_JUNJA = 0x17,  /* IME Junja mode */
    VK_FINAL = 0x18,  /* IME final mode */
    VK_HANJA = 0x19,  /* IME Hanja mode */
    VK_KANJI = 0x19,  /* IME Kanji mode */
    VK_ESCAPE = 0x1B,  /* ESC key */
    VK_CONVERT = 0x1C,  /* IME convert */
    VK_NONCONVERT = 0x1D,  /* IME nonconvert */
    VK_ACCEPT = 0x1E,  /* IME accept */
    VK_MODECHANGE = 0x1F,  /* IME mode change request */
    VK_SPACE = 0x20,  /* SPACEBAR */
    VK_PRIOR = 0x21,  /* PAGE UP key */
    VK_NEXT = 0x22,  /* PAGE DOWN key */
    VK_END = 0x23,  /* END key */
    VK_HOME = 0x24,  /* HOME key */
    VK_LEFT = 0x25,  /* LEFT ARROW key */
    VK_UP = 0x26,  /* UP ARROW key */
    VK_RIGHT = 0x27,  /* RIGHT ARROW key */
    VK_DOWN = 0x28,  /* DOWN ARROW key */
    VK_SELECT = 0x29,  /* SELECT key */
    VK_PRINT = 0x2A,  /* PRINT key */
    VK_EXECUTE = 0x2B,  /* EXECUTE key */
    VK_SNAPSHOT = 0x2C,  /* PRINT SCREEN key */
    VK_INSERT = 0x2D,  /* INS key */
    VK_DELETE = 0x2E,  /* DEL key */
    VK_HELP = 0x2F,  /* HELP key */
    VK_0 = 0x30,  /* 0 key */
    VK_1 = 0x31,  /* 1 key */
    VK_2 = 0x32,  /* 2 key */
    VK_3 = 0x33,  /* 3 key */
    VK_4 = 0x34,  /* 4 key */
    VK_5 = 0x35,  /* 5 key */
    VK_6 = 0x36,  /* 6 key */
    VK_7 = 0x37,  /* 7 key */
    VK_8 = 0x38,  /* 8 key */
    VK_9 = 0x39,  /* 9 key */
    VK_A = 0x41,  /* A key */
    VK_B = 0x42,  /* B key */
    VK_C = 0x43,  /* C key */
    VK_D = 0x44,  /* D key */
    VK_E = 0x45,  /* E key */
    VK_F = 0x46,  /* F key */
    VK_G = 0x47,  /* G key */
    VK_H = 0x48,  /* H key */
    VK_I = 0x49,  /* I key */
    VK_J = 0x4A,  /* J key */
    VK_K = 0x4B,  /* K key */
    VK_L = 0x4C,  /* L key */
    VK_M = 0x4D,  /* M key */
    VK_N = 0x4E,  /* N key */
    VK_O = 0x4F,  /* O key */
    VK_P = 0x50,  /* P key */
    VK_Q = 0x51,  /* Q key */
    VK_R = 0x52,  /* R key */
    VK_S = 0x53,  /* S key */
    VK_T = 0x54,  /* T key */
    VK_U = 0x55,  /* U key */
    VK_V = 0x56,  /* V key */
    VK_W = 0x57,  /* W key */
    VK_X = 0x58,  /* X key */
    VK_Y = 0x59,  /* Y key */
    VK_Z = 0x5A,  /* Z key */
    VK_LWIN = 0x5B,  /* Left Windows key (Microsoft Natural Keyboard) */
    VK_RWIN = 0x5C,  /* Right Windows key (Microsoft Natural Keyboard) */
    VK_APPS = 0x5D,  /* Applications key (Microsoft Natural Keyboard) */
    VK_SLEEP = 0x5F,  /* Computer Sleep key */
    VK_NUMPAD0 = 0x60,  /* Numeric keypad 0 key */
    VK_NUMPAD1 = 0x61,  /* Numeric keypad 1 key */
    VK_NUMPAD2 = 0x62,  /* Numeric keypad 2 key */
    VK_NUMPAD3 = 0x63,  /* Numeric keypad 3 key */
    VK_NUMPAD4 = 0x64,  /* Numeric keypad 4 key */
    VK_NUMPAD5 = 0x65,  /* Numeric keypad 5 key */
    VK_NUMPAD6 = 0x66,  /* Numeric keypad 6 key */
    VK_NUMPAD7 = 0x67,  /* Numeric keypad 7 key */
    VK_NUMPAD8 = 0x68,  /* Numeric keypad 8 key */
    VK_NUMPAD9 = 0x69,  /* Numeric keypad 9 key */
    VK_MULTIPLY = 0x6A,  /* Multiply key */
    VK_ADD = 0x6B,  /* Add key */
    VK_SEPARATOR = 0x6C,  /* Separator key */
    VK_SUBTRACT = 0x6D,  /* Subtract key */
    VK_DECIMAL = 0x6E,  /* Decimal key */
    VK_DIVIDE = 0x6F,  /* Divide key */
    VK_F1 = 0x70,  /* F1 key */
    VK_F2 = 0x71,  /* F2 key */
    VK_F3 = 0x72,  /* F3 key */
    VK_F4 = 0x73,  /* F4 key */
    VK_F5 = 0x74,  /* F5 key */
    VK_F6 = 0x75,  /* F6 key */
    VK_F7 = 0x76,  /* F7 key */
    VK_F8 = 0x77,  /* F8 key */
    VK_F9 = 0x78,  /* F9 key */
    VK_F10 = 0x79,  /* F10 key */
    VK_F11 = 0x7A,  /* F11 key */
    VK_F12 = 0x7B,  /* F12 key */
    VK_F13 = 0x7C,  /* F13 key */
    VK_F14 = 0x7D,  /* F14 key */
    VK_F15 = 0x7E,  /* F15 key */
    VK_F16 = 0x7F,  /* F16 key */
    VK_F17 = 0x80,  /* F17 key */
    VK_F18 = 0x81,  /* F18 key */
    VK_F19 = 0x82,  /* F19 key */
    VK_F20 = 0x83,  /* F20 key */
    VK_F21 = 0x84,  /* F21 key */
    VK_F22 = 0x85,  /* F22 key */
    VK_F23 = 0x86,  /* F23 key */
    VK_F24 = 0x87,  /* F24 key */
    VK_NUMLOCK = 0x90,  /* NUM LOCK key */
    VK_SCROLL = 0x91,  /* SCROLL LOCK key */
    VK_LSHIFT = 0xA0,  /* Left SHIFT key */
    VK_RSHIFT = 0xA1,  /* Right SHIFT key */
    VK_LCONTROL = 0xA2,  /* Left CONTROL key */
    VK_RCONTROL = 0xA3,  /* Right CONTROL key */
    VK_LMENU = 0xA4,  /* Left MENU key */
    VK_RMENU = 0xA5,  /* Right MENU key */
    VK_BROWSER_BACK = 0xA6,  /*  Browser Back key */
    VK_BROWSER_FORWARD = 0xA7,  /*  Browser Forward key */
    VK_BROWSER_REFRESH = 0xA8,  /*  Browser Refresh key */
    VK_BROWSER_STOP = 0xA9,  /*  Browser Stop key */
    VK_BROWSER_SEARCH = 0xAA,  /*  Browser Search key */
    VK_BROWSER_FAVORITES = 0xAB,  /*  Browser Favorites key */
    VK_BROWSER_HOME = 0xAC,  /*  Browser Launch and Home key */
    VK_VOLUME_MUTE = 0xAD,  /*  Volume Mute key */
    VK_VOLUME_DOWN = 0xAE,  /*  Volume Down key */
    VK_VOLUME_UP = 0xAF,  /*  Volume Up key */
    VK_MEDIA_NEXT_TRACK = 0xB0,  /*  Next Track key */
    VK_MEDIA_PREV_TRACK = 0xB1,  /*  Previous Track key */
    VK_MEDIA_STOP = 0xB2,  /*  Stop Media key */
    VK_MEDIA_PLAY_PAUSE = 0xB3,  /*  Play/Pause Media key */
    VK_LAUNCH_MAIL = 0xB4,  /*  Launch Mail key */
    VK_LAUNCH_MEDIA_SELECT = 0xB5,  /*  Select Media key */
    VK_LAUNCH_APP1 = 0xB6,  /*  Launch Application 1 key */
    VK_LAUNCH_APP2 = 0xB7,  /*  Launch Application 2 key */
    VK_OEM_1 = 0xBA,  /*  For the US standard keyboard, the ';:' key */
    VK_OEM_PLUS = 0xBB,  /*  For any country/region, the '+' key */
    VK_OEM_COMMA = 0xBC,  /*  For any country/region, the ',' key */
    VK_OEM_MINUS = 0xBD,  /*  For any country/region, the '-' key */
    VK_OEM_PERIOD = 0xBE,  /*  For any country/region, the '.' key */
    VK_OEM_2 = 0xBF,  /*  For the US standard keyboard, the '/?' key */
    VK_OEM_3 = 0xC0,  /*  For the US standard keyboard, the '`~' key */
    VK_OEM_4 = 0xDB,  /*  For the US standard keyboard, the '[{' key */
    VK_OEM_5 = 0xDC,  /*  For the US standard keyboard, the '\|' key */
    VK_OEM_6 = 0xDD,  /*  For the US standard keyboard, the ']}' key */
    VK_OEM_7 = 0xDE,  /*  For the US standard keyboard, the 'single-quote/double-quote' key */
    VK_OEM_8 = 0xDF,  /*   */
    VK_OEM_102 = 0xE2,  /*  either the '<>' key or the '\|' key on the RT 102-key keyboard */
    VK_PROCESSKEY = 0xE5,  /* Windows 95, Windows NT 4.0, and  IME PROCESS key */
    VK_PACKET = 0xE7,  /*  Used to pass Unicode characters as if they were keystrokes. */
    VK_ATTN = 0xF6,  /* Attn key */
    VK_CRSEL = 0xF7,  /* CrSel key */
    VK_EXSEL = 0xF8,  /* ExSel key */
    VK_EREOF = 0xF9,  /* Erase EOF key */
    VK_PLAY = 0xFA,  /* Play key */
    VK_ZOOM = 0xFB,  /* Zoom key */
    VK_NONAME = 0xFC,  /* Reserved for future use */
    VK_PA1 = 0xFD,  /* PA1 key */
    VK_OEM_CLEAR = 0xFE  /* Clear key */
};

/* ref: http://techdocs.altium.com/display/FPGA/PS2+Keyboard+Scan+Codes */
enum ps2_keycode_scanmode2_enum {
    PS2_KEY_ESC = 0x76,
    PS2_KEY_F1 = 0x05,
    PS2_KEY_F2 = 0x06,
    PS2_KEY_F3 = 0x04,
    PS2_KEY_F4 = 0x0C,
    PS2_KEY_F5 = 0x03,
    PS2_KEY_F6 = 0x0B,
    PS2_KEY_F7 = 0x83,
    PS2_KEY_F8 = 0x0A,
    PS2_KEY_F9 = 0x01,
    PS2_KEY_F10 = 0x09,
    PS2_KEY_F11 = 0x78,
    PS2_KEY_F12 = 0x07,
    PS2_KEY_SCROLL_LOCK = 0x7E,
    PS2_KEY_TILDE = 0x0E,
    PS2_KEY_1 = 0x16,
    PS2_KEY_2 = 0x1E,
    PS2_KEY_3 = 0x26,
    PS2_KEY_4 = 0x25,
    PS2_KEY_5 = 0x2E,
    PS2_KEY_6 = 0x36,
    PS2_KEY_7 = 0x3D,
    PS2_KEY_8 = 0x3E,
    PS2_KEY_9 = 0x46,
    PS2_KEY_0 = 0x45,
    PS2_KEY_SUBTRACT = 0x4E,
    PS2_KEY_EQUALS = 0x55,
    PS2_KEY_BACKSPACE = 0x66,
    PS2_KEY_TAB = 0x0D,
    PS2_KEY_Q = 0x15,
    PS2_KEY_W = 0x1D,
    PS2_KEY_E = 0x24,
    PS2_KEY_R = 0x2D,
    PS2_KEY_T = 0x2C,
    PS2_KEY_Y = 0x35,
    PS2_KEY_U = 0x3C,
    PS2_KEY_I = 0x43,
    PS2_KEY_O = 0x44,
    PS2_KEY_P = 0x4D,
    PS2_KEY_LBRACKET = 0x54,
    PS2_KEY_RBRACKET = 0x5B,
    PS2_KEY_BACKSLASH = 0x5D,
    PS2_KEY_CAPS_LOCK = 0x58,
    PS2_KEY_A = 0x1C,
    PS2_KEY_S = 0x1B,
    PS2_KEY_D = 0x23,
    PS2_KEY_F = 0x2B,
    PS2_KEY_G = 0x34,
    PS2_KEY_H = 0x33,
    PS2_KEY_J = 0x3B,
    PS2_KEY_K = 0x42,
    PS2_KEY_L = 0x4B,
    PS2_KEY_COLON = 0x4C,
    PS2_KEY_TICK = 0x52,
    PS2_KEY_ENTER = 0x5A,
    PS2_KEY_SHIFT_LEFT = 0x12,
    PS2_KEY_Z = 0x1A,
    PS2_KEY_X = 0x22,
    PS2_KEY_C = 0x21,
    PS2_KEY_V = 0x2A,
    PS2_KEY_B = 0x32,
    PS2_KEY_N = 0x31,
    PS2_KEY_M = 0x3A,
    PS2_KEY_COMMA = 0x41,
    PS2_KEY_DOT = 0x49,
    PS2_KEY_SLASH = 0x4A,
    PS2_KEY_SHIFT_RIGHT = 0x59,
    PS2_KEY_CTRL_LEFT = 0x14,
    PS2_KEY_ALT_LEFT = 0x11,
    PS2_KEY_SPACEBAR = 0x29,
    PS2_KEY_NUM_LOCK = 0x77,
    PS2_KEY_NUM_MULTIPLY = 0x7C,
    PS2_KEY_NUM_MINUS = 0x7B,
    PS2_KEY_NUM_7 = 0x6C,
    PS2_KEY_NUM_8 = 0x75,
    PS2_KEY_NUM_9 = 0x7D,
    PS2_KEY_NUM_PLUS = 0x79,
    PS2_KEY_NUM_4 = 0x6B,
    PS2_KEY_NUM_5 = 0x73,
    PS2_KEY_NUM_6 = 0x74,
    PS2_KEY_NUM_1 = 0x69,
    PS2_KEY_NUM_2 = 0x72,
    PS2_KEY_NUM_3 = 0x7A,
    PS2_KEY_NUM_0 = 0x70,
    PS2_KEY_NUM_DOT = 0x71,

    PS2_KEY_PRTSCR = 0xE012, /* 0xE012E07C */
    PS2_KEY_PAUSE = 0xE114, /* 0xE11477E1F014E077 */
    PS2_KEY_WINDOWS_LEFT = 0xE01F,
    PS2_KEY_ALT_RIGHT = 0xE011,
    PS2_KEY_WINDOWS_RIGHT = 0xE027,
    PS2_KEY_MENUS = 0xE02F,
    PS2_KEY_CTRL_RIGHT = 0xE014,
    PS2_KEY_INSERT = 0xE070,
    PS2_KEY_HOME = 0xE06C,
    PS2_KEY_PAGE_UP = 0xE07D,
    PS2_KEY_DELETE = 0xE071,
    PS2_KEY_END = 0xE069,
    PS2_KEY_PAGE_DOWN = 0xE07A,
    PS2_KEY_UP_ARROW = 0xE075,
    PS2_KEY_LEFT_ARROW = 0xE06B,
    PS2_KEY_DOWN_ARROW = 0xE072,
    PS2_KEY_RIGHT_ARROW = 0xE074,
    PS2_KEY_NUM_DIVIDE = 0xE04A,
    PS2_KEY_NUM_ENTER = 0xE05A
};

/* All callbacks are optional, set to NULL if don't care. */
void keycode_init(
    keycode_state_t *s,
    void (*handle_keyevent_callback)(int16_t vkey, bool pressed, void *cookie),
    void (*handle_chartyped_callback)(int c, void *cookie),
    void (*handle_led_state_changed_callback)(void *cookie)
);

#if KEYBOARD_KEY_DEBUG
const char* keycode_vkey_desc(uint16_t vk);
#endif

int16_t keycode_info_char_modifier(keycode_info_t *info, bool ctrl, bool shift);

int16_t keycode_info_char(keycode_state_t *s, keycode_info_t *info);

int16_t keycode_ps2_to_vkey(int32_t ps2_keycode);

keycode_info_t *keycode_process_vkey_event(keycode_state_t *s, int32_t vkey, bool pressed,
                                           void* cookie);

int16_t keycode_process_vkey_event_to_char(keycode_state_t *s, int32_t vkey, bool pressed,
                                           void* cookie);

bool keycode_get_async_vkey_state(keycode_state_t *s, int32_t vkey);

