#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

#
# Device framework makefile
#
#  parse the system specification and generate a system CapDL spec
#  compile the capDL-loader with this spec
#  use dite to combine  all the required module executable files into the system-image
#
#  Module makers need to put in that they depend on CAPDL_LOADER_BOOT in their
#  module Kconfig
#

.PHONY: specs use-capDL-loader

dm:
	@echo "[DM] building..."
	$(Q)$(MAKE) --no-print-directory --directory=$(TOOLS_ROOT)/dm \
		STAGING_ROOT=$(BUILD_ROOT)/dm BUILD_ROOT=$(STAGE_ROOT)/dm
	@echo "[DM] done."

capDL-loader: kernel_elf common libsel4 libmuslc libcpio libsel4elf libsel4muslcsys
	@echo "[capDL-loader] building..."
	$(Q)CFLAGS= LDFLAGS= $(MAKE) $(MAKE_SILENT) -C $(BUILD_BASE)/$@ -f $(srctree)/.config -f $(TOOLS_ROOT)/dm/capDL-loader/Makefile \
		BUILD_DIR=$(BUILD_BASE)/$@ SOURCE_DIR=$(TOOLS_ROOT)/dm/capDL-loader/ V=$(V) \
		STAGE_DIR=$(STAGE_BASE) \
	TOOLPREFIX=$(CONFIG_CROSS_COMPILER_PREFIX:"%"=%)
	@echo "[capDL-loader] done"

SEL4_SYSTEM_SPEC_NAME := $(patsubst %",%,$(patsubst "%,%,${CONFIG_SYSTEM_SPEC}))
# ")") # fix syntax highlighting

#
# Build spec database
#

CAPDL_SPECDIR      = $(patsubst "%",%,${CONFIG_CAPDL_SPECDIR})

SEL4_SPECDIR       = $(STAGE_BASE)/$(CAPDL_SPECDIR)

CAPDL_SPECS        = $(CAPDL_SPECDIR)/system/$(SEL4_SYSTEM_SPEC_NAME) $(CAPDL_SPECDIR)/platforms/$(ARCH) $(CAPDL_SPECDIR)/devices
SEL4_SPECS         = $(patsubst %, $(STAGE_BASE)/%, $(CAPDL_SPECS))

CAPDL_MODULE_SPECS = $(patsubst %, ${SEL4_MODULES_PATH}/%/module.spec, $(filter-out capDL-loader,$(modules-y)))
SEL4_MODULE_SPECS  = $(patsubst %, ${SEL4_SPECDIR}/modules/%/module.spec,$(filter-out capDL-loader,$(modules-y)))

#Copy the module, platform and system specs to stage
specs: $(SEL4_SPECS) $(SEL4_MODULE_SPECS)

$(SEL4_SPECS): $(CAPDL_SPECS)
	$(Q) file=$@ ;\
	echo -n "[STAGE/SPEC] "; echo -n "/$${file#$(SEL4_SPECDIR)/}"; \
	if [ -d $${file#$(STAGE_BASE)/} ]; then echo "/*"; else echo ""; fi; \
	mkdir -p `dirname $$file`; \
	cp -a ${CAPDL_SPECDIR}/$${file#${SEL4_SPECDIR}/} `dirname $$file`  ;

${SEL4_SPECDIR}/modules/%/module.spec: ${SEL4_MODULES_PATH}/%/module.spec
	$(Q) file=$@; \
	echo -n "[STAGE/SPEC] "; echo "$${file#${SEL4_SPECDIR}/modules/}"; \
	mkdir -p `dirname $$file` ; \
	cp -a ${SEL4_MODULES_PATH}/$${file#${SEL4_SPECDIR}/modules/} $$file ;


SEL4_SYSTEM_CAPDL = $(STAGE_BASE)/include/system-capDL-spec.h

#
# Generate system capDL specification
#
# NOTE: dm will call make after parsing the specfications
#       to build the required drivers
#

#Exported variables will be getEnved by dm to know where to place the system capdl file
$(SEL4_SYSTEM_CAPDL): export STAGE_DIR:=$(STAGE_BASE)
$(SEL4_SYSTEM_CAPDL): export PATH:=$(STAGE_ROOT)/dm:$(PATH)
$(SEL4_SYSTEM_CAPDL): specs dm
	@echo "[GEN_CAPDL_SPEC]"
	${Q}dm --pci=${CONFIG_PCI_DUMP} --fdt=${CONFIG_FDT_FILE} $(SEL4_SPECDIR)/system/$(SEL4_SYSTEM_SPEC_NAME)
	@echo "[GEN_CAPDL_SPEC] done."
$(BUILD_BASE)/capDL-loader/archive.o: export TOOLPREFIX=$(CONFIG_CROSS_COMPILER_PREFIX:"%"=%)
$(BUILD_BASE)/capDL-loader/archive.o: $(SEL4_SYSTEM_CAPDL)
	@echo -n "[CPIO ARCHIVE]"
	$(Q)mkdir -p $(dir $@)
	@echo -e $(patsubst %,"\\n   [CPIO] % ",$(filter-out capDL-loader,$(modules)))
	$(Q)${COMMON_PATH}/files_to_obj.sh $@ _capdl_archive $(patsubst %,$(STAGE_BASE)/bin/%,$(notdir $(filter-out capDL-loader, $(modules))));
	@echo "[CPIO ARCHIVE] done."

use-capDL-loader: $(BUILD_BASE)/capDL-loader/archive.o capDL-loader capDL-$(ARCH)-$(PLAT)



