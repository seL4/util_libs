#
# Copyright 2018, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

function(ApplyData61ElfLoaderSettings)
    if(KernelArchARM)
        if(${KernelARMPlatform} STREQUAL "hikey" AND ${KernelArmSel4Arch} STREQUAL "aarch64")
            set(ElfloaderImage "efi" CACHE STRING "" FORCE)
        elseif(${KernelARMPlatform} IN_LIST "tx1;hikey;bbone_black")
            set(ElfloaderImage "binary" CACHE STRING "" FORCE)
        elseif(${KernelARMPlatform} STREQUAL "tk1" AND ${KernelArmSel4Arch} STREQUAL "arm_hyp")
            set(ElfloaderMode "hypervisor" CACHE STRING "" FORCE)
            set(ElfloaderMonitorHook ON CACHE BOOL "" FORCE)
        endif()
    endif()
endfunction()
