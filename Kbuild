#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the GNU General Public License version 2. Note that NO WARRANTY is provided.
# See "LICENSE_GPLv2.txt" for details.
#
# @TAG(NICTA_GPL)
#

libs-$(CONFIG_LIB_ETHIF) += libethdrivers
libethdrivers-${CONFIG_LIB_LWIP} += liblwip
libethdrivers: ${libethdivers-y} libutils libsel4 $(libc) libplatsupport common


