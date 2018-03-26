#
# Copyright 2017, Data61
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

# Make build options a visible choice, default it to Debug
set(force "")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(force "FORCE")
endif()
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Set the user mode build type (kernel build ignores this)" ${force})
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;RelWithDebInfo;MinSizeRel")
mark_as_advanced(CLEAR CMAKE_BUILD_TYPE)

# Setup base flags as defined by the kernel before including the rest
include(${KERNEL_FLAGS_PATH})

if ((${CMAKE_BUILD_TYPE} STREQUAL "Release") OR (${CMAKE_BUILD_TYPE} STREQUAL "MinSizeRel"))
    option(UserLinkerGCSections "Perform dead code and data removal
        Build user level with -ffunction-sections and -fdata-sections and
        link with --gc-sections. The first two options place each function
        and data in a different section such that --gc-sections is able
        to effectively discard sections that are unused after a reachability
        analysis. This does not interact well with debug symbols generated
        by -g and can in some cases result in larger object files and binaries"

        ON
    )

    if (UserLinkerGCSections)
        add_compile_options(-ffunction-sections -fdata-sections)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections ")
    endif()
endif()

add_compile_options(-nostdinc -fno-pic -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -ftls-model=local-exec)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc++")
set(CMAKE_C_STANDARD 11)
set(LinkPageSize "0x1000" CACHE STRING "Page size to be used for linker")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -nostdlib -z max-page-size=${LinkPageSize}")

if(KernelArchX86)
    add_compile_options(-mtls-direct-seg-refs)
endif()

# find the compilers crtbegin and crtend files
separate_arguments(c_arguments UNIX_COMMAND "${CMAKE_C_FLAGS}")
execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${c_arguments} -print-file-name=crtbegin.o
    OUTPUT_VARIABLE CRTBeginFile
    ERROR_VARIABLE IgnoreErrorOutput
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${c_arguments} -print-file-name=crtend.o
    OUTPUT_VARIABLE CRTEndFile
    ERROR_VARIABLE IgnoreErrorOutput
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${c_arguments} --print-file-name libgcc_eh.a
    OUTPUT_VARIABLE libgcc_eh
    ERROR_VARIABLE IgnoreErrorOutput
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(libgcc "-lgcc")
if(NOT "${libgcc_eh}" STREQUAL "libgcc_eh.a")
    set(libgcc "${libgcc} -lgcc_eh")
endif()
set(CRTObjFiles "${CMAKE_CURRENT_BINARY_DIR}/lib/crt1.o ${CMAKE_CURRENT_BINARY_DIR}/lib/crti.o ${CRTBeginFile}")
set(FinObjFiles "${CRTEndFile} ${CMAKE_CURRENT_BINARY_DIR}/lib/crtn.o")

# -lgcc has to be given twice since whilst normally it should be put at the end, we implement
# some libgcc dependencies in our libc
set(common_link_string "<LINK_FLAGS> ${CRTObjFiles} <OBJECTS> ${libgcc} <LINK_LIBRARIES> \
    ${libgcc} <LINK_LIBRARIES> ${FinObjFiles} -o <TARGET>")
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER>  <FLAGS> <CMAKE_C_LINK_FLAGS> ${common_link_string}")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER>  <FLAGS> <CMAKE_CXX_LINK_FLAGS> ${common_link_string}")
set(CMAKE_ASM_LINK_EXECUTABLE "<CMAKE_ASM_COMPILER>  <FLAGS> <CMAKE_ASM_LINK_FLAGS> ${common_link_string}")

# We want to check what we can set the -mfloat-abi to on arm and if that matches what is requested
if(KernelSel4ArchAarch64)
    if(NOT KernelHaveFPU)
        add_compile_options(-mgeneral-regs-only)
    endif()
elseif(KernelArchARM)
    # Define a helper macro for performing our own compilation tests for floating point
    function(SimpleCCompilationTest var flags)
        if (NOT (DEFINED "${var}"))
            message(STATUS "Performing test ${var} with flags ${flags}")
            file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test_program.c" "void main(void){}")
            execute_process(COMMAND "${CMAKE_C_COMPILER}" ${flags} -o test_program test_program.c
                RESULT_VARIABLE result
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                OUTPUT_QUIET
                ERROR_QUIET
            )
            file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/test_program.c")
            file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/test_program")
            if ("${result}" EQUAL 0)
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
        if (NOT SOFT_FLOAT)
            message(FATAL_ERROR "Kernel does not support hardware floating point but toolchain cannot build software floating point")
        else()
            add_compile_options(-mfloat-abi=soft)
        endif()
    endif()
endif()
