#
# Copyright 2019, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

# This module provides a function GenerateSimulateScript which will place a `simulate`
# script in the build directory for running produced images in Qemu.
include_guard(GLOBAL)

find_file(
    SIMULATE_SCRIPT simulate.py
    PATHS "${CMAKE_CURRENT_LIST_DIR}/../simulate_scripts/"
    CMAKE_FIND_ROOT_PATH_BOTH
)
find_file(
    CONFIGURE_FILE_SCRIPT configure_file.cmake
    PATHS "${CMAKE_CURRENT_LIST_DIR}"
    CMAKE_FIND_ROOT_PATH_BOTH
)

# Help macro for testing a config and appending to a list that is destined for a qemu -cpu line
macro(TestQemuCPUFeature config feature string)
    if(${config})
        set(${string} "${${string}},+${feature}")
    else()
        set(${string} "${${string}},-${feature}")
    endif()
endmacro(TestQemuCPUFeature)

# Function for the user for configuration the simulation script. Valid values for property are
#  'GRAPHIC' if set to TRUE disables the -nographic flag
#  'MEM_SIZE' if set will override the memory size given to qemu
function(SetSimulationScriptProperty property value)
    # define the target if it doesn't already exist
    if(NOT (TARGET simulation_script_prop_target))
        add_custom_target(simulation_script_prop_target)
    endif()
    set_property(TARGET simulation_script_prop_target PROPERTY "${property}" "${value}")
endfunction(SetSimulationScriptProperty)

macro(SetDefaultMemSize default)
    set(
        QemuMemSize
        "$<IF:$<BOOL:$<TARGET_PROPERTY:simulation_script_prop_target,MEM_SIZE>>,$<TARGET_PROPERTY:simulation_script_prop_target,MEM_SIZE>,${default}>"
    )
endmacro(SetDefaultMemSize)

# Helper function that generates targets that will attempt to generate a ./simulate style script
function(GenerateSimulateScript)
    set(error "")
    set(KERNEL_IMAGE_NAME "$<TARGET_PROPERTY:rootserver_image,KERNEL_IMAGE_NAME>")
    set(IMAGE_NAME "$<TARGET_PROPERTY:rootserver_image,IMAGE_NAME>")
    # Define simulation script target if it doesn't exist to simplify the generator expressions
    if(NOT (TARGET simulation_script_prop_target))
        add_custom_target(simulation_script_prop_target)
    endif()
    set(
        sim_graphic_opt
        "$<IF:$<BOOL:$<TARGET_PROPERTY:simulation_script_prop_target,GRAPHIC>>,,-nographic>"
    )
    set(sim_serial_opt "")
    set(sim_cpu "")
    set(sim_cpu_opt "")
    set(sim_machine "")
    if(KernelArchX86)
        # Try and simulate the correct micro architecture and features
        if(KernelX86MicroArchNehalem)
            set(sim_cpu "Nehalem")
        elseif(KernelX86MicroArchGeneric)
            set(sim_cpu "qemu64")
        elseif(KernelX86MicroArchWestmere)
            set(sim_cpu "Westmere")
        elseif(KernelX86MicroArchSandy)
            set(sim_cpu "SandyBridge")
        elseif(KernelX86MicroArchIvy)
            set(sim_cpu "IvyBridge")
        elseif(KernelX86MicroArchHaswell)
            set(sim_cpu "Haswell")
        elseif(KernelX86MicroArchBroadwell)
            set(sim_cpu "Broadwell")
        else()
            set(error "Unknown x86 micro-architecture for simulation")
        endif()
        TestQemuCPUFeature(KernelVTX vme sim_cpu_opt)
        TestQemuCPUFeature(KernelHugePage pdpe1gb sim_cpu_opt)
        TestQemuCPUFeature(KernelFPUXSave xsave sim_cpu_opt)
        TestQemuCPUFeature(KernelXSaveXSaveOpt xsaveopt sim_cpu_opt)
        TestQemuCPUFeature(KernelXSaveXSaveC xsavec sim_cpu_opt)
        TestQemuCPUFeature(KernelFSGSBaseInst fsgsbase sim_cpu_opt)
        TestQemuCPUFeature(KernelSupportPCID invpcid sim_cpu_opt)
        set(sim_cpu "${sim_cpu}")
        set(sim_cpu_opt "${sim_cpu_opt},enforce")
        set(QemuBinaryMachine "qemu-system-x86_64")
        set(sim_serial_opt "-serial mon:stdio")
        SetDefaultMemSize("512M")
    elseif(KernelPlatformKZM)
        set(QemuBinaryMachine "qemu-system-arm")
        set(sim_machine "kzm")
        SetDefaultMemSize("128M")
    elseif(KernelPlatformSabre)
        set(QemuBinaryMachine "qemu-system-arm")
        set(sim_serial_opt "-serial null -serial mon:stdio")
        set(sim_machine "sabrelite")
        SetDefaultMemSize("1024M")
    elseif(KernelPlatformZynq7000)
        set(QemuBinaryMachine "qemu-system-arm")
        set(sim_serial_opt "-serial null -serial mon:stdio")
        set(sim_machine "xilinx-zynq-a9")
        SetDefaultMemSize("1024M")
    elseif(KernelPlatformWandQ)
        set(QemuBinaryMachine "qemu-system-arm")
        set(sim_serial_opt "-serial mon:stdio")
        set(sim_machine "sabrelite")
        SetDefaultMemSize("2048M")
    elseif(KernelPlatformRpi3 AND KernelSel4ArchAarch64)
        set(QemuBinaryMachine "qemu-system-aarch64")
        set(sim_serial_opt "-serial null -serial mon:stdio")
        set(sim_machine "raspi3")
        SetDefaultMemSize("1024M")
    elseif(KernelPlatformSpike)
        if(KernelSel4ArchRiscV32)
            set(binary "qemu-system-riscv32")
            SetDefaultMemSize("2000M")
            set(sim_machine "virt")
        elseif(KernelSel4ArchRiscV64)
            set(binary "qemu-system-riscv64")
            SetDefaultMemSize("4095M")
            set(sim_machine "spike_v1.10")
        endif()
        set(QemuBinaryMachine "${binary}")
        set(sim_serial_opt "-serial mon:stdio")
    else()
        set(error "Unsupported platform or architecture for simulation")
    endif()
    set(sim_path "${CMAKE_BINARY_DIR}/simulate")
    if(NOT "${error}" STREQUAL "")
        set(script "#!/bin/sh\\necho ${error} && exit 1\\n")
        add_custom_command(
            OUTPUT "${sim_path}"
            COMMAND
                printf "${script}" > "${sim_path}"
            COMMAND chmod u+x "${sim_path}"
            VERBATIM
        )
    else()
        add_custom_command(
            OUTPUT "${sim_path}"
            COMMAND
                ${CMAKE_COMMAND} -DCONFIGURE_INPUT_FILE=${SIMULATE_SCRIPT}
                -DCONFIGURE_OUTPUT_FILE=${sim_path} -DQEMU_SIM_BINARY=${QemuBinaryMachine}
                -DQEMU_SIM_CPU=${sim_cpu} -DQEMU_SIM_MACHINE=${sim_machine}
                -DQEMU_SIM_CPU_OPT=${sim_cpu_opt} -DQEMU_SIM_GRAPHIC_OPT=${sim_graphic_opt}
                -DQEMU_SIM_SERIAL_OPT=${sim_serial_opt} -DQEMU_SIM_MEM_SIZE_OPT=${QemuMemSize}
                -DQEMU_SIM_KERNEL_FILE=${KERNEL_IMAGE_NAME} -DQEMU_SIM_INITRD_FILE=${IMAGE_NAME} -P
                ${CONFIGURE_FILE_SCRIPT}
            COMMAND chmod u+x "${sim_path}"
            VERBATIM COMMAND_EXPAND_LISTS
        )
    endif()
    add_custom_target(simulate_gen ALL DEPENDS "${sim_path}")
endfunction(GenerateSimulateScript)
