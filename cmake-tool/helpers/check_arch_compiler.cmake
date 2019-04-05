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

if("${KernelSel4Arch}" STREQUAL "ia32")
    set(compiler_variable "defined(__i386)")
elseif("${KernelSel4Arch}" STREQUAL "x86_64")
    set(compiler_variable "defined(__x86_64)")
elseif("${KernelSel4Arch}" STREQUAL "aarch32" OR "${KernelSel4Arch}" STREQUAL "arm_hyp")
    if(KernelArchArmV7ve)
        set(compiler_variable "defined(__ARM_ARCH_7VE__)")
    elseif(KernelArchArmV7a)
        set(compiler_variable "defined(__ARM_ARCH_7A__)")
    elseif(KernelArchArmV6)
        set(compiler_variable "defined(__ARM_ARCH_6__)")
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
message("${KernelSel4Arch}")
set(arch_test "
#if ${compiler_variable}
    int main() {return 0;}
#else
#error Invalid arch
#endif
")
unset(compiler_arch_test CACHE)
check_c_source_compiles("${arch_test}" compiler_arch_test)

if(NOT compiler_arch_test)
    message(SEND_ERROR "Compiler: ${CMAKE_C_COMPILER} isn't for seL4_arch: ${KernelSel4Arch}")
endif()
