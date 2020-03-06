#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

# Try to set the cache dir based on environment variable, but override
# with value passed in as CMake -D argument.
set(cache_dir "$ENV{SEL4_CACHE_DIR}")
if(NOT ${SEL4_CACHE_DIR} STREQUAL "")
    set(cache_dir "${SEL4_CACHE_DIR}")
endif()
# Convert to an absolute path.
if((NOT ("${cache_dir}" STREQUAL "")) AND NOT IS_ABSOLUTE cache_dir)
    get_filename_component(cache_dir "${cache_dir}" ABSOLUTE BASE_DIR "${CMAKE_BINARY_DIR}")
endif()
set(MEMOIZE_CACHE_DIR "${cache_dir}" CACHE INTERNAL "" FORCE)

# This function wraps a call to add_custom_command and may instead use an alternative cached
# copy if it can already find a version that has been built before.
# If git_directory is provided and the git directory has any uncommitted changes, then the
# cache is bypassed and will always build from source.
# key: A key to distinguish different memoized commands by, and also used in diagnostic output
# replace_dir: Directory to tar and cache. This tar'd directory is what gets expanded in cache hits.
# git_directory: A directory in a Git repository for performing clean/dirty check.
# extra_arguments: A string of extra arguments that if change invalidate previous cache entries.
# replace_files: Subset list of files to tar from replace_dir. If empty string then the whole dir
#   will be cached.
function(memoize_add_custom_command key replace_dir git_directory extra_arguments replace_files)

    message(STATUS "Detecting cached version of: ${key}")

    # If a cache directory isn't set then call the underlying function and return.
    if("${MEMOIZE_CACHE_DIR}" STREQUAL "")
        message(
            STATUS
                "  No cache path given. Set SEL4_CACHE_DIR to a path to enable caching binary artifacts."
        )
        add_custom_command(${ARGN})
        return()
    endif()

    set(dirty OFF)

    # Check if the git directory has any changes.
    # Sets dirty to ON if there are changes
    # sets git_source_hash to the git hash
    if(NOT ${git_directory} STREQUAL "")
        find_package(Git REQUIRED)

        execute_process(
            COMMAND
                "${GIT_EXECUTABLE}" diff --exit-code HEAD --
            WORKING_DIRECTORY "${git_directory}"
            RESULT_VARIABLE res
            OUTPUT_VARIABLE out
            ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(res EQUAL 0)

        else()
            set(dirty ON)
        endif()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
            WORKING_DIRECTORY "${git_directory}"
            RESULT_VARIABLE res
            OUTPUT_VARIABLE out
            ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(git_source_hash ${out})
    endif()

    # Set the directory if no replace_files are set
    if(replace_files STREQUAL "")
        set(replace_files .)
    endif()

    # Set the cache dir based on a hash of the git hash and extra arguments
    # Note: We don't use the entire args to add_custom_command as inputs to the
    # hash as they will change based on the path of the build directory.
    # also it's too hard to guarantee that we can perfectly know whether a cache
    # entry is valid or not, so delegate figuring out to the caller.
    set(hash_string ${extra_arguments} ${git_source_hash})
    string(MD5 hash "${hash_string}")
    set(cache_dir ${MEMOIZE_CACHE_DIR}/${key}/${hash})
    if(dirty)
        # The git directory has changed, so don't use the cache
        message(STATUS "  ${key} is dirty - will build from source")
        add_custom_command(${ARGN})
    else()
        if(EXISTS ${cache_dir}/code.tar.gz)
            # Cache hit. Create a different call to add_custom_command that
            # merely unpacks the result from the last build instance into the
            # target directory.
            message(STATUS "  Found valid cache entry for ${key}")

            # Set a cmake rebuild dependency on the cache file so if it changes
            # cmake will get automatically rerun
            set_property(
                DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                APPEND
                PROPERTY CMAKE_CONFIGURE_DEPENDS "${cache_dir}/code.tar.gz"
            )

            # As we use the same output file we extract it from the args passed in.
            if(NOT "${ARGV5}" STREQUAL "OUTPUT")
                message(FATAL_ERROR "OUTPUT must be first argument to this function")
            endif()
            add_custom_command(
                OUTPUT
                    # This has to match up with the OUTPUT variable of the call to add_custom_command
                    ${ARGV6}
                    # If we have to rebuild, first clear the temporary build directory as
                    # we have no correctly captured the output files or dependencies
                COMMAND rm -r ${replace_dir}
                COMMAND mkdir -p ${replace_dir}
                COMMAND
                    tar -C ${replace_dir} -xf ${cache_dir}/code.tar.gz
                DEPENDS ${deps} COMMAND_EXPAND_LISTS
                COMMENT "Using cache ${key} build"
            )
        else()
            # Don't have a previous build in the cache.  Create the rule but add
            # some commands on the end to cache the result in our cache.
            message(STATUS "  Not found cache entry for ${key} - will build from source")
            add_custom_command(
                ${ARGN}
                COMMAND mkdir -p ${cache_dir}
                COMMAND
                    tar -zcf code.tar.gz -C ${replace_dir} ${replace_files}
                COMMAND mv code.tar.gz ${cache_dir}/
            )
        endif()
    endif()
endfunction()
