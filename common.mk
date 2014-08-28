#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

### CCACHE
########################################
# if ccache is in our path, use it!
ifeq ($(CONFIG_BUILDSYS_USE_CCACHE),y)
CCACHE=$(shell which ccache)
else
CCACHE=
endif

### Goanna
ifeq (${CONFIG_BUILDSYS_USE_GOANNA},y)
GOANNA=$(shell which goannacc)
else
GONNA=
endif
ifneq (${GOANNA},)
GOANNA += --license-server=goanna.ken.nicta.com.au --silent-profile \
    --profile=${CONFIG_BUILDSYS_GOANNA_PROFILE}
endif

### Verbose building
########################################

# Set V=1 for verbose building, this can be passed in on the command line
# Set V=2 to have make echo out commands before executing them

ifeq ($V, 1)
	Q =
else
ifeq ($V, 2)
	Q =
else
ifeq ($V, 3)
	Q =
else
	Q = @
endif
endif
endif

CC  := $(CCACHE) $(if ${GOANNA},${GOANNA},$(TOOLPREFIX)gcc$(TOOLSUFFIX))
ASM := $(CCACHE) $(TOOLPREFIX)gcc$(TOOLSUFFIX)
LD  := $(CCACHE) $(TOOLPREFIX)ld$(TOOLSUFFIX)
AR  := $(CCACHE) $(TOOLPREFIX)ar$(TOOLSUFFIX)
CPP := $(CCACHE) $(TOOLPREFIX)cpp$(TOOLSUFFIX)
OBJCOPY := $(CCACHE) $(TOOLPREFIX)objcopy$(TOOLSUFFIX)

# Default path configuration (useful for local development)
SEL4_LIBDIR     ?= $(STAGE_DIR)/lib
SEL4_INCLUDEDIR ?= $(STAGE_DIR)/include
SEL4_BINDIR     ?= $(STAGE_DIR)/bin
SEL4_KERNEL     ?= $(STAGE_DIR)/kernel/kernel.elf

# Compile configuration
INCLUDE_PATH := $(SEL4_INCLUDEDIR) $(INCLUDE_DIRS)
WARNINGS     := all

# the NK_BUILD is used in the Makefile.flags file to distinguish
# kernel build vs non-kernel build compiler options
export NK_BUILD=y

CFLAGS += $(INCLUDE_PATH:%=-I%)

# Strip enclosing quotes.
CONFIG_USER_CFLAGS:=$(patsubst %",%,$(patsubst "%,%,${CONFIG_USER_CFLAGS}))
#")") Help syntax-highlighting editors.

# We use start and end groups to recursively find symbols when linking.
# However we cannot use these when performing link time optimisations.
# We define these as variables here such that they can be overridden
# if LTO is enabled
STARTGROUP := -Wl,--start-group
ENDGROUP := -Wl,--end-group

ifeq (${CONFIG_USER_CFLAGS},)
    CFLAGS += $(WARNINGS:%=-W%) -nostdinc -std=gnu99

    ifeq (${CONFIG_USER_OPTIMISATION_Os},y)
        CFLAGS += -Os
    endif
    ifeq (${CONFIG_USER_OPTIMISATION_O0},y)
        CFLAGS += -O0
    endif
    ifeq (${CONFIG_USER_OPTIMISATION_O1},y)
        CFLAGS += -O1
    endif
    ifeq (${CONFIG_USER_OPTIMISATION_O3},y)
        CFLAGS += -O3
    endif
    ifeq (${CONFIG_USER_OPTIMISATION_O2},y)
        CFLAGS += -O2
    endif

    ifeq (${CONFIG_LINK_TIME_OPTIMISATIONS},y)
        CFLAGS += -flto
        STARTGROUP :=
        ENDGROUP :=
    endif

    CFLAGS += $(NK_CFLAGS)
    CFLAGS += -fno-stack-protector
else
    # Override the cflags with Kconfig-specified flags
	CFLAGS += ${CONFIG_USER_CFLAGS}
endif

ifeq (${CONFIG_WHOLE_PROGRAM_OPTIMISATIONS_USER},y)
    LDFLAGS += -fwhole-program
endif

LIBGCC := -lgcc
EH := $(shell $(CC) --print-file-name libgcc_eh.a)
ifneq "${EH}" "libgcc_eh.a"
  LIBGCC += -lgcc_eh
endif


LDFLAGS += $(NK_LDFLAGS) \
		   $(SEL4_LIBDIR:%=-L%) \
		   $(STARTGROUP) \
		   $(LIBS:%=-l%) \
		   $(LIBGCC) \
		   $(ENDGROUP) \
		   $(CFLAGS) \
		   -static -nostdlib

ENTRY_POINT ?= _start

# Force start symbol to be linked in if need be - the user may already have it in a
# .o file, otherwise this will pull it from a library such as
# libsel4platsupport.
LDFLAGS += -u ${ENTRY_POINT}

# Set the entry point
LDFLAGS += -e ${ENTRY_POINT}

# Object files
OBJFILES = $(ASMFILES:%.S=%.o) $(CFILES:%.c=%.o) $(OFILES)

# Define standard crt files if are building against a C library that has them
ifeq (${CONFIG_HAVE_CRT},y)
	CRTOBJFILES ?= $(SEL4_LIBDIR)/crt1.o $(SEL4_LIBDIR)/crti.o $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
	FINOBJFILES ?= $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o) $(SEL4_LIBDIR)/crtn.o
else
	CRTOBJFILES ?=
	FINOBJFILES ?=
endif

# Copy a file.
cp_file = \
	@echo " [STAGE] $(notdir $2)"; cp -f $(1) $(2)

# Where to look for header dependencies
vpath %.h $(INCLUDE_PATH)

# Where to look for library dependencies
vpath %.a $(SEL4_LIBDIR)

# Where to find the sources
vpath %.c $(SOURCE_DIR)
vpath %.S $(SOURCE_DIR)

# Default is to build/install all targets
default: $(PRIORITY_TARGETS) install-headers $(TARGETS)

#
# For each ".bin" or ".a" target, we also setup a rule to copy it into a final
# binaries output directory.
#
default: $(patsubst %.bin,$(SEL4_BINDIR)/%,$(filter %.bin,$(TARGETS)))
default: $(patsubst %.a,$(SEL4_LIBDIR)/%.a,$(filter %.a,$(TARGETS)))

$(SEL4_BINDIR)/%: %.bin
	$(Q)mkdir -p $(dir $@)
	$(call cp_file,$<,$@)

$(SEL4_LIBDIR)/%.a: %.a
	$(Q)mkdir -p $(dir $@)
	$(call cp_file,$<,$@)

.PHONY: install-headers

HDRFILES += $(srctree)/include/generated/autoconf.h

install-headers:
	@if [ -n "$(HDRFILES)" ] ; then \
		mkdir -p $(SEL4_INCLUDEDIR) ; \
		echo " [HEADERS]"; \
		for file in $(HDRFILES); do \
			echo -n " [STAGE] "; echo -n `basename $$file`; \
			if [ -d $$file ]; then echo "/*"; else echo; fi; \
			cp -aL $$file $(SEL4_INCLUDEDIR) ; \
		done; \
	fi
	@if [ -n "$(RHDRFILES)" ] ; then \
		mkdir -p $(SEL4_INCLUDEDIR) ; \
		echo " [HEADERS] "; \
		for hdrfile in $(RHDRFILES) ; do \
			source=`echo "$$hdrfile" | sed 's/^\(.*\)[ \t][^ \t]*$$/\1/'` ; \
			dest=$(SEL4_INCLUDEDIR)/`echo "$$hdrfile" | sed 's/^.*[ \t]\([^ \t]*\)$$/\1/'` ; \
			mkdir -p $$dest; \
			cp -a $$source $$dest ; \
			echo -n " [STAGE]"; basename $$dest; \
		done ; \
	fi

ifeq (${CONFIG_BUILDSYS_CPP_SEPARATE},y)
%.o: %.c_pp
	@echo " [CC] $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) -x c $(CFLAGS) -c $< -o $@
else
%.o: %.c $(HFILES) | install-headers
	@echo " [CC] $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(call make-depend,$<,$@,$(patsubst %.o,%.d,$@))
	$(Q)$(CC) -x c $(CFLAGS) -c $< -o $@
endif

.PRECIOUS: %.c_pp
%.c_pp: %.c $(HFILES) | install-headers
	@echo " [CPP] $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(call make-depend,$<,$@,$(patsubst %.c_pp,%.d,$@))
	$(Q)$(CC) -E \
        $(if ${CONFIG_BUILDSYS_CPP_RETAIN_COMMENTS},-C -CC,) \
        $(if ${CONFIG_BUILDSYS_CPP_LINE_DIRECTIVES},,-P) \
        $(CFLAGS) -c $< -o $@

%.o: %.S $(HFILES) | install-headers
	@echo " [ASM] $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(call make-depend,$<,$@,$(patsubst %.o,%.d,$@))
	$(Q)$(ASM) $(CFLAGS) -c $< -o $@

%.a: $(OBJFILES)
	@echo " [AR] $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(AR) r $@ $(OBJFILES) > /dev/null 2>&1

%.bin: %.elf
	$(call cp_file,$<,$@)

%.elf: $(CRTOBJFILES) $(FINOBJFILES)
%.elf: $(OBJFILES)
	@echo " [LINK] $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CRTOBJFILES) $^ $(FINOBJFILES) $(LDFLAGS) -o $@

%.img: %.bin $(COBBLER) $(SEL4_KERNEL)
	@echo " [IMG] $@"
	$(Q)$(COBBLER) -k $(SEL4_KERNEL) -o $@ $<

$(TARGETS): $(LIBS:%=-l%)

# Avoid inadvertently passing local shared libraries with the same names as ours to the
# linker. This is a hack and will need to be changed if we start depending on SOs.
# (Default is .LIBPATTERNS = lib%.so lib%.a)
.LIBPATTERNS = lib%.a

DEPS = $(patsubst %.c,%.d,$(CFILES)) $(patsubst %.S,%.d,$(ASMFILES))

ifneq "$(MAKECMDGOALS)" "clean"
  -include ${DEPS}
endif

# $(call make-depend,source-file,object-file,depend-file)
define make-depend
  ${CC} -MM            \
         -MF $3         \
         -MP            \
         -MT $2         \
         $(CFLAGS)      \
         $(CPPFLAGS)    \
         $1
endef
