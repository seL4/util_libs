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

# Helper function for modifying the linker flags of a target to set the entry point as _sel4_start
function(SetSeL4Start target)
    set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " -u _sel4_start -e _sel4_start ")
endfunction(SetSeL4Start)

if(KernelSel4ArchIA32)
    set(LinkOFormat "elf32-i386")
elseif(KernelSel4ArchX86_64)
    set(LinkOFormat "elf64-x86-64")
elseif(KernelSel4ArchAarch32 OR KernelSel4ArchArmHyp)
    set(LinkOFormat "elf32-littlearm")
elseif(KernelSel4ArchAarch64)
    set(LinkOFormat "elf64-littleaarch64")
endif()

# Function for declaring rules to build a cpio archive that can be linked
# into another target
function(MakeCPIO output_name input_files)
    set(append "")
    foreach(file IN LISTS input_files)
        list(APPEND commands
            "cd `dirname ${file}`"
            "echo `basename ${file}` | cpio ${append} --quiet -o -H newc --file=${CMAKE_CURRENT_BINARY_DIR}/archive.cpio"
        )
        set(append "--append")
    endforeach()
    file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cpio_script.sh CONTENT "${commands}")
    add_custom_command(OUTPUT ${output_name}
        COMMAND rm -f archive.cpio
        COMMAND chmod u+x cpio_script.sh
        COMMAND ./cpio_script.sh
        COMMAND echo "SECTIONS { ._archive_cpio : ALIGN(4) { _cpio_archive = . ; *(.*) ; _cpio_archive_end = . ; } }"
            > link.ld
        COMMAND ${CROSS_COMPILER_PREFIX}ld -T link.ld --oformat ${LinkOFormat} -r -b binary archive.cpio -o ${output_name}
        BYPRODUCTS archive.cpio link.ld
        DEPENDS ${input_files} cpio_script.sh
        VERBATIM
        COMMENT "Generate CPIO archive ${output_name}"
    )
endfunction(MakeCPIO)

# We need to the real non symlinked list path in order to find the linker script that is in
# the common-tool directory
get_filename_component(real_list "${CMAKE_CURRENT_LIST_DIR}" REALPATH)

function(DeclareRootserver rootservername)
    mark_as_advanced(IMAGE_NAME KERNEL_IMAGE_NAME)
    SetSeL4Start(${rootservername})
    set_property(TARGET ${rootservername} APPEND_STRING PROPERTY LINK_FLAGS " -T ${real_list}/../common-tool/tls_rootserver.lds ")
    if("${KernelArch}" STREQUAL "x86")
        set(IMAGE_NAME "${CMAKE_BINARY_DIR}/images/${rootservername}-image-${KernelSel4Arch}-${KernelPlatform}" CACHE STRING "")
        set(KERNEL_IMAGE_NAME "${CMAKE_BINARY_DIR}/images/kernel-${KernelSel4Arch}-${KernelPlatform}" CACHE STRING "")
        # Declare targets for building the final kernel image
        if(Kernel64)
            add_custom_command(
                OUTPUT "${KERNEL_IMAGE_NAME}"
                COMMAND ${CROSS_COMPILE_PREFIX}objcopy -O elf32-i386 $<TARGET_FILE:kernel.elf> "${KERNEL_IMAGE_NAME}"
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
        add_custom_command(OUTPUT "${IMAGE_NAME}"
            COMMAND cp $<TARGET_FILE:${rootservername}> "${IMAGE_NAME}"
            DEPENDS ${rootservername}
        )
        add_custom_target(rootserver_image ALL DEPENDS "${IMAGE_NAME}" "${KERNEL_IMAGE_NAME}" kernel.elf $<TARGET_FILE:${rootservername}> ${rootservername})
    elseif("${KernelArch}" STREQUAL "arm")
        set(IMAGE_NAME "${CMAKE_BINARY_DIR}/images/${rootservername}-image-arm-${KernelPlatform}")
        if(KernelPlatImx6)
            set(PlatformEntryAddr 0x20000000)
        elseif(KernelPlatformKZM OR KernelPlatformOMAP3 OR KernelPlatformAM335X)
            set(PlatformEntryAddr 0x82000000)
        elseif(KernelPlatExynos5 OR KernelPlatformExynos4)
            set(PlatformEntryAddr 0x41000000)
        elseif(KernelPlatformHikey OR KernelPlatformTx1)
            if (KernelSel4ArchAarch64)
                set(PlatformEntryAddr 0)
            else()
                set(PlatformEntryAddr 0x1000)
            endif()
        elseif(KernelPlatformAPQ8064)
            set(PlatformEntryAddr 0x82008000)
        elseif(KernelPlatformJetson)
            set(PlatformEntryAddr 0x90000000)
        elseif(KernelPlatformZynq7000)
            set(PlatformEntryAddr 0x10000000)
        else()
            message(FATAL_ERROR "Unknown platform when generating image")
        endif()
        separate_arguments(c_arguments UNIX_COMMAND "${CMAKE_C_FLAGS}")
        set(elfloader_output "${IMAGE_NAME}")
        if(NOT "${ElfloaderImage}" STREQUAL "elf")
            set(elfloader_output rootserver.bin)
            # If not an elf we construct an intermediate rule to do an objcopy to binary
            add_custom_command(OUTPUT "${imagename}"
                COMMAND ${CROSS_COMPILER_PREFIX}objcopy -O binary rootserver.bin "${IMAGE_NAME}"
                DEPENDS ${elfloader_output}
            )
        endif()
        add_custom_command(OUTPUT "${elfloader_output}"
            COMMAND cp $<TARGET_FILE:kernel.elf> cpio/kernel.elf
            COMMAND cp $<TARGET_FILE:${rootservername}> cpio/${rootservername}
            COMMAND ${CROSS_COMPILER_PREFIX}strip --strip-all cpio/kernel.elf cpio/${rootservername}
            COMMAND cd cpio && ls | cpio --quiet -o -H newc > ${CMAKE_CURRENT_BINARY_DIR}/elf_archive.cpio && cd ..
            # Convert userspace / kernel into an archive which can then be linked
            # against the elfloader binary. Change to the directory of archive.cpio
            # before this operation to avoid polluting the symbol table with references
            # to the temporary directory.
            COMMAND ${CROSS_COMPILER_PREFIX}ld -T "${ElfloaderArchiveBinLds}" --oformat ${LinkOFormat}
                -r -b binary ${CMAKE_CURRENT_BINARY_DIR}/elf_archive.cpio -o elf_archive.o
            COMMAND
                echo ${c_arguments} -I$<JOIN:$<TARGET_PROPERTY:Configuration,INTERFACE_INCLUDE_DIRECTORIES>,:-I>
                    -P -E -o linker.lds_pp -x c ${ElfloaderLinkerScript}
                | sed "s/:/ /g"
                | xargs
                    ${CMAKE_C_COMPILER}
            COMMAND ${CROSS_COMPILER_PREFIX}ld -T linker.lds_pp --oformat ${LinkOFormat}
                $<TARGET_FILE:elfloader> elf_archive.o -Ttext=${PlatformEntryAddr} -o ${elfloader_output}
            COMMAND ${CROSS_COMPILER_PREFIX}strip --strip-all ${elfloader_output}
            BYPRODUCTS cpio/kernel.elf "cpio/${rootservername}" ${CMAKE_CURRENT_BINARY_DIR}/elf_archive.cpio elf_archive.o linker.lds_pp
            # TODO: this should just have a dependency on elfloader_Config instead of Configuration,
            # and the above TARGET_PROPERTY should reflect that. But currently the linker script
            # wants to include the legacy autoconf.h, so we need to give it access to the entire
            # configuration space
            DEPENDS kernel.elf ${rootservername} elfloader Configuration
            VERBATIM
        )
        add_custom_target(rootserver_image ALL DEPENDS "${IMAGE_NAME}" kernel.elf ${rootservername} elfloader Configuration)
    endif()
endfunction(DeclareRootserver)

# Define function for constructing a target for a library from a legacy build system Makefile
# This is incredibly fragile and rarely works out of the box. Generally needs tweaking or
# can be used as inspiration for rolling your own wrapper
function(AddLegacyLibrary target_name lib_name glob_deps other_deps)
    set(LEGACY_STAGE_DIR "${CMAKE_BINARY_DIR}/stage/${KernelArch}/${KernelPlatform}")
    file(GLOB deps RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" ${glob_deps})

    # Check if our other_deps have any interesting header directories we should use
    # TODO: This should probably use generator expressions and TARGET_PROPERTY to
    # avoid the problem of calling get_property on targets that are not yet defined
    foreach(dep ${other_deps})
        get_property(dep_paths TARGET ${dep} PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
        foreach(dep_path ${dep_paths})
            set(extra_nk_cflags "${extra_nk_cflags} -I${dep_path}")
        endforeach()
    endforeach()
    set(nk_cflags "${CMAKE_C_FLAGS} \
                -I${LEGACY_STAGE_DIR}/include \
                -I${LEGACY_STAGE_DIR}/include/${KernelArch} \
                -I${LEGACY_STAGE_DIR}/include/${KernelPlatform} \
                -I${CMAKE_CURRENT_BINARY_DIR}/stage/include \
                -I${CMAKE_CURRENT_BINARY_DIR}/stage/include/${KernelArch} \
                -I${CMAKE_CURRENT_BINARY_DIR}/stage/include/${KernelPlatform} \
                -I$<JOIN:$<TARGET_PROPERTY:Configuration,INTERFACE_INCLUDE_DIRECTORIES>, -I> \
                ${extra_nk_cflags}"
    )
    if(Kernel64)
        list(APPEND extra_environment "KERNEL_64=y")
    else()
        list(APPEND extra_environment "KERNEL_32=y")
    endif()
    if(CCACHEFOUND)
        set(ccache "ccache")
    endif()

    add_custom_command(
        OUTPUT "${LEGACY_STAGE_DIR}/lib/${lib_name}"
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/stage/placeholder"
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/generated/autoconf.h"
        OUTPUT .config
        COMMAND touch "${CMAKE_CURRENT_BINARY_DIR}/stage/placeholder"
        COMMAND cp -a "$<TARGET_PROPERTY:Configuration,INTERFACE_INCLUDE_DIRECTORIES>/autoconf.h" "${CMAKE_CURRENT_BINARY_DIR}/include/generated/autoconf.h"
        COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/kbuild/tools"
        COMMAND cp -a "${COMMON_PATH}/../kbuild-tool" "${CMAKE_CURRENT_BINARY_DIR}/kbuild/tools/kbuild"
        # What we want to do is "echo ${LEGACY_CONFIG_ENV} | sed 's/ /\n/g'" but there is a bug in the
        # ninja generator where the \n will create a newline in the output file in the middle of a string
        # thus resulting in a syntax error. Therefore we do a stupid work around to avoid having to write \n
        # in the command string
        COMMAND echo ${LEGACY_CONFIG_ENV} | sh -c "read line; for file in $line; do echo $file; done" > .config
        # NK_CFLAGS and NK_FLAGS (NK_ASFLAGS?) should get set as well
        # also TOOLPREFIX
        COMMAND
            # Setup environment variables
            export NK_CFLAGS=${nk_cflags}
                NK_LDFLAGS=${CMAKE_EXE_LINKER_FLAGS}
                CPPFLAGS=${nk_cflags}
                SEL4_COMMON=${COMMON_PATH}
                ARCH=${KernelArch}
                PLAT=${KernelPlatform}
                TYPE_SUFFIX=${KernelWordSize}
                ${LEGACY_CONFIG_ENV}
                ${extra_environment}
                "TOOLPREFIX=${ccache} ${CROSS_COMPILE_PREFIX}"
                SEL4_ARCH=${KernelSel4Arch} &&
            CFLAGS= LDFLAGS=
            # Invoke make in the binary directory on the Makefile
            make -C "${CMAKE_CURRENT_BINARY_DIR}" -f "${CMAKE_CURRENT_SOURCE_DIR}/Makefile"
                -rR "--include-dir=${CMAKE_CURRENT_BINARY_DIR}/kbuild"
            # Arguments we pass to Make
            "BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            "SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            # Initialize stage as a private stage directory so that we can construct
            # sensible include directory on the final target
            "STAGE_DIR=${CMAKE_CURRENT_BINARY_DIR}/stage"
            # srctree is normally the root directory, but we will put it as the binary directory
            # instead to isolate builds
            "srctree=${CMAKE_CURRENT_BINARY_DIR}"
        COMMAND cp -a "${CMAKE_CURRENT_BINARY_DIR}/stage/lib" "${LEGACY_STAGE_DIR}"
        COMMAND cp -a "${CMAKE_CURRENT_BINARY_DIR}/stage/include" "${LEGACY_STAGE_DIR}"
        DEPENDS ${deps} ${other_deps} Configuration
        VERBATIM
        COMMENT "Invoking legacy compilation for ${target_name}"
    )

    add_custom_target(${target_name}_gen
        DEPENDS "${LEGACY_STAGE_DIR}/lib/${lib_name}" ${other_deps}
    )

    # Pretend that the library we just generated is an 'imported' library
    # This allows us to talk about our customly generated library like it's
    # a first class library target in CMake (well almost)
    add_library(${target_name}_imported STATIC IMPORTED)
    add_dependencies(${target_name}_imported ${target_name}_gen ${other_deps} Configuration)
    set_property(TARGET ${target_name}_imported PROPERTY IMPORTED_LOCATION "${LEGACY_STAGE_DIR}/lib/${lib_name}")

    # Define an interface library for our target. This interface library is needed as we cannot
    # describe include directories on imported libraries. The solution is to have this interface
    # library that describes the include directories, and then describe an 'INTERFACE_LINK_LIBRARY'
    # which is a library that must be linked in when this interface library is used.
    add_library(${target_name} INTERFACE)
    add_dependencies(${target_name} ${target_name}_gen ${other_deps} ${target_name}_imported Configuration)
    set_property(TARGET ${target_name} PROPERTY INTERFACE_LINK_LIBRARIES "${target_name}_imported")
    target_include_directories(${target_name} INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/stage/include")
endfunction(AddLegacyLibrary)
