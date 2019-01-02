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

project(libethdrivers C)

set(configure_string "")

config_string(LibEthdriverRXDescCount LIB_ETHDRIVER_RX_DESC_COUNT
    "Number of RX descriptors in the descriptor ring for the driver."
    DEFAULT 128
    UNQUOTE
)

config_string(LibEthdriverTXDescCount LIB_ETHDRIVER_TX_DESC_COUNT
    "Number of TX descriptors in the descriptor ring for the driver."
    DEFAULT 128
    UNQUOTE
)

config_string(LibEthdriverNumPreallocatedBuffers LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS
    "Number of preallocated DMA buffers
    To avoid allocating and freeing buffers continuously the driver
    can preallocate a base amount internally."
    DEFAULT 512
    UNQUOTE
)

config_string(LibEthdriverPreallocatedBufSize LIB_ETHDRIVER_PREALLOCATED_BUF_SIZE
    "Size of preallocated DMA buffers
    The size of each preallocated buffer that will be used for RX and
    TX allocation requests. This needs to be the maximum of the RX buffer
    size and the MTU. Currently the largest RX buffer of any of the
    implemented drivers is 2048, and the MTU is 1500"
    DEFAULT 2048
    UNQUOTE
)

config_option(LibEthdriverPicoTCBAsyncDriver LIB_PICOTCP_ASYNC_DRIVER
    "Async driver for PicoTcp
    Use an async instead of a polling driver for PicoTCP."
    DEFAULT ON
)

add_config_library(ethdrivers "${configure_string}")

file(GLOB sources src/plat/${KernelPlatform}/*.c src/plat/${KernelPlatform}/uboot/*.c src/plat/${KernelPlatform}/cpsw/*.c
    src/*.c
)

add_library(ethdrivers STATIC EXCLUDE_FROM_ALL ${sources})
target_include_directories(ethdrivers PRIVATE src/plat/${KernelPlatform}/cpsw)
target_include_directories(ethdrivers PUBLIC include plat_include/${KernelPlatform} arch_include/${KernelArch})
target_link_libraries(ethdrivers Configuration muslc platsupport)
if(LibLwip)
    target_link_libraries(ethdrivers lwip)
endif()
if(LibPicotcp)
    target_link_libraries(ethdrivers picotcp)
endif()
