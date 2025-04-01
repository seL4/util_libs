#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

# This module provides `DeclareRootserver` for making an executable target a rootserver
# and bundling it with a kernel.elf and any required loaders into an `images` directory
# in the top level build directory.
include_guard(GLOBAL)

# Helper function for modifying the linker flags of a target to set the entry point as _sel4_start
function(SetSeL4Start target)
    set_property(
        TARGET ${target}
        APPEND_STRING
        PROPERTY LINK_FLAGS " -Wl,-u_sel4_start -Wl,-e_sel4_start "
    )
endfunction(SetSeL4Start)

# We need to the real non symlinked list path in order to find the linker script that is in
# the common-tool directory
find_file(
    TLS_ROOTSERVER tls_rootserver.lds
    PATHS "${CMAKE_CURRENT_LIST_DIR}"
    CMAKE_FIND_ROOT_PATH_BOTH
)
mark_as_advanced(TLS_ROOTSERVER)

find_file(UIMAGE_TOOL make-uimage PATHS "${CMAKE_CURRENT_LIST_DIR}" CMAKE_FIND_ROOT_PATH_BOTH)
mark_as_advanced(UIMAGE_TOOL)
include(CMakeDependentOption)
cmake_dependent_option(UseRiscVOpenSBI "Use OpenSBI." ON "KernelArchRiscV" OFF)

if(UseRiscVOpenSBI)
    set(OPENSBI_PATH "${CMAKE_SOURCE_DIR}/tools/opensbi" CACHE STRING "OpenSBI Folder location")
    mark_as_advanced(FORCE OPENSBI_PATH)
endif()

function(DeclareRootserver rootservername)
    SetSeL4Start(${rootservername})
    set_property(
        TARGET ${rootservername}
        APPEND_STRING
        PROPERTY LINK_FLAGS " -Wl,-T ${TLS_ROOTSERVER} "
    )
    if("${KernelArch}" STREQUAL "x86")
        set(
            IMAGE_NAME
            "${CMAKE_BINARY_DIR}/images/${rootservername}-image-${KernelSel4Arch}-${KernelPlatform}"
        )
        set(
            KERNEL_IMAGE_NAME
            "${CMAKE_BINARY_DIR}/images/kernel-${KernelSel4Arch}-${KernelPlatform}"
        )
        # Declare targets for building the final kernel image
        if(Kernel64)
            add_custom_command(
                OUTPUT "${KERNEL_IMAGE_NAME}"
                COMMAND
                    ${CMAKE_OBJCOPY} -O elf32-i386 $<TARGET_FILE:kernel.elf> "${KERNEL_IMAGE_NAME}"
                VERBATIM
                DEPENDS kernel.elf
                COMMENT "objcopy kernel into bootable elf"
            )
        else()
            add_custom_command(
                OUTPUT "${KERNEL_IMAGE_NAME}"
                COMMAND cp $<TARGET_FILE:kernel.elf> "${KERNEL_IMAGE_NAME}"
                VERBATIM
                DEPENDS kernel.elf
            )
        endif()
        add_custom_command(
            OUTPUT "${IMAGE_NAME}"
            COMMAND cp $<TARGET_FILE:${rootservername}> "${IMAGE_NAME}"
            DEPENDS ${rootservername}
        )
        add_custom_target(
            rootserver_image ALL
            DEPENDS
                "${IMAGE_NAME}"
                "${KERNEL_IMAGE_NAME}"
                kernel.elf
                $<TARGET_FILE:${rootservername}>
                ${rootservername}
        )
    elseif(KernelArchARM OR KernelArchRiscV)
        set(
            IMAGE_NAME
            "${CMAKE_BINARY_DIR}/images/${rootservername}-image-${KernelArch}-${KernelPlatform}"
        )
        set(elf_target_file $<TARGET_FILE:elfloader>)
        if(KernelArchRiscV)
            if(UseRiscVOpenSBI)
                # When using OpenSBI, the whole system image (usually consisting
                # of the ELF-Loader, a device tree, the kernel, and a userland)
                # is packaged as an OpenSBI payload.

                if("${CROSS_COMPILER_PREFIX}" STREQUAL "")
                    message(FATAL_ERROR "CROSS_COMPILER_PREFIX not set.")
                endif()

                if("${KernelOpenSBIPlatform}" STREQUAL "")
                    message(FATAL_ERROR "KernelOpenSBIPlatform not set.")
                endif()

                if(NOT OPENSBI_PLAT_XLEN)
                    set(OPENSBI_PLAT_XLEN "${KernelWordSize}")
                endif()

                if(NOT OPENSBI_PLAT_ISA)
                    set(OPENSBI_PLAT_ISA "rv${OPENSBI_PLAT_XLEN}imafdc")

                    # Determine if GNU toolchain is used and if yes,
                    # whether GCC version >= 11.3 (implies binutils version >= 2.38)
                    if(
                        CMAKE_ASM_COMPILER_ID STREQUAL "GNU"
                        AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL "11.3"
                    )
                        # Manually enable Zicsr and Zifencei extensions
                        # This became necessary due to a change in the
                        # default ISA version in GNU binutils 2.38 which
                        # is the default binutils version shipped with GCC 11.3
                        string(APPEND OPENSBI_PLAT_ISA "_zicsr_zifencei")
                    endif()
                endif()

                if(NOT OPENSBI_PLAT_ABI)
                    if(Kernel32)
                        set(OPENSBI_PLAT_ABI "ilp32d")
                    else()
                        set(OPENSBI_PLAT_ABI "lp64d")
                    endif()
                endif()

                file(GLOB_RECURSE deps)
                set(OPENSBI_BINARY_DIR "${CMAKE_BINARY_DIR}/opensbi")
                # OPENSBI_PLAYLOAD is a binary dump of the system image ELF
                set(OPENSBI_PLAYLOAD "${OPENSBI_BINARY_DIR}/payload")
                # OPENSBI_SYSTEM_IMAGE_ELF is the OpenSBI EFL file that contains
                # our system image as firmware payload
                set(
                    OPENSBI_SYSTEM_IMAGE_ELF
                    "${OPENSBI_BINARY_DIR}/platform/${KernelOpenSBIPlatform}/firmware/fw_payload.elf"
                )

                add_custom_command(
                    OUTPUT "${OPENSBI_SYSTEM_IMAGE_ELF}"
                    COMMAND mkdir -p "${OPENSBI_BINARY_DIR}"
                    COMMAND
                        make -s -C "${OPENSBI_PATH}" O="${OPENSBI_BINARY_DIR}"
                        PLATFORM="${KernelOpenSBIPlatform}" clean
                    COMMAND
                        ${CMAKE_OBJCOPY} -O binary "${elf_target_file}" "${OPENSBI_PLAYLOAD}"
                    COMMAND
                        make -C "${OPENSBI_PATH}" O="${OPENSBI_BINARY_DIR}"
                        CROSS_COMPILE=${CROSS_COMPILER_PREFIX} PLATFORM="${KernelOpenSBIPlatform}"
                        PLATFORM_RISCV_XLEN=${OPENSBI_PLAT_XLEN}
                        PLATFORM_RISCV_ISA=${OPENSBI_PLAT_ISA}
                        PLATFORM_RISCV_ABI=${OPENSBI_PLAT_ABI} FW_PAYLOAD_PATH="${OPENSBI_PLAYLOAD}"
                    DEPENDS "${elf_target_file}" elfloader ${USES_TERMINAL_DEBUG}
                )
                # overwrite elf_target_file, it's no longer the ElfLoader but
                # the OpenSBI ELF (which contains the ElfLoader as payload)
                set(elf_target_file "${OPENSBI_SYSTEM_IMAGE_ELF}")
            endif()
        endif()

        if(NOT ElfloaderImage)
            # Seems the Elfloader CMake project was not included?
            message(FATAL_ERROR "ElfloaderImage is not set.")
        endif()
        set(binary_efi_list "binary;efi")
        if(${ElfloaderImage} IN_LIST binary_efi_list)
            # If not an elf we construct an intermediate rule to do an objcopy to binary
            add_custom_command(
                OUTPUT "${IMAGE_NAME}"
                COMMAND
                    ${CMAKE_OBJCOPY} -O binary ${elf_target_file} "${IMAGE_NAME}"
                DEPENDS ${elf_target_file} elfloader
            )
        elseif("${ElfloaderImage}" STREQUAL "uimage")
            # Construct payload for U-Boot.
            if(KernelSel4ArchAarch32)
                set(UIMAGE_ARCH "arm")
            elseif(KernelSel4ArchAarch64)
                set(UIMAGE_ARCH "arm64")
            elseif(KernelArchRiscV)
                set(UIMAGE_ARCH "riscv")
            else()
                message(
                    FATAL_ERROR "uimage: Unsupported architecture: ${KernelArch}/${KernelSel4Arch}"
                )
            endif()

            # Older versions of CMake don't look for readelf when initializing other binutils tools.
            if(NOT CMAKE_READELF)
                find_program(CMAKE_READELF NAMES ${CROSS_COMPILER_PREFIX}readelf)
                if(NOT CMAKE_READELF)
                    message(
                        FATAL_ERROR
                            "Could not find a valid readelf program: ${CROSS_COMPILER_PREFIX}readelf.
                        ElfloaderImage type 'uimage' cannot be built."
                    )
                endif()
            endif()

            add_custom_command(
                OUTPUT "${IMAGE_NAME}"
                COMMAND
                    ${UIMAGE_TOOL} ${CMAKE_OBJCOPY} ${CMAKE_READELF} ${elf_target_file}
                    ${UIMAGE_ARCH} ${IMAGE_NAME}
                DEPENDS ${elf_target_file} elfloader
            )
        else()
            add_custom_command(
                OUTPUT "${IMAGE_NAME}"
                COMMAND
                    ${CMAKE_COMMAND} -E copy ${elf_target_file} "${IMAGE_NAME}"
                DEPENDS ${elf_target_file} elfloader
            )
        endif()
        add_custom_target(rootserver_image ALL DEPENDS "${IMAGE_NAME}" elfloader ${rootservername})
        # Set the output name for the rootserver instead of leaving it to the generator. We need
        # to do this so that we can put the rootserver image name as a property and have the
        # elfloader pull it out using a generator expression, since generator expression cannot
        # nest (i.e. in the expansion of $<TARGET_FILE:tgt> 'tgt' cannot itself be a generator
        # expression. Nor can a generator expression expand to another generator expression and
        # get expanded again. As a result we just fix the output name and location of the rootserver
        set_property(TARGET "${rootservername}" PROPERTY OUTPUT_NAME "${rootservername}")
        get_property(rootimage TARGET "${rootservername}" PROPERTY OUTPUT_NAME)
        get_property(dir TARGET "${rootservername}" PROPERTY BINARY_DIR)
        set_property(TARGET rootserver_image PROPERTY ROOTSERVER_IMAGE "${dir}/${rootimage}")
    else()
        message(FATAL_ERROR "Unsupported architecture.")
    endif()
    # Store the image and kernel image as properties
    # We use relative paths to the build directory
    file(RELATIVE_PATH IMAGE_NAME_REL ${CMAKE_BINARY_DIR} ${IMAGE_NAME})
    if(NOT "${KERNEL_IMAGE_NAME}" STREQUAL "")
        file(RELATIVE_PATH KERNEL_IMAGE_NAME_REL ${CMAKE_BINARY_DIR} ${KERNEL_IMAGE_NAME})
    endif()
    set_property(TARGET rootserver_image PROPERTY IMAGE_NAME "${IMAGE_NAME_REL}")
    set_property(TARGET rootserver_image PROPERTY KERNEL_IMAGE_NAME "${KERNEL_IMAGE_NAME_REL}")
endfunction(DeclareRootserver)
