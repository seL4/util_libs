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

# Some of the targets in here use bashisms, so force bash as the interpreter.
SHELL=/bin/bash
ifeq ($(wildcard ${SHELL}),)
$(error Prerequisite ${SHELL} not found)
endif

export SEL4_CMDLINE="console=0x3f8 debug=0x2f8 max_num_nodes=8"
PHONY += harddisk-images
harddisk-images: $(patsubst %,%-harddisk-image,$(apps))

ARCH_NAME := ${SEL4_ARCH}

%-harddisk-image: % common kernel_elf FORCE
	@echo "[COBBLER] $@"
	$(Q)$(SEL4_COMMON)/cobbler -k "$(STAGE_BASE)/kernel.elf" -a $(SEL4_CMDLINE) \
		-o "$(IMAGE_ROOT)/$@-$(ARCH_NAME)-$(PLAT)" "$(STAGE_BASE)/bin/$<" 2>&1 \
		| while read line; do echo " [COBBLER] $$line"; done; \
		exit $${PIPESTATUS[0]}
ifeq ($(VMWARE_DISK_IMAGE),y)
	rm -f "${IMAGE_ROOT}/$@.vmdk"
	rm -f "${IMAGE_ROOT}/$@.vdi"
	VBoxManage convertfromraw "${IMAGE_ROOT}/$@-$(ARCH_NAME)-$(PLAT)" \
		"${IMAGE_ROOT}/$@.vmdk" -format VMDK
	VBoxManage convertfromraw "${IMAGE_ROOT}/$@-$(ARCH_NAME)-$(PLAT)" \
		"${IMAGE_ROOT}/$@.vdi" -format VDI
endif

%-image: % kernel_elf common FORCE
	@echo "[GEN_IMAGE] $@"
	$(Q)objcopy -O elf32-i386 "$(STAGE_BASE)/kernel.elf" "$(IMAGE_ROOT)/kernel-$(ARCH_NAME)-$(PLAT)"
	$(Q)cp -f "$(STAGE_BASE)/bin/$<" "$(IMAGE_ROOT)/$@-$(ARCH_NAME)-$(PLAT)"

#New target to make using capDL-loader
capDL-$(ARCH_NAME)-$(PLAT): capDL-loader kernel_elf common FORCE
	@echo "[GEN_IMAGE] $@"
	$(Q)$(call cp_if_changed, "$(STAGE_BASE)/kernel.elf", \
		"$(IMAGE_ROOT)/kernel-$(ARCH_NAME)-$(PLAT)")
	$(Q)$(call cp_if_changed, "$(STAGE_BASE)/bin/$<", \
		"$(IMAGE_ROOT)/$@")
