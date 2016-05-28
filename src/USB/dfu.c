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
#include <libopencm3/usb/dfu.h>

#include "composite_usb_conf.h"
#include "dfu.h"

#if DFU_AVAILABLE

const struct usb_dfu_descriptor dfu_function = {
    .bLength = sizeof(struct usb_dfu_descriptor),
    .bDescriptorType = DFU_FUNCTIONAL,
    .bmAttributes = USB_DFU_CAN_DOWNLOAD | USB_DFU_WILL_DETACH,
    .wDetachTimeout = 255,
    .wTransferSize = 1024,
    .bcdDFUVersion = 0x011A,
};

/* User callbacks */
static GenericCallback dfu_detach_request_callback = NULL;

static int dfu_control_class_request(usbd_device *usbd_dev,
                                     struct usb_setup_data *req,
                                     uint8_t **buf, uint16_t *len,
                                     usbd_control_complete_callback* complete) {
    (void)complete;
    (void)buf;
    (void)len;

    if (req->wIndex != INTF_DFU) {
        return USBD_REQ_NEXT_CALLBACK;
    }

    int status = USBD_REQ_NOTSUPP;
    switch (req->bRequest) {
        case DFU_DETACH: {
            if (dfu_detach_request_callback != NULL) {
                dfu_detach_request_callback();
                status = USBD_REQ_HANDLED;
            }
            break;
        }
        case DFU_GETSTATUS:
        case DFU_GETSTATE: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
        case DFU_DNLOAD:
        case DFU_UPLOAD:
        case DFU_CLRSTATUS:
        case DFU_ABORT: {
            /* Stall the control pipe */
            usbd_ep_stall_set(usbd_dev, 0x00, 1);
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

static void dfu_set_config(usbd_device* usbd_dev, uint16_t wValue) {
    (void)usbd_dev;
    (void)wValue;

    cmp_usb_register_control_class_callback(INTF_DFU, dfu_control_class_request);
}

void dfu_setup(usbd_device* usbd_dev, GenericCallback on_detach_request) {
    (void)usbd_dev;
    dfu_detach_request_callback = on_detach_request;
    cmp_usb_register_set_config_callback(dfu_set_config);
}

#endif
