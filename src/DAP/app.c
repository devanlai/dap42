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
    BUFFER_KIND_HID_RESPONSE,
#endif
#if BULK_AVAILABLE
    BUFFER_KIND_BULK,
    BUFFER_KIND_BULK_RESPONSE,
#endif
};

struct usb_buffer {
    uint8_t buffer_kind;
    uint8_t data[DAP_PACKET_SIZE];
    uint8_t size;
};

static volatile struct usb_buffer buffers[DAP_PACKET_QUEUE_SIZE];

// Incoming data is written here
static volatile uint8_t inbox_tail;
// The command to execute is read from here
static uint8_t process_head;
// Outgoing data is read from here
static volatile uint8_t outbox_head;

static GenericCallback dfu_request_callback = NULL;

_Static_assert(HID_AVAILABLE || BULK_AVAILABLE,
               "CMSIS-DAP needs at at least one transport interface class");

// Goal: When there is a message received, store it in a buffer and increment
// the buffer counter.

#if HID_AVAILABLE
static bool on_receive_hid_report(uint8_t* data, uint16_t len) {
    memcpy((void*)buffers[inbox_tail].data, (const void*)data, len);
    buffers[inbox_tail].buffer_kind = BUFFER_KIND_HID;
    buffers[inbox_tail].size = len;
    inbox_tail = (inbox_tail + 1) % DAP_PACKET_QUEUE_SIZE;

    return ((inbox_tail + 1) % DAP_PACKET_QUEUE_SIZE) != outbox_head;
}

static void on_send_hid_report(uint8_t* data, uint16_t* len) {
    if (outbox_head != process_head && buffers[outbox_head].buffer_kind == BUFFER_KIND_HID_RESPONSE) {
        memcpy((void*)data, (const void*)buffers[outbox_head].data,
               buffers[outbox_head].size);
        *len = buffers[outbox_head].size;

        outbox_head = (outbox_head + 1) % DAP_PACKET_QUEUE_SIZE;
    } else {
        *len = 0;
    }
}
#endif

#if BULK_AVAILABLE
static bool on_receive_bulk_report(uint8_t* data, uint16_t len) {
    memcpy((void*)buffers[inbox_tail].data, (const void*)data, len);
    buffers[inbox_tail].buffer_kind = BUFFER_KIND_BULK;
    buffers[inbox_tail].size = len;
    inbox_tail = (inbox_tail + 1) % DAP_PACKET_QUEUE_SIZE;

    return ((inbox_tail + 1) % DAP_PACKET_QUEUE_SIZE) != outbox_head;
}

static void on_send_bulk_report(uint8_t* data, uint16_t* len) {
    if (outbox_head != process_head && buffers[outbox_head].buffer_kind == BUFFER_KIND_BULK_RESPONSE) {
        memcpy((void*)data, (const void*)buffers[outbox_head].data,
               buffers[outbox_head].size);
        *len = buffers[outbox_head].size;

        outbox_head = (outbox_head + 1) % DAP_PACKET_QUEUE_SIZE;
    } else {
        *len = 0;
    }
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
    inbox_tail = 0;
    process_head = 0;
    outbox_head = 0;
    DAP_Setup();
}

bool DAP_app_update(void) {
    uint8_t response_buffer[DAP_PACKET_SIZE];
    bool active = false;

    if (process_head != inbox_tail) {
        memset(response_buffer, 0, DAP_PACKET_SIZE);
        uint32_t result = DAP_ExecuteCommand((const uint8_t *)buffers[process_head].data,
                                             response_buffer);
        uint32_t response_bytes = result & 0xffff;
        if (response_bytes > DAP_PACKET_SIZE) {
            asm("bkpt #0");
        }
        memcpy((void *)buffers[process_head].data, response_buffer, response_bytes);
        buffers[process_head].size = response_bytes;
        switch (buffers[process_head].buffer_kind) {
#if HID_AVAILABLE
            case BUFFER_KIND_HID:
                buffers[process_head].buffer_kind = BUFFER_KIND_HID_RESPONSE;
                break;
#endif
#if BULK_AVAILABLE
            case BUFFER_KIND_BULK:
                buffers[process_head].buffer_kind = BUFFER_KIND_BULK_RESPONSE;
                break;
#endif
            default:
                asm("bkpt #1");
        }
        process_head = (process_head + 1) % DAP_PACKET_QUEUE_SIZE;
        active = true;
    }

#if HID_AVAILABLE
    if (hid_get_in_ep_idle() &&
        (outbox_head != process_head) &&
        (buffers[outbox_head].buffer_kind == BUFFER_KIND_HID_RESPONSE)) {
        if (hid_send_report((const uint8_t*)buffers[outbox_head].data, buffers[outbox_head].size)) {
            outbox_head = (outbox_head + 1) % DAP_PACKET_QUEUE_SIZE;
            active = true;
        }
    }
#endif

#if BULK_AVAILABLE
    if (bulk_get_in_ep_idle() &&
        (outbox_head != process_head) &&
        (buffers[outbox_head].buffer_kind == BUFFER_KIND_BULK_RESPONSE)) {
        if (bulk_send_report((const uint8_t*)buffers[outbox_head].data, buffers[outbox_head].size)) {
            outbox_head = (outbox_head + 1) % DAP_PACKET_QUEUE_SIZE;
            active = true;
        }
    }
#endif

    return active;
}

void DAP_app_setup(usbd_device* usbd_dev, GenericCallback on_dfu_request) {
    DAP_Setup();
#if HID_AVAILABLE
    hid_setup(usbd_dev, on_send_hid_report, on_receive_hid_report);
#endif
#if BULK_AVAILABLE
    bulk_setup(usbd_dev, on_send_bulk_report, on_receive_bulk_report);
#endif
    dfu_request_callback = on_dfu_request;

    cmp_usb_register_reset_callback(DAP_app_reset);
}
