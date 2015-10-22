/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _SCU_H_
#define _SCU_H_

void scu_enable(void *_scu_base);
unsigned int scu_get_core_count(void *_scu_base);

#endif /* _SCU_H_ */

