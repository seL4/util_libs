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

#ifndef ENABLE_ANSI_ESC_SEQENCES
#define ENABLE_ANSI_ESC_SEQENCES true
#endif

#if ENABLE_ANSI_ESC_SEQENCES

#include <utils/ansi_color.h>

/* just set the background color */
#define ANSI_BG_COLOR(backcolor)       \
    COLOR_PREFIX                       \
    COLOR_BACKGROUND COLOR_##backcolor \
    COLOR_SUFFIX

#define A_ERASE      COLOR_PREFIX "K"
#define A_CLEAR      COLOR_PREFIX "2J"
#define A_BG_RESET   COLOR_PREFIX "49" COLOR_SUFFIX
#define A_FG_RESET   COLOR_PREFIX "39" COLOR_SUFFIX

#else
/* undefine everything to disable ansi escape sequences */
#define ANSI_BG_COLOR(x)
#define A_ERASE
#define A_CLEAR

#define COLOR_RESET
#define COLOR_BOLD
#define COLOR_ITALIC
#define COLOR_UNDERLINE
#define COLOR_BLINK
#define COLOR_SUFFIX
#define ANSI_COLOR(x)
#define ANSI_COLOR2(x)
#define COLORIZE(...)
#define COLORIZE2(...)
#define A_BG_RESET
#define A_FG_RESET

#endif /* ENABLE_ANSI_ESC_SEQUENCES */

#define A_RESET      COLOR_RESET
#define A_BOLD       COLOR_BOLD # COLOR_SUFFIX
#define A_ITALIC     COLOR_ITALIC # COLOR_SUFFIX
#define A_UDSCORE    COLOR_UNDERLINE # COLOR_SUFFIX
#define A_BLINK      COLOR_BLINK # COLOR_SUFFIX

/* foreground colors */
#define A_FG_K       ANSI_COLOR(BLACK)
#define A_FG_R       ANSI_COLOR(RED)
#define A_FG_G       ANSI_COLOR(GREEN)
#define A_FG_Y       ANSI_COLOR(YELLOW)
#define A_FG_B       ANSI_COLOR(BLUE)
#define A_FG_M       ANSI_COLOR(MAGENTA)
#define A_FG_C       ANSI_COLOR(CYAN)
#define A_FG_W       ANSI_COLOR(WHITE)

/* background colors */
#define A_BG_K       ANSI_BG_COLOR(BLACK)
#define A_BG_R       ANSI_BG_COLOR(RED)
#define A_BG_G       ANSI_BG_COLOR(GREEN)
#define A_BG_Y       ANSI_BG_COLOR(YELLOW)
#define A_BG_B       ANSI_BG_COLOR(BLUE)
#define A_BG_M       ANSI_BG_COLOR(MAGENTA)
#define A_BG_C       ANSI_BG_COLOR(CYAN)
#define A_BG_W       ANSI_BG_COLOR(WHITE)
