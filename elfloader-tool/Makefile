#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the GNU General Public License version 2. Note that NO WARRANTY is provided.
# See "LICENSE_GPLv2.txt" for details.
#
# @TAG(NICTA_GPL)
#
#
# Targets
TARGETS := elfloader.o

# Source files required to build the target
CFILES   := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c))
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/*.c))
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/plat/$(PLAT)/*.c))
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/binaries/elf/*.c))
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/armv/$(ARMV)/*.c))
ASMFILES := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/plat/$(PLAT)/*.S))
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/armv/$(ARMV)/*.S))

NK_CFLAGS += -D_XOPEN_SOURCE=700

ifeq ($(ARMV),armv8-a)
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/armv/$(ARMV)/$(SEL4_ARCH)/*.S))
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/armv/$(ARMV)/$(SEL4_ARCH)/*.c))
endif

ifeq ($(__ARM_32__),y)
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/32/*.S))
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/32/*.c))
INCLUDE_DIRS += $(SOURCE_DIR)/include/arch-$(ARCH)/32
INCLUDE_DIRS += $(SOURCE_DIR)/include/arch-$(ARCH)/armv/$(ARMV)/32
endif

ifeq ($(__ARM_64__),y)
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/64/*.S))
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/64/*.c))
INCLUDE_DIRS += $(SOURCE_DIR)/include/arch-$(ARCH)/64
INCLUDE_DIRS += $(SOURCE_DIR)/include/arch-$(ARCH)/armv/$(ARMV)/64
endif

ifeq ($(__EFI__),y)
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(SOURCE_DIR)/src/binaries/efi/efi_head.S)
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/binaries/efi/*.c))
endif

INCLUDE_DIRS += $(SOURCE_DIR)/include/arch-$(ARCH)/
INCLUDE_DIRS += $(SOURCE_DIR)/include/plat/$(PLAT)/
INCLUDE_DIRS += $(SOURCE_DIR)/include/

LIBS = cpio

NK_CFLAGS += -ffreestanding -Wall -Werror -W -Wextra

include $(SEL4_COMMON)/common.mk

#
# We produce a partially linked object file here which, to be used, will be
# eventually relinked with the compiled kernel and user images forming
# a bootable ELF file.
#
elfloader.o: $(OBJFILES)
	@echo " [LINK] $@"
	${Q}$(CC) -r $^ $(LDFLAGS) -o $@
