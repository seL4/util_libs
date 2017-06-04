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

#ifndef _PLATSUPPORT_PLAT_IRQ_COMBINER_H_
#define _PLATSUPPORT_PLAT_IRQ_COMBINER_H_

enum irq_combiner_id {
    IRQ_COMBINER0,
    NIRQ_COMBINERS
};

#define SYSMMU_G2D1_CIRQ        COMBINER_IRQ(24, 6)
#define SYSMMU_G2D0_CIRQ        COMBINER_IRQ(24, 5)
#define SYSMMU_FIMC_LITE11_CIRQ COMBINER_IRQ(24, 2)
#define SYSMMU_FIMC_LITE10_CIRQ COMBINER_IRQ(24, 1)
#define SYSMMU_DRCISP1_CIRQ     COMBINER_IRQ(11, 7)
#define SYSMMU_DRCISP0_CIRQ     COMBINER_IRQ(11, 6)
#define SYSMMU_ODC1_CIRQ        COMBINER_IRQ(11, 1)
#define SYSMMU_ODC0_CIRQ        COMBINER_IRQ(11, 0)
#define SYSMMU_ISP1_CIRQ        COMBINER_IRQ(10, 7)
#define SYSMMU_ISP0_CIRQ        COMBINER_IRQ(10, 6)
#define SYSMMU_DIS01_CIRQ       COMBINER_IRQ(10, 5)
#define SYSMMU_DIS00_CIRQ       COMBINER_IRQ(10, 4)
#define SYSMMU_DIS11_CIRQ       COMBINER_IRQ( 9, 5)
#define SYSMMU_DIS10_CIRQ       COMBINER_IRQ( 9, 4)
#define SYSMMU_MFCL1_CIRQ       COMBINER_IRQ( 8, 6)
#define SYSMMU_MFCL0_CIRQ       COMBINER_IRQ( 8, 5)
#define SYSMMU_TV_M01_CIRQ      COMBINER_IRQ( 7, 5)
#define SYSMMU_TV_M00_CIRQ      COMBINER_IRQ( 7, 4)
#define SYSMMU_MDMA11_CIRQ      COMBINER_IRQ( 7, 3)
#define SYSMMU_MDMA10_CIRQ      COMBINER_IRQ( 7, 2)
#define SYSMMU_MDMA01_CIRQ      COMBINER_IRQ( 7, 1)
#define SYSMMU_MDMA00_CIRQ      COMBINER_IRQ( 7, 0)
#define SYSMMU_SSS1_CIRQ        COMBINER_IRQ( 6, 7)
#define SYSMMU_SSS0_CIRQ        COMBINER_IRQ( 6, 6)
#define SYSMMU_RTIC1_CIRQ       COMBINER_IRQ( 6, 5)
#define SYSMMU_RTIC0_CIRQ       COMBINER_IRQ( 6, 4)
#define SYSMMU_MFCR1_CIRQ       COMBINER_IRQ( 6, 3)
#define SYSMMU_MFCR0_CIRQ       COMBINER_IRQ( 6, 2)
#define SYSMMU_ARM1_CIRQ        COMBINER_IRQ( 6, 1)
#define SYSMMU_ARM0_CIRQ        COMBINER_IRQ( 6, 0)
#define SYSMMU_3DNR1_CIRQ       COMBINER_IRQ( 5, 7)
#define SYSMMU_3DNR0_CIRQ       COMBINER_IRQ( 5, 6)
#define SYSMMU_MCUISP1_CIRQ     COMBINER_IRQ( 5, 5)
#define SYSMMU_MCUISP0_CIRQ     COMBINER_IRQ( 5, 4)
#define SYSMMU_SCALERCISP1_CIRQ COMBINER_IRQ( 5, 3)
#define SYSMMU_SCALERCISP0_CIRQ COMBINER_IRQ( 5, 2)
#define SYSMMU_FDISP1_CIRQ      COMBINER_IRQ( 5, 1)
#define SYSMMU_FDISP0_CIRQ      COMBINER_IRQ( 5, 0)
#define SYSMMU_JPEGX1_CIRQ      COMBINER_IRQ( 4, 3)
#define SYSMMU_JPEGX0_CIRQ      COMBINER_IRQ( 4, 2)
#define SYSMMU_ROTATOR1_CIRQ    COMBINER_IRQ( 4, 1)
#define SYSMMU_ROTATOR0_CIRQ    COMBINER_IRQ( 4, 0)
#define SYSMMU_SCALERPISP1_CIRQ COMBINER_IRQ( 3, 7)
#define SYSMMU_SCALERPISP0_CIRQ COMBINER_IRQ( 3, 6)
#define SYSMMU_FIMC_LITE01_CIRQ COMBINER_IRQ( 3, 5)
#define SYSMMU_FIMC_LITE00_CIRQ COMBINER_IRQ( 3, 4)
#define SYSMMU_DISP1_M01_CIRQ   COMBINER_IRQ( 3, 3)
#define SYSMMU_DISP1_M00_CIRQ   COMBINER_IRQ( 3, 2)
#define SYSMMU_FIMC_LITE21_CIRQ COMBINER_IRQ( 3, 1)
#define SYSMMU_FIMC_LITE20_CIRQ COMBINER_IRQ( 3, 0)
#define SYSMMU_GSCL31_CIRQ      COMBINER_IRQ( 2, 7)
#define SYSMMU_GSCL30_CIRQ      COMBINER_IRQ( 2, 6)
#define SYSMMU_GSCL21_CIRQ      COMBINER_IRQ( 2, 5)
#define SYSMMU_GSCL20_CIRQ      COMBINER_IRQ( 2, 4)
#define SYSMMU_GSCL11_CIRQ      COMBINER_IRQ( 2, 3)
#define SYSMMU_GSCL10_CIRQ      COMBINER_IRQ( 2, 2)
#define SYSMMU_GSCL01_CIRQ      COMBINER_IRQ( 2, 1)
#define SYSMMU_GSCL00_CIRQ      COMBINER_IRQ( 2, 0)

#define XEINT0_CIRQ             COMBINER_IRQ(23, 0)
#define XEINT1_CIRQ             COMBINER_IRQ(24, 0)
#define XEINT2_CIRQ             COMBINER_IRQ(25, 0)
#define XEINT3_CIRQ             COMBINER_IRQ(25, 1)
#define XEINT4_CIRQ             COMBINER_IRQ(26, 0)
#define XEINT5_CIRQ             COMBINER_IRQ(26, 1)
#define XEINT6_CIRQ             COMBINER_IRQ(27, 0)
#define XEINT7_CIRQ             COMBINER_IRQ(27, 1)
#define XEINT8_CIRQ             COMBINER_IRQ(28, 0)
#define XEINT9_CIRQ             COMBINER_IRQ(28, 1)
#define XEINT10_CIRQ            COMBINER_IRQ(29, 0)
#define XEINT11_CIRQ            COMBINER_IRQ(29, 1)
#define XEINT12_CIRQ            COMBINER_IRQ(30, 0)
#define XEINT13_CIRQ            COMBINER_IRQ(30, 1)
#define XEINT14_CIRQ            COMBINER_IRQ(31, 0)
#define XEINT15_CIRQ            COMBINER_IRQ(31, 1)



#define EXYNOS5_IRQ_COMBINER_PADDR 0x10440000
#define EXYNOS5_IRQ_COMBINER_SIZE  0x1000

#define EXYNOS_IRQ_COMBINER_PADDR EXYNOS5_IRQ_COMBINER_PADDR
#define EXYNOS_IRQ_COMBINER_SIZE  EXYNOS5_IRQ_COMBINER_SIZE

/**
 * Initialise the IRQ combiner with a provided address for IO access
 * @param[in]  base     The memory address of the combiner registers
 * @param[out] combiner An IRQ combiner structure to populate
 * @return              0 on success.
 */
int exynos_irq_combiner_init(void* base, irq_combiner_t* combiner);

#endif /* _PLATSUPPORT_PLAT_IRQ_COMBINER_H_ */
