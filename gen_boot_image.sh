#!/bin/bash

#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the GNU General Public License version 2. Note that NO WARRANTY is provided.
# See "LICENSE_GPLv2.txt" for details.
#
# @TAG(NICTA_GPL)
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
    "apq8064")
        ENTRY_ADDR=0x82008000;
        FORMAT=elf32-littlearm
        ;;
    "imx31"|"omap3"|"am335x"|"omap4")
        ENTRY_ADDR=0x82000000
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
cp -f ${USER_IMAGE} ${TEMP_DIR}/cpio
pushd "${TEMP_DIR}/cpio" &>/dev/null
printf "kernel.elf\n$(basename ${USER_IMAGE})\n" | cpio --quiet -o -H newc > ${TEMP_DIR}/archive.cpio

# Strip CPIO metadata if possible.
which cpio-strip > /dev/null
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
        --oformat ${FORMAT} -r -b binary archive.cpio \
        -o "${TEMP_DIR}/archive.o" || fail
popd >/dev/null

#
# Link everything together to produce the final ELF image.
#
${TOOLPREFIX}ld -T "${SCRIPT_DIR}/linker.lds" \
        --oformat ${FORMAT} \
        "${SCRIPT_DIR}/elfloader.o" "${TEMP_DIR}/archive.o" \
        -Ttext=${ENTRY_ADDR} -o "${OUTPUT_FILE}" \
        || fail


# Done
rm -rf ${TEMP_DIR}
exit 0
