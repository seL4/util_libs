#
# Copyright 2017, Data61
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

# Generate the default global configuration
get_property(config_list GLOBAL PROPERTY CONFIG_LIBRARIES)
generate_autoconf(Configuration "${config_list}")
# For legacy purposes we set the HAVE_AUTOCONF definition for any target that uses this
# configuration library
target_compile_definitions(Configuration INTERFACE HAVE_AUTOCONF)
