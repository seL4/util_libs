/*
 * Copyright 2019, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <assert.h>
#include <satadrivers/ide.h>
#include <satadrivers/common.h>
#include <utils/zf_log.h>
#include <unistd.h>
#include <string.h>

struct IDEChannelRegisters {
    uint16_t base;  // I/O Base.
    uint16_t ctrl;  // Control Base
} channels[NUM_IDE_CHANNELS];

/*
 * Purpose: Poll the device through the IDE Controller to see if Data Transfer is ready
 *
 * Inputs:
 *   - *io_ops: IO Operations
 *   - channel: channel to check
 *   - advanced_check: flag to do some advanced checking of status bits
 *
 * Returns: success (0) failure (code)
 *
 */
uint8_t ide_polling(ps_io_ops_t *io_ops, uint8_t channel, uint32_t advanced_check)
{
    uint8_t err = ATA_NO_ERR;
    uint32_t res = 0;
    uint32_t bus = channels[channel].base;      /* Bus Base, like 0x1F0 which is also data port. */
    uint32_t ctrl = channels[channel].ctrl; /* Control Base, like 0x1F0 which is also data port. */

    /* Assuming an I/O read on the ATA bus takes at least 100 ns,
     * 4 consecutive reads with incur at least a 400 ns delay.
     */
    for (int i = 0; i < 4; i++) {
        ps_io_port_in(&io_ops->io_port_ops, (uint32_t)(ctrl + ATA_REG_ALTSTATUS), 1, &res);
    }

    /* Wait for BSY to be cleared */
    ps_io_port_in(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_STATUS), 1, &res);
    while (res & ATA_SR_BSY) {
        ps_io_port_in(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_STATUS), 1, &res);
    }

    if (advanced_check) {
        /* Read Status Register */
        ps_io_port_in(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_STATUS), 1, &res);

        /* Check For Errors */
        if (res & ATA_SR_ERR) {
            err = ATA_ERR;
        }
        /* Check If Device fault */
        else if (res & ATA_SR_DF) {
            err = ATA_DRIVE_FAULT;
        }
        /* Check Data Request Ready */
        else if (0 == (res & ATA_SR_DRQ)) {
            err = ATA_DRQ_UNSET;
        } else {
            err = ATA_NO_ERR;
        }
    }
    return err;
}

/*
 * Purpose: Print the IDE Device Error
 *
 * Inputs:
 *   - drive: drive to get error information from
 *   - err: error input
 *
 * Returns: a different error value. not sure about why, but it does...
 *
 */
uint8_t ide_print_error(uint32_t drive, uint8_t err)
{
    /* Bus Base, like 0x1F0 which is also data port. */
    uint32_t bus = channels[sata_devices[drive].Channel].base;
    if (ATA_NO_ERR == err) {
        return err;
    }

    /* Create a local copy of the ide device structure for local manipulation */
    ps_io_ops_t *io_ops = sata_devices[drive].io_ops;

    ZF_LOGE("IDE:");
    if (ATA_DRIVE_FAULT == err) {
        ZF_LOGE("- Device Fault\n     ");
        err = DEV_FAULT_ERR;
    } else if (ATA_ERR == err) {
        uint32_t st = 0;
        ps_io_port_in(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_ERROR), 1, &st);

        if (st & ATA_ER_AMNF) {
            ZF_LOGE("- No Address Mark Found\n     ");
            err = INV_ADDR_ERR;
        }
        if (st & ATA_ER_TK0NF) {
            ZF_LOGE("- No Media or Media Error\n     ");
            err = NO_MEDIA_ERR;
        }
        if (st & ATA_ER_ABRT) {
            ZF_LOGE("- Command Aborted\n     ");
            err = CMD_ABORT_ERR;
        }
        if (st & ATA_ER_MCR) {
            ZF_LOGE("- No Media or Media Error\n     ");
            err = NO_MEDIA_ERR;
        }
        if (st & ATA_ER_IDNF) {
            ZF_LOGE("- ID mark not Found\n     ");
            err = ID_NOT_FOUND_ERR;
        }
        if (st & ATA_ER_MC) {
            ZF_LOGE("- No Media or Media Error\n     ");
            err = NO_MEDIA_ERR;
        }
        if (st & ATA_ER_UNC) {
            ZF_LOGE("- Uncorrectable Data Error\n     ");
            err = UNCORRECT_DATA_ERR;
        }
        if (st & ATA_ER_BBK) {
            ZF_LOGE("- Bad Sectors\n     ");
            err = BAD_SECT_ERR;
        }
    } else if (ATA_DRQ_UNSET == err) {
        ZF_LOGE("- Reads Nothing\n     ");
        err = READ_NOTHING_ERR;
    } else if (4 == err) {
        ZF_LOGE("- Write Protected\n     ");
        err = WRITE_PROTECT_ERR;
    }
    // Use the channel and the drive respectively as an index into the array
    ZF_LOGV("- [%s %s] %s\n",
    (const char *[]) {
        "Primary", "Secondary"
    }[sata_devices[drive].Channel],
    (const char *[]) {
        "Master", "Slave"
    }[sata_devices[drive].Drive],
    sata_devices[drive].Model);

    return err;
}

/* Purpose: Handles how data is accessed from the ATA device
 *
 * Inputs:
 *   - drive: The drive number which can be from 0 to 3.
 *   - lba: The LBA address which allows us to access disks up to 2TB.
 *   - numsects:  The number of sectors to be read, it is a char, as reading more than
 *                256 sector immediately may performance issues. If numsects is 0, the
 *                ATA controller will know that we want 256 sectors.
 *   - buf: Buffer to write or read data from
 *
 * Returns: success (0) failure (code)
 */
uint8_t ide_ata_access(uint8_t direction, uint8_t drive, uint32_t lba,
                       uint16_t numsects, uint8_t *buf)
{
    uint8_t   lba_mode,
              cmd,
              flush_cmd;
    uint8_t   lba_io[NUM_LBA_BYTES];
    uint32_t  channel = sata_devices[drive].Channel;  /* Read the Channel */
    uint32_t  slavebit = sata_devices[drive].Drive;   /* Read the Drive [Master/Slave] */
    /* Bus Base, like 0x1F0 which is also data port. */
    uint32_t  bus = channels[channel].base;
    /* Control Base, like 0x1F0 which is also data port. */
    uint32_t  ctrl = channels[channel].ctrl;
    uint16_t  cyl, i;
    uint8_t   head, sect;
    uint32_t  bufp_val = 0;

    uint32_t res = 0;
    int error = 0;

    if (NULL == buf) {
        ZF_LOGE("IDE: Invalid buffer for ATA access.\n");
        return -1;
    }

    ps_io_ops_t *io_ops = sata_devices[drive].io_ops;

    error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(ctrl + ATA_REG_CONTROL), 1,
                           ATA_CTRL_NIEN);
    assert(!error);

    /* Select one from LBA28, LBA48 or CHS */

    /* Sure Drive should support LBA in this case, or you are giving a wrong LBA */
    if (lba >= LBA28_MAX_SIZE) {
        // LBA48:
        lba_mode  = LBA_48_MODE;
        flush_cmd = ATA_CMD_CACHE_FLUSH_EXT;
        lba_io[0] = (lba & 0x000000FF) >> 0;
        lba_io[1] = (lba & 0x0000FF00) >> 8;
        lba_io[2] = (lba & 0x00FF0000) >> 16;
        lba_io[3] = (lba & 0xFF000000) >> 24;
        lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        head      = 0; // Lower 4-bits of HDDEVSEL are not used here.
    } else if (sata_devices[drive].Capabilities & 0x200) {
        // LBA28:
        lba_mode  = LBA_28_MODE;
        flush_cmd = ATA_CMD_CACHE_FLUSH;
        lba_io[0] = (lba & 0x00000FF) >> 0;
        lba_io[1] = (lba & 0x000FF00) >> 8;
        lba_io[2] = (lba & 0x0FF0000) >> 16;
        lba_io[3] = 0; // These Registers are not used here.
        lba_io[4] = 0; // These Registers are not used here.
        lba_io[5] = 0; // These Registers are not used here.
        head      = (lba & 0xF000000) >> 24;
    } else {
        // CHS:
        lba_mode  = LBA_CHS_MODE;
        flush_cmd = ATA_CMD_CACHE_FLUSH;
        sect      = (lba % NUM_SECT_PER_TRACK) + 1; // Sectors are indexed starting at 1
        cyl       = (lba + 1  - sect) / (16 * NUM_SECT_PER_TRACK);
        lba_io[0] = sect;
        lba_io[1] = (cyl >> 0) & 0xFF;
        lba_io[2] = (cyl >> 8) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        // Head number is written to HDDEVSEL lower 4-bits.
        head      = (lba + 1  - sect) % (16 * NUM_SECT_PER_TRACK) / (NUM_SECT_PER_TRACK);
    }

    /* Wait if the drive is busy */
    error = ps_io_port_in(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_STATUS), 1, &res);
    assert(!error);

    while (res & ATA_SR_BSY) {
        error = ps_io_port_in(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_STATUS), 1, &res);
        assert(!error);
    }

    /* Select Drive from the controller */
    if (LBA_CHS_MODE == lba_mode) {
        /* Drive & CHS. */
        error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_HDDEVSEL), 1,
                               (SET_CHS | (slavebit << 4) | head));
        assert(!error);
    } else {
        /* Drive & LBA */
        error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_HDDEVSEL), 1,
                               (SET_LBA | (slavebit << 4) | head));
        assert(!error);
    }

    // (V) Write Parameters;
    if (LBA_48_MODE == lba_mode) {
        error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_SECCOUNT), 1,
                               (uint8_t)(numsects >> 8));
        assert(!error);
        error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_LBA_LO), 1,
                               (lba_io[3]));
        assert(!error);
        error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_LBA_MID), 1,
                               (lba_io[4]));
        assert(!error);
        error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_LBA_HI), 1,
                               (lba_io[5]));
        assert(!error);
    }

    error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_SECCOUNT), 1,
                           (uint8_t)(numsects & 0x00FF));
    assert(!error);
    error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_LBA_LO), 1,
                           (lba_io[0]));
    assert(!error);
    error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_LBA_MID), 1,
                           (lba_io[1]));
    assert(!error);
    error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_LBA_HI), 1,
                           (lba_io[2]));
    assert(!error);

    if (direction == ATA_READ) {
        switch (lba_mode) {
        case LBA_CHS_MODE:
            cmd = ATA_CMD_READ_PIO;
            break;
        case LBA_28_MODE:
            cmd = ATA_CMD_READ_PIO;
            break;
        case LBA_48_MODE:
            cmd = ATA_CMD_READ_PIO_EXT;
            break;
        default:
            return ATA_DRIVE_FAULT;
            break;
        }
    } else if (direction == ATA_WRITE) {
        switch (lba_mode) {
        case LBA_CHS_MODE:
            cmd = ATA_CMD_WRITE_PIO;
            break;
        case LBA_28_MODE:
            cmd = ATA_CMD_WRITE_PIO;
            break;
        case LBA_48_MODE:
            cmd = ATA_CMD_WRITE_DMA_EXT;
            break;
        default:
            return ATA_DRIVE_FAULT;
            break;
        }
    } else {
        return ATA_DRIVE_FAULT;
    }

    /* Send the Command */
    error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_COMMAND), 1, (cmd));
    assert(!error);

    if (ATA_READ == direction) {
        /* PIO Read. */
        for (i = 0; i < numsects; i++) {
            /* Polling, set error and exit if there is */
            error = ide_polling(io_ops, channel, 1);
            if (error) {
                return error;
            }
            for (int j = 0; j < ATA_WORDS; j++) {
                /* Do 16 bit reads in ATA_WORDS
                 * Get the pointer to copy data into
                 *   i * ATA_WORDS * 2 is the start of the current sector in bytes
                 *   j * 2 is the offset in bytes in the current sector
                 */
                error = ps_io_port_in(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_DATA), 2,
                                      &bufp_val);
                memcpy(&buf[i * ATA_WORDS * 2 + j * 2], &bufp_val, 2);
                assert(!error);
            }
        }
    } else {
        /* PIO Write */
        for (i = 0; i < numsects; i++) {
            ide_polling(io_ops, channel, 0);
            for (int j = 0; j < ATA_WORDS; j++) {
                /* Do 16 bit writes in ATA_WORDS
                 * Get the pointer to copy data from
                 *   i * ATA_WORDS * 2 is the start of the current sector in bytes
                 *   j * 2 is the offset in bytes in the current sector
                 */
                memcpy(&bufp_val, &buf[i * ATA_WORDS * 2 + j * 2], 2);
                error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_DATA), 2,
                                       bufp_val);
                assert(!error);
            }
        }
        error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(bus + ATA_REG_COMMAND), 1,
                               (flush_cmd));
        assert(!error);
        ide_polling(io_ops, channel, 0);
    }

    return 0;
}

/*
 * Purpose: Initialize the IDE Device
 *
 * Inputs:
 *   - io_ops: IO Operation Functions
 *
 * Returns: 0
 *
 */
int ide_init(ps_io_ops_t *io_ops)
{
    uint8_t ide_buf[BUFFER_ID_SPACE] = {0};   // Buffer to read the identification space into
    uint32_t res = 0;
    int error = 0;
    int i = 0;
    int j = 0;
    int k = 0;
    int count = 0;

    ZF_LOGV("IDE: In Init\n");

    channels[ATA_PRIMARY].base = (PRIMARY_CMD_BASE);
    channels[ATA_PRIMARY].ctrl = (PRIMARY_CTRL_BASE);

    /* Disable IRQs */
    error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(channels[ATA_PRIMARY].ctrl
                                                            + ATA_REG_CONTROL), 1, ATA_CTRL_NIEN);
    assert(!error);

    ZF_LOGV("IDE: Disable IRQs\n");

    for (i = 0; i < NUM_IDE_CHANNELS; i++) {
        ZF_LOGV("IDE: Identify drive %d\n", i);
        uint8_t err = 0, type = DEV_SATA;
        sata_devices[count].Reserved = 0; /* Assuming that no drive here. */

        /* Select Drive */
        error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(channels[ATA_PRIMARY].base
                                                                + ATA_REG_HDDEVSEL), 1, (SET_CHS | (i << 4)));
        assert(!error);
        sata_delay(DELAY_CYCLES);

        ZF_LOGV("IDE: Selected drive\n");

        /* Send ATA Identify Command */
        error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(channels[ATA_PRIMARY].base
                                                                + ATA_REG_COMMAND), 1, ATA_CMD_IDENTIFY);
        assert(!error);
        sata_delay(DELAY_CYCLES);

        ZF_LOGV("IDE: Sent ATA Identify Command\n");

        /* Polling */
        error = ps_io_port_in(&io_ops->io_port_ops, (uint32_t)(channels[ATA_PRIMARY].base
                                                               + ATA_REG_STATUS), 1, &res);
        assert(!error);
        if (0 == res) {
            continue; /* If Status = 0, No Device */
        }

        ZF_LOGV("IDE: Polled\n");

        while (1) {
            error = ps_io_port_in(&io_ops->io_port_ops, (uint32_t)(channels[ATA_PRIMARY].base
                                                                   + ATA_REG_STATUS), 1, &res);
            assert(!error);

            if ((res & ATA_SR_ERR)) {
                /* If Err, Device is not ATA */
                err = 1;
                break;
            }
            if (!(res & ATA_SR_BSY) && (res & ATA_SR_DRQ)) {
                break; /* Everything is right. */
            }
        }

        ZF_LOGV("IDE: Got Status: 0x%x\n", res);

        /* Probe for ATAPI Devices */
        if (0 != err) {
            error = ps_io_port_in(&io_ops->io_port_ops, (uint32_t)(channels[ATA_PRIMARY].base
                                                                   + ATA_REG_LBA_MID), 1, &res);
            assert(!error);
            uint8_t signatureByteLow = res;

            ZF_LOGV("IDE: Got ATA_REG_LBA1\n");

            error = ps_io_port_in(&io_ops->io_port_ops, (uint32_t)(channels[ATA_PRIMARY].base
                                                                   + ATA_REG_LBA_HI), 1, &res);
            assert(!error);
            uint8_t signatureByteHigh = res;

            ZF_LOGV("IDE: Got ATA_REG_LBA2\n");

            if (0x14 == signatureByteLow && 0xEB == signatureByteHigh) {
                type = DEV_SATAPI;
            } else if (0x69 == signatureByteLow && 0x96 == signatureByteHigh) {
                type = DEV_SATAPI;
            } else {
                continue; /* Unknown Type (may not be a device) */
            }

            error = ps_io_port_out(&io_ops->io_port_ops, (uint32_t)(channels[ATA_PRIMARY].base
                                                                    + ATA_REG_COMMAND), 1, ATA_CMD_IDENTIFY_PACKET);
            assert(!error);

            sata_delay(DELAY_CYCLES);
        }

        ZF_LOGV("IDE: Sent ATA_CMD_IDENTIFY_PACKET\n");

        /* Read Identification Space of the Device */
        for (j = 0; j < ATA_WORDS; j++) {
            error = ps_io_port_in(&io_ops->io_port_ops, (uint32_t)(channels[ATA_PRIMARY].base
                                                                   + ATA_REG_DATA), 2, (uint32_t *)&ide_buf[j * 2]);
            assert(!error);
        }
        ZF_LOGV("IDE: Read DATA\n");

        /* Read Device Parameters */
        sata_devices[count].Reserved     = 1;
        sata_devices[count].Type         = type;
        sata_devices[count].Channel      = 0;
        sata_devices[count].Drive        = i;
        memcpy(&sata_devices[count].Signature, ide_buf + ATA_IDENT_DEVICETYPE, sizeof(uint16_t));
        memcpy(&sata_devices[count].Capabilities, ide_buf + ATA_IDENT_CAPABILITIES, sizeof(uint16_t));
        memcpy(&sata_devices[count].CommandSets, ide_buf + ATA_IDENT_COMMANDSETS, sizeof(uint32_t));
        sata_devices[count].io_ops       = io_ops;

        /* Get Size */
        if (sata_devices[count].CommandSets & (1 << 26)) {
            /* Device uses 48-Bit Addressing */
            memcpy(&sata_devices[count].Size, ide_buf + ATA_IDENT_MAX_LBA_EXT, sizeof(uint32_t));
        } else {
            /* Device uses CHS or 28-bit Addressing */
            memcpy(&sata_devices[count].Size, ide_buf + ATA_IDENT_MAX_LBA, sizeof(uint32_t));
        }

        /* String indicates model of device (like Western Digital HDD and SONY DVD-RW...) */
        for (k = 0; k < MODEL_STRING_SIZE - 1; k += 2) {
            sata_devices[count].Model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
            sata_devices[count].Model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];
        }
        sata_devices[count].Model[MODEL_STRING_SIZE - 1] = 0; // Terminate String.

        count++;
    }

    /* Print Summary */
    for (i = 0; i < NUM_IDE_CHANNELS; i++) {
        if (1 == sata_devices[i].Reserved) {
            ZF_LOGV(" (%d) Found %s Drive %dGB - %s\n", i,
            (const char *[]) {
                "ATA", "ATAPI"
            }[sata_devices[i].Type],  /* Type */
            sata_devices[i].Size / 1024 / 1024 / 2,                  /* Size */
            sata_devices[i].Model);
        }
    }

    return error;
}
