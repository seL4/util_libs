#
# Copyright 2019, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

# This module contains functions for creating a cpio archive containing a list
# of input files and turning it into an object file that can be linked into a binary.

if(KernelSel4ArchIA32)
    set(LinkOFormat "elf32-i386")
elseif(KernelSel4ArchX86_64)
    set(LinkOFormat "elf64-x86-64")
elseif(KernelSel4ArchAarch32 OR KernelSel4ArchArmHyp)
    set(LinkOFormat "elf32-littlearm")
elseif(KernelSel4ArchAarch64)
    set(LinkOFormat "elf64-littleaarch64")
elseif(KernelSel4ArchRiscV32)
    set(LinkOFormat "elf32-littleriscv")
elseif(KernelSel4ArchRiscV64)
    set(LinkOFormat "elf64-littleriscv")
endif()

# Checks the existence of an argument to cpio -o.
# flag refers to a variable in the parent scope that contains the argument, if
# the argument isn't supported then the flag is set to the empty string in the parent scope.
function(CheckCPIOArgument flag)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cpio-testfile "Testfile contents")
    execute_process(
        COMMAND bash -c "echo cpio-testfile | cpio ${${flag}} -o"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        OUTPUT_QUIET ERROR_QUIET
        RESULT_VARIABLE result
    )
    if(result)
        set(${flag} "" PARENT_SCOPE)
    endif()
    file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/cpio-testfile)
endfunction()

# Function for declaring rules to build a cpio archive that can be linked
# into another target
function(MakeCPIO output_name input_files)
    cmake_parse_arguments(PARSE_ARGV 2 MAKE_CPIO "" "CPIO_SYMBOL" "DEPENDS")
    if(NOT "${MAKE_CPIO_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(FATAL_ERROR "Unknown arguments to MakeCPIO")
    endif()
    set(archive_symbol "_cpio_archive")
    if(NOT "${MAKE_CPIO_CPIO_SYMBOL}" STREQUAL "")
        set(archive_symbol ${MAKE_CPIO_CPIO_SYMBOL})
    endif()
    # Check that the reproducible flag is available. Don't use it if it isn't.
    set(reproducible_flag "--reproducible")
    CheckCPIOArgument(reproducible_flag)
    set(append "")
    set(commands "")
    foreach(file IN LISTS input_files)
        # Try and generate reproducible cpio meta-data as we do this:
        # - touch -d @0 file sets the modified time to 0
        # - --owner=root:root sets user and group values to 0:0
        # - --reproducible creates reproducible archives with consistent inodes and device numbering
        list(
            APPEND
                commands
                "bash;-c;cd `dirname ${file}` && mkdir -p temp_${output_name} && cd temp_${output_name} && cp -a ${file} . && touch -d @0 `basename ${file}` && echo `basename ${file}` | cpio ${append} ${reproducible_flag} --owner=root:root --quiet -o -H newc --file=${CMAKE_CURRENT_BINARY_DIR}/archive.${output_name}.cpio && rm `basename ${file}` && cd ../ && rmdir temp_${output_name};&&"
        )
        set(append "--append")
    endforeach()
    list(APPEND commands "true")

    # RiscV doesn't support linking with -r
    if(KernelArchRiscV)
        set(relocate "")
    else()
        set(relocate "-r")
    endif()
    add_custom_command(
        OUTPUT ${output_name}
        COMMAND rm -f archive.${output_name}.cpio
        COMMAND ${commands}
        COMMAND
            echo
            "SECTIONS { ._archive_cpio : ALIGN(4) { ${archive_symbol} = . ; *(.*) ; ${archive_symbol}_end = . ; } }"
            > link.${output_name}.ld
        COMMAND
            ${CROSS_COMPILER_PREFIX}ld -T link.${output_name}.ld
            --oformat
                ${LinkOFormat} ${relocate} -b binary archive.${output_name}.cpio -o ${output_name}
                BYPRODUCTS archive.${output_name}.cpio link.${output_name}.ld
        DEPENDS ${input_files} ${MAKE_CPIO_DEPENDS}
        VERBATIM
        COMMENT "Generate CPIO archive ${output_name}"
    )
endfunction(MakeCPIO)
