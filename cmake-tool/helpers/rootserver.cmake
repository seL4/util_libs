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

if(KernelArchRiscV)
    set(BBL_PATH ${CMAKE_SOURCE_DIR}/tools/riscv-pk CACHE STRING "BBL Folder location")
    mark_as_advanced(FORCE BBL_PATH)
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
            # On RISC-V we need to package up our final elf image into the Berkeley boot loader
            # which is what the following custom command is achieving

            # TODO: Currently we do not support native RISC-V builds, because there
            # is no native environment to test this. Thus CROSS_COMPILER_PREFIX is
            # always set and the BBL build below uses it to create the
            # "--host=..." parameter. For now, make the build fail if
            # CROSS_COMPILER_PREFIX if not set. It seems that native builds can
            # simply omit the host parameter.
            if("${CROSS_COMPILER_PREFIX}" STREQUAL "")
                message(FATAL_ERROR "CROSS_COMPILER_PREFIX not set.")
            endif()

            # Get host string which is our cross compiler minus the trailing '-'
            string(
                REGEX
                REPLACE
                    "^(.*)-$"
                    "\\1"
                    host
                    ${CROSS_COMPILER_PREFIX}
            )
            get_filename_component(host ${host} NAME)
            if(KernelSel4ArchRiscV32)
                set(march rv32imafdc)
            else()
                set(march rv64imafdc)
            endif()
            file(GLOB_RECURSE deps)
            add_custom_command(
                OUTPUT "${CMAKE_BINARY_DIR}/bbl/bbl"
                COMMAND mkdir -p ${CMAKE_BINARY_DIR}/bbl
                COMMAND
                    cd ${CMAKE_BINARY_DIR}/bbl && ${BBL_PATH}/configure
                    --quiet
                    --host=${host}
                    --with-arch=${march}
                    --with-payload=${elf_target_file}
                        && make -s clean && make -s > /dev/null
                DEPENDS ${elf_target_file} elfloader ${USES_TERMINAL_DEBUG}
            )
            set(elf_target_file "${CMAKE_BINARY_DIR}/bbl/bbl")
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

            if("${KernelArmSel4Arch}" STREQUAL "aarch32")
                set(UIMAGE_ARCH "arm")
            elseif(KernelSel4ArchAarch64)
                set(UIMAGE_ARCH "arm64")
            else()
                message(FATAL_ERROR "uimage: Unsupported architecture: ${KernelArch}")
            endif()

            add_custom_command(
                OUTPUT "${IMAGE_NAME}"
                COMMAND
                    ${UIMAGE_TOOL} ${CMAKE_OBJCOPY} ${elf_target_file} ${UIMAGE_ARCH} ${IMAGE_NAME}
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
