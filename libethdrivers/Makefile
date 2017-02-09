#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the GNU General Public License version 2. Note that NO WARRANTY is provided.
# See "LICENSE_GPLv2.txt" for details.
#
# @TAG(NICTA_GPL)
#

# Targets
TARGETS := libethdrivers.a

# Source files required to build the target
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/plat/$(PLAT)/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/plat/$(PLAT)/uboot/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/*.c))

# Header files/directories this library provides
HDRFILES := \
    $(wildcard ${SOURCE_DIR}/include/*) \
    $(wildcard ${SOURCE_DIR}/plat_include/$(PLAT)/*) \
    $(wildcard ${SOURCE_DIR}/arch_include/${ARCH}/*)

include $(SEL4_COMMON)/common.mk
