/*
 * Copyright (c) 2015, Devan Lai
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdint.h>
#include <string.h>

#include "DAP/CMSIS_DAP_hal.h"
#include "DAP/CMSIS_DAP.h"

#include "USB/composite_usb_conf.h"
#include "USB/hid.h"
#include "DAP/app.h"

static uint8_t request_buffers[DAP_PACKET_SIZE][DAP_PACKET_QUEUE_SIZE];
static uint8_t response_buffers[DAP_PACKET_SIZE][DAP_PACKET_QUEUE_SIZE];

static uint8_t inbox_tail;
static uint8_t process_head;
static uint8_t outbox_head;

static GenericCallback dfu_request_callback = NULL;

static void on_receive_report(uint8_t* data, uint16_t len) {
    memcpy((void*)request_buffers[inbox_tail], (const void*)data, len);
    inbox_tail = (inbox_tail + 1) % DAP_PACKET_QUEUE_SIZE;
}

static void on_send_report(uint8_t* data, uint16_t* len) {
    if (outbox_head != process_head) {
        memcpy((void*)data, (const void*)response_buffers[outbox_head],
               DAP_PACKET_SIZE);
        *len = DAP_PACKET_SIZE;

        outbox_head = (outbox_head + 1) % DAP_PACKET_QUEUE_SIZE;
    } else {
        *len = 0;
    }
}

uint32_t DAP_ProcessVendorCommand(uint8_t* request, uint8_t* response) {
    if (request[0] == ID_DAP_Vendor31) {
        if (request[1] == 'D' && request[2] == 'F' && request[3] == 'U') {
            response[0] = request[0];
            response[1] = DAP_OK;
            if (dfu_request_callback) {
                dfu_request_callback();
                response[1] = DAP_OK;
            } else {
                response[1] = DAP_ERROR;
            }
            return 2;
        } else {
            response[0] = request[0];
            response[1] = DAP_ERROR;
            return 2;
        }
    }

    response[0] = ID_DAP_Invalid;
    return 1;
}

static void DAP_app_reset(void) {
    inbox_tail = process_head = outbox_head = 0;
    DAP_Setup();
}

bool DAP_app_update(void) {
    bool active = false;

    if (process_head != inbox_tail) {
        memset(response_buffers[process_head], 0, DAP_PACKET_SIZE);
        DAP_ProcessCommand(request_buffers[process_head],
                           response_buffers[process_head]);
        process_head = (process_head + 1) % DAP_PACKET_QUEUE_SIZE;
        active = true;
    }

    if (outbox_head != process_head) {
        if (hid_send_report(response_buffers[outbox_head], DAP_PACKET_SIZE)) {
            outbox_head = (outbox_head + 1) % DAP_PACKET_QUEUE_SIZE;
        }
        active = true;
    }

    return active;
}

void DAP_app_setup(usbd_device* usbd_dev, GenericCallback on_dfu_request) {
    DAP_Setup();
    hid_setup(usbd_dev, &on_send_report, &on_receive_report);
    dfu_request_callback = on_dfu_request;

    cmp_usb_register_reset_callback(DAP_app_reset);
}
