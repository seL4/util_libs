/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include "debug.h"
void
dump_regs(uint32_t* start, int size){
    int i, j;
    for(i = 0; i < size/sizeof(*start); ){
        printf("+0x%03x: ",((uint32_t)start)&0xfff);
        for(j = 0; j < 4; j++, i++, start++){
            printf("0x%08x ", *start);
        }
        printf("\n");
    }
}


