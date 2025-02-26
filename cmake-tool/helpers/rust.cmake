#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)
include_guard(GLOBAL)

find_path(
    RUST_TARGET_PATH "x86_64-sel4-none.json"
    PATHS ${CMAKE_CURRENT_LIST_DIR}/../rust_targets
    CMAKE_FIND_ROOT_PATH_BOTH
)

# Return absoulute filepath of the rust custom target directory
function(RustGetCustomTargetPath return)
    set(${return} "${RUST_TARGET_PATH}" PARENT_SCOPE)
endfunction()

# add_library but for rust libraries. Invokes cargo in the SOURCE_DIR that is provided,
# all build output is placed in BUILD_DIR or CMAKE_CURRENT_BINARY_DIR if BUILD_DIR isn't provided.
# lib_name: Name of library that is created
# SOURCE_DIR: source directory of cargo project
# BUILD_DIR: directory for cargo build output
# TARGET: custom target to use. See in ../rust_targets/ for list of available targets.
# LIB_FILENAME: filename of library created by cargo
# DEPENDS: And target or file dependencies that need to be run before cargo
function(RustAddLibrary lib_name)
    cmake_parse_arguments(PARSE_ARGV 1 RUST "" "SOURCE_DIR;BUILD_DIR;TARGET;LIB_FILENAME" "DEPENDS")
    if(NOT "${RUST_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(FATAL_ERROR "Unknown arguments to RustAddLibrary ${RUST_UNPARSED_ARGUMENTS}")
    endif()
    if("${RUST_SOURCE_DIR}" STREQUAL "")
        message(FATAL_ERROR "SOURCE_DIR must be set for RustAddLibrary")
    endif()
    if("${RUST_LIB_FILENAME}" STREQUAL "")
        message(FATAL_ERROR "LIB_FILENAME must be set for RustAddLibrary")
    endif()
    if("${RUST_BUILD_DIR}" STREQUAL "")
        set(RUST_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    RustGetCustomTargetPath(target_path)
    if("${RUST_TARGET}" STREQUAL "")
        message(
            FATAL_ERROR
                "TARGET must be set for RustAddLibrary. Valid options can be found in: ${target_path}"
        )
    endif()
    string(FIND "${RUST_TARGET}" ".json" pos)
    if(NOT ${pos} EQUAL -1)
        message(FATAL_ERROR "${RUST_TARGET} cannot contain .json")
    endif()

    find_program(XARGO_TOOL xargo)
    if("${XARGO_TOOL}" STREQUAL "XARGO_TOOL-NOTFOUND")
        message(FATAL_ERROR "Could not find tool xargo. Install with `cargo install xargo` \
    or see https://github.com/japaric/xargo")
    endif()

    add_custom_target(
        ${libmain}_custom
        BYPRODUCTS
        ${RUST_BUILD_DIR}/${RUST_LIB_FILENAME}
        ${USES_TERMINAL_DEBUG}
        DEPENDS ${RUST_DEPENDS}
        WORKING_DIRECTORY ${RUST_SOURCE_DIR}
        COMMAND
            ${CMAKE_COMMAND} -E env RUST_TARGET_PATH=${target_path} ${XARGO_TOOL} build
            --target ${RUST_TARGET}
            --target-dir ${RUST_BUILD_DIR} -Z unstable-options
            --out-dir ${RUST_BUILD_DIR}
    )

    add_library(${lib_name} STATIC IMPORTED GLOBAL)
    set_property(
        TARGET ${lib_name}
        PROPERTY IMPORTED_LOCATION "${RUST_BUILD_DIR}/${RUST_LIB_FILENAME}"
    )
    add_dependencies(${lib_name} ${libmain}_custom)
endfunction()
