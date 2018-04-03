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
# Some of the targets in here use bashisms, so force bash as the interpreter.
SHELL=/bin/bash
ifeq ($(wildcard ${SHELL}),)
$(error Prerequisite ${SHELL} not found)
endif

elfloader: export STAGE_DIR=$(STAGE_BASE)
elfloader: export BUILD_DIR=$(BUILD_BASE)/$@
elfloader: export SOURCE_DIR=${TOOLS_ROOT}/elfloader
elfloader: libcpio common FORCE
	@echo "[elfloader] building..."
	$(Q)mkdir -p $(BUILD_DIR)
	$(Q)CFLAGS= LDFLAGS= $(MAKE) $(MAKE_SILENT) \
		--directory=$(BUILD_DIR) --file=$(SOURCE_DIR)/Makefile \
		TOOLPREFIX=$(CONFIG_CROSS_COMPILER_PREFIX:"%"=%)
	$(Q)mkdir -p $(SEL4_COMMON)/elfloader
	$(call cp_if_changed, "$(BUILD_DIR)/elfloader.o", \
		"$(SEL4_COMMON)/elfloader/elfloader.o")
	$(call cp_if_changed, "$(SOURCE_DIR)/src/archive.bin.lds", \
		"$(SEL4_COMMON)/elfloader/archive.bin.lds")
	$(call cp_if_changed, "$(SOURCE_DIR)/src/arch-$(ARCH)/linker.lds", \
		"$(SEL4_COMMON)/elfloader/linker.lds")
	$(call cp_if_changed, "$(SOURCE_DIR)/gen_boot_image.sh", \
		"$(SEL4_COMMON)/elfloader/gen_boot_image.sh")
	@echo "[elfloader] done."

%-image: export TOOLPREFIX=$(CONFIG_CROSS_COMPILER_PREFIX:"%"=%)
%-image: export V
%-image: % kernel_elf common elfloader FORCE
	@echo "[GEN_IMAGE] $@-$(ARCH)-$(PLAT)"
	$(Q)$(SEL4_COMMON)/elfloader/gen_boot_image.sh $(STAGE_BASE)/kernel.elf \
	$(STAGE_BASE)/bin/$< $(IMAGE_ROOT)/$@-$(ARCH)-$(PLAT) 2>&1 \
		| while read line; do echo " [GEN_IMAGE] $$line"; done; \
		exit $${PIPESTATUS[0]}
	@# The following commands use riscv-pk to package up the ELF image in a bbl boot loader image.
	$(Q)mkdir -p $(BUILD_BASE)/bbl
	$(Q)(cd $(BUILD_BASE)/bbl && \
	${TOOLS_ROOT}/riscv-pk/configure --quiet \
		--host=$(CONFIG_CROSS_COMPILER_PREFIX:"%-"=%) \
		--with-payload=$(IMAGE_ROOT)/$@-$(ARCH)-$(PLAT) && \
	$(MAKE) $(MAKE_SILENT) clean && $(MAKE) $(MAKE_SILENT) > /dev/null)
	$(Q)cp $(BUILD_BASE)/bbl/bbl $(IMAGE_ROOT)/$@-$(ARCH)-$(PLAT)

capDL-$(ARCH)-$(PLAT): export TOOLPREFIX=$(CONFIG_CROSS_COMPILER_PREFIX:"%"=%)
capDL-$(ARCH)-$(PLAT): kernel_elf common elfloader
	@echo "[GEN_IMAGE] $@"
	$(Q)$(SEL4_COMMON)/elfloader/gen_boot_image.sh $(STAGE_BASE)/kernel.elf \
		$(STAGE_BASE)/bin/capDL-loader $(IMAGE_ROOT)/$@
