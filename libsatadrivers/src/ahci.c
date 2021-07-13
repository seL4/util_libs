/*
 * Copyright 2019, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * A lot of this code was based on code found at http://wiki.osdev.org/AHCI
 * which is in the public domain.
 */
#include <satadrivers/ide.h>
#include <satadrivers/ahci.h>
#include <unistd.h>
#include <string.h>
#include <sel4/sel4.h>
#include <platsupport/delay.h>

#include <satadrivers/ahci_types.h>
#include <utils/zf_log.h>

#define GET_LOWER_32BITS(val) ((uint32_t)(val))
#define GET_UPPER_32BITS(val) ((uint32_t)((val) >> 32))

typedef struct ahci_port {
    uint8_t in_use: 1;
    uint8_t port_num: 7;
    uint32_t clb;
    uint32_t ctba;
    uint32_t fb;
} ahci_port_t;

typedef struct ahci_dev {
    hba_mem_t *abar;
    hba_cmd_hdr_t *clb;
    uint32_t clb_size;
    uint32_t *ctba;
    uint32_t ctba_size;
    uint32_t *fb;
    uint32_t fb_size;
    uint8_t *buf;
    uint32_t buf_size;
    ps_io_ops_t *io_ops;
} ahci_dev_t;

ahci_dev_t g_ahci;
ahci_port_t device_list[NUM_MAX_PORTS] = { 0 };

const char *drive_type_str[DEV_NUM_DRIVE_TYPES] = {
    "SATA",
    "SATAPI",
    "SEMB",
    "PM",
};

const char *error_str[17] = {
    "Recovered Data Integrity",
    "Recovered Communication",
    "Transient Data Integrity",
    "Persistent Com or Data Integrity",
    "Protocol",
    "Internal",
    "PhyRdy Change",
    "Phy Internal",
    "Comm Wake",
    "10B to 8B Decode",
    "Disparity",
    "CRC",
    "Handshake",
    "Link Sequence",
    "Transport state transition",
    "Unknown FIS Type",
    "Exchanged",
};

int probe_ports(hba_mem_t *abar);
void start_cmd(hba_port_t *port);
void stop_cmd(hba_port_t *port);

static int find_cmdslot(hba_port_t *port);
static int check_type(hba_port_t *port);
static int reset_ahci_controller(hba_port_t *port);
static int soft_reset_port(hba_port_t *port);
static int hard_reset_controller(void);

static int construct_cmd_header(hba_cmd_hdr_t *cmdheader, uint8_t command, uint16_t num_sects);
static int construct_cmd_tbl(hba_cmd_tbl_t *cmdtbl, uint16_t num_sects, uint16_t prdt_len, uint8_t *data_buf);
static int construct_cmd_fis(fis_reg_h2d_t *cmdfis, uint8_t command, uint16_t num_sects, uint32_t lba);

static int validate_config(ahci_intel_config_t *config);
static int validate_memory_space(int active_ports);

/* Debugging Functions */
static void print_port_status(hba_port_t *port);
static void print_port_error(hba_port_t *port);
static void print_global_status(void);
static void print_port_base_addresses(hba_port_t *port);
static void check_for_errors(hba_port_t *port);

/*
 * Purpose: Used to probe the ports for connected devices
 *
 * Inputs:
 *   - *abar: a pointer to the memory space containing the AHCI controller registers
 *
 * Returns: number of active ports
 *
 */
int probe_ports(hba_mem_t *abar)
{
    // Search disk in impelemented ports
    int i = 0;
    int active_count = 0;

    uint32_t pi = abar->pi;

    for (i = 0; i < NUM_MAX_PORTS; i++) {
        if (pi & 1) {
            int dt = check_type((hba_port_t *)&abar->ports[i]);
            if ((DEV_NULL < dt) && (DEV_NUM_DRIVE_TYPES >= dt)) {
                ZF_LOGV("%s drive found at port %d", drive_type_str[dt - 1], i);
                device_list[active_count].port_num = i;
                active_count++;
            } else {
                ZF_LOGV("No drive found at port %d", i);
            }
        }

        pi >>= 1;
    }

    return active_count;
}

/*
 * Purpose: Used to check the device type
 *
 * Inputs:
 *   - *port: a pointer to the memory space that contains the port specific registers
 *
 * Returns: success (0) or failure (error code)
 *
 */
static int check_type(hba_port_t *port)
{
    int spin = 0;

    ZF_LOGV("SSTS: %x", port->ssts);

    uint8_t det = GET_DET_BITS(port->ssts);

    ZF_LOGV("DET: %x", det);

    switch (det) {
    case HBA_PORT_DET_NO_DEV:
        return DEV_NULL;
    case HBA_PORT_DET_PHY_OFFLINE:
        ZF_LOGV("Device is offline for some reason");
        return DEV_NULL;
    case HBA_PORT_DET_PRESENT_NO_COM:
        spin = 0;

        while ((GET_DET_BITS(port->ssts) != HBA_PORT_DET_PRESENT_AND_COM) && (TIMEOUT_10S > spin)) {
            ps_mdelay(1);
            spin++;
        }
        if (TIMEOUT_10S <= spin) {
            ZF_LOGV("AHCI: Port communication is hung");
            return DEV_NULL;
        }
        ZF_LOGV("NEW DET: %x", GET_DET_BITS(port->ssts));
    case HBA_PORT_DET_PRESENT_AND_COM:
        break;
    default:
        ZF_LOGV("AHCI: Port DET is not a valid value");
        return DEV_NULL;
    }

    uint8_t ipm = GET_IPM_BITS(port->ssts);
    ZF_LOGV("IPM: %x", ipm);

    switch (ipm) {
    case HBA_PORT_IPM_NO_DEV:
        return DEV_NULL;
    case HBA_PORT_IPM_PARTIAL_PWR:
        ZF_LOGV("Device is in Partial power management state");
        return DEV_NULL;
    case HBA_PORT_IPM_SLUMBER_PWR:
        ZF_LOGV("Device is in Slumber power management state");
        return DEV_NULL;
    case HBA_PORT_IPM_DEVSLEEP_PWR:
        ZF_LOGV("Device is in DevSleep power management state");
        return DEV_NULL;
    case HBA_PORT_IPM_ACTIVE:
        break;
    default:
        ZF_LOGV("Device is not in a valid power state");
        return DEV_NULL;
    }

    switch (port->sig) {
    case SATA_SIG_ATAPI:
        return DEV_SATAPI;
    case SATA_SIG_SEMB:
        return DEV_SEMB;
    case SATA_SIG_PM:
        return DEV_PM;
    default:
        return DEV_SATA;
    }
}

/*
 * Purpose: Used to relocate the command and data structure buffers used for communication
 *
 * Inputs:
 *   - *port: a pointer to the memory space that contains the port specific registers
 *   - io_slot: the slot in memory that io buffers will be rebased to
 *
 * Returns: success (0) or failure (error code)
 *
 */
int port_rebase(hba_port_t *port, int io_slot)
{
    ZF_LOGV("AHCI: port rebase");

    if ((0 > io_slot) || (NUM_MAX_PORTS < io_slot)) {
        return AHCI_INVALID_NUM_ERR;
    }

    stop_cmd(port); // Stop command engine

    // Command list offset: 1K*portno
    // Command list entry size = 32
    // Command list entry maxim count = 32
    // Command list maxim size = 32*32 = 1K per port
    ZF_LOGV("AHCI: rebase clb");
    uint64_t port_clb = (uint64_t)((uint8_t *)g_ahci.clb + (io_slot * CMD_LIST_SIZE));
    port->clb = GET_LOWER_32BITS(port_clb);
    port->clbu = GET_UPPER_32BITS(port_clb);
    memset((void *)port_clb, 0, CMD_LIST_SIZE);


    // FIS offset: 256*portno
    // FIS entry size = 256 bytes per port
    ZF_LOGV("AHCI: rebase fb");
    uint64_t port_fb = (uint64_t)((uint8_t *)g_ahci.fb + (io_slot * RCV_FIS_SIZE));
    port->fb = GET_LOWER_32BITS(port_fb);
    port->fbu = GET_UPPER_32BITS(port_fb);
    memset((void *)port_fb, 0, RCV_FIS_SIZE);

    // Command table offset: 8K*portno
    // Command table size = 256*32 = 8K per port
    ZF_LOGV("AHCI: rebase ctba");
    hba_cmd_hdr_t *cmdheader = (hba_cmd_hdr_t *)port_clb;

    uint64_t ctba_base = (uint64_t)((uint8_t *)g_ahci.ctba
                                    + (io_slot * ENTRIES_PER_CMDLST * CMD_TBL_SIZE));

    for (int i = 0; i < ENTRIES_PER_CMDLST; i++) {
        cmdheader[i].prdtl = NUM_PRDT_ENTRIES; // 8 prdt entries per command table
        // 256 bytes per command table
        // Command table offset: 8K*portno + cmdheader_index*256
        uint64_t port_ctba = ctba_base + (i * CMD_TBL_SIZE);
        cmdheader[i].ctba = GET_LOWER_32BITS(port_ctba);
        cmdheader[i].ctbau = GET_UPPER_32BITS(port_ctba);

        memset((void *)port_ctba, 0, CMD_TBL_SIZE);
    }

    start_cmd(port);    // Start command engine

    print_port_status(port);

    return AHCI_NO_ERR;
}

/*
 * Purpose: Used to start the AHCI controllers DMA engine for a specific port
 *
 * Inputs:
 *   - *port: a pointer to the memory space that contains the port specific registers
 *
 * Returns: nothing
 *
 */
void start_cmd(hba_port_t *port)
{
    ZF_LOGV("AHCI: start port DMA engine");

    // Wait until CR (bit15) is cleared
    while (port->cmd & HBA_PxCMD_CR);

    // Set FRE (bit4) and ST (bit0)
    port->cmd |= HBA_PxCMD_FRE;

    ps_mdelay(500);

    port->cmd |= HBA_PxCMD_ST;
}

/*
 * Purpose: Used to stop the AHCI controllers DMA engine for a specific port
 *
 * Inputs:
 *   - *port: a pointer to the memory space that contains the port specific registers
 *
 * Returns: nothing
 *
 */
void stop_cmd(hba_port_t *port)
{
    ZF_LOGV("AHCI: stop port DMA engine");

    // Clear ST (bit0)
    port->cmd &= ~HBA_PxCMD_ST;

    // Wait until CR (bit15) is cleared
    while (port->cmd & HBA_PxCMD_CR);

    // Clear FRE (bit4)
    port->cmd &= ~HBA_PxCMD_FRE;

    // Wait until FR (bit14) is cleared
    while (port->cmd & HBA_PxCMD_FR);
}

/*
 * Purpose: Used to create and execute a read or write command
 *
 * Inputs:
 *   - command: the command to be executed (read or write)
 *   - drive: the port number of the device to execute the command on
 *   - lba: the address at which to start reading or writing
 *   - count: the length of the data to be read or written
 *   - *buf: the buffer used to transfer or receive data from the device
 *
 * Returns: success (0) or failure (error code)
 *
 */
int ahci_exec_cmd(uint8_t command, uint8_t drive, uint32_t lba, uint16_t count, uint8_t *buf)
{
    int spin = 0; // Spin lock timeout counter
    int slot = 0;
    uint8_t *bufptr = g_ahci.buf;
    int i;
    int error = AHCI_NO_ERR;

    hba_port_t *port = &g_ahci.abar->ports[device_list[drive].port_num];
    port->is = (uint32_t) -1;       // Clear pending interrupt bits

    check_for_errors(port);
    // Find free slot in port's command list
    slot = find_cmdslot(port);
    if (-1 == slot) {
        ZF_LOGE("AHCI: Command List is full");
        ZF_LOGE("PxSACT: 0x%x", port->sact);
        ZF_LOGE("PxCI: 0x%x", port->ci);
        ZF_LOGE("PxCMD_ST: 0x%x", port->cmd);
        ZF_LOGE("PxIS: 0x%x", port->is);
        ZF_LOGE("PxTFD: 0x%x", port->tfd);
        ZF_LOGE("PxSERR: 0x%x", port->serr);
        return AHCI_CMDLST_FULL_ERR;
    }

    // Verify g_ahci.buf can handle transfer size requested
    if ((count * SATA_BLK_SIZE) > g_ahci.buf_size) {
        ZF_LOGE("AHCI: Requested transfer exceeds the data buffer size");
        return AHCI_CMD_FAILED_ERR;
    }
    // Construct command header in free slot
    uint64_t port_clb = ((uint64_t)port->clbu << 32) + (uint64_t)port->clb;
    hba_cmd_hdr_t *cmdheader = (hba_cmd_hdr_t *)port_clb;
    cmdheader += slot;

    error = construct_cmd_header(cmdheader, command, count);
    if (AHCI_NO_ERR != error) {
        return error;
    }

    // Construct command table
    hba_cmd_tbl_t *cmdtbl = (hba_cmd_tbl_t *)(((uint64_t)cmdheader->ctbau << 32) + (uint64_t)cmdheader->ctba);

    error = construct_cmd_tbl(cmdtbl, count, cmdheader->prdtl, bufptr);
    if (AHCI_NO_ERR != error) {
        return error;
    }

    // Setup command structure
    fis_reg_h2d_t *cmdfis = (fis_reg_h2d_t *) &cmdtbl->cfis;

    error = construct_cmd_fis(cmdfis, command, count, lba);
    if (AHCI_NO_ERR != error) {
        return error;
    }

    if (ATA_WRITE == command) {
        memcpy(g_ahci.buf, buf, cmdheader->prdtl * BYTES_PER_PRDT);
    }

    // The below loop waits until the port is no longer busy before issuing a new command
    while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && (TIMEOUT_10S > spin)) {
        ps_mdelay(1);
        spin++;
    }
    if (TIMEOUT_10S == spin) {
        ZF_LOGE("AHCI: Port is hung. Try increasing the TIMEOUT_10S cycles");
        return AHCI_PORT_HUNG_ERR;
    }

    port->ci = 1 << slot; // Issue command

    // Reset timeout counter
    spin = 0;
    // Wait for completion
    while (1) {
        if ((port->ci & (1 << slot)) == 0) {
            break;
        }
        if (port->is & HBA_PxIS_TFES) {  // Task file error
            ZF_LOGE("AHCI: Read disk error");
            return AHCI_READ_DISK_ERR;
        }
        if (port->is & HBA_PxIS_IFS) {  // Interface Fatal error
            ZF_LOGE("AHCI: Fatal interface error");
            print_port_status(port);
            print_port_error(port);
            print_port_base_addresses(port);

            if (AHCI_NO_ERR != reset_ahci_controller(port)) {
                ZF_LOGE("AHCI: Controller did not reset!");
            }
            return AHCI_READ_DISK_ERR;
        }
        if (TIMEOUT_10S < spin) {
            ZF_LOGE("AHCI: Timed out waiting for completion!");
            return AHCI_TIMEOUT_ERR;
        }
        ps_mdelay(1);
        spin++;
    }

    // Check again
    if (port->is & HBA_PxIS_TFES) {
        ZF_LOGE("AHCI: Read disk error");
        return AHCI_READ_DISK_ERR;
    }

    if ((ATA_READ == command) || (ATA_IDENTIFY == command)) {
        memcpy(buf, g_ahci.buf, cmdheader->prdtl * BYTES_PER_PRDT);
    }
    return AHCI_NO_ERR;
}

/*
 * Purpose: Used to fill data into the a command header structure
 *
 * Inputs:
 *   - *cmdheader: a pointer to the command header structure
 *   - command: the command request to be sent (read or write)
 *   - num_sects: the length of the data to be read or written
 *
 * Returns: success (0) or failure (error code)
 *
 */
static int construct_cmd_header(hba_cmd_hdr_t *cmdheader, uint8_t command, uint16_t num_sects)
{
    if (NULL == cmdheader) {
        ZF_LOGE("AHCI: command header is NULL");
        return AHCI_NULL_PTR_ERR;
    }

    if (ATA_READ == command) {
        cmdheader->w = 0;       // Read from device
    } else if (ATA_WRITE == command) {
        cmdheader->w = 1;       // Write to device
    } else if (ATA_IDENTIFY == command) {
        cmdheader->w = 0;       // Read from device
    } else {
        ZF_LOGE("AHCI: Command not found");
        return AHCI_CMD_NOT_FOUND_ERR;
    }

    cmdheader->cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t); // Command FIS size
    cmdheader->prdtl = PRDT_ENTRIES_NEEDED(num_sects);    // PRDT entries count

    if (NUM_PRDT_ENTRIES < cmdheader->prdtl) {
        ZF_LOGE("AHCI: Command table cannot handle transfer size requested");
        return AHCI_CMD_FAILED_ERR;
    }

    return AHCI_NO_ERR;
}

/*
 * Purpose: Used to fill data into the command table structure
 *
 * Inputs:
 *   - *cmdtbl: a pointer to the command table structure
 *   - num_sects: the length of the data to be read or written
 *   - prdt_len: physical region descriptor table length
 *   - *data_buf: the buffer used to transfer or receive data from the device
 *
 * Returns: success (0) or failure (error code)
 *
 */
static int construct_cmd_tbl(hba_cmd_tbl_t *cmdtbl, uint16_t num_sects, uint16_t prdt_len,
                             uint8_t *data_buf)
{
    int i;

    if (NULL == cmdtbl) {
        ZF_LOGE("AHCI: command table is NULL");
        return AHCI_NULL_PTR_ERR;
    }

    memset(cmdtbl, 0, CMD_TBL_SIZE);

    // 4K bytes (8 sectors) per PRDT
    for (i = 0; i < (prdt_len - 1); i++) {
        cmdtbl->prdt_entry[i].dba = GET_LOWER_32BITS(data_buf);
        cmdtbl->prdt_entry[i].dbau = GET_UPPER_32BITS(data_buf);
        cmdtbl->prdt_entry[i].dbc = DATA_BLK_SIZE;
        cmdtbl->prdt_entry[i].i = 0;
        data_buf += BYTES_PER_PRDT;
        num_sects -= SECTORS_PER_PRDT;    // 8 sectors
    }
    // Last entry
    cmdtbl->prdt_entry[i].dba = GET_LOWER_32BITS(data_buf);
    cmdtbl->prdt_entry[i].dbau = GET_UPPER_32BITS(data_buf);
    cmdtbl->prdt_entry[i].dbc = SECTORS_TO_BYTES(num_sects);
    cmdtbl->prdt_entry[i].i = 1;

    return AHCI_NO_ERR;
}

/*
 * Purpose: Used to fill data into a command frame information structure
 *
 * Inputs:
 *   - *cmdfis: a pointer to the command frame information structure
 *   - command: the command request to be sent (read or write)
 *   - num_sects: the length of the data to be read or written
 *   - lba: the address at which to start reading or writing
 *
 * Returns: success (0) or failure (error code)
 *
 */
static int construct_cmd_fis(fis_reg_h2d_t *cmdfis, uint8_t command, uint16_t num_sects, uint32_t lba)
{
    if (NULL == cmdfis) {
        ZF_LOGE("AHCI: command frame information structure is NULL");
        return AHCI_NULL_PTR_ERR;
    }

    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // signifies structure is a command

    if (ATA_READ == command) {
        cmdfis->command = ATA_CMD_READ_DMA_EXT;
    } else if (ATA_WRITE == command) {
        cmdfis->command = ATA_CMD_WRITE_DMA_EXT;
    } else if (ATA_IDENTIFY == command) {
        cmdfis->command = ATA_CMD_IDENTIFY;
    } else {
        ZF_LOGE("AHCI: Command not found");
        return AHCI_CMD_NOT_FOUND_ERR;
    }

    // Using LBA 48 mode
    cmdfis->lba0 = GET_BYTE0(lba);
    cmdfis->lba1 = GET_BYTE1(lba);
    cmdfis->lba2 = GET_BYTE2(lba);
    cmdfis->device = 1 << 6; // LBA mode

    cmdfis->lba3 = GET_BYTE3(lba);
    cmdfis->lba4 = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
    cmdfis->lba5 = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.

    cmdfis->countl = GET_BYTE0(num_sects);
    cmdfis->counth = GET_BYTE1(num_sects);

    return AHCI_NO_ERR;
}

/*
 * Purpose: Used to find an empty slot in the command list
 *
 * Inputs:
 *   - *port: a pointer to the memory space that contains the port specific registers
 *
 * Returns: the free slot or failure
 *
 */
static int find_cmdslot(hba_port_t *port)
{
    // If not set in SACT and CI, the slot is free
    uint32_t slots = (port->sact | port->ci);
    int num_slots = GET_NUM_SLOTS(g_ahci.abar->cap);

    for (int i = 0; i < num_slots; i++) {
        if ((slots & 1) == 0) {
            return i;
        }
        slots >>= 1;
    }
    ZF_LOGE("Cannot find free command list entry");
    return -1;
}

/*
 * Purpose: Used to reset the controller on a fatal error
 *
 * Inputs:
 *   - *port: a pointer to the memory space that contains the port specific registers
 *
 * Returns: success or failure (error code)
 *
 */
static int reset_ahci_controller(hba_port_t *port)
{
    ZF_LOGV("AHCI: reset controller");
    int status = 0;

    status = soft_reset_port(port);
    if (AHCI_TIMEOUT_ERR == status) {
        status = hard_reset_controller();
    }

    return status;
}

/*
 * Purpose: Used to soft reset the port on which there is a fatal error
 *
 * Inputs:
 *   - *port: a pointer to the memory space that contains the port specific registers
 *
 * Returns: success or failure (error code)
 *
 */
static int soft_reset_port(hba_port_t *port)
{
    uint32_t tmp;
    int spin = 0;
    int err = AHCI_NO_ERR;

    stop_cmd(port);

    // set PxSCTL.DET to 1
    tmp = ~PxSCTL_DET_MASK & port->sctl;
    port->sctl = tmp | PxSCTL_DET_COMRESET;

    ps_mdelay(1);

    // set PxSCTL.DET to 0
    tmp = ~PxSCTL_DET_MASK & port->sctl;
    port->sctl = tmp;

    // wait for communication to re-establish
    while ((GET_DET_BITS(port->ssts) != HBA_PORT_DET_PRESENT_AND_COM) && (TIMEOUT_10S > spin)) {
        ps_mdelay(1);
        spin++;
    }
    if (TIMEOUT_10S <= spin) {
        ZF_LOGE("AHCI: Timed out on reset");
        err = AHCI_TIMEOUT_ERR;
    }

    // clear errors that were set due to reset
    port->serr |= 0xFFFFFFFF;

    start_cmd(port);

    return err;
}

/*
 * Purpose: Used to hard reset the whole controller on a fatal error
 *
 * Inputs:
 *   - *port: a pointer to the memory space that contains the port specific registers
 *
 * Returns: success or failure (error code)
 *
 */
static int hard_reset_controller(void)
{
    int spin = 0;
    int err = AHCI_NO_ERR;

    g_ahci.abar->ghc |= GHC_HR_BIT;

    // wait HBA reset to complete
    while (((g_ahci.abar->ghc & GHC_HR_BIT) != HBA_RESET_DONE) && (TIMEOUT_1S > spin)) {
        ps_mdelay(1);
        spin++;
    }
    if (TIMEOUT_1S <= spin) {
        ZF_LOGE("AHCI: Hardware is hung or locked");
        err = AHCI_TIMEOUT_ERR;
    } else {
        int active_ports = 0;
        g_ahci.abar->ghc |= GHC_AHCI_EN_BIT;
        active_ports = probe_ports(g_ahci.abar);
        for (int i = 0; i < active_ports; i++) {
            g_ahci.abar->ports[device_list[i].port_num].serr |= 0xFFFFFFFF;
            port_rebase(&g_ahci.abar->ports[device_list[i].port_num], i);
        }
    }

    return err;
}

/*
 * Purpose: Used to initialize the AHCI driver
 *
 * Inputs:
 *   - *io_ops: the io functions
 *   - *config: the driver configuration data
 *
 * Returns: success (0) or failure (error code)
 *
 */
int ahci_init(ps_io_ops_t *io_ops, void *config)
{
    int error = AHCI_NO_ERR;
    int active_ports = 0;
    char mbuf[BUFFER_ID_SPACE] = {0};
    char model[MODEL_STRING_SIZE] = {0};
    ahci_intel_config_t *ahci_config = (ahci_intel_config_t *) config;

    error = validate_config(ahci_config);
    if (AHCI_NO_ERR != error) {
        return error;
    }

    // Connect driver to host memory space
    g_ahci.abar = (ahci_config->bar0);
    g_ahci.clb  = (ahci_config->clb);
    g_ahci.ctba = (ahci_config->ctba);
    g_ahci.fb   = (ahci_config->fb);
    g_ahci.buf  = (ahci_config->data);

    g_ahci.clb_size  = (ahci_config->clb_size);
    g_ahci.ctba_size = (ahci_config->ctba_size);
    g_ahci.fb_size   = (ahci_config->fb_size);
    g_ahci.buf_size  = (ahci_config->data_size);

    g_ahci.io_ops = io_ops;

    // Make sure AHCI controller is enabled
    if (g_ahci.abar->ghc & GHC_AHCI_EN_BIT) {
        ZF_LOGV("AHCI IS ENABLED");
    } else {
        ZF_LOGE("AHCI IS DISABLED");
        return AHCI_NOT_ENABLED_ERR;
    }

    print_global_status();

    active_ports = probe_ports(g_ahci.abar);
    ZF_LOGV("FINISHED PROBING");

    if (NUM_MAX_PORTS < active_ports) {
        return AHCI_INVALID_NUM_ERR;
    }

    error = validate_memory_space(active_ports);
    if (AHCI_NO_ERR != error) {
        return error;
    }

    for (int i = 0; i < active_ports; i++) {
        port_rebase(&g_ahci.abar->ports[device_list[i].port_num], i);
    }

    for (int count = 0; count < active_ports; count++) {
        ZF_LOGV("AHCI IDENTIFY PORT %d", count);
        ahci_exec_cmd(ATA_IDENTIFY, count, 0, 1, mbuf);

        ZF_LOGV("GET MODEL");
        for (int k = 0; k < MODEL_STRING_SIZE - 1; k += 2) {
            model[k] = mbuf[ATA_IDENT_MODEL + k + 1];
            model[k + 1] = mbuf[ATA_IDENT_MODEL + k];
        }
        model[MODEL_STRING_SIZE - 1] = 0; // Terminate String.

        ZF_LOGV("DRIVE %d MODEL: %s", count, model);

        /* Read Device Parameters */
        sata_devices[count].Reserved     = 1;
        sata_devices[count].Type         = DEV_SATA;
        sata_devices[count].Channel      = 0;
        sata_devices[count].Drive        = count;
        memcpy(&sata_devices[count].Signature, mbuf + ATA_IDENT_DEVICETYPE, sizeof(uint16_t));
        memcpy(&sata_devices[count].Capabilities, mbuf + ATA_IDENT_CAPABILITIES, sizeof(uint16_t));
        memcpy(&sata_devices[count].CommandSets, mbuf + ATA_IDENT_COMMANDSETS, sizeof(uint32_t));
        sata_devices[count].io_ops       = io_ops;

        /* Get Size */
        if (sata_devices[count].CommandSets & IDENT_LBA48_SUPPORT_BIT) {
            /* Device uses 48-Bit Addressing */
            memcpy(&sata_devices[count].Size, mbuf + ATA_IDENT_MAX_LBA_EXT, sizeof(uint32_t));
        } else {
            /* Device uses CHS or 28-bit Addressing */
            memcpy(&sata_devices[count].Size, mbuf + ATA_IDENT_MAX_LBA, sizeof(uint32_t));
        }
    }
    ZF_LOGV("INIT COMPLETE");

    return error;
}

static int validate_config(ahci_intel_config_t *config)
{

    int error = AHCI_NO_ERR;

    if (NULL != config) {
        if (NULL == config->bar0) {
            ZF_LOGE("AHCI: BAR0 PTR is NULL");
            error = AHCI_CONFIG_ERR;
        }
        if (NULL == config->clb) {
            ZF_LOGE("AHCI: CLB PTR is NULL");
            error = AHCI_CONFIG_ERR;
        }
        if (NULL == config->ctba) {
            ZF_LOGE("AHCI: CTBA PTR is NULL");
            error = AHCI_CONFIG_ERR;
        }
        if (NULL == config->fb) {
            ZF_LOGE("AHCI: FB PTR is NULL");
            error = AHCI_CONFIG_ERR;
        }
        if (NULL == config->data) {
            ZF_LOGE("AHCI: DATA PTR is NULL");
            error = AHCI_CONFIG_ERR;
        }
        if (0 == config->clb_size) {
            ZF_LOGE("AHCI: CLB size cannot be 0");
            error = AHCI_CONFIG_ERR;
        }
        if (0 == config->ctba_size) {
            ZF_LOGE("AHCI: CTBA size cannot be 0");
            error = AHCI_CONFIG_ERR;
        }
        if (0 == config->fb_size) {
            ZF_LOGE("AHCI: FB size cannot be 0");
            error = AHCI_CONFIG_ERR;
        }
        if (0 == config->data_size) {
            ZF_LOGE("AHCI: DATA size cannot be 0");
            error = AHCI_CONFIG_ERR;
        }
    } else {
        ZF_LOGE("AHCI: No valid config provided");
        error = AHCI_CONFIG_ERR;
    }

    return error;
}

static int validate_memory_space(int active_ports)
{

    int error = AHCI_NO_ERR;

    if (g_ahci.clb_size < (active_ports * CMD_LIST_SIZE)) {
        ZF_LOGE("AHCI: Command List memory space not big enough to handle all active ports");
        error = AHCI_MEM_SIZE_ERR;
    }
    if (g_ahci.ctba_size < (active_ports * ENTRIES_PER_CMDLST * CMD_TBL_SIZE)) {
        ZF_LOGE("AHCI: Command Table memory space not big enough to handle all active ports");
        error = AHCI_MEM_SIZE_ERR;
    }
    if (g_ahci.fb_size < (active_ports * RCV_FIS_SIZE)) {
        ZF_LOGE("AHCI: Recieve FIS memory space not big enough to handle all active ports");
        error = AHCI_MEM_SIZE_ERR;
    }

    return error;
}

static void print_port_status(hba_port_t *port)
{
    ZF_LOGV("PxCLB: 0x%x", port->clb);
    ZF_LOGV("PxCLBU: 0x%x", port->clbu);
    ZF_LOGV("PxFB: 0x%x", port->fb);
    ZF_LOGV("PxFBU: 0x%x", port->fbu);
    ZF_LOGV("PxIS: 0x%x", port->is);
    ZF_LOGV("PxIE: 0x%x", port->ie);
    ZF_LOGV("PxCMD: 0x%x", port->cmd);
    ZF_LOGV("PxTFD: 0x%x", port->tfd);
    ZF_LOGV("PxSIG: 0x%x", port->sig);
    ZF_LOGV("PxSSTS: 0x%x", port->ssts);
    ZF_LOGV("PxSCTL: 0x%x", port->sctl);
    ZF_LOGV("PxSERR: 0x%x", port->serr);
    ZF_LOGV("PxSACT: 0x%x", port->sact);
    ZF_LOGV("PxCI: 0x%x", port->ci);
    ZF_LOGV("PxSNTF: 0x%x", port->sntf);
    ZF_LOGV("PxFBS: 0x%x", port->fbs);
}

static void print_global_status(void)
{
    ZF_LOGV("CAP: %x", g_ahci.abar->cap);
    ZF_LOGV("GHC: %x", g_ahci.abar->ghc);
    ZF_LOGV("IS: %x", g_ahci.abar->is);
    ZF_LOGV("PI: %x", g_ahci.abar->pi);
    ZF_LOGV("VS: %x", g_ahci.abar->vs);
    ZF_LOGV("CCC_CTL: %x", g_ahci.abar->ccc_ctl);
    ZF_LOGV("CCC_PORTS: %x", g_ahci.abar->ccc_pts);
    ZF_LOGV("EM_LOC: %x", g_ahci.abar->em_loc);
    ZF_LOGV("EM_CTL: %x", g_ahci.abar->em_ctl);
    ZF_LOGV("CAP2: %x", g_ahci.abar->cap2);
    ZF_LOGV("BOHC: %x", g_ahci.abar->bohc);
}

static void print_port_error(hba_port_t *port)
{
    ZF_LOGV("PxIS: 0x%x", port->is);
    ZF_LOGV("PxSERR: 0x%x", port->serr);
}

static void check_for_errors(hba_port_t *port)
{
    int i = 0;
    int err_type = 0;
    volatile uint32_t err = port->serr;

    for (i = 0; i < 27; i++) {
        // if not reserved bits
        if (!PxSERR_RESERVED_BITS(i)) {
            if (err & 1) {
                ZF_LOGE("ERROR: %s error", error_str[err_type]);
                ZF_LOGE("PxSERR: 0x%x", port->serr);
            }
            err_type++;
        }

        err >>= 1;
    }
}

static void print_port_base_addresses(hba_port_t *port)
{
    ZF_LOGV("PxCLB: 0x%x", port->clb);
    ZF_LOGV("PxCLBU: 0x%x", port->clbu);
    ZF_LOGV("PxFB: 0x%x", port->fb);
    ZF_LOGV("PxFBU: 0x%x", port->fbu);
}
