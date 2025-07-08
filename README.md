<!--
     Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)

     SPDX-License-Identifier: CC-BY-SA-4.0
-->

Collection of OS independent utility libs:

* libcpio - a library for parsing CPIO files.
* libelf - a library for parsing ELF files.
* libethdrivers - a library for ethernet drivers.
* libpci - a library for PCI drivers.
* libplatsupport - a library of platform support utilities, interfaces for interacting with drivers, timer drivers, serial drivers and clock drivers.
* libfdt - a library for flat device tree manipulation [libfdt](https://github.com/dgibson/dtc), [libfdt-linux](https://github.com/torvalds/linux/tree/master/scripts/dtc/libfdt)
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
  * libpicotcp - a wrapper library to include [picotcp](https://github.com/tass-belgium/picotcp).
