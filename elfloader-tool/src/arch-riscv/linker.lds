/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */
OUTPUT_ARCH( "riscv" )

ENTRY( _start )

SECTIONS
{

  /*--------------------------------------------------------------------*/
  /* Code and read-only segment                                         */
  /*--------------------------------------------------------------------*/

  /* Begining of code and text segment */
  . = 0x0000000080400000;
  _text = .;
  /* . = 0x00000000ff000000; *
  _ftext = .;
  PROVIDE( eprol = . );


  /* text: Program code section */
  .start :
    {
        *(.text.start)
    }

  .text :
  {
    *(.text)
  }

  /* End of code and read-only segment */
  PROVIDE( etext = . );
  _etext = .;

  /*--------------------------------------------------------------------*/
  /* Initialized data segment                                           */
  /*--------------------------------------------------------------------*/

  /* Start of initialized data segment */
  . = ALIGN(16);
   _fdata = .;

  /* Start of data section */

    _sdata = .;
   .sdata : {
    __global_pointer$ = . + 0x800;
    *(.sdata*)
  }
    .srodata : {
    *(.srodata*)
  }

  /* data: Writable data */
  .data :
  {
    *(.data)
    *(.data.*)
  }

  /* End of initialized data segment */
  . = ALIGN(4);
  PROVIDE( edata = . );
  _edata = .;

  /*--------------------------------------------------------------------*/
  /* Uninitialized data segment                                         */
  /*--------------------------------------------------------------------*/

  /* Start of uninitialized data segment */
  . = .;
  _fbss = .;

  /* sbss: Uninitialized writeable small data section */
  . = .;

  /* bss: Uninitialized writeable data section */
  . = .;
  _bss_start = .;
  .bss :
  {
    *(.bss)
    *(.bss.*)
    *(.sbss*)
  }

  /* rodata: Read-only data */
  .rodata :
  {
    *(.rodata)
    *(.rodata.*)

    . = ALIGN(8);
    _archive_start = .;
      *(._archive_cpio)
    _archive_end = .;
  }

_end = .;
}
