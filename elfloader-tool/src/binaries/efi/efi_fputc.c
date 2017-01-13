/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <binaries/efi/efi.h>
#include <printf.h>

int efi_fputc(int c, FILE *stream)
{
    (void) stream;
    /* obtain a pointer to EFI output console */
    efi_simple_text_output_protocol_t * efi_console_out = get_efi_con_out();

    /* EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL only accepts unicode chars */
    uint16_t char_to_print[2] = { c, 0 };

    if (char_to_print[0] == '\n')
    {
        uint16_t nl[2] = { '\r', 0 };
        efi_console_out->output_string(efi_console_out, nl);
    }
    efi_console_out->output_string(efi_console_out, char_to_print);

    return 0;
}

