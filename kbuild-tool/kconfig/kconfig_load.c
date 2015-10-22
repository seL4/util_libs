/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "lkc.h"

#define P(name,type,arg)	type (*name ## _p) arg
#include "lkc_proto.h"
#undef P

void kconfig_load(void)
{
	void *handle;
	char *error;

	handle = dlopen("./libkconfig.so", RTLD_LAZY);
	if (!handle) {
		handle = dlopen("./scripts/kconfig/libkconfig.so", RTLD_LAZY);
		if (!handle) {
			fprintf(stderr, "%s\n", dlerror());
			exit(1);
		}
	}

#define P(name,type,arg)			\
{						\
	name ## _p = dlsym(handle, #name);	\
        if ((error = dlerror()))  {		\
                fprintf(stderr, "%s\n", error);	\
		exit(1);			\
	}					\
}
#include "lkc_proto.h"
#undef P
}
