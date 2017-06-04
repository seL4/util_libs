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

#define A_RESET      "\033[0m"
#define A_BOLD       "\033[1m"
#define A_ITALIC     "\033[3m"
#define A_UDSCORE    "\033[4m"
#define A_BLINK      "\033[5m"

#define A_FG_K       "\033[30m"
#define A_FG_R       "\033[31m"
#define A_FG_G       "\033[32m"
#define A_FG_Y       "\033[33m"
#define A_FG_B       "\033[34m"
#define A_FG_M       "\033[35m"
#define A_FG_C       "\033[36m"
#define A_FG_W       "\033[37m"
#define A_FG_RESET   "\033[39m"

#define A_BG_K       "\033[40m"
#define A_BG_R       "\033[41m"
#define A_BG_G       "\033[42m"
#define A_BG_Y       "\033[43m"
#define A_BG_B       "\033[44m"
#define A_BG_M       "\033[45m"
#define A_BG_C       "\033[46m"
#define A_BG_W       "\033[47m"
#define A_BG_RESET   "\033[49m"

#define A_ERASE      "\033[K"
#define A_CLEAR      "\033[2J"

#else /* ENABLE_ANSI_ESC_SEQENCES */

#define A_RESET      ""
#define A_BOLD       ""
#define A_ITALIC     ""
#define A_UDSCORE    ""
#define A_BLINK      ""

#define A_FG_K       ""
#define A_FG_R       ""
#define A_FG_G       ""
#define A_FG_Y       ""
#define A_FG_B       ""
#define A_FG_M       ""
#define A_FG_C       ""
#define A_FG_W       ""
#define A_FG_RESET   ""

#define A_BG_K       ""
#define A_BG_R       ""
#define A_BG_G       ""
#define A_BG_Y       ""
#define A_BG_B       ""
#define A_BG_M       ""
#define A_BG_C       ""
#define A_BG_W       ""
#define A_BG_RESET   ""

#define A_ERASE      ""
#define A_CLEAR      ""

#endif /* ENABLE_ANSI_ESC_SEQENCES */
