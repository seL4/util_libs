<!--
  Copyright 2017, Data61
  Commonwealth Scientific and Industrial Research Organisation (CSIRO)
  ABN 41 687 119 230.

  This software may be distributed and modified according to the terms of
  the BSD 2-Clause license. Note that NO WARRANTY is provided.
  See "LICENSE_BSD2.txt" for details.

  @TAG(DATA61_BSD)
-->
Collection of OS independent utility libs:

* libcpio - a library for parsing CPIO files.
* libelf - a library for parsing ELF files.
* libethdrivers - a library for ethernet drivers.
* libpci - a library for PCI drivers.
* libplatsupport - a library of platform support utilities, interfaces for interacting with drivers, timer drivers, serial drivers and clock drivers.
* libutils - a library of generic utilities including:
  * ansi.h - utilities for formatting ansi output.
  * arith.h - utilities for arithmetic, ie MAX, MIN, ROUND_UP etc.
  * assume.h - provides ASSUME, which allows the user to provide hints to gcc.
  * builtin.h - defines conventient macros for using builtin gcc attributes.
  * compile_time.h - provides compile time asserts.
  * debug.h - various debugging macros.
  * formats.h - formats for printf.
  * list.h - a basic, void * pointer based list implementation.
  * math.h - provies complex math, ie. muldivu64.
  * page.h - provides virtual memory page operations.
  * sglib.h - an open source template library that provides arrays, lists, red-black trees etc.
  * stringify.h - provides macros for creating even more macros.
  * time.h - provides temporal constants (i.e US_IN_S)
  * util.h - includes all util header files.
  * verification.h - macros for verification in Isabelle.
  * zf_log_config.h - provides zf_log config.
  * zf_log.h - an open source logging library.
