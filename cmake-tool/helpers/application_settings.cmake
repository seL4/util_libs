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

function(ApplyCommonSimulationSettings)
    if (KernelArchX86)
        # Generally we cannot simulate some more recent features
        set(KernelSupportPCID OFF CACHE BOOL "" FORCE)
        set(KernelFSGSBase msr CACHE STRING "" FORCE)
        set(KernelIOMMU OFF CACHE BOOL "" FORCE)
        set(KernelFPU FXSAVE CACHE STRING "" FORCE)
    endif()
endfunction()

function(ApplyCommonReleaseVerificationSettings release verification)
    # Setup flags for different combinations of 'release' (performance optimized builds) and
    # 'verification' (verification friendly features) builds
    if (release)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
    else()
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
    endif()
    if (verification)
        set(KernelVerificationBuild ON CACHE BOOL "" FORCE)
    else()
        set(KernelVerificationBuild OFF CACHE BOOL "" FORCE)
        set(KernelPrinting ON CACHE BOOL "" FORCE)
    endif()
    # If neither release nor verification then enable debug facilities, otherwise turn them off
    if ((NOT release) AND (NOT verification))
        set(KernelDebugBuild ON CACHE BOOL "" FORCE)
    else()
        set(KernelDebugBuild OFF CACHE BOOL "" FORCE)
    endif()
endfunction()
