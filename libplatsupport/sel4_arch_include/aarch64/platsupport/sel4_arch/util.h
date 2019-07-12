/*
* Copyright 2019, Data61
* Commonwealth Scientific and Industrial Research Organisation (CSIRO)
* ABN 41 687 119 230.
*
* This software may be distributed and modified according to the terms of
* the BSD 2-Clause license. Note that NO WARRANTY is provided.
* See "LICENSE_BSD2.txt" for details.
*
* @TAG(DATA61_BSD)
*/

#pragma once

#define COPROC_WRITE_WORD(R,W) asm volatile ("msr " R  ", %0" :: "r"(W))
#define COPROC_READ_WORD(R,W)  asm volatile ("mrs %0, " R : "=r" (W))
#define COPROC_WRITE_64(R,W)   COPROC_WRITE_WORD(R,W)
#define COPROC_READ_64(R,W)    COPROC_READ_WORD(R,W)

/* control reigster for the el1 physical timer */
#define CNTP_CTL "cntp_ctl_el0"
/* holds the compare value for the el1 physical timer */
#define CNTP_CVAL "cntp_cval_el0"
/* holds the 64-bit physical count value */
#define CNTPCT "cntpct_el0"
/* frequency of the timer */
#define CNTFRQ "cntfrq_el0"
