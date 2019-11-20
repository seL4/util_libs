#!/usr/bin/env python3

# Copyright 2019, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)

import subprocess
import sys
import argparse
import signal

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--binary', dest='gdb_binary', type=str,
                        help='GDB binary', default='@GDB_BINARY@')
    parser.add_argument('-f', '--file', dest='target_executable', type=str,
                        help='File to be passed to GDB', default='@QEMU_SIM_INITRD_FILE@')
    parser.add_argument('--extra-gdb-args', dest='extra_gdb_args', type=str,
                        help='Additional arguments to pass to gdb', default='')
    args = parser.parse_args()
    return args

if __name__ == "__main__":
    args = parse_args()

    gdb_command_opts = [args.gdb_binary, args.extra_gdb_args, '-ex "target remote :1234"', args.target_executable]
    gdb_command = " ".join(gdb_command_opts)

    print(gdb_command)
    # Extra newline to make the command easier to see
    print("\n")

    # Ignore SIGINT/Ctrl-C here and forward it to the gdb instance
    signal.signal(signal.SIGINT, signal.SIG_IGN)

    subprocess.call(gdb_command, shell=True)
