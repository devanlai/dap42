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
#if HID_AVAILABLE
#include "USB/hid.h"
#endif
#include "DAP/app.h"
#if BULK_AVAILABLE
#include "USB/bulk.h"
#endif

enum UsbBufferKind {
    BUFFER_KIND_EMPTY,
#if HID_AVAILABLE
    BUFFER_KIND_HID,
#endif
#if BULK_AVAILABLE
    BUFFER_KIND_BULK,
#endif
};

struct usb_buffer {
    uint8_t buffer_kind;
    uint8_t data[DAP_PACKET_SIZE];
};

static volatile struct usb_buffer buffers[DAP_PACKET_QUEUE_SIZE];


// Incoming data is written here
static volatile uint8_t buffer_tail;

// As hid data is responded to, this value approaches the buffer head.
#if HID_AVAILABLE
static volatile uint8_t hid_head;
#endif

// As bulk data is responded to, this value approaches the buffer head.
#if BULK_AVAILABLE
static volatile uint8_t bulk_head;
#endif

static GenericCallback dfu_request_callback = NULL;

// Goal: When there is a message received, store it in a buffer and increment
// the buffer counter.

#if HID_AVAILABLE
static bool on_receive_hid_report(uint8_t* data, uint16_t len) {
    memcpy((void*)buffers[buffer_tail].data, (const void*)data, len);
    buffers[buffer_tail].buffer_kind = BUFFER_KIND_HID;
    buffer_tail = (buffer_tail + 1) % DAP_PACKET_QUEUE_SIZE;

    return ((buffer_tail + 1) % DAP_PACKET_QUEUE_SIZE) != hid_head;
}

// static void on_send_hid_report(uint8_t* data, uint16_t* len) {
    // if (outbox_head != process_head) {
    //     memcpy((void*)data, (const void*)response_buffers[outbox_head],
    //            DAP_PACKET_SIZE);
    //     *len = DAP_PACKET_SIZE;
// }
#endif

#if BULK_AVAILABLE
static bool on_receive_bulk_report(uint8_t* data, uint16_t len) {
    memcpy((void*)buffers[buffer_tail].data, (const void*)data, len);
    buffers[buffer_tail].buffer_kind = BUFFER_KIND_BULK;
    buffer_tail = (buffer_tail + 1) % DAP_PACKET_QUEUE_SIZE;

    return ((buffer_tail + 1) % DAP_PACKET_QUEUE_SIZE) != bulk_head;
}
#endif

uint32_t DAP_ProcessVendorCommand(const uint8_t* request, uint8_t* response) {
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
            return ((4U << 16) | 2U);
        } else {
            response[0] = request[0];
            response[1] = DAP_ERROR;
            return ((4U << 16) | 2U);
        }
    }

    response[0] = ID_DAP_Invalid;
    return ((1U << 16) | 1U);
}

static void DAP_app_reset(void) {
#if BULK_AVAILABLE
    bulk_head = 0;
#endif
#if HID_AVAILABLE
    hid_head = 0;
#endif
    buffer_tail = 0;
    DAP_Setup();
}

bool DAP_app_update(void) {
    uint8_t response_buffer[DAP_PACKET_SIZE];
    bool active = false;

#if HID_AVAILABLE
    while (hid_head != buffer_tail) {
        if (buffers[hid_head].buffer_kind != BUFFER_KIND_HID) {
            hid_head = (hid_head + 1) % DAP_PACKET_QUEUE_SIZE;
            continue;
        }

        buffers[hid_head].buffer_kind = BUFFER_KIND_EMPTY;
        memset(response_buffer, 0, DAP_PACKET_SIZE);

        uint32_t result = DAP_ExecuteCommand((const uint8_t *)buffers[hid_head].data,
                           response_buffer);
        uint32_t response_bytes = result & 0xffff;
        if (response_bytes >= 64) {
            asm("bkpt #0");
        }
        if (!hid_send_report(response_buffer, response_bytes)) {
            asm("bkpt #2");
        }

        hid_head = (hid_head + 1) % DAP_PACKET_QUEUE_SIZE;
        active = true;
    }
#endif

#if BULK_AVAILABLE
    while (bulk_head != buffer_tail) {
        if (buffers[bulk_head].buffer_kind != BUFFER_KIND_BULK) {
            bulk_head = (bulk_head + 1) % DAP_PACKET_QUEUE_SIZE;
            continue;
        }

        buffers[bulk_head].buffer_kind = BUFFER_KIND_EMPTY;
        memset(response_buffer, 0, DAP_PACKET_SIZE);

        uint32_t result = DAP_ExecuteCommand((const uint8_t *)buffers[bulk_head].data,
                                             response_buffer);
        uint32_t response_bytes = result & 0xffff;
        if (response_bytes > 64) {
            asm("bkpt #0");
        }

        if (!bulk_send_report(response_buffer, response_bytes)) {
            // If this fails, then the packet was dropped. This hasn't been
            // ecnountered in testing.
            asm("bkpt #1");
        }

        bulk_head = (bulk_head + 1) % DAP_PACKET_QUEUE_SIZE;
        active = true;
    }
#endif

    return active;
}

void DAP_app_setup(usbd_device* usbd_dev, GenericCallback on_dfu_request) {
    DAP_Setup();
#if HID_AVAILABLE
    hid_setup(usbd_dev, NULL, on_receive_hid_report);
#endif
#if BULK_AVAILABLE
    bulk_setup(usbd_dev, on_receive_bulk_report);
#endif
    dfu_request_callback = on_dfu_request;

    cmp_usb_register_reset_callback(DAP_app_reset);
}
