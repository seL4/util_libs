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
TARGETS := libplatsupport.a

CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/mach/$(MACH)/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/plat/$(PLAT)/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/arch/$(ARCH)/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/plat/$(PLAT)/acpi/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/*.c))

INCLUDE_DIRS += $(SOURCE_DIR)/plat_include/$(PLAT)

# Header files/directories this library provides
HDRFILES := $(wildcard ${SOURCE_DIR}/include/*) \
            $(wildcard ${SOURCE_DIR}/plat_include/$(PLAT)/*) \
            $(wildcard ${SOURCE_DIR}/arch_include/$(ARCH)/*)

ifneq ($(MACH),)
HDRFILES += $(wildcard ${SOURCE_DIR}/mach_include/$(MACH)/*)
endif

include $(SEL4_COMMON)/common.mk
