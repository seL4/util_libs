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

cmake_minimum_required(VERSION 3.8.2)
include_guard(GLOBAL)

find_program(HOLMAKE_BIN NAMES "Holmake")
find_program(CAKEML_BIN NAMES "cake")

find_file(
    BUILD_SCRIPT_IN buildScript.sml.in
    PATHS ${CMAKE_CURRENT_LIST_DIR}
    CMAKE_FIND_ROOT_PATH_BOTH
)
find_file(HOLMAKEFILE_IN Holmakefile.in PATHS ${CMAKE_CURRENT_LIST_DIR} CMAKE_FIND_ROOT_PATH_BOTH)

# Turns a selection of CakeML HOL scripts (Script.sml files) into a library that can
# be linked against. Note that the library *only* contains the output of the CakeML
# compiler, and it may have hanging symbols to ffi functions that need to be fullfilled
# The position argument library_name is the name of the library that will be created and
# is directly passed to addlibrary in the end
# Other arguments are
#  SOURCES - One or more Script.sml files to use when performing compilation. These will
#   all get copied into a flat directory structure for the actual build
#  TRANSLATION_THEORY - This is the name of the theory that does the final `append_prog`
#   command. As a result you almost certainly pass a TRANSLATION_THEORYScript.sml file
#   as one of the SOURCES
#  CAKEML_ENTRY - Defines the 'entry' function in your CakeML program that should get called
#   by the CakeML runtime on startup
#  RUNTIME_ENTRY - Symbol exported from the library that you can call to enter the CakeML
#   runtime. Note that CakeML does not return and so this function is divergent
#  STACK_SIZE - Size of the stack for the CakeML runtime in MB - default is 1000
#  HEAP_SIZE - Size of the heap for the CakeML runtime in MB - defaultis 1000
#  DEPENDS - List of any additional targets to depend upon
#  INCLUDES - List of any additional INCLUDES to add to the INCLUDES list in the Holmakefile
function(DeclareCakeMLLib library_name)
    cmake_parse_arguments(
        PARSE_ARGV
        1
        PARSE_CML_LIB
        ""
        "TRANSLATION_THEORY;RUNTIME_ENTRY;CAKEML_ENTRY;STACK_SIZE;HEAP_SIZE"
        "SOURCES;DEPENDS;INCLUDES"
    )
    if(NOT "${PARSE_CML_LIB_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(
            FATAL_ERROR "Unknown arguments to DeclareCakeMLLib ${PARSE_CML_LIB_UNPARSED_ARGUMENTS}"
        )
    endif()
    # require TRANSLATION_THEORY
    if("${PARSE_CML_LIB_TRANSLATION_THEORY}" STREQUAL "")
        message(FATAL_ERROR "Must provide TRANSLATION_THEORY to DeclareCakeMLLib")
    endif()
    # require CAKEML_ENTRY
    if("${PARSE_CML_LIB_CAKEML_ENTRY}" STREQUAL "")
        message(FATAL_ERROR "Must provide CAKEML_ENTRY to DeclareCakeMLLib")
    endif()
    # default RUNTIME_ENTRY to main
    if("${PARSE_CML_LIB_RUNTIME_ENTRY}" STREQUAL "")
        set(PARSE_CML_LIB_RUNTIME_ENTRY "main")
    endif()
    # default stack and heap size
    if("${PARSE_CML_LIB_STACK_SIZE}" STREQUAL "")
        set(PARSE_CML_LIB_STACK_SIZE "1000")
    endif()
    if("${PARSE_CML_LIB_HEAP_SIZE}" STREQUAL "")
        set(PARSE_CML_LIB_HEAP_SIZE "1000")
    endif()
    # Work out what --target we need to pass to cake
    if(KernelSel4ArchX86_64)
        set(CAKE_TARGET "x64")
    elseif(KernelArchArmV6 OR KernelArchArmV7a OR KernelArchArmV7ve)
        set(CAKE_TARGET "arm6")
    elseif(KernelArchArmV8a)
        set(CAKE_TARGET "arm8")
    else()
        # We don't generate an error right here incase configuration is still going on
        # Setting the target to "unknown" will happily cause 'cake' to fail if it gets to that point
        set(CAKE_TARGET "unknown")
    endif()
    # Check we have all the right tools setup
    if(${HOLMAKE_BIN} STREQUAL "HOLMAKE_BIN-NOTFOUND")
        message(FATAL_ERROR "Holmake not found. Expected to be on the system PATH")
    endif()
    set(CAKEMLDIR "$ENV{CAKEMLDIR}" CACHE STRING "Path to the CakeML compiler")
    if(("${CAKEMLDIR}" STREQUAL "") AND NOT ("$ENV{CAKEMLDIR}" STREQUAL ""))
        message(
            FATAL_ERROR
                "CAKEMLDIR is set in the environment, but our CAKEMLDIR is not. Please run ccmake and update CAKEMLDIR"
        )
    endif()
    if(NOT EXISTS "${CAKEMLDIR}")
        message(FATAL_ERROR "CAKEMLDIR \"${CAKEMLDIR}\" is not a valid directory")
    endif()
    if(${CAKEML_BIN} STREQUAL "CAKEML_BIN-NOTFOUND")
        message(FATAL_ERROR "Failed to find cake binary. Consider cmake -DCAKEML_BIN=/path/to/cake")
    endif()
    # Generate a directory at the top level of the binary directory for placing our CakeML related
    # files. Placing all the CakeML related sources together simplifies development of of CakeML
    # projects
    set(CML_DIR "${CMAKE_BINARY_DIR}/cakeml_${library_name}")
    set(SEXP_FILE "${CML_DIR}/cake.sexp")
    set(ASM_FILE "${CML_DIR}/cake.S")
    set(BUILD_SCRIPT "${CML_DIR}/buildScript.sml")
    set(HOLMAKEFILE "${CML_DIR}/Holmakefile")
    # Ensure this desired directory exists
    file(MAKE_DIRECTORY "${CML_DIR}")
    # Generate rule for symlinking our sources
    set(stampfile "${CMAKE_CURRENT_BINARY_DIR}/${library_name}cakeml_symlink.stamp")
    add_custom_command(
        OUTPUT
            "${stampfile}"
            # First remove any existing symlinks to prevent any confusion if dependencies are changed
            # between builds.
        COMMAND
            bash -c "find '${CML_DIR}' -name '*.sml' -type l | xargs rm -f"
            # Symlink any of our files. We have to escape into some inline shell expressions here since
            # we want to support our sources containing generator expressions, and we do not know the value
            # of these until build time
        COMMAND
            ln -s ${PARSE_CML_LIB_SOURCES} "${CML_DIR}"
        COMMAND
            touch "${stampfile}"
            # We do not depend upon the sources here as there is no need to recreate the symlinks if the
            # sources change. This requires that the later build rules do depend upon the original sources
            COMMAND_EXPAND_LISTS
        VERBATIM
    )
    add_custom_target(
        ${library_name}cakeml_symlink_theory_files
        DEPENDS ${library_name}cakeml_copy.stamp
    )
    # Write out a build script.
    configure_file(${BUILD_SCRIPT_IN} ${BUILD_SCRIPT} @ONLY)

    # Write out a Holmakefile
    string(
        REPLACE
            ";"
            " "
            CAKEML_INCLUDES_SPACE_SEP
            "${PARSE_CML_LIB_INCLUDES}"
    )
    configure_file(${HOLMAKEFILE_IN} ${HOLMAKEFILE} @ONLY)

    # Extra options for Holmake, provide them to init-build.sh with -DHOL_EXTRA_OPTS="--arg1;--arg2"
    set(
        HOL_EXTRA_OPTS "$ENV{HOL_EXTRA_OPTS}"
        CACHE STRING "Extra arguments to provide to Holmake when building CakeML code"
    )
    add_custom_command(
        OUTPUT "${ASM_FILE}" BYPRODUCTS "${SEXP_FILE}"
        COMMAND
            env CAKEML_DIR=${CAKEMLDIR} ${HOLMAKE_BIN}
            --quiet ${HOL_EXTRA_OPTS}
        COMMAND
            sh -c
            "${CAKEML_BIN} --sexp=true --exclude_prelude=true --heap_size=${PARSE_CML_LIB_HEAP_SIZE} --stack_size=${PARSE_CML_LIB_STACK_SIZE} --target=${CAKE_TARGET} < ${SEXP_FILE} > ${ASM_FILE}"
            # the 'cake' program is garbage and does not return an exit code upon failure and instead
            # just outputs nothing over stdout. We therefore test for an empty file and then both delete
            # the file to trigger rebuilds in future and generate an explicit error code
        COMMAND sh -c "if ! [ -s ${ASM_FILE} ]; then rm ${ASM_FILE}; fi"
        COMMAND
            test -s "${ASM_FILE}"
            # 'cake' currently just outputs a global 'main' symbol as its entry point and this is not
            # configurable. We don't expect many other plain strings to be in the assembly file so we
            # do a somewhat risky 'sed' to change the name of the main function
        COMMAND
            sed -i "s/cdecl(main)/cdecl(${PARSE_CML_LIB_RUNTIME_ENTRY})/g" "${ASM_FILE}"
        DEPENDS
            ${PARSE_CML_LIB_SOURCES}
            "${stampfile}"
            "${BUILD_SCRIPT}"
            "${HOLMAKEFILE}"
            ${PARSE_CML_LIB_DEPENDS}
        WORKING_DIRECTORY "${CML_DIR}"
        VERBATIM
    )
    add_custom_target(${library_name}cakeml_asm_theory_target DEPENDS "${ASM_FILE}")
    add_library(${library_name} STATIC EXCLUDE_FROM_ALL "${ASM_FILE}")

    # Holmake doesn't support concurrent builds and so we need to serialise any Cakeml
    # libraries that we build. We do this by adding a dependency on the previously defined
    # library for each library we define.
    get_property(cmake_serialise GLOBAL PROPERTY CAKEML_PREVIOUS_LIBRARY)
    if(NOT "${cmake_serialise}" STREQUAL "")
        add_dependencies(${library_name} ${cmake_serialise})
    endif()
    set_property(GLOBAL PROPERTY CAKEML_PREVIOUS_LIBRARY ${library_name})

    add_dependencies(${library_name} ${library_name}cakeml_asm_theory_target)
endfunction(DeclareCakeMLLib)
