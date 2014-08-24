/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


#include "../../services.h"

void
ps_nsdelay(unsigned int _ns){
    volatile unsigned int t = _ns / 5;
    while(t--);
}

void
ps_usdelay(unsigned int _us){
    volatile unsigned int us = _us;
    while(us--){
        ps_nsdelay(1000);
    }
}

void
ps_msdelay(unsigned int _ms){
    volatile unsigned int ms = _ms;
    while(ms--){
        ps_usdelay(1000);
    }
}

