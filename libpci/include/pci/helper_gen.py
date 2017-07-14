#!/usr/bin/python3
#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

import re, os;

helper_lines = open("helper.h", "r").readlines();
helper_out = open("../../src/helper.c", "w");

# Generate header.
print ( "// WARNING: This file is generated. DO NOT EDIT.", file=helper_out );
print ( "// Look in include/pci/helper_gen.y instead.\n", file=helper_out );
print ( "#include <pci/helper.h>\n", file=helper_out );

# Generate vendor IDs.
print ( "char* libpci_vendorID_str(int vid) {", file=helper_out );

for line in helper_lines:
    line = line.strip();
    r = re.search(r'^#define PCI_VENDOR_ID_([A-Z0-9_]+)\s+(\w+)', line);
    if not r: continue;

    (vendor, val) = (r.group(1).lower(), r.group(2));
    print ( "    if (vid == %s) return \"%s\";" % (val, vendor), file=helper_out );

print ( "    return \"Unknown vendor ID.\"; ", file=helper_out );
print ( "}\n", file=helper_out );

# Generate device IDs.
print ( "char* libpci_deviceID_str(int vid, int did) {", file=helper_out );
vendor = ""; vval = "";
for line in helper_lines:
    line = line.strip();

    rv = re.search(r'^#define PCI_VENDOR_ID_([A-Z0-9_]+)\s+(\w+)', line);
    if rv:
        (vendor, vval) = (rv.group(1).lower(), rv.group(2));

    r = re.search(r'^#define PCI_DEVICE_ID_([A-Z0-9]+_[A-Z0-9_]+)\s+(\w+)', line);
    if not r: continue;

    (device, val) = (r.group(1).lower(), r.group(2));
    print ( "    if (vid == %s && did == %s) return \"%s\";" % (vval, val, device), file=helper_out );

print ( "    return \"Unknown device ID.\"; ", file=helper_out );
print ( "}\n", file=helper_out );
