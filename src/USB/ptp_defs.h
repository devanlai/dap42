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

#ifndef PTP_DEFS_H
#define PTP_DEFS_H

#define USB_CLASS_IMAGE                     0x06
#define USB_IMAGE_SUBCLASS_STILL_IMAGING    0x01

#define USB_PTP_REQ_CANCEL_REQ              0x64
#define USB_PTP_REQ_GET_EXTENDED_EVENT_DATA 0x65
#define USB_PTP_REQ_DEVICE_RESET            0x66
#define USB_PTP_REQ_GET_DEVICE_STATUS       0x67

/* Table 5.2-2 Format of Cancel Request Data */
struct usb_ptp_cancel_request {
    uint16_t code;
    uint32_t transactionID;
} __attribute__((packed));

/* Table 5.2-4 Data Format of Get Extended Event Data Request */
struct usb_ptp_extended_event_data_request_header {
    uint16_t code;
    uint32_t transactionID;
    uint16_t numParams;
    uint8_t remainder[0];
} __attribute__((packed));

/* Table 5.2-6  */
struct usb_ptp_get_device_status_request {
    uint16_t length;
    uint16_t code;
    uint8_t remainder[0];
} __attribute__((packed));

/* Table 7.1-1 Generic Container Structure */
struct usb_ptp_data_block {
    uint32_t length;
    uint16_t type;
    uint16_t code;
    uint32_t transactionID;
    uint8_t payload[0];
} __attribute__((packed));

struct usb_ptp_command_block {
    uint32_t length;
    uint16_t type;
    uint16_t code;
    uint32_t transactionID;
    uint32_t parameters[0];
} __attribute__((packed));

struct usb_ptp_response_block {
    uint32_t length;
    uint16_t type;
    uint16_t code;
    uint32_t transactionID;
    uint32_t parameters[0];
} __attribute__((packed));

/* Table 7.3-1 Asynchronous Event Interrupt Data Format */
struct usb_ptp_async_event {
    uint32_t length;
    uint16_t type;
    uint16_t code;
    uint32_t transactionID;
    uint32_t parameters[3];
} __attribute__((packed));

#endif
