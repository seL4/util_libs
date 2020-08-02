#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

function(check_arch_clang)
    if("${KernelSel4Arch}" STREQUAL "ia32")
        string(REGEX MATCH "^x86_64" correct_triple ${TRIPLE})
    elseif("${KernelSel4Arch}" STREQUAL "x86_64")
        string(REGEX MATCH "^x86_64" correct_triple ${TRIPLE})
    elseif("${KernelSel4Arch}" STREQUAL "aarch32" OR "${KernelSel4Arch}" STREQUAL "arm_hyp")
        string(REGEX MATCH "^arm" correct_triple ${TRIPLE})
    elseif("${KernelSel4Arch}" STREQUAL "aarch64")
        string(REGEX MATCH "^aarch64" correct_triple ${TRIPLE})
    elseif("${KernelSel4Arch}" STREQUAL "riscv32")
        string(REGEX MATCH "^riscv(32|64)" correct_triple ${TRIPLE})
    elseif("${KernelSel4Arch}" STREQUAL "riscv64")
        string(REGEX MATCH "^riscv64" correct_triple ${TRIPLE})
    else()
        message(SEND_ERROR "KernelSel4Arch is not set to a valid arch")
    endif()

    if(NOT correct_triple)
        message(SEND_ERROR "Clang Triple: ${TRIPLE} isn't for seL4_arch: ${KernelSel4Arch}")
    endif()

endfunction()

function(check_arch_gcc)
    if("${KernelSel4Arch}" STREQUAL "ia32")
        set(compiler_variable "defined(__i386)")
    elseif("${KernelSel4Arch}" STREQUAL "x86_64")
        set(compiler_variable "defined(__x86_64)")
    elseif("${KernelSel4Arch}" STREQUAL "aarch32" OR "${KernelSel4Arch}" STREQUAL "arm_hyp")
        if(KernelArchArmV7a OR KernelArchArmV7ve)
            set(compiler_variable "defined(__ARM_ARCH_7A__)")
        elseif(KernelArchArmV6)
            set(compiler_variable "defined(__ARM_ARCH_6__)")
        elseif(KernelArchArmV8a)
            set(compiler_variable "defined(__ARM_ARCH_8A__)")
        endif()
    elseif("${KernelSel4Arch}" STREQUAL "aarch64")
        set(compiler_variable "defined(__aarch64__)")
    elseif("${KernelSel4Arch}" STREQUAL "riscv32")
        set(compiler_variable "__riscv_xlen == 32")
    elseif("${KernelSel4Arch}" STREQUAL "riscv64")
        set(compiler_variable "__riscv_xlen == 64")
    else()
        message(SEND_ERROR "KernelSel4Arch is not set to a valid arch")
    endif()

    set(arch_test "
#if ${compiler_variable}
    int main() {return 0;}
#else
#error Invalid arch
#endif
    ")

    check_c_source_compiles("${arch_test}" compiler_arch_test)

    if(NOT compiler_arch_test)
        message(SEND_ERROR "Compiler: ${CMAKE_C_COMPILER} isn't for seL4_arch: ${KernelSel4Arch}")
    endif()

endfunction()

function(check_arch_compiler)
    if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
        check_arch_clang()
    else()
        check_arch_gcc()
    endif()
endfunction()
