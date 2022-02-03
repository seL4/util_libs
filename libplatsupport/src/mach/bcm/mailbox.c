/*
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * More information about the mailboxes:
 * - https://github.com/raspberrypi/firmware/wiki/Mailboxes
 * - https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 * - https://github.com/raspberrypi/documentation/blob/JamesH65-mailbox_docs/configuration/mailboxes/propertiesARM-VC.md
 */

#include <stdint.h>
#include <string.h>

#include <utils/util.h>
#include <platsupport/plat/mailbox.h>
#include "../../services.h"

#define DMA_PAGE_SIZE       4096
#define DMA_ALIGNEMENT      4096

// See: https://github.com/raspberrypi/firmware/wiki/Accessing-mailboxes#addresses-as-data
#define VC_BASE_CACHED      0x40000000
#define VC_BASE_UNCACHED    0xC0000000
#define VC_BASE             VC_BASE_UNCACHED

// See: https://github.com/raspberrypi/firmware/wiki/Accessing-mailboxes#sample-code
#define MAILBOX_EMPTY       0x40000000
#define MAILBOX_FULL        0x80000000

// See: https://github.com/raspberrypi/firmware/wiki/Mailboxes#channels
#define MAILBOX_CHANNEL     8

typedef volatile struct {
    uint32_t read;    // 0x00
    uint32_t reg_04;  // 0x04
    uint32_t reg_08;  // 0x08
    uint32_t reg_0c;  // 0x0c
    uint32_t reg_10;  // 0x10
    uint32_t reg_14;  // 0x14
    uint32_t status0; // 0x18
    uint32_t reg_1c;  // 0x1c
    uint32_t write;   // 0x20
    uint32_t reg_24;  // 0x24
    uint32_t reg_28;  // 0x28
    uint32_t reg_2c;  // 0x2c
    uint32_t reg_30;  // 0x30
    uint32_t reg_34;  // 0x34
    uint32_t status1; // 0x38
} mailbox_regs_t;

static inline mailbox_regs_t *get_mailbox_regs(mailbox_t *mbox)
{
    return (mbox != NULL && mbox->priv != NULL) ? (mailbox_regs_t *)((uintptr_t)mbox->priv + 0x880)
           : NULL;
}

static inline uint32_t get_bus_address(uint32_t addr)
{
    return (((addr) & ~VC_BASE) | VC_BASE);
}

/*
 * Mailbox read operation
 *
 * 1. Read the status register until the empty flag is not set
 * 2. Read data from the read register
 * 3. If the lower four bits do not match the channel number desired then repeat
 *    from 1
 * 4. The upper 28 bits are the returned data
 *
 * See: https://github.com/raspberrypi/documentation/blob/JamesH65-mailbox_docs/configuration/mailboxes/accessing.md#general-procedure
 */
static int mailbox_read(
    mailbox_regs_t *mailbox,
    uint32_t        channel,
    uint32_t       *rsp
)
{
    uint32_t value;
    do {
        while (mailbox->status0 & MAILBOX_EMPTY) {
            // busy loop
        }
        value = mailbox->read;
    } while ((value & 0xF) != channel);

    *rsp =  value & ~0xF;

    return 0;
}

/*
 * Mailbox write operation
 *
 * 1. Read the status register until the full flag is not set
 * 2. Write the data (shifted into the upper 28 bits) combined with the channel
 *    (in the lower four bits) to the write register
 *
 * See: https://github.com/raspberrypi/documentation/blob/JamesH65-mailbox_docs/configuration/mailboxes/accessing.md#general-procedure
 */
static int mailbox_write(
    mailbox_regs_t *mailbox,
    uint32_t        channel,
    uint32_t        data
)
{
    while (mailbox->status1 & MAILBOX_FULL) {
        // busy loop
    }
    mailbox->write = (data & ~0xF) | (channel & 0xF);

    return 0;
}

/*
 * Mailbox Command
 *
 * A command should only be issued in case the mailbox is empty. If the mailbox
 * is not empty, it is said to be busy. In this case, it is the reponsibility of
 * the caller to handle this situation.
 *
 * Every mailbox command consists of a request and a response. For this purpose
 * PropertyTags must be created and configured for request. Afterwards, the
 * mailbox interface is notified that there is a message waiting in the buffer.
 * This signalling is done with the mailbox_write operation, that writes the
 * buffer_address and the channel number into the write register. The
 * mailbox_read operation makes sure that the response waiting in the mailbox
 * interface belongs to the issued request by checking the current
 * buffer_address and channel number in the read register.
 */
static int mailbox_command(
    mailbox_t  *mbox,
    uint32_t    channel,
    uint32_t    data,
    uint32_t   *rsp
)
{
    mailbox_regs_t *mailbox = get_mailbox_regs(mbox);
    if (mailbox == NULL) {
        ZF_LOGE("Mailbox is invalid!");
        return MAILBOX_ERR_INVALID;
    }

    // mailbox command should start operation with an empty mailbox
    if (!(mailbox->status0 & MAILBOX_EMPTY)) {
        ZF_LOGE("Mailbox is busy!");
        return MAILBOX_ERR_BUSY;
    }

    if (mailbox_write(mailbox, channel, data) != 0) {
        ZF_LOGE("Error when writing to mailbox!");
        return MAILBOX_ERR_WRITE;
    }
    if (mailbox_read(mailbox, channel, rsp) != 0) {
        ZF_LOGE("Error when reading from mailbox!");
        return MAILBOX_ERR_READ;
    }
    return 0;
}

static int mailbox_message(
    mailbox_t  *mbox,
    uint32_t    tag_id,
    void       *request_tag,
    uint32_t    request_tag_size,
    void       *response_tag,
    uint32_t    response_tag_size
)
{
    // ToDo: Implement the mailbox interface to handle multiple concatenated
    //       tags per message. Currently, we assume only one tag per message.
    // See:  https://github.com/raspberrypi/documentation/blob/JamesH65-mailbox_docs/configuration/mailboxes/propertiesARM-VC.md#message-content

    // Prepare Mailbox
    uint32_t tag_size = MAX(request_tag_size, response_tag_size);
    MailboxInterface_PropertyBuffer_t  *buffer  = (MailboxInterface_PropertyBuffer_t *)mbox->buffer;
    buffer->buffer_size = sizeof(MailboxInterface_PropertyBuffer_t)
                          + tag_size
                          + sizeof(uint32_t);
    buffer->code        = CODE_BUFFER_REQUEST_PROCESS;

    MailboxInterface_PropertyTag_t     *tags    = (MailboxInterface_PropertyTag_t *)buffer->tags;
    memcpy(tags, request_tag, request_tag_size);
    tags->tag_id              = tag_id;
    tags->value_buffer_size   = tag_size - sizeof(MailboxInterface_PropertyTag_t);
    tags->value_length        = tags->value_buffer_size & ~VALUE_LENGTH_RESPONSE;

    uint32_t *end_tag = (uint32_t *)(tags + tag_size);
    *end_tag = 0;
    assert((uintptr_t)(const void *)end_tag % 4 == 0);

    // Mailbox command
    uint32_t rsp = 0;
    uint32_t buffer_address = get_bus_address((uint32_t) mbox->phys_addr);
    int status = mailbox_command(mbox,
                                 MAILBOX_CHANNEL,
                                 buffer_address,
                                 &rsp);

    // Response Evaluation
    // Buffer format: https://github.com/raspberrypi/documentation/blob/JamesH65-mailbox_docs/configuration/mailboxes/propertiesARM-VC.md#buffer-contents
    // Tag format: https://github.com/raspberrypi/documentation/blob/JamesH65-mailbox_docs/configuration/mailboxes/propertiesARM-VC.md#tag-format
    if (status != 0) {
        ZF_LOGE("Mailbox command error - code: %d!", status);
        return status;
    }

    if (rsp != buffer_address) {
        ZF_LOGE("Mailbox response should be buffer address!");
        return MAILBOX_ERR_INTERNAL;
    }

    if (buffer->code != CODE_BUFFER_RESPONSE_SUCCESS) {
        ZF_LOGE("Mailbox response is not successful!");
        return MAILBOX_ERR_BUFFER;
    }

    if (tags->tag_id != tag_id) {
        ZF_LOGE("Wrong tag id returned!");
        return MAILBOX_ERR_TAG;
    }

    if (!(tags->value_length & VALUE_LENGTH_RESPONSE)) {
        ZF_LOGE("Received tag is not a response!");
        return MAILBOX_ERR_TAG;
    }

    if ((tags->value_length &= ~VALUE_LENGTH_RESPONSE) == 0) {
        ZF_LOGE("Value buffer has length 0 bytes!");
        return MAILBOX_ERR_TAG;
    }

    memcpy(response_tag, tags, response_tag_size);

    return MAILBOX_OK;
}

int mailbox_init(ps_io_ops_t *io_ops, mailbox_t *mailbox)
{
    void *reg = NULL;
    MAP_IF_NULL(io_ops, MAILBOX, reg);
    mailbox->priv = reg;
    mailbox->dma_man = &io_ops->dma_manager;
    mailbox->message = &mailbox_message;
    mailbox->buffer  = mailbox->dma_man->dma_alloc_fn(
                           mailbox->dma_man->cookie,
                           DMA_PAGE_SIZE,
                           DMA_ALIGNEMENT,
                           0,
                           PS_MEM_NORMAL);
    if (mailbox->buffer == NULL) {
        ZF_LOGE("DMA allocation failed.");
        return MAILBOX_ERR_DMA;
    }
    mailbox->phys_addr = mailbox->dma_man->dma_pin_fn(mailbox->dma_man->cookie,
                                                      mailbox->buffer,
                                                      DMA_PAGE_SIZE);
    return 0;
}

int mailbox_destroy(mailbox_t *mailbox)
{
    mailbox->dma_man->dma_free_fn(mailbox->dma_man->cookie,
                                  mailbox->buffer,
                                  DMA_PAGE_SIZE);
    mailbox->dma_man->dma_unpin_fn(mailbox->dma_man->cookie,
                                   mailbox->buffer,
                                   DMA_PAGE_SIZE);
    return 0;
}
