#!/bin/bash

#
# Copyright 2017, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the GNU General Public License version 2. Note that NO WARRANTY is provided.
# See "LICENSE_GPLv2.txt" for details.
#
# @TAG(DATA61_GPL)
#

# Echo all commands if V=3; maximum verbosity.
if [ 0${V} -ge 3 ]; then
    set -x
fi

# Check usage.
if [ $# -ne 3 ]; then
    echo "Usage: $0 <kernel elf> <user elf> <output file>"
    exit 1
fi
KERNEL_IMAGE=$1
USER_IMAGE=$2
OUTPUT_FILE=$3

case "$PLAT" in
    "realview")
        ENTRY_ADDR=0x72000000
        FORMAT=elf32-littlearm
        ;;
    "nslu2")
        ENTRY_ADDR=0x00100000;
        FORMAT=elf32-bigarm
        ;;
    "exynos4"|"exynos5")
        ENTRY_ADDR=0x41000000;
        FORMAT=elf32-littlearm
        ;;
    "imx6")
        ENTRY_ADDR=0x20000000;
        FORMAT=elf32-littlearm
        ;;
    "imx7")
        ENTRY_ADDR=0x82000000;
        FORMAT=elf32-littlearm
        ;;
    "zynq7000")
        ENTRY_ADDR=0x10000000;
        FORMAT=elf32-littlearm
        ;;
    "zynqmp")
        ENTRY_ADDR=0x10000000;
        if [ "$SEL4_ARCH" == "aarch64" ]
        then
            FORMAT=elf64-littleaarch64
        else
            FORMAT=elf32-littlearm
        fi
        ;;
    "apq8064")
        ENTRY_ADDR=0x82008000;
        FORMAT=elf32-littlearm
        ;;
    "spike")
        ENTRY_ADDR=0x0000000080400000;
        if [ "$KERNEL_64" == "y" ]
        then
            FORMAT=elf64-littleriscv
        else
            FORMAT=elf32-littleriscv
        fi
        ;;
    "imx31"|"omap3"|"am335x"|"omap4")
        ENTRY_ADDR=0x82000000
        FORMAT=elf32-littlearm
        ;;
    "hikey")
        if [ "$SEL4_ARCH" == "aarch64" ]
        then
            ENTRY_ADDR=0
            FORMAT=elf64-littleaarch64
        else
            ENTRY_ADDR=0x1000
            FORMAT=elf32-littlearm
        fi
        ;;
    "tx1")
        ENTRY_ADDR=0x0
        FORMAT=elf64-littleaarch64
        ;;
    "tk1")
        ENTRY_ADDR=0x90000000
        FORMAT=elf32-littlearm
        ;;
    "bcm2837")
        ENTRY_ADDR=0x20000000
        FORMAT=elf32-littlearm
        ;;
    *)
        echo "$0: Invalid platform \"$PLAT\""
        exit -1
    ;;
esac

# Ensure files exist.
if [ ! -e ${KERNEL_IMAGE} ]; then
    echo "File '${KERNEL_IMAGE}' does not exist."; exit 1
else
    # Only echo what kernel image we're using if verbosity is turned on. 0 is
    # prepended to ${V} because V may not be set.
    if [ 0${V} -ge 2 ]; then
        echo "Using ${KERNEL_IMAGE} as kernel image";
    fi
fi

if [ ! -e ${USER_IMAGE} ]; then
    echo "File '${USER_IMAGE}' does not exist."; exit 1
else
    # Only echo what user image we're using if verbosity is turned on. 0 is
    # prepended to ${V} because V may not be set.
    if [ 0${V} -ge 2 ]; then
        echo "Using ${USER_IMAGE} as rootserver image";
    fi
fi

# Get the script's location.
if [ "`uname`" == "Darwin" ]; then
    # MacOS doesn't have GNU readlink.
    SCRIPT_PATH=$([[ ${BASH_SOURCE[0]} = /* ]] && echo "${BASH_SOURCE[0]}" || echo "${PWD}/${BASH_SOURCE[0]}")
else
    SCRIPT_PATH=$(readlink -f ${BASH_SOURCE[0]})
fi
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

# Create working directory.
# Warning: mktemp functions differently on Linux and OSX.
TEMP_DIR=`mktemp -d -t seL4XXXX`
fail() {
    echo "(failed)" > /dev/stderr
    rm -rf ${TEMP_DIR}
    exit 1
}

# Generate an archive of the userspace and kernel images.
mkdir -p "${TEMP_DIR}/cpio"
cp -f ${KERNEL_IMAGE} ${TEMP_DIR}/cpio/kernel.elf
cp -f ${USER_IMAGE} ${TEMP_DIR}/cpio/
if [ "${STRIP}" = "y" ]; then
    ${TOOLPREFIX}strip --strip-all ${TEMP_DIR}/cpio/*
fi

dd if=/dev/urandom of=dummypayload bs=4 count=1
cp dummypayload ${TEMP_DIR}/cpio

if [ "${HASH}" = "y" ]; then
    if [ "${HASH_SHA}" = "y" ]; then
        # (2 Invocations so the hash gets printed on the terminal)
        # SHA256 hash of entire Kernel ELF and store it to .bin file. Add the .bin file to CPIO archive
        sha256sum ${TEMP_DIR}/cpio/kernel.elf
        sha256sum ${TEMP_DIR}/cpio/kernel.elf | cut -d ' ' -f 1 | xxd -r -p > ${TEMP_DIR}/cpio/kernel.bin

        # SHA256 hash of entire Application ELF and store it to .bin file. Add the .bin file to CPIO archive
        sha256sum ${TEMP_DIR}/cpio/$(basename ${USER_IMAGE})
        sha256sum ${TEMP_DIR}/cpio/$(basename ${USER_IMAGE}) | cut -d ' ' -f 1 | xxd -r -p > ${TEMP_DIR}/cpio/app.bin
    else
        # MD5 hash of entire Kernel ELF and store it to .bin file. Add the .bin file to CPIO archive
        md5sum ${TEMP_DIR}/cpio/kernel.elf
        md5sum ${TEMP_DIR}/cpio/kernel.elf | cut -d ' ' -f 1 | xxd -r -p > ${TEMP_DIR}/cpio/kernel.bin

        # MD5 hash of entire Application ELF and store it to .bin file. Add the .bin file to CPIO archive
        md5sum ${TEMP_DIR}/cpio/$(basename ${USER_IMAGE})
        md5sum ${TEMP_DIR}/cpio/$(basename ${USER_IMAGE}) | cut -d ' ' -f 1 | xxd -r -p > ${TEMP_DIR}/cpio/app.bin
    fi
fi

pushd "${TEMP_DIR}/cpio" &>/dev/null
#printf "kernel.elf\ndummypayload\n$(basename ${USER_IMAGE})\n" | cpio --quiet --block-size=8 --io-size=8 -o -H newc > ${TEMP_DIR}/archive.cpio
printf "kernel.elf\n$(basename ${USER_IMAGE})\n" | cpio --quiet --block-size=8 --io-size=8 -o -H newc > ${TEMP_DIR}/archive.cpio

# Strip CPIO metadata if possible.
which cpio-strip >/dev/null 2>/dev/null
if [ $? -eq 0 ]; then
    cpio-strip ${TEMP_DIR}/archive.cpio
fi

popd &>/dev/null

#
# Convert userspace / kernel into an archive which can then be linked
# against the elfloader binary. Change to the directory of archive.cpio
# before this operation to avoid polluting the symbol table with references
# to the temporary directory.
#
pushd "${TEMP_DIR}" >/dev/null
${TOOLPREFIX}ld -T "${SCRIPT_DIR}/archive.bin.lds" \
        --oformat ${FORMAT} -b binary archive.cpio \
        -o "${TEMP_DIR}/archive.o" || fail
popd >/dev/null

#
# Clearup the linker script for target platform.
#
${TOOLPREFIX}gcc ${CPPFLAGS} -P -E \
        -o "${SCRIPT_DIR}/linker.lds_pp" \
        -x c "${SCRIPT_DIR}/linker.lds"

TEXT_OPTION="--section-start .start="

# EFI images must be relocatable
if [ "${__EFI__}" == "y" ]; then
    PIE="-pie"
    TEXT_OPTION=
    ENTRY_ADDR=
fi

#
# Link everything together to produce the final ELF image.
#
${TOOLPREFIX}ld -T "${SCRIPT_DIR}/linker.lds_pp" \
        --oformat ${FORMAT} \
        "${SCRIPT_DIR}/elfloader.o" "${TEMP_DIR}/archive.o" \
        ${TEXT_OPTION}${ENTRY_ADDR} ${PIE} -o "${OUTPUT_FILE}" \
        || fail
if [ "${STRIP}" = "y" ]; then
    ${TOOLPREFIX}strip --strip-all ${OUTPUT_FILE}
fi

#
# Remove ELF stuff to have an PE32+/COFF executable file.
#
if [ "${__EFI__}" == "y" ]; then
    ${TOOLPREFIX}objcopy -O binary ${OUTPUT_FILE} ${OUTPUT_FILE}.efi
    mv ${OUTPUT_FILE}.efi ${OUTPUT_FILE}
fi

#
# Remove ELF stuff to have a binary file.
#
if [ "${__binary__}" == "y" ]; then
    ${TOOLPREFIX}objcopy -O binary ${OUTPUT_FILE} ${OUTPUT_FILE}.bin
    mv ${OUTPUT_FILE}.bin ${OUTPUT_FILE}
fi

# Done
rm -rf ${TEMP_DIR}
exit 0
