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
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/plat-$(PLAT)/*.c))
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/utils/*.c))
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/elf/*.c))
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/plat-$(PLAT)/*.S))

NK_CFLAGS += -D_XOPEN_SOURCE=700

ifeq ($(__ARM_32__),y)
ASMFILES := $(patsubst $(SOURCE_DIR)/%,%,$(SOURCE_DIR)/src/arch-$(ARCH)/32/crt0.S)
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/32/*.c))
ifeq ($(ARMV),armv6)
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(SOURCE_DIR)/src/arch-$(ARCH)/32/mmu-v6.S)
endif
ifeq ($(ARMV),armv7-a)
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(SOURCE_DIR)/src/arch-$(ARCH)/32/smc.S)
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(SOURCE_DIR)/src/arch-$(ARCH)/32/mmu-v7a.S)
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(SOURCE_DIR)/src/arch-$(ARCH)/32/mmu-v7a-hyp.S)
endif
endif

ifeq ($(__ARM_64__),y)
ASMFILES := $(patsubst $(SOURCE_DIR)/%,%,$(SOURCE_DIR)/src/arch-$(ARCH)/64/crt0.S)
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/64/*.c))
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(SOURCE_DIR)/src/arch-$(ARCH)/64/traps.S)
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(SOURCE_DIR)/src/arch-$(ARCH)/64/mmu-v8a.S)
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(SOURCE_DIR)/src/arch-$(ARCH)/64/mmu-v8a-hyp.S)
endif

ifeq ($(__EFI__),y)
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(SOURCE_DIR)/src/arch-$(ARCH)/efi/head.S)
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch-$(ARCH)/efi/*.c))
endif

INCLUDE_DIRS += $(SOURCE_DIR)/src/arch-arm/plat-$(PLAT)/

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
