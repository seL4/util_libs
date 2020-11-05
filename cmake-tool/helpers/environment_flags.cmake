#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)
include_guard(GLOBAL)

macro(add_default_compilation_options)
    # Setup base flags as defined by the kernel before including the rest
    include(${KERNEL_FLAGS_PATH})

    if(("${CMAKE_BUILD_TYPE}" STREQUAL "Release") OR ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel"))
        option(UserLinkerGCSections "Perform dead code and data removal
            Build user level with -ffunction-sections and -fdata-sections and
            link with --gc-sections. The first two options place each function
            and data in a different section such that --gc-sections is able
            to effectively discard sections that are unused after a reachability
            analysis. This does not interact well with debug symbols generated
            by -g and can in some cases result in larger object files and binaries" ON)

        if(UserLinkerGCSections)
            add_compile_options(-ffunction-sections -fdata-sections)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections ")
        endif()
    endif()
    mark_as_advanced(UserLinkerGCSections)

    add_compile_options(
        -nostdinc
        -fno-pic
        -fno-pie
        -fno-stack-protector
        -fno-asynchronous-unwind-tables
        -ftls-model=local-exec
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc++")
    set(CMAKE_C_STANDARD 11)
    set(LinkPageSize "0x1000" CACHE STRING "Page size to be used for linker")
    mark_as_advanced(LinkPageSize)
    set(
        CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -static -nostdlib -z max-page-size=${LinkPageSize}"
    )

    if(KernelArchX86)
        add_compile_options(-mtls-direct-seg-refs)
    endif()

    if(KernelSel4ArchAarch32)
        add_compile_options(-mtp=soft)
    endif()

    # Don't allow unaligned data store/load instructions as this will cause an alignment
    # fault on any seL4 memory regions that are uncached as the mapping attributes the kernel
    # uses causes alignment checks to be enabled.
    if(KernelSel4ArchAarch64)
        add_compile_options(-mstrict-align)
        if(NOT CMAKE_C_COMPILER_VERSION)
            message(FATAL_ERROR "CMAKE_C_COMPILER_VERSION is not set")
        endif()
        # special handling for GCC 10 and above
        if(
            (CMAKE_C_COMPILER_ID STREQUAL "GNU")
            AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL "10.0.0")
        )
            add_compile_options(-mno-outline-atomics)
        endif()
    elseif(KernelSel4ArchAarch32)
        add_compile_options(-mno-unaligned-access)
    endif()
endmacro()

macro(gcc_print_file_name var file)
    if(NOT (DEFINED "${var}"))
        separate_arguments(c_arguments UNIX_COMMAND "${CMAKE_C_FLAGS}")
        # Append the target flag to the arguments if we are using clang so the
        # correct crt files are found
        if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
            list(APPEND c_arguments "${CMAKE_C_COMPILE_OPTIONS_TARGET}${CMAKE_C_COMPILER_TARGET}")
        endif()
        execute_process(
            COMMAND ${CMAKE_C_COMPILER} ${c_arguments} -print-file-name=${file}
            OUTPUT_VARIABLE ${var}
            ERROR_VARIABLE IgnoreErrorOutput
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set("${var}" "${${var}}" CACHE INTERNAL "")
    endif()
endmacro()

macro(find_libgcc_files)

    # find the compilers crtbegin and crtend files
    gcc_print_file_name(CRTBeginFile crtbegin.o)
    gcc_print_file_name(CRTEndFile crtend.o)
    gcc_print_file_name(libgcc_eh libgcc_eh.a)
    set(libgcc "-lgcc")
    if(NOT "${libgcc_eh}" STREQUAL "libgcc_eh.a")
        set(libgcc "${libgcc} -lgcc_eh")
    endif()
endmacro()

# Call check_c_source_runs but set the cache variables
# that cause the generated executable to not be run
# on cross_compilation targets.
macro(check_c_source_runs_cross_compile program var)
    set(${var}_EXITCODE 0 CACHE INTERNAL "")
    set(${var}_EXITCODE__TRYRUN_OUTPUT "" CACHE INTERNAL "")
    check_c_source_runs(${program} ${var})
endmacro()

macro(add_fpu_compilation_options)
    # We want to check what we can set the -mfloat-abi to on arm and if that matches what is requested
    if(KernelSel4ArchAarch64)
        if(NOT KernelHaveFPU)
            add_compile_options(-mgeneral-regs-only)
        endif()
    elseif(KernelArchARM)
        find_libgcc_files()
        include(CheckCSourceRuns)
        # Set the link libraries to use our crt and libgcc files.
        # This picks up inconsistencies in floating point ABIs.
        set(CMAKE_REQUIRED_LIBRARIES ${CRTBeginFile} ${libgcc} ${CRTEndFile})
        set(test_program "void main(void){}")
        if(KernelHaveFPU)
            set(CMAKE_REQUIRED_FLAGS "-mfloat-abi=hard")
            check_c_source_runs_cross_compile(${test_program} HARD_FLOAT)
            if(NOT HARD_FLOAT)
                set(CMAKE_REQUIRED_FLAGS "-mfloat-abi=softfp")
                check_c_source_runs_cross_compile(${test_program} SOFTFP_FLOAT)
                if(NOT SOFTFP_FLOAT)
                    message(
                        WARNING "Kernel supports hardware floating point but toolchain does not"
                    )
                    add_compile_options(-mfloat-abi=soft)
                else()
                    add_compile_options(-mfloat-abi=softfp)
                endif()
            else()
                add_compile_options(-mfloat-abi=hard)
            endif()
        else()
            set(CMAKE_REQUIRED_FLAGS "-mfloat-abi=soft")
            check_c_source_runs_cross_compile(${test_program} SOFT_FLOAT)
            if(NOT SOFT_FLOAT)
                message(
                    SEND_ERROR
                        "Kernel does not support hardware floating point but toolchain cannot build software floating point"
                )
            else()
                add_compile_options(-mfloat-abi=soft)
            endif()
        endif()
    endif()
endmacro()
