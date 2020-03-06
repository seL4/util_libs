#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.8.2)

include_guard(GLOBAL)

# Function for declaring target object files that are produced by an external project. This adds a custom_command
# that forces a stale check on the object file after the External projects install step.
# external_proj_target: Target name of the external project
# external_prog_output_dir: Location of binary or install directory for external project
# FILES: List of object files that exist in the external project
function(DeclareExternalProjObjectFiles external_proj_target external_proj_output_dir)
    # Parse the given files object files
    cmake_parse_arguments(PARSE_ARGV 2 EXT_OBJECT "" "" "FILES")
    if(NOT "${EXT_OBJECT_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(FATAL_ERROR "Unknown arguments to DeclareExternalProjObjectFiles")
    endif()
    # Necessary for FILES to be passed
    if(NOT EXT_OBJECT_FILES)
        message(FATAL_ERROR "NO FILES declared for ${external_proj_target}")
    endif()
    # Get external project binary and stamp dir properties
    ExternalProject_Get_property(${external_proj_target} STAMP_DIR)
    foreach(obj_file IN LISTS EXT_OBJECT_FILES)
        # Generate a unique name based on the object files location
        set(file_path ${external_proj_output_dir}/${obj_file})
        list(APPEND objfiles ${file_path})
    endforeach()
    ExternalProject_Add_StepTargets(${external_proj_target} install)
    add_custom_command(
        OUTPUT ${objfiles}
        COMMAND true
        DEPENDS ${external_proj_target}-install ${STAMP_DIR}/${external_proj_target}-install
    )
endfunction(DeclareExternalProjObjectFiles)
