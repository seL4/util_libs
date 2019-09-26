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

cmake_minimum_required(VERSION 3.7.2)
project(libpicotcp NONE)

include(external-project-helpers)

set(configure_string "")

config_option(LibPicotcp LIB_PICOTCP "Build libpicotcp" DEFAULT OFF)
config_option(
    LibPicotcpBsd
    LIB_PICOTCP_BSD
    "Build libpicotcp with bsd socket support"
    DEFAULT
    OFF
    DEPENDS
    LibPicotcp
)
mark_as_advanced(LibPicotcp LibPicotcpBsd)
add_config_library(picotcp "${configure_string}")

if(LibPicotcp)
    # find pico tcp location
    find_file(PICOTCP_PATH picotcp PATHS ${CMAKE_SOURCE_DIR}/projects CMAKE_FIND_ROOT_PATH_BOTH)
    mark_as_advanced(FORCE PICOTCP_PATH)
    if("${PICOTCP_PATH}" STREQUAL "PICOTCP_PATH-NOTFOUND")
        message(
            FATAL_ERROR "Failed to find picotcp. Consider cmake -DPICOTCP_PATH=/path/to/picotcp"
        )
    endif()

    # extract compiler args from cmake
    get_property(compile_options DIRECTORY PROPERTY COMPILE_OPTIONS)
    separate_arguments(cmake_c_flags_sep NATIVE_COMMAND "${CMAKE_C_FLAGS}")
    list(APPEND compile_options "${cmake_c_flags_sep}")

    # add the location of muslc headers to the cflags
    string(
        APPEND compile_options
        " -I$<JOIN:$<TARGET_PROPERTY:muslc,INTERFACE_INCLUDE_DIRECTORIES>, -I>"
    )

    # generate a build.sh script to avoid nasty escape issues, we then invoke this in ExternalProject_Add
    file(
        GENERATE
        OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/picotcp_external/picotcp/build.sh"
        CONTENT
        "${CMAKE_COMMAND} -E env \
        \"PLATFORM_CFLAGS=$<JOIN:${compile_options}, >\" \
        \"CROSS_COMPILE=${CROSS_COMPILER_PREFIX}\" \
        make -s -j"
    )

    add_custom_target(get_muslc)
    add_dependencies(get_muslc muslc)

    # build picotcp using its very own build system
    include(ExternalProject)
    ExternalProject_Add(
        picotcp_external
        SOURCE_DIR
        "picotcp_external"
        DOWNLOAD_COMMAND
        rsync
        -qur
        --exclude='.git'
        ${PICOTCP_PATH}
        <SOURCE_DIR>
        UPDATE_COMMAND
        ""
        CONFIGURE_COMMAND
        ""
        BUILD_COMMAND
        cd
        <SOURCE_DIR>/picotcp/
        &&
        sh
        build.sh
        BUILD_IN_SOURCE
        1
        INSTALL_COMMAND
        ""
        EXCLUDE_FROM_ALL
        BUILD_BYPRODUCTS
        "<SOURCE_DIR>/picotcp/build/lib/libpicotcp.a"
        DEPENDS
        get_muslc
    )

    file(
        GLOB
            deps
            ${PICOTCP_PATH}/modules/*.c
            ${PICOTCP_PATH}/modules/*.h
            ${PICOTCP_PATH}/stack/*.c
            ${PICOTCP_PATH}/include/**/*.h
    )

    # Add file dependencies to picotcp
    ExternalProject_Add_StepDependencies(
        picotcp_external
        download
        DEPENDS
        ${PICOTCP_PATH}/Makefile
        DEPENDS
        ${deps}
    )
    # get the dir that the picotcp sources have been copied to by ExternalProject_Add
    ExternalProject_Get_Property(picotcp_external SOURCE_DIR)
    set(BUILD_DIR "${SOURCE_DIR}/picotcp/build/")

    DeclareExternalProjObjectFiles(picotcp_external "${BUILD_DIR}/lib/" FILES "libpicotcp.a")

    # create a library which is the sources
    add_library(picotcp_sources STATIC IMPORTED GLOBAL)
    add_dependencies(picotcp_sources picotcp_external)
    set_property(TARGET picotcp_sources PROPERTY IMPORTED_LOCATION "${BUILD_DIR}/lib/libpicotcp.a")

    # create a library which is the header files, that depends on the sources
    add_library(picotcp INTERFACE)
    add_dependencies(picotcp picotcp_sources)
    set_property(TARGET picotcp PROPERTY INTERFACE_LINK_LIBRARIES picotcp_sources)
    target_include_directories(
        picotcp
        INTERFACE
            "${BUILD_DIR}/include" "$<TARGET_PROPERTY:picotcp_Config,INTERFACE_INCLUDE_DIRECTORIES>"
            "$<TARGET_PROPERTY:sel4_autoconf,INTERFACE_INCLUDE_DIRECTORIES>"
    )
endif()
if(LibPicotcpBsd)
    # find pico tcp location
    find_file(
        PICOTCP_BSD_PATH picotcp-bsd
        PATHS ${CMAKE_SOURCE_DIR}/projects
        CMAKE_FIND_ROOT_PATH_BOTH
    )
    mark_as_advanced(FORCE PICOTCP_BSD_PATH)
    if("${PICOTCP_BSD_PATH}" STREQUAL "PICOTCP_BSD_PATH-NOTFOUND")
        message(
            FATAL_ERROR
                "Failed to find picotcp-bsd. Consider cmake -DPICOTCP_BSD_PATH=/path/to/picotcp-bsd"
        )
    endif()

    add_definitions(-DSTDSOCKET)
    add_definitions(-U__linux__)
    add_definitions(-D_SYS_POLL_H)

    # create picotcp_bsd library
    add_library(
        picotcp_bsd
        STATIC
        EXCLUDE_FROM_ALL
        ${PICOTCP_BSD_PATH}/pico_bsd_sockets.c
        ${PICOTCP_BSD_PATH}/pico_osal_noos.c
    )
    target_include_directories(picotcp_bsd PUBLIC ${PICOTCP_BSD_PATH})
    target_link_libraries(picotcp_bsd muslc picotcp picotcp_Config sel4_autoconf)
endif()
