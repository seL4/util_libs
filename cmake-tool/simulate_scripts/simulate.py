#!/usr/bin/env python3

# Copyright 2018, Data61
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
import time


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--binary', dest="qemu_sim_binary", type=str,
                        help="QEMU binary", default="@QEMU_SIM_BINARY@")
    parser.add_argument('-d', '--gdbserver', dest="qemu_gdbserver", action='store_true',
                        help="Tell QEMU to wait for gdb on port 1234")
    parser.add_argument('-M', '--machine', dest="qemu_sim_machine", type=str,
                        help="QEMU Machine", default="@QEMU_SIM_MACHINE@")
    parser.add_argument('-c', '--cpu', dest='qemu_sim_cpu', type=str,
                        help="QEMU CPU", default="@QEMU_SIM_CPU@")
    parser.add_argument('-o', '--cpu-opt', dest='qemu_sim_cpu_opt', type=str,
                        help="QEMU CPU Options", default="@QEMU_SIM_CPU_OPT@")
    parser.add_argument('-g', '--graphic', dest='qemu_sim_graphic_opt', type=str,
                        help="QEMU Graphic Options", default="@QEMU_SIM_GRAPHIC_OPT@")
    parser.add_argument('-s', '--serial', dest='qemu_sim_serial_opt', type=str,
                        help="QEMU Serial Options", default="@QEMU_SIM_SERIAL_OPT@")
    parser.add_argument('-m', '--mem-size', dest='qemu_sim_mem_size', type=str,
                        help="QEMU Memory Size Option", default="@QEMU_SIM_MEM_SIZE_OPT@")
    parser.add_argument('-a', '--args', dest='qemu_sim_args', type=str,
                        help="Arguments to pass onto QEMU", default="@QEMU_SIM_ARGS@")
    parser.add_argument('-k', '--kernel', dest='qemu_sim_kernel_file', type=str,
                        help="Kernel file to pass onto QEMU", default="@QEMU_SIM_KERNEL_FILE@")
    parser.add_argument('-i', '--initrd', dest='qemu_sim_initrd_file', type=str,
                        help="Initrd file to pass onto QEMU", default="@QEMU_SIM_INITRD_FILE@")
    parser.add_argument('--extra-qemu-args', dest='qemu_sim_extra_args', type=str,
                        help="Additional arguments to pass onto QEMU", default="")
    parser.add_argument('--extra-cpu-opts', dest='qemu_sim_extra_cpu_opts', type=str,
                        help="Additional cpu options to append onto the existing CPU options",
                        default="")
    args = parser.parse_args()
    return args


if __name__ == "__main__":
    args = parse_args()

    if args.qemu_sim_kernel_file == "":
        qemu_sim_images_entry = "-kernel " + args.qemu_sim_initrd_file
    else:
        qemu_sim_images_entry = "-kernel " + args.qemu_sim_kernel_file + " -initrd " + args.qemu_sim_initrd_file

    qemu_sim_cpu_entry = ""
    if args.qemu_sim_cpu != "":
        qemu_sim_cpu_entry = "-cpu " + args.qemu_sim_cpu + args.qemu_sim_cpu_opt + \
            ("," + args.qemu_sim_extra_cpu_opts if args.qemu_sim_extra_cpu_opts else "")

    qemu_sim_machine_entry = ""
    if args.qemu_sim_machine:
        qemu_sim_machine_entry = "-machine " + args.qemu_sim_machine

    qemu_gdbserver_command = ""
    if args.qemu_gdbserver:
        qemu_gdbserver_command = "-s -S"

    qemu_sim_mem_size_entry = "-m size=" + args.qemu_sim_mem_size

    qemu_simulate_command_opts = [args.qemu_sim_binary, qemu_sim_machine_entry, qemu_sim_cpu_entry, args.qemu_sim_graphic_opt,
                                  args.qemu_sim_serial_opt, qemu_sim_mem_size_entry, args.qemu_sim_extra_args, qemu_sim_images_entry,
                                  qemu_gdbserver_command]
    qemu_simulate_command = " ".join(qemu_simulate_command_opts)


    print(qemu_simulate_command)

    if qemu_gdbserver_command != "":
        print("\nWaiting for GDB on port 1234...")

    qemu_status = subprocess.call(qemu_simulate_command, shell=True)

    if qemu_status != 0:
        delay = 5 # in seconds
        msg = "\nQEMU failed; resetting terminal in {d}".format(d=delay) \
            + " seconds--interrupt to abort\n"
        sys.stderr.write(msg)
        time.sleep(delay)

    subprocess.call("tput reset", shell=True)
