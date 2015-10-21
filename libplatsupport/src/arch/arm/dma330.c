/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */



/* This is core ARM IP, but we will limit the compile to EXYNOS5 for now */
#ifdef PLAT_EXYNOS5

#include <platsupport/dma330.h>
#include "string.h"


//#define DMA330_DEBUG
#ifdef DMA330_DEBUG
#define D330(...) printf("DMA330: " __VA_ARGS__)
#else
#define D330(...) do{}while(0)
#endif

/* Debug control */
#define DBGCMD_EXEC            0b00
#define DBGINST_CHDBG          BIT(0)
#define DBGINST_CH(x)          ((x) << 8)

/* Manager Status */
#define MGRSTS_NS              BIT(9)
#define MGRSTS_STATUS(x)       ((x) & 0xf)
#define DBGSTS_BUSY            BIT(0)

/* Channel Status */
#define CHSTS_NS               BIT(21)
#define CHSTS_STATUS(x)        ((x) & 0xf)
#define CHSTS_STOPPED          0b0000
#define CHSTS_EXECUTING        0b0001
#define CHSTS_CACHEMISS        0b0010
#define CHSTS_PCUPDATE         0b0011
#define CHSTS_WFE              0b0100
#define CHSTS_BARRIER          0b0101
#define CHSTS_BUSY             0b0110
#define CHSTS_WFP              0b0111
#define CHSTS_KILLING          0b1000
#define CHSTS_COMPLETING       0b1001
#define CHSTS_COMPLETING_FAULT 0b1110
#define CHSTS_FAULT            0b1111

/* Channel control */
#define CCR_AUTO_INC           BIT(0)
#define CCR_BURST_SIZE(x)      (((x) & 0x7) << 1)
#define CCR_BURST_LEN(x)       (((x) & 0xf) << 4)
#define CCR_PROT_CTRL(x)       (((x) & 0x7) << 8)
#define CCR_CACHE_CTRL(x)      (((x) & 0x7) << 11)
#define CCR_ENDIAN_SWAP_SZ(x)  (((x) & 0xf) << 28)
#define CCR_CFG_DST(x)         ((x) << 14)
#define CCR_CFG_SRC(x)         ((x) <<  0)

/* Fault type */
#define FT_UNDEFINST           BIT(0)
#define FT_OPERAND             BIT(1)
#define FTMG_DMAGO_ERR         BIT(4)
#define FT_EVENT_ERR           BIT(5)
#define FTCH_PERIPH_ERR        BIT(6)
#define FTCH_DATA_ERR          BIT(7)
#define FTCH_FIFO_ERR          BIT(12)
#define FT_PREFETCH            BIT(16)
#define FTCH_WRITE_ERR         BIT(17)
#define FTCH_READ_ERR          BIT(18)
#define FT_DBGINST             BIT(30)
#define FTMG_LOCKUP_ERR        BIT(31)

#define PTR64(x)  (uint64_t)(uintptr_t)(x)

/* DMAC instruction set */
#define DMAI_ADDHW_SRC(x)      /* DMAADDH       */ (((x) << 8) | 0x54)
#define DMAI_ADDHW_DST(x)      /* DMAADDH       */ (((x) << 8) | 0x56)
#define DMAI_END               /* DMAEND        */ (0x00)
#define DMAI_FLASH_PERIPH(x)   /* DMAFLUSHP     */ (((x) << 11) | 0x35)
#define DMAI_GO(ch, pc)        /* DMAGO         */ ((PTR64(pc) << 16) | ((ch) << 8) | 0xa0)
#define DMAI_NS_GO(ch, pc)     /* DMAGO         */ (DMAI_GO(ch, pc) | BIT(1))
#define DMAI_LD                /* DMALD[S|B]    */ (0x04)
#define DMAI_LDS               /* DMALD[S|B]    */ (DMAI_LD | 0x1)
#define DMAI_LDB               /* DMALD[S|B]    */ (DMAI_LD | 0x3)
#define DMAI_LDPS(p)           /* DMALDP<S|B>   */ (((p) << 11) | 0x25)
#define DMAI_LDPB(p)           /* DMALDP<S|B>   */ (DMAI_LDPS(p) | 0x2)
#define DMAI_LP(lc, i)         /* DMALP         */ (((i) <<  8) | 0x20 | ((lc) << 1))
#define DMAI_LPFEEND(lc, jmp)  /* DMALPEND[S|B] */ ((PTR64(jmp) << 8) | 0x28 | ((lc) << 2))
#define DMAI_LPEND(lc, jmp)    /* DMALPEND[S|B] */ (DMAI_LPFEEND(lc, jmp) | BIT(4))
#define DMAI_LPENDS(lc, jmp)   /* DMALPEND[S|B] */ (DMAI_LPEND(lc, jmp) | 0x1)
#define DMAI_LPENDB(lc, jmp)   /* DMALPEND[S|B] */ (DMAI_LPEND(lc, jmp) | 0x3)
#define DMAI_LPFEENDS(lc, jmp) /* DMALPEND[S|B] */ (DMAI_LPFEEND(lc, jmp) | 0x1)
#define DMAI_LPFEENDB(lc, jmp) /* DMALPEND[S|B] */ (DMAI_LPFEEND(lc, jmp) | 0x3)
/* DMALPFE       */
/* DMAKILL       */
#define DMAI_MOV_SAR(a)        /* DMAMOV        */ ((PTR64(a) << 16) | 0xBC)
#define DMAI_MOV_DAR(a)        /* DMAMOV        */ (DMAI_MOV_SAR(a) | (0x2 << 8))
#define DMAI_MOV_CCR(a)        /* DMAMOV        */ (DMAI_MOV_SAR(a) | (0x1 << 8))
#define DMAI_NOP               /* DMANOP        */ (0x18)
/* DMARMB        */
#define DMAI_SEV(e)            /* DMASEV        */ (((e) << 11) | 0x34)
#define DMAI_ST                /* DMAST[S|B]    */ (0x08)
#define DMAI_STS               /* DMAST[S|B]    */ (0x09)
#define DMAI_STB               /* DMAST[S|B]    */ (0x0B)
/* DMASTP<S|B>   */
/* DMASTZ        */
/* DMAWFE        */
/* DMAWFP<S|B|P> */
#define DMAI_WMB             /* DMAWMB        */ (0x13)

/* DMAC instruction sizes */
#define DMAISZ_ADDHW_SRC(...)    3
#define DMAISZ_ADDHW_DST(...)    3
#define DMAISZ_END               1
#define DMAISZ_FLASH_PERIPH(...) 2
#define DMAISZ_GO(...)           6
#define DMAISZ_NS_GO(...)        6
#define DMAISZ_LD                1
#define DMAISZ_LDS               1
#define DMAISZ_LDB               1
#define DMAISZ_LDPS(...)         2
#define DMAISZ_LDPB(...)         2
#define DMAISZ_LP(...)           2
#define DMAISZ_LPEND(...)        2
#define DMAISZ_LPENDS(...)       2
#define DMAISZ_LPENDB(...)       2
/* DMALPFE       */
/* DMAKILL       */
#define DMAISZ_MOV_SAR(...)      6
#define DMAISZ_MOV_DAR(...)      6
#define DMAISZ_MOV_CCR(...)      6
#define DMAISZ_NOP               1
/* DMARMB        */
#define DMAISZ_SEV(...)          2
#define DMAISZ_ST                1
#define DMAISZ_STS               1
#define DMAISZ_STB               1
/* DMASTP<S|B>   */
/* DMASTZ        */
/* DMAWFE        */
/* DMAWFP<S|B|P> */
#define DMAISZ_WMB               1


#define APPEND_INSTRUCTION(buf, op)                   \
    do {                                              \
        uint64_t code = DMAI_##op;                    \
        int i;                                        \
        /* memcpy in muslc causes alignment faults */ \
        for(i = 0; i < DMAISZ_##op; i++) {            \
            *buf++ = code;                            \
            code >>= 8;                               \
        }                                             \
    } while(0)

typedef volatile struct dma330_map {
    /* 0x000 */
    struct {
        uint32_t dsr;           /* RO */
        uint32_t dpc;           /* RO */
        uint32_t reserved0[6];
        uint32_t inten;         /* RW */
        uint32_t int_event_ris; /* RO */
        uint32_t intmis;        /* RO */
        uint32_t intclr;        /* WO */
        uint32_t fsm;           /* RO */
        uint32_t fsc;           /* RO */
        uint32_t ftm;           /* RO */
        uint32_t reserved1[1];
        uint32_t ftc[8];        /* RO */
        uint32_t reserved2[40];
    } ctrl;
    /* 0x100 */
    struct {
        uint32_t csr;           /* RO */
        uint32_t cpc;           /* RO */
    } chstat[8];
    uint32_t reserved0[176];
    /* 0x400 */
    struct dma330_axi_map {
        uint32_t sar;           /* RO */
        uint32_t dar;           /* RO */
        uint32_t ccr;           /* RO */
        uint32_t lc[2];         /* RO */
        uint32_t reserved[3];
    } axi[8];
    uint32_t reserved1[512];
    /* 0xd00 */
    struct {
        uint32_t dbgstatus;     /* RO */
        uint32_t dbgcmd;        /* WO */
        uint32_t dbginst[2];    /* WO */
        uint32_t reserved[60];
    } debug;
    /* 0xe00 */
    struct {
        uint32_t cr[5];         /* RO */
        uint32_t crd;           /* RO */
        uint32_t reserved[58];
    } config;
    /* 0xf00 */
    uint32_t reserved2[56];
    /* 0xfe0 */
    struct {
        uint32_t periph[4];     /* RO */
        uint32_t pcell[4];      /* RO */
    } id;
} dma330_map_t;


struct channel_data {
    dma330_signal_cb cb;
    void* token;
};

struct dma330_dev {
    dma330_map_t* regs;
    struct channel_data channel_data[8];
} _dma330_dev[NPL330];

static inline int
dmac_busy(dma330_t dma330)
{
    dma330_map_t* regs = dma330->regs;
    return !!(regs->debug.dbgstatus & DBGSTS_BUSY);
}

static inline uintptr_t
dmac_get_pc(dma330_t dma330, int channel)
{
    dma330_map_t* regs = dma330->regs;
    return regs->chstat[channel].cpc;
}

static inline uint32_t
dmac_has_fault(dma330_t dma330, int channel)
{
    dma330_map_t* regs = dma330->regs;
    if (channel < 0) {
        return regs->ctrl.fsm & BIT(0);
    } else {
        return regs->ctrl.fsc & BIT(channel);
    }
}

static inline uint32_t
dmac_get_status(dma330_t dma330, int channel)
{
    dma330_map_t* regs = dma330->regs;
    if (channel < 0) {
        return regs->ctrl.dsr;
    } else {
        return regs->chstat[channel].csr;
    }
}

static inline uint32_t
dmac_get_fault_type(dma330_t dma330, int channel)
{
    dma330_map_t* regs = dma330->regs;
    if (channel < 1) {
        return regs->ctrl.ftm;
    } else {
        return regs->ctrl.ftc[channel];
    }
}

UNUSED static void
dmac_channel_dump(dma330_t dma330, int channel)
{
    dma330_map_t* regs = dma330->regs;
    char* sts_str;
    char* sec_str;
    uint32_t v, sts;
    int i = channel;
    if (i < 0) {
        v = regs->ctrl.dsr;
        sts = MGRSTS_STATUS(v);
        sec_str = (v & MGRSTS_NS) ? "non-secure, " : "secure, ";
    } else {
        v = regs->chstat[i].csr;
        sts = CHSTS_STATUS(v);
        sec_str = (v & CHSTS_NS) ? "non-secure, " : "secure, ";
    }
    switch (sts) {
    case CHSTS_STOPPED:
        sts_str = "stopped";
        sec_str = "";
        break;
    case CHSTS_EXECUTING:
        sts_str = "running";
        break;
    case CHSTS_CACHEMISS:
        sts_str = "cache miss";
        break;
    case CHSTS_PCUPDATE:
        sts_str = "updating PC";
        break;
    case CHSTS_WFE:
        sts_str = "waiting for event";
        break;
    case CHSTS_BARRIER:
        sts_str = "waiting for barrier completion";
        break;
    case CHSTS_BUSY:
        sts_str = "queue busy";
        break;
    case CHSTS_WFP:
        sts_str = "waiting for peripheral";
        break;
    case CHSTS_KILLING:
        sts_str = "killing";
        break;
    case CHSTS_COMPLETING:
        sts_str = "completing";
        break;
    case CHSTS_COMPLETING_FAULT:
        sts_str = "fault complete";
        break;
    case CHSTS_FAULT:
        sts_str = "faulting";
        break;
    default:
        sts_str = "<reserved>";
        break;
    }

    if (i < 0) {
        printf("[ Manager ] Status: 0x%08x (%s%s)\n", v, sec_str, sts_str);
        printf(" MGR PC: 0x%08x\n", regs->ctrl.dpc);
        printf("     fs: 0x%08x\n", regs->ctrl.fsm);
        printf("  Fault: 0x%08x\n", regs->ctrl.ftm);
        printf("  fs/ch: 0x%08x\n", regs->ctrl.fsc);
    } else {
        printf("[Channel %d] Status: 0x%08x (%s%s)\n", i, v, sec_str, sts_str);
        printf("     PC: 0x%08x\n", regs->chstat[i].cpc);
        v = regs->axi[i].sar;
        printf("    SRC: 0x%08x\n", v);
        v = regs->axi[i].dar;
        printf("    DST: 0x%08x\n", v);
        v = regs->axi[i].lc[0];
        printf("  LOOP0: 0x%08x\n", v);
        v = regs->axi[i].lc[1];
        printf("  LOOP1: 0x%08x\n", v);
        v = regs->axi[i].ccr;
        printf(" Config: 0x%08x\n", v);
        v = regs->ctrl.ftc[i];
        printf("  Fault: 0x%08x\n", v);
    }
}

UNUSED static void
dmac_dump(dma330_t dma330)
{
    dma330_map_t* regs = dma330->regs;
    uint32_t v;
    int i;
    printf("#### DMA330 ####\n");
    v = regs->debug.dbgstatus;
    printf("dbg_sts: 0x%08x (%s)\n", v, (v & DBGSTS_BUSY) ? "busy" : "idle");
    v = regs->ctrl.inten;
    printf("INT  en: 0x%08x\n", v);
    v = regs->ctrl.int_event_ris;
    printf("INT evt: 0x%08x\n", v);
    v = regs->ctrl.intmis;
    printf("INT mis: 0x%08x\n", v);
    v = regs->ctrl.intclr;
    printf("INT clr: 0x%08x\n", v);
    v = regs->config.cr[0];
    printf("Config0: 0x%08x\n", v);
    v = regs->config.cr[1];
    printf("Config1: 0x%08x\n", v);
    v = regs->config.cr[2];
    printf("Config2: 0x%08x\n", v);
    v = regs->config.cr[3];
    printf("Config3: 0x%08x\n", v);
    v = regs->config.cr[4];
    printf("Config4: 0x%08x\n", v);
    v = regs->config.crd;
    printf("Configd: 0x%08x\n", v);
    printf("---------------\n");

    for (i = -1; i < 8; i++) {
        dmac_channel_dump(dma330, i);
    }
    printf("################\n");
}

UNUSED static void
dmac_print_fault(dma330_t dma330, int channel)
{
    dma330_map_t* regs = dma330->regs;
    uint32_t ft, src, dst, pc;
    if (channel < 0) {
        ft = regs->ctrl.ftm;
        pc = regs->ctrl.dpc;
        src = dst = 0;
    } else {
        ft = regs->ctrl.ftc[channel];
        pc = regs->chstat[channel].cpc;
        src = regs->axi[channel].sar;
        dst = regs->axi[channel].dar;
    }
    if (ft & FT_DBGINST) {
        pc = 0xdeadbeef;
    }

    printf("DMAC fault @ 0x%08x: ", pc);
    if (ft & FT_UNDEFINST) {
        printf("Undefined instruction. ");
    }
    if (ft & FT_OPERAND) {
        printf("Inavid operand. ");
    }
    if (ft & FTMG_DMAGO_ERR) {
        printf("DMAGO error. ");
    }
    if (ft & FT_EVENT_ERR) {
        printf("Event error. ");
    }
    if (ft & FTCH_PERIPH_ERR) {
        printf("Peripheral error. ");
    }
    if (ft & FTCH_DATA_ERR) {
        printf("Data error. ");
    }
    if (ft & FTCH_FIFO_ERR) {
        printf("FIFO error. ");
    }
    if (ft & FT_PREFETCH) {
        printf("Prefetch error. ");
    }
    if (ft & FTCH_WRITE_ERR) {
        printf("Write error to 0x%08x. ", dst);
    }
    if (ft & FTCH_READ_ERR) {
        printf("Read error from 0x%08x. ", src);
    }
    if (ft & FTMG_LOCKUP_ERR) {
        printf("Lockup error. ");
    }
    printf("\n");
}


UNUSED static void
program_dump(void *vbin)
{
    uint8_t* bin = (uint8_t*)vbin;
    int i = 0;
    printf("DMAC program @ 0x%08x", (uint32_t)bin);
    while ((bin[0] | bin[1] | bin[2] | bin[3] | bin[4] | bin[5]) != 0) {
        if ((i % 4) == 0) {
            printf("\n0x%03x: ", i);
        }
        printf("0x%02x ", *bin++);
        i++;
    }
    printf("\n----\n");
}

static void
dmac_exec(dma330_t dma330, uint64_t instruction, int channel)
{
    dma330_map_t* regs = dma330->regs;
    uint32_t inst1, inst0;
    inst1 = instruction >> 16;
    inst0 = instruction << 16;
    if (channel < 0) {
        /* Manager thread, no extra bits to select */
    } else if (channel < 8) {
        inst0 |= DBGINST_CHDBG;
        inst0 |= DBGINST_CH(channel);
    } else {
        assert(!"Invalid channel");
    }
    regs->debug.dbginst[0] = inst0;
    regs->debug.dbginst[1] = inst1;

    while (dmac_busy(dma330));
    regs->debug.dbgcmd = DBGCMD_EXEC;
}

int
dma330_init_base(enum dma330_id id, void* dma330_base, clock_sys_t* clk_sys, dma330_t* dma330)
{
    assert(sizeof(struct dma330_map) == 0x1000);
    assert(id >= 0);
    assert(id < NPL330);
    assert(dma330_base);
    assert(clk_sys);
    assert(dma330);

    if (dma330_base == NULL) {
        return -1;
    } else {
        struct dma330_dev* dev;
        uint32_t v;
        dev = &_dma330_dev[id];
        *dma330 = dev;
        memset(dev, 0, sizeof(*dev));
        dev->regs = dma330_base;
        /* Check peripheral ID */
        v = 0;
        v |= dev->regs->id.periph[0] << 0;
        v |= dev->regs->id.periph[1] << 8;
        v |= dev->regs->id.periph[2] << 16;
        v |= dev->regs->id.periph[3] << 24;
        if ((v & 0x000fffff) != 0x41330) {
            LOG_ERROR("Invalid peripheral ID for DMA330\n");
            return -1;
        }
        /* Check primecell ID */
        v = 0;
        v |= dev->regs->id.pcell[0] << 0;
        v |= dev->regs->id.pcell[1] << 8;
        v |= dev->regs->id.pcell[2] << 16;
        v |= dev->regs->id.pcell[3] << 24;
        if (v != 0xB105F00D) {
            LOG_ERROR("Invalid PrimeCell ID for DMA330\n");
            return -1;
        }
        /* Success! */
        return 0;
    }
};

int
dma330_init(enum dma330_id id, struct ps_io_ops* ops, dma330_t* dma330)
{
    void* base;
    assert(dma330);
    assert(ops);
    assert(id >= 0);
    assert(id < NPL330);
    if (_dma330_dev[id].regs == NULL) {
        uintptr_t pbase = dma330_paddr[id];
        base = ps_io_map(&ops->io_mapper, pbase, DMA330_SIZE, 0, PS_MEM_NORMAL);
        return dma330_init_base(id, base, &ops->clock_sys, dma330);
    } else {
        *dma330 = &_dma330_dev[id];
        return 0;
    }
};

int
dma330_xfer(dma330_t* dma330_ptr, int ch, uintptr_t program, dma330_signal_cb cb, void* token)
{
    dma330_t dma330 = *dma330_ptr;
    if (ch < 0 || ch > 8) {
        return -1;
    }
    if (dma330->channel_data[ch].cb != NULL) {
        return -1;
    }
    if (CHSTS_STATUS(dmac_get_status(dma330, ch)) != CHSTS_STOPPED) {
        return -1;
    }

    D330("Executing 0x%x on channel %d\n", program, ch);
    dma330->channel_data[ch].cb = cb;
    dma330->channel_data[ch].token = token;

    dmac_exec(dma330, DMAI_NS_GO(ch, program), -1);
    if (cb == NULL) {
        UNUSED uint32_t status;
        while (dmac_get_status(dma330, ch) == CHSTS_EXECUTING);
        status = CHSTS_STATUS(dmac_get_status(dma330, ch));
        D330("Transfer @ 0x%x completed with status: 0x%x\n", program, status);
        if (dmac_has_fault(dma330, ch)) {
            dmac_print_fault(dma330, ch);
        }
    } else {
        dma330->regs->ctrl.inten |= (0xf << (ch * 4));
    }

    return 0;
}

int
dma330_handle_irq(dma330_t* dma330_ptr)
{
    dma330_t dma330 = *dma330_ptr;
    uint32_t int_stat;

    int_stat = dma330->regs->ctrl.intmis;
    while (int_stat) {
        struct channel_data *cdata;
        int resume;
        int sig, ch;
        /* Search the bitfield for the next signal and determine its owner */
        sig = CTZ(int_stat);
        ch = sig / 4;
        sig &= 0x3;
        cdata = &dma330->channel_data[ch];

        D330("IRQ: Channel %d.%d\n", ch, sig);

        /* We should only get an IRQ if a callback is registered */
        assert(cdata->cb);
        if (cdata->cb) {
            uintptr_t pc = dma330->regs->chstat[ch].cpc;
            uint32_t stat = dma330->regs->chstat[ch].csr;
            /* Call the provided callback */
            resume = cdata->cb(dma330_ptr, sig, pc, stat, cdata->token);
        } else {
            resume = 0;
        }
        /* Clean up the transfer */
        if (!resume || dmac_get_status(dma330, ch) == CHSTS_STOPPED) {
            uint32_t irq_mask = (0xf << (ch * 4));
            dma330->regs->ctrl.intclr = irq_mask;
            dma330->regs->ctrl.inten &= ~irq_mask;
            int_stat &= ~irq_mask;
            cdata->cb = NULL;
            cdata->token = NULL;
        } else {
            dma330->regs->ctrl.intclr = BIT(sig);
            int_stat &= ~BIT(sig);
        }
    }
    return 0;
}

/****************************
 *** Compiler and presets ***
 ****************************/

int
dma330_compile(char* source_code, void* bin)
{
    assert(!"Not implemented");
    return -1;
}

void
dma330_copy_compile(int channel, void* vbin)
{
#if 0
    /* Reserved bytes */
    "DMAMOV src, 0x00000000;"   /* Set source        */
    "DMAMOV dst, 0x00000000;"   /* Set destination   */
    "DMAMOV cfg, 0x00000000;"   /* Set configuration */
    /* Program code */
    "DMALP  cnt, 0x00;"         /* Loop X times      */
    " DMALD"                    /* Load from src     */
    " DMAST"                    /* Store to dst      */
    "DMALPEND"                  /* Loop end          */
    "DMAWMB"                    /* Write barrier     */
    "DMAEND"                    /* End program       */
#endif
    uint8_t* bin = (uint8_t*)vbin;
    uint8_t* loop0;
    /* Place holders for configuration */
    APPEND_INSTRUCTION(bin, MOV_SAR(0));
    APPEND_INSTRUCTION(bin, MOV_DAR(0));
    APPEND_INSTRUCTION(bin, MOV_CCR(0));
    APPEND_INSTRUCTION(bin, LP(0, 0));

    (loop0 = bin);
    {
        APPEND_INSTRUCTION(bin, LD);
        APPEND_INSTRUCTION(bin, ST);
    }
    APPEND_INSTRUCTION(bin, LPEND(0, bin - loop0));

    APPEND_INSTRUCTION(bin, WMB);
    APPEND_INSTRUCTION(bin, SEV(channel * 4));
    APPEND_INSTRUCTION(bin, END);
}

int
dma330_copy_configure(uintptr_t pdst, uintptr_t psrc, size_t len, void* vbin)
{
    D330("Copy configure @ 0x%x: 0x%x -> 0x%x (%d bytes)\n",
         (uint32_t)vbin, (uint32_t)psrc, (uint32_t)pdst, len);
    char* bin = (char*)vbin;
    uint32_t cfg = 0;
    cfg |= CCR_CFG_SRC(CCR_PROT_CTRL(2) | CCR_AUTO_INC);
    cfg |= CCR_CFG_DST(CCR_PROT_CTRL(2) | CCR_AUTO_INC);

    if (len > 255) {
        return -1;
    } else if (len <= 0) {
        return -1;
    } else {
        APPEND_INSTRUCTION(bin, MOV_SAR(psrc));
        APPEND_INSTRUCTION(bin, MOV_DAR(pdst));
        APPEND_INSTRUCTION(bin, MOV_CCR(cfg));
        APPEND_INSTRUCTION(bin, LP(0, len - 1));
    }

    return 0;
}

#endif

