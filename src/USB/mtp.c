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

#include "composite_usb_conf.h"
#include "mtp.h"

#if MTP_AVAILABLE

/* User callbacks */
static HostOutFunction mtp_recv_callback = NULL;
static HostInFunction mtp_send_callback = NULL;

static usbd_device* mtp_usbd_dev = NULL;

/* Receive MTP data from the host */
static void mtp_bulk_data_out(usbd_device *usbd_dev, uint8_t ep) {
    uint8_t buf[USB_MTP_MAX_PACKET_SIZE];
    uint16_t len = usbd_ep_read_packet(usbd_dev, ep, (void*)buf, sizeof(buf));
    if (len > 0 && (mtp_recv_callback != NULL)) {
        mtp_recv_callback(buf, len);
    }
}

/* Send more MTP data to the host */
static void mtp_bulk_data_in(usbd_device *usbd_dev, uint8_t ep) {
    if (mtp_send_callback != NULL) {
        uint8_t buf[USB_MTP_MAX_PACKET_SIZE];
        uint16_t len = 0;
        mtp_send_callback(buf, &len);
        if (len > 0) {
            usbd_ep_write_packet(usbd_dev, ep, (const void*)buf, len);
        }
    }
}

static int mtp_control_class_request(usbd_device *usbd_dev,
                                     struct usb_setup_data *req,
                                     uint8_t **buf, uint16_t *len,
                                     usbd_control_complete_callback* complete) {
    (void)complete;
    (void)usbd_dev;

    if (req->wIndex != INTF_MTP) {
        return USBD_REQ_NEXT_CALLBACK;
    }

    int status;
    switch (req->bRequest) {
        case USB_PTP_REQ_CANCEL_REQ: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
        case USB_PTP_REQ_GET_EXTENDED_EVENT_DATA: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
        case USB_PTP_REQ_DEVICE_RESET: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
        case USB_PTP_REQ_GET_DEVICE_STATUS: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
        default: {
            status = USBD_REQ_NEXT_CALLBACK;
            break;
        }
    }

    return status;
}

static void mtp_set_config(usbd_device *usbd_dev, uint16_t wValue) {
    (void)wValue;

    usbd_ep_setup(usbd_dev, ENDP_MTP_DATA_OUT, USB_ENDPOINT_ATTR_BULK, 64,
                  &mtp_bulk_data_out);
    usbd_ep_setup(usbd_dev, ENDP_MTP_DATA_IN, USB_ENDPOINT_ATTR_BULK, 64,
                  &mtp_bulk_data_in);
    usbd_ep_setup(usbd_dev, ENDP_MTP_EVENT_IN, USB_ENDPOINT_ATTR_INTERRUPT, 64, NULL);

    cmp_usb_register_control_class_callback(INTF_MTP, mtp_control_class_request);
}

void mtp_setup(usbd_device* usbd_dev,
               HostOutFunction mtp_recv_cb,
               HostInFunction mtp_send_cb) {
    mtp_usbd_dev = usbd_dev;
    mtp_recv_callback = mtp_recv_cb;
    mtp_send_callback = mtp_send_cb;

    cmp_usb_register_set_config_callback(mtp_set_config);
}

bool mtp_send_event(struct usb_ptp_async_event* event) {
    uint16_t sent = usbd_ep_write_packet(mtp_usbd_dev, ENDP_MTP_EVENT_IN,
                                         (const void*)(event),
                                         (uint16_t)(event->length));
    return (sent != 0);
}

#endif
