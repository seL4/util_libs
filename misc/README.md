<!--
  Copyright 2020, Data61
  Commonwealth Scientific and Industrial Research Organisation (CSIRO)
  ABN 41 687 119 230.
  This software may be distributed and modified according to the terms of
  the BSD 2-Clause license. Note that NO WARRANTY is provided.
  See "LICENSE_BSD2.txt" for details.
  @TAG(DATA61_BSD)

-->

# seL4_tools misc tools

These are a collection of tools or configuration files that relate to seL4 in some way.

## Style tools

Many of the files are for use in styling sources for various languages.

Files for styling a particular language:
- `style-c.sh`, `astylerc`: Style a single c file using `astyle` and the astylerc config
- `style-cmake.sh`: Style a CMake file using `cmake-format`. Will also look for `.cmake-format.yaml`
  files in repo directories.
- `style-py.sh`: Style python files using `autopep8`
- `.gitlint`: Configuration file for `gitlint` tool for checking Git commit messages.
- `is-valid-shell-script`: Script for checking valid shell script syntax.

Scripts for batching multiple style operations across different files:
- `style.sh`: Finds any `.stylefilter` files in local directories and calls `style.py`
- `style.py`, `filter.py`: Filters an input list of files based on `.stylefilter` and then calls the relevant
  style script based on the remaining file's extensions.
- `style-changed.sh`: Styles all changed files in current Git repository
- `style-all.sh`: Styles all files in current Git repository

## Other

- `whence.py`: A tool for determining source code provenance for imported repositories without history.
- `cpio-strip.c`/`Makefile.cpio_strip`: A program for stripping metadata from CPIO archives to enable
  reproducible builds. (Recent versions of cpio support this with the `--reproducible` flag)
- `cobbler`: Build a qemu-bootable harddisk image.
