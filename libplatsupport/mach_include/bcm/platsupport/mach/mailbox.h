/*
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <utils/util.h>
#include <platsupport/io.h>

// Mailbox status codes
enum {
    MAILBOX_OK                  = 0,
    MAILBOX_ERR_INTERNAL        = -1,
    MAILBOX_ERR_INVALID         = -2,
    MAILBOX_ERR_BUSY            = -3,
    MAILBOX_ERR_DMA             = -4,
    MAILBOX_ERR_READ            = -5,
    MAILBOX_ERR_WRITE           = -6,
    MAILBOX_ERR_BUFFER          = -7,
    MAILBOX_ERR_TAG             = -8
};

typedef struct mailbox {
    /**
     * Mailbox request/response message.
     * @param   mbox                Initialized mailbox driver instance.
     * @param   tag_id              Tag ID encoding a mailbox command.
     * @param   request_tag         Pointer to request tag struct object.
     * @param   request_tag_size    Size of request tag struct object.
     * @param   response_tag        Pointer to response tag struct object.
     * @param   response_tag_size   Size of response tag struct object.
     * @return 0 on success. Non-zero on error.
     */
    int (*message)(struct mailbox   *mbox,
                   uint32_t         tag_id,
                   void             *request_tag,
                   uint32_t         request_tag_size,
                   void             *response_tag,
                   uint32_t         response_tag_size);
    /* Device data */
    void *priv;
    /* DMA allocator */
    ps_dma_man_t *dma_man;
    /* Buffer for mailbox requests/responses */
    void *buffer;
    /* Physical Address of the DMA region */
    uintptr_t phys_addr;
} mailbox_t;

// See: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
#define CODE_BUFFER_REQUEST_PROCESS     0x00000000
#define CODE_BUFFER_RESPONSE_SUCCESS    0x80000000
#define CODE_BUFFER_RESPONSE_FAILURE    0x80000001
typedef struct MailboxInterface_PropertyBuffer {
    uint32_t    buffer_size;
    uint32_t    code;
    uint8_t     tags[0];
}
MailboxInterface_PropertyBuffer_t;

// See: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
// bit 0-30: value length in bytes
// bit 31:   request (clear) / response (set)
#define VALUE_LENGTH_RESPONSE           (1u << 31)
typedef struct MailboxInterface_PropertyTag {
    uint32_t    tag_id;
    uint32_t    value_buffer_size;
    uint32_t    value_length;
}
MailboxInterface_PropertyTag_t;

/**
 * Initialise the mailbox subsystem and provide a handle for access
 * @param[in]  io_ops   io operations for device initialisation
 * @param[out] mailbox  A mailbox handle structure to initialise
 * @return              0 on success, errno value otherwise
 */
int mailbox_init(ps_io_ops_t *io_ops, mailbox_t *mailbox);

/**
 * Destroy the mailbox by freeing the allocated DMA region.
 * @param[in] mailbox   A mailbox handle structure to destroy
 * @return              0 on success, errno value otherwise
 */
int mailbox_destroy(mailbox_t *mailbox);
