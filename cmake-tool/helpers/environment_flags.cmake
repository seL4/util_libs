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

cmake_minimum_required(VERSION 3.7.2)
include_guard(GLOBAL)

macro(add_default_compilation_options)
    # Setup base flags as defined by the kernel before including the rest
    include(${KERNEL_FLAGS_PATH})

    if((${CMAKE_BUILD_TYPE} STREQUAL "Release") OR (${CMAKE_BUILD_TYPE} STREQUAL "MinSizeRel"))
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
endmacro()

macro(gcc_print_file_name var file)
    if(NOT (DEFINED "${var}"))
        separate_arguments(c_arguments UNIX_COMMAND "${CMAKE_C_FLAGS}")
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


macro(add_fpu_compilation_options)
    # We want to check what we can set the -mfloat-abi to on arm and if that matches what is requested
    if(KernelSel4ArchAarch64)
        if(NOT KernelHaveFPU)
            add_compile_options(-mgeneral-regs-only)
        endif()
    elseif(KernelArchARM)
        # Define a helper macro for performing our own compilation tests for floating point
        function(SimpleCCompilationTest var flags)
            if(NOT (DEFINED "${var}"))
                message(STATUS "Performing test ${var} with flags ${flags}")
                file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test_program.c" "void main(void){}")
                execute_process(
                    COMMAND
                        "${CMAKE_C_COMPILER}" ${flags} -o test_program test_program.c
                    RESULT_VARIABLE result
                    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                    OUTPUT_QUIET ERROR_QUIET
                )
                file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/test_program.c")
                file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/test_program")
                if("${result}" EQUAL 0)
                    set("${var}" TRUE CACHE INTERNAL "")
                    message(STATUS "Test ${var} PASSED")
                else()
                    set("${var}" FALSE CACHE INTERNAL "")
                    message(STATUS "Test ${var} FAILED")
                endif()
            endif()
        endfunction(SimpleCCompilationTest)
        if(KernelHaveFPU)
            SimpleCCompilationTest(HARD_FLOAT "-mfloat-abi=hard")
            if(NOT HARD_FLOAT)
                SimpleCCompilationTest(SOFTFP_FLOAT "-mfloat-abi=softfp")
                if(NOT SOFTFP_FLOAT)
                    message(WARNING "Kernel supports hardware floating point but toolchain does not")
                    add_compile_options(-mfloat-abi=soft)
                else()
                    add_compile_options(-mfloat-abi=softfp)
                endif()
            else()
                add_compile_options(-mfloat-abi=hard)
            endif()
        else()
            set(CMAKE_REQUIRED_FLAGS "-mfloat-abi=soft")
            SimpleCCompilationTest(SOFT_FLOAT "-mfloat-abi=soft")
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
