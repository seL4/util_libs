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

# Targets
TARGETS := libcpio.a

# Source files required to build the target
CFILES := $(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c)))

# Header files/directories this library provides
HDRFILES := $(sort $(wildcard $(SOURCE_DIR)/include/*))

CFLAGS += -Wall -W

ifeq (${ARCH}, riscv)
	# libcpio is linked into both user code as well as the elfloader, in the case of the elfloader
	# we need to compile such that we can run at any virtual address, and not just low ones.
	# This results in the libcpio code being slightly less efficient for non elfloader users.
	CFLAGS += -mcmodel=medany
endif

include $(SEL4_COMMON)/common.mk
