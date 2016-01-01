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
#include <libopencm3/usb/cdc.h>

#include "composite_usb_conf.h"
#include "cdc.h"

/* User callbacks */
static HostOutFunction cdc_rx_callback = NULL;
static HostInFunction cdc_tx_callback = NULL;
static SetControlLineStateFunction cdc_set_control_line_state_callback = NULL;
static SetLineCodingFunction cdc_set_line_coding_callback = NULL;

static int cdc_control_class_request(usbd_device *usbd_dev,
                                     struct usb_setup_data *req,
                                     uint8_t **buf, uint16_t *len,
                                     usbd_control_complete_callback* complete) {
    (void)complete;
    (void)usbd_dev;

    if (req->wIndex != INTF_CDC_DATA && req->wIndex != INTF_CDC_COMM) {
        return USBD_REQ_NEXT_CALLBACK;
    }
    int status = USBD_REQ_NOTSUPP;

    switch (req->bRequest) {
        case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
            /*
             * This Linux cdc_acm driver requires this to be implemented
             * even though it's optional in the CDC spec, and we don't
             * advertise it in the ACM functional descriptor.
             */

            bool dtr = (req->wValue & (1 << 0)) != 0;
            bool rts = (req->wValue & (1 << 1)) != 0;

            if (cdc_set_control_line_state_callback) {
                cdc_set_control_line_state_callback(dtr, rts);
            }

            status = USBD_REQ_HANDLED;
            break;
        }
        case USB_CDC_REQ_SET_LINE_CODING: {
            const struct usb_cdc_line_coding *coding;

            if (*len < sizeof(struct usb_cdc_line_coding)) {
                status = USBD_REQ_NOTSUPP;
            } else if (cdc_set_line_coding_callback) {
                coding = (const struct usb_cdc_line_coding *)(*buf);
                if (cdc_set_line_coding_callback(coding)) {
                    status = USBD_REQ_HANDLED;
                } else {
                    status = USBD_REQ_NOTSUPP;
                }
            } else {
                /* No callback - accept whatever is requested */
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

/* Receive data from the host */
static void cdc_bulk_data_out(usbd_device *usbd_dev, uint8_t ep) {
    uint8_t buf[USB_CDC_MAX_PACKET_SIZE];
    uint16_t len = usbd_ep_read_packet(usbd_dev, ep, (void*)buf, sizeof(buf));
    if (len > 0 && (cdc_rx_callback != NULL)) {
        cdc_rx_callback(buf, len);
    }
}

static void cdc_set_config(usbd_device *usbd_dev, uint16_t wValue) {
    (void)wValue;

    usbd_ep_setup(usbd_dev, ENDP_CDC_DATA_OUT, USB_ENDPOINT_ATTR_BULK, 64,
                  cdc_bulk_data_out);
    usbd_ep_setup(usbd_dev, ENDP_CDC_DATA_IN, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    usbd_ep_setup(usbd_dev, ENDP_CDC_COMM_IN, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        cdc_control_class_request);
}

static usbd_device* cdc_usbd_dev;

void cdc_setup(usbd_device* usbd_dev,
               HostInFunction cdc_tx_cb,
               HostOutFunction cdc_rx_cb,
               SetControlLineStateFunction set_control_line_state_cb,
               SetLineCodingFunction set_line_coding_cb) {
    cdc_usbd_dev = usbd_dev;
    cdc_tx_callback = cdc_tx_cb;
    cdc_rx_callback = cdc_rx_cb;
    cdc_set_control_line_state_callback = set_control_line_state_cb,
    cdc_set_line_coding_callback = set_line_coding_cb;

    usbd_register_set_config_callback(usbd_dev, cdc_set_config);
}

bool cdc_send_data(const uint8_t* data, size_t len) {
    uint16_t sent = usbd_ep_write_packet(cdc_usbd_dev, ENDP_CDC_DATA_IN,
                                         (const void*)data,
                                         (uint16_t)len);
    return (sent != 0);
}
