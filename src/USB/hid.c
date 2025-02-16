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

#include <stdlib.h>

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>

#include "composite_usb_conf.h"
#include "hid.h"

#if HID_AVAILABLE

static volatile bool needs_zlp;

const uint8_t hid_report_descriptor[] = {
    0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,        // Usage (0x01)
    0xA1, 0x01,        // Collection (Application)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x40,        //   Report Count (64)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x40,        //   Report Count (64)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,        //   Report Count (1)
    0x09, 0x01,        //   Usage (0x01)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection

    // 33 bytes
};

const struct full_usb_hid_descriptor hid_function = {
    .hid_descriptor = {
        .bLength = sizeof(hid_function),
        .bDescriptorType = USB_DT_HID,
        .bcdHID = 0x0111,
        .bCountryCode = 0,
        .bNumDescriptors = 1,
    },
    .hid_report = {
        .bReportDescriptorType = USB_DT_REPORT,
        .wDescriptorLength = sizeof(hid_report_descriptor),
    },
};

/* User callbacks */
static HostOutFunction hid_report_out_callback = NULL;
static HostInFunction hid_report_in_callback = NULL;

static usbd_device* hid_usbd_dev = NULL;
static volatile bool hid_in_ep_idle = true;

static enum usbd_request_return_codes
hid_control_standard_request(usbd_device *usbd_dev,
                             struct usb_setup_data *req,
                             uint8_t **buf, uint16_t *len,
                             usbd_control_complete_callback* complete) {
    (void)complete;
    (void)usbd_dev;

    if (req->wIndex != INTF_HID) {
        return USBD_REQ_NEXT_CALLBACK;
    }

    int status = USBD_REQ_NOTSUPP;
    switch (req->bRequest) {
        case USB_REQ_GET_DESCRIPTOR: {
            uint8_t descriptorType = (uint8_t)((req->wValue >> 8) & 0xFF);
            uint8_t descriptorIndex = (uint8_t)(req->wValue & 0xFF);
            if (descriptorType == USB_DT_REPORT) {
                if (descriptorIndex == 0) {
                    *buf = (uint8_t *)hid_report_descriptor;
                    *len = sizeof(hid_report_descriptor);
                    status = USBD_REQ_HANDLED;
                }
            }
            break;
        }
        case USB_REQ_SET_DESCRIPTOR: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
        default: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
    }

    return status;
}

static enum usbd_request_return_codes
hid_control_class_request(usbd_device *usbd_dev,
                          struct usb_setup_data *req,
                          uint8_t **buf, uint16_t *len,
                          usbd_control_complete_callback* complete) {
    (void)complete;
    (void)usbd_dev;

    if (req->wIndex != INTF_HID) {
        return USBD_REQ_NEXT_CALLBACK;
    }

    enum usbd_request_return_codes status = USBD_REQ_NOTSUPP;
    switch (req->bRequest) {
        case USB_HID_REQ_GET_REPORT: {
            // Note: support for this request is mandatory, but in practice
            // it is unusable since we have a dedicated IN endpoint that
            // we push the latest report to as soon as it is available.
            if (*buf != NULL) {
                *len = 0;
                status = USBD_REQ_HANDLED;
            }
            break;
        }
        case USB_HID_REQ_SET_REPORT: {
            if ((hid_report_out_callback != NULL) && (*len > 0))
            {
                // TODO: flow control
                hid_report_out_callback(*buf, *len);
                status = USBD_REQ_HANDLED;
            }
            break;
        }
        default: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
    }

    return status;
}

/* After finishing sending a report to the host, possibly
 * start sending another report to the host */
static void hid_interrupt_in(usbd_device *usbd_dev, uint8_t ep) {
    if (hid_report_in_callback != NULL) {
        uint8_t buf[USB_HID_MAX_PACKET_SIZE];
        uint16_t len = 0;
        hid_report_in_callback(buf, &len);
        if (len > 0) {
            usbd_ep_write_packet(usbd_dev, ep, (const void*)buf, len);
            hid_in_ep_idle = false;
        } else {
            // No data ready to transmit now; we will not receive another
            // interrupt until someone manually sends data via hid_send_report()
            hid_in_ep_idle = true;
        }
    }
}

/* Receive data from the host */
static void hid_interrupt_out(usbd_device *usbd_dev, uint8_t ep) {
    uint8_t buf[USB_HID_MAX_PACKET_SIZE];
    uint16_t len = usbd_ep_read_packet(usbd_dev, ep, (void*)buf, sizeof(buf));
    // TODO: set NAK for flow control if the callback indicates it isn't ready
    // to accept more packets.
    if (len > 0 && (hid_report_out_callback != NULL)) {
        hid_report_out_callback(buf, len);
    }
}

static void hid_set_config(usbd_device* usbd_dev, uint16_t wValue) {
    (void)wValue;

    usbd_ep_setup(usbd_dev, ENDP_HID_REPORT_OUT, USB_ENDPOINT_ATTR_INTERRUPT, 64,
                  hid_interrupt_out);
    usbd_ep_setup(usbd_dev, ENDP_HID_REPORT_IN, USB_ENDPOINT_ATTR_INTERRUPT, 64,
                  hid_interrupt_in);
    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        hid_control_standard_request);

    cmp_usb_register_control_class_callback(INTF_HID, hid_control_class_request);
}


bool hid_send_report(const uint8_t* report, size_t len) {
    uint16_t sent = usbd_ep_write_packet(hid_usbd_dev,
                                         ENDP_HID_REPORT_IN,
                                         report,
                                         (uint16_t)len);
    if (sent != 0) {
        // The report is ready to transmit and will eventually generate an interrupt
        hid_in_ep_idle = false;
        return true;
    }

    return false;
}

bool hid_get_in_ep_idle(void) {
    return hid_in_ep_idle;
}

void hid_setup(usbd_device* usbd_dev,
               HostInFunction report_send_cb,
               HostOutFunction report_recv_cb) {
    hid_usbd_dev = usbd_dev;
    hid_report_in_callback = report_send_cb;
    hid_report_out_callback = report_recv_cb;

    cmp_usb_register_set_config_callback(hid_set_config);
}
#endif
