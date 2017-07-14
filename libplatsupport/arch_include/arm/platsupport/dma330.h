/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#ifndef _PLATSUPPORT_ARCH_DMA330_H
#define _PLATSUPPORT_ARCH_DMA330_H

#include <platsupport/io.h>
#include <stdint.h>

#include <platsupport/plat/dma330.h>

/* ARM PL-330 (DMA-330) DMA controller */

struct dma330_dev;
typedef struct dma330_dev* dma330_t;

/**
 * Callback for signal handling
 * @param[in] dma330  A handle to the DMA330 controller that was monitoring the event
 * @param[in] signal  The signal, in the address space of the channel, that cause the IRQ
 * @param[in] pc      The instruction pointer of the thread that caused the signal
 * @param[in] status  The status of the thread that caused the signal as presented in the csr register.
 * @param[in] token   The token that the caller registered with this callback.
 * @return            The application should return 0 if the transfer should be halted.
 */
typedef int (*dma330_signal_cb)(dma330_t* dma330, int signal, uintptr_t pc, uint32_t status, void* token);

/**
 * Initialise the DMA controller
 * @praam[in]  id      The DMA device ID to be initialised
 * @param[in]  ops     io operations for the initialisation process
 * @param[out] dma330  on success, contains a handle to the initialised device
 * @return             0 on success
 */
int dma330_init(enum dma330_id id, struct ps_io_ops* ops, dma330_t* dma330);

/**
 * Initialise the DMA controller with a provided base address
 * @praam[in]  id          The DMA device ID to be initialised
 * @param[in]  dma330_base The base address of the device for IO access.
 * @param[in]  clk_sys     A handle to the clock subsytem for initialisation
 * @param[out] dma330      on success, contains a handle to the initialised device
 * @return                 0 on success
 */
int dma330_init_base(enum dma330_id id, void* dma330_base, clock_sys_t* clk_sys,
                     dma330_t* dma330);

/**
 * Initiates a DMA transfer
 * @param[in] dma330  a handle to the dma device
 * @param[in] channel The channel to use for the transfer
 * @param[in] program The physical address of the DMA330 micro-code to execute.
 * @param[in] cb      A callback to call when the channel thread produces an event.
 * @param[in] token   A token to pass, unmodified, to the callback.
 * @return            0 on success
 */
int dma330_xfer(dma330_t* dma330, int channel, uintptr_t program, dma330_signal_cb cb, void* token);

/**
 * Allows the dma engine to handle an IRQ
 * @param[in] dma330 a handle to the dma device
 * @return           0 if an IRQ was indeed pending
 */
int dma330_handle_irq(dma330_t* dma330);

/***********************
 *** Program presets ***
 ***********************/

/**
 * Compiles a dma330 program.
 * 32 bytes will be reserved at the beginning of the program for argument passing
 * @param[in]  source_code  DMA330 assembly program
 * @param[out] bin          The resulting binary program
 * @return                  0 on success, or line number on failure
 */
int dma330_compile(char* source_code, void* bin);

/**
 * Loads a preset micro code for a copy program
 * The copy program sends signal #0 + channel when complete
 * @param[in]  channel      The channel which the program will be executed on
 * @param[out] bin          An address to store the compiled dma330 program to
 */
void dma330_copy_compile(int channel, void* bin);

/**
 * Configures a copy program.
 * The source and destination must be hard coded into the micro code. This
 * function takes an existing binary and adjusts the relevant parameters.
 * @param[in]    psrc  The physical source address of the copy operation
 * @param[in]    pdst  The physical destination address of the copy operation
 * @param[in]    len   The number of bytes to copy
 * @param[inout] vbin  The virtual address of a compiled copy program binary
 */
int dma330_copy_configure(uintptr_t psrc, uintptr_t pdst, size_t len, void* vbin);

#endif /* _PLATSUPPORT_ARCH_DMA330_H */
