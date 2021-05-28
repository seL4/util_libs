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

#include <platsupport/plat/mailbox_util.h>

static int mbox_req_resp(mailbox_t   *mbox,
                         uint32_t    tag_id,
                         void        *req,
                         uint32_t    req_size,
                         void        *resp,
                         uint32_t    resp_size)
{
    // ToDo: could add a max loop counter to timeout here to exit the polling
    //       loop if there is no reponse
    for (;;) {
        int status = mbox->message(mbox,
                                   tag_id,
                                   req,
                                   req_size,
                                   resp,
                                   resp_size);
        if (MAILBOX_ERR_BUSY != status) {
            return status;
        }
    }
}

bool bcm2837_set_power_state_on(mailbox_t *mbox, uint32_t device_id)
{
    PropertyTag_SetPowerState_Request_t TagRequest = {
        .device_id = device_id,
        .state  = SET_POWER_STATE_ON | SET_POWER_STATE_WAIT
    };

    PropertyTag_SetPowerState_Response_t TagResponse;

    int status = mbox_req_resp(mbox,
                               TAG_SET_POWER_STATE,
                               &TagRequest,
                               sizeof(TagRequest),
                               &TagResponse,
                               sizeof(TagResponse));

    if (MAILBOX_OK != status
        || (TagResponse.state & SET_POWER_STATE_NO_DEVICE)
        || !(TagResponse.state & SET_POWER_STATE_ON)) {
        ZF_LOGE("Failed to set power state on - error code: %d!", status);
        return false;
    }

    return true;
}

/**
 * Get clock rate of certain device clock.
 *
 * @param mbox      Initialized mailbox driver instance
 * @param clock_id  ID of clock
 * @return          requested clock rate on success, 0 on failure (e.g. clock id
 *                  is not valid -> clock does not exist)
 */
int bcm2837_get_clock_rate(mailbox_t *mbox, uint32_t clock_id)
{
    PropertyTag_GetClockRate_Request_t TagRequest = {
        .clock_id = clock_id
    };

    PropertyTag_GetClockRate_Response_t TagResponse;

    int status = mbox_req_resp(mbox,
                               TAG_GET_CLOCK_RATE,
                               &TagRequest,
                               sizeof(TagRequest),
                               &TagResponse,
                               sizeof(TagResponse));

    if (MAILBOX_OK != status) {
        ZF_LOGE("Failed to get clock rate of clock %d - error code: %d!", clock_id, status);
        TagResponse.rate = 0;
    }

    return TagResponse.rate;
}
