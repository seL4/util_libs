#
# Copyright 2022, UNSW (ABN 57 195 873 179)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libvirtio NONE)

add_library(virtio INTERFACE)
target_include_directories(virtio INTERFACE include)
