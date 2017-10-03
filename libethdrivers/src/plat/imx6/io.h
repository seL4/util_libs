/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#pragma once

#define __arch_getl(addr)         *((volatile uint32_t*)(addr))
#define __arch_getw(addr)         *((volatile uint16_t*)(addr))
#define __arch_getb(addr)         *((volatile uint8_t*)(addr))

#define __arch_putl(val, addr)    *((volatile uint32_t*)(addr)) = val
#define __arch_putw(val, addr)    *((volatile uint16_t*)(addr)) = val
#define __arch_putb(val, addr)    *((volatile uint8_t*)(addr)) = val

//#define __raw_writel(...) writel(__VA_ARGS__)
//#define __raw_readl(...) readl(__VA_ARGS__)

#define __raw_writeb(v,a)	__arch_putb(v,a)
#define __raw_writew(v,a)	__arch_putw(v,a)
#define __raw_writel(v,a)	__arch_putl(v,a)

#define __raw_readb(a)		__arch_getb(a)
#define __raw_readw(a)		__arch_getw(a)
#define __raw_readl(a)		__arch_getl(a)

/*
 * TODO: The kernel offers some more advanced versions of barriers, it might
 * have some advantages to use them instead of the simple one here.
 */
#define dmb()     asm volatile("dmb" ::: "memory")
#define dsb()     asm volatile("dsb" ::: "memory")
#define isb()     asm volatile("isb" ::: "memory")
#define __iormb()	dmb()
#define __iowmb()	dmb()

#define writeb(v,c)	({ uint8_t  __v = v; __iowmb(); __arch_putb(__v,c); __v; })
#define writew(v,c)	({ uint16_t __v = v; __iowmb(); __arch_putw(__v,c); __v; })
#define writel(v,c)	({ uint32_t __v = v; __iowmb(); __arch_putl(__v,c); __v; })

#define readb(c)	({ uint8_t  __v = __arch_getb(c); __iormb(); __v; })
#define readw(c)	({ uint16_t __v = __arch_getw(c); __iormb(); __v; })
#define readl(c)	({ uint32_t __v = __arch_getl(c); __iormb(); __v; })

