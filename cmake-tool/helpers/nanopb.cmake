#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

include_guard(GLOBAL)

# set up some paths
set(GENERATOR_PATH ${CMAKE_BINARY_DIR}/nanopb/generator)
set(NANOPB_GENERATOR_EXECUTABLE ${GENERATOR_PATH}/nanopb_generator.py)
set(GENERATOR_CORE_DIR ${GENERATOR_PATH}/proto)
set(GENERATOR_CORE_SRC ${GENERATOR_CORE_DIR}/nanopb.proto)

# figure out where nanopb is
if(NOT DEFINED NANOPB_SRC_ROOT_FOLDER)
    find_file(NANOPB_SRC_ROOT_FOLDER nanopb PATHS ${CMAKE_SOURCE_DIR} NO_CMAKE_FIND_ROOT_PATH)
endif()
mark_as_advanced(FORCE NANOPB_SRC_ROOT_FOLDER)
if("${NANOPB_SRC_ROOT_FOLDER}" STREQUAL "NANOPB_SRC_ROOT_FOLDER-NOTFOUND")
    message(
        FATAL_ERROR "Failed to find nanopb. Consider cmake -DNANOPB_SRC_ROOT_FOLDER=/path/to/nanopb"
    )
endif()

# include the nanopb cmake stuff - portions of it have been forked to this file,
# but much of it is still useful.
list(APPEND CMAKE_MODULE_PATH "${NANOPB_SRC_ROOT_FOLDER}/extra/")
find_package(Nanopb REQUIRED)

# generate nanopb runtime library
file(GLOB nanopb_src ${NANOPB_SRC_ROOT_FOLDER}/*.h ${NANOPB_SRC_ROOT_FOLDER}/*.c)

add_library(nanopb STATIC EXCLUDE_FROM_ALL ${nanopb_src})
target_include_directories(nanopb PUBLIC ${NANOPB_SRC_ROOT_FOLDER})
target_link_libraries(nanopb muslc)

# Treat the source diretory as immutable.
#
# Copy the generator directory to the build directory before
# compiling python and proto files.
add_custom_command(
    PRE_BUILD
    TARGET nanopb
    COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        ARGS
            ${NANOPB_GENERATOR_SOURCE_DIR}
            ${GENERATOR_PATH}
            BYPRODUCTS
            ${NANOPB_GENERATOR_EXECUTABLE}
            ${GENERATOR_CORE_SRC}
    VERBATIM
)

# This is a fork of nanopb's NANOPB_GENERATE_CPP function
# for seL4
function(SEL4_GENERATE_PROTOBUF SRCS HDRS)
    cmake_parse_arguments(NANOPB_GENERATE_CPP "" "RELPATH" "" ${ARGN})
    if(NOT NANOPB_GENERATE_CPP_UNPARSED_ARGUMENTS)
        return()
    endif()

    set(GENERATOR_PATH ${CMAKE_BINARY_DIR}/nanopb/generator)
    set(NANOPB_GENERATOR_EXECUTABLE ${GENERATOR_PATH}/nanopb_generator.py)

    set(GENERATOR_CORE_DIR ${GENERATOR_PATH}/proto)
    set(GENERATOR_CORE_SRC ${GENERATOR_CORE_DIR}/nanopb.proto)

    set(_nanopb_include_path "-I${NANOPB_SRC_ROOT_FOLDER}/src")
    if(DEFINED NANOPB_IMPORT_DIRS)
        foreach(DIR ${NANOPB_IMPORT_DIRS})
            get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
            list(APPEND _nanopb_include_path "-I${ABS_PATH}")
        endforeach()
    endif()
    list(REMOVE_DUPLICATES _nanopb_include_path)

    if(NANOPB_GENERATE_CPP_APPEND_PATH)
        # Create an include path for each file specified
        foreach(FIL ${NANOPB_GENERATE_CPP_UNPARSED_ARGUMENTS})
            get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
            get_filename_component(ABS_PATH ${ABS_FIL} PATH)
            list(APPEND _nanopb_include_path "-I${ABS_PATH}")
        endforeach()
    else()
        set(_nanopb_include_path "-I${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    if(NANOPB_GENERATE_CPP_RELPATH)
        list(APPEND _nanopb_include_path "-I${NANOPB_GENERATE_CPP_RELPATH}")
    endif()

    if(DEFINED NANOPB_IMPORT_DIRS)
        foreach(DIR ${NANOPB_IMPORT_DIRS})
            get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
            list(APPEND _nanopb_include_path "-I${ABS_PATH}")
        endforeach()
    endif()

    list(REMOVE_DUPLICATES _nanopb_include_path)

    set(NANOPB_GENERATOR_PLUGIN ${GENERATOR_PATH}/protoc-gen-nanopb)

    if(NANOPB_GENERATE_CPP_RELPATH)
        get_filename_component(ABS_ROOT ${NANOPB_GENERATE_CPP_RELPATH} ABSOLUTE)
    endif()
    foreach(FIL ${NANOPB_GENERATE_CPP_UNPARSED_ARGUMENTS})
        get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
        get_filename_component(FIL_WE ${FIL} NAME_WE)
        get_filename_component(FIL_DIR ${FIL} PATH)
        set(FIL_PATH_REL)
        if(ABS_ROOT)
            # Check that the file is under the given "RELPATH"
            string(FIND ${ABS_FIL} ${ABS_ROOT} LOC)
            if(${LOC} EQUAL 0)
                string(
                    REPLACE
                        "${ABS_ROOT}/"
                        ""
                        FIL_REL
                        ${ABS_FIL}
                )
                get_filename_component(FIL_PATH_REL ${FIL_REL} PATH)
                file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${FIL_PATH_REL})
            endif()
        endif()
        if(NOT FIL_PATH_REL)
            set(FIL_PATH_REL ".")
        endif()

        list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_PATH_REL}/${FIL_WE}.pb.c")
        list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_PATH_REL}/${FIL_WE}.pb.h")

        set(NANOPB_PLUGIN_OPTIONS)
        set(NANOPB_OPTIONS_DIRS)

        # If there an options file in the same working directory, set it as a dependency
        set(NANOPB_OPTIONS_FILE ${FIL_DIR}/${FIL_WE}.options)
        if(EXISTS ${NANOPB_OPTIONS_FILE})
            # Get directory as lookups for dependency options fail if an options
            # file is used. The options is still set as a dependency of the
            # generated source and header.
            get_filename_component(options_dir ${NANOPB_OPTIONS_FILE} DIRECTORY)
            list(APPEND NANOPB_OPTIONS_DIRS ${options_dir})
        else()
            set(NANOPB_OPTIONS_FILE)
        endif()

        # If the dependencies are options files, we need to pass the directories
        # as arguments to nanopb
        foreach(depends_file ${NANOPB_DEPENDS})
            get_filename_component(ext ${depends_file} EXT)
            if(ext STREQUAL ".options")
                get_filename_component(depends_dir ${depends_file} DIRECTORY)
                list(APPEND NANOPB_OPTIONS_DIRS ${depends_dir})
            endif()
        endforeach()

        if(NANOPB_OPTIONS_DIRS)
            list(REMOVE_DUPLICATES NANOPB_OPTIONS_DIRS)
        endif()

        foreach(options_path ${NANOPB_OPTIONS_DIRS})
            set(NANOPB_PLUGIN_OPTIONS "${NANOPB_PLUGIN_OPTIONS} -I${options_path}")
        endforeach()

        if(NANOPB_OPTIONS)
            set(NANOPB_PLUGIN_OPTIONS "${NANOPB_PLUGIN_OPTIONS} ${NANOPB_OPTIONS}")
        endif()

        add_custom_command(
            OUTPUT
                "${CMAKE_CURRENT_BINARY_DIR}/${FIL_PATH_REL}/${FIL_WE}.pb.c"
                "${CMAKE_CURRENT_BINARY_DIR}/${FIL_PATH_REL}/${FIL_WE}.pb.h"
            COMMAND
                ${PROTOBUF_PROTOC_EXECUTABLE}
                ARGS
                    -I${GENERATOR_PATH}
                    -I${GENERATOR_CORE_DIR}
                    -I${CMAKE_CURRENT_BINARY_DIR}
                    ${_nanopb_include_path}
                --plugin=protoc-gen-nanopb=${NANOPB_GENERATOR_PLUGIN}
                    "--nanopb_out=${NANOPB_PLUGIN_OPTIONS}:${CMAKE_CURRENT_BINARY_DIR}" ${ABS_FIL}
            DEPENDS
                ${ABS_FIL}
                ${GENERATOR_CORE_SRC}
                ${GENERATOR_CORE_PYTHON_SRC}
                ${NANOPB_OPTIONS_FILE}
                ${NANOPB_DEPENDS}
            COMMENT "Running C++ protocol buffer compiler using nanopb plugin on ${FIL}"
            VERBATIM
        )

    endforeach()

    set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
    set(${SRCS} ${${SRCS}} ${NANOPB_SRCS} PARENT_SCOPE)
    set(${HDRS} ${${HDRS}} ${NANOPB_HDRS} PARENT_SCOPE)
endfunction()
