#
# Copyright 2022, UNSW (ABN 57 195 873 179)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

project(libvirtio NONE)

add_library(virtio INTERFACE)
target_include_directories(virtio INTERFACE include)
