/*
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <platsupport/plat/mailbox.h>

// Clock IDs: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface#clocks
enum {
    CLOCK_ID_EMMC = 1,
    CLOCK_ID_UART,
    CLOCK_ID_ARM,
    CLOCK_ID_CORE,
    CLOCK_ID_V3D,
    CLOCK_ID_H264,
    CLOCK_ID_ISP,
    CLOCK_ID_SDRAM,
    CLOCK_ID_PIXEL,
    CLOCK_ID_PWM,
    CLOCK_ID_HEVC,
    CLOCK_ID_EMMC2,
    CLOCK_ID_M2MC,
    CLOCK_ID_PIXEL_BVB
};

// Device IDs: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface#power
enum {
    DEVICE_ID_SD_CARD = 0,
    DEVICE_ID_UART0,
    DEVICE_ID_UART1,
    DEVICE_ID_USB_HCD,
    DEVICE_ID_I2C0,
    DEVICE_ID_I2C1,
    DEVICE_ID_I2C2,
    DEVICE_ID_SPI,
    DEVICE_ID_CCP2TX
};

////////////////////////////////////////////////////////////////////////////////
//--------------------------------Property Tags-------------------------------//
////////////////////////////////////////////////////////////////////////////////

// See: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
#define TAG_SET_POWER_STATE     0x00028001
#define TAG_GET_CLOCK_RATE      0x00030002
#define TAG_GET_MAC_ADDRESS     0x00010003

// TODO: Add more property tags if needed

////////////////////////////////////////////////////////////////////////////////
//-----------------------------Request/Reponse Tags---------------------------//
////////////////////////////////////////////////////////////////////////////////

// See: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface#set-power-state
#define SET_POWER_STATE_OFF             (0u << 0)
#define SET_POWER_STATE_ON              (1u << 0)
#define SET_POWER_STATE_WAIT            (1u << 1)
typedef struct {
    MailboxInterface_PropertyTag_t tag;
    uint32_t device_id;
    uint32_t state;
}
PropertyTag_SetPowerState_Request_t;

#define SET_POWER_STATE_NO_DEVICE       (1u << 1)
typedef struct {
    MailboxInterface_PropertyTag_t tag;
    uint32_t device_id;
    uint32_t state;
}
PropertyTag_SetPowerState_Response_t;

// See: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface#get-clock-rate
typedef struct {
    MailboxInterface_PropertyTag_t tag;
    uint32_t clock_id;
}
PropertyTag_GetClockRate_Request_t;

typedef struct {
    MailboxInterface_PropertyTag_t tag;
    uint32_t clock_id;
    uint32_t rate;
}
PropertyTag_GetClockRate_Response_t;

// See: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface#get-board-mac-address
#define MAC_ADDRESS_SIZE 6
typedef struct {
    MailboxInterface_PropertyTag_t tag;
}
PropertyTag_GetMACAddress_Request_t;

typedef struct {
    MailboxInterface_PropertyTag_t tag;
    uint8_t mac_address[MAC_ADDRESS_SIZE];
}
PropertyTag_GetMACAddress_Response_t;

// TODO: Add more request/reponse tags if needed

////////////////////////////////////////////////////////////////////////////////
//-----------------------------Mailbox Functions------------------------------//
////////////////////////////////////////////////////////////////////////////////

bool mailbox_set_power_state_on(mailbox_t *mbox, uint32_t device_id);

int mailbox_get_clock_rate(mailbox_t *mbox, uint32_t clock_id);

bool mailbox_get_mac_address(mailbox_t *mbox, uint8_t buffer[MAC_ADDRESS_SIZE]);

// TODO: Add more mailbox functions if needed
