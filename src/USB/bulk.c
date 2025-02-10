/*
 * Copyright (c) 2025, Sean Cross
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

#include <stdbool.h>
#include <stdlib.h>

#include <libopencm3/usb/usbd.h>

#include "composite_usb_conf.h"
#include "bulk.h"

#if BULK_AVAILABLE

/* `true` if we've sent a packet that's 64-bytes, which needs to
   be followed by a Zero Length Packet. */
static volatile bool needs_zlp;

/* User callbacks */
static HostOutFunction bulk_out_callback = NULL;

static usbd_device* bulk_usbd_dev = NULL;

/* Handle sending a zero-length packet to the host */
static void bulk_in(usbd_device *usbd_dev, uint8_t ep) {
    if (needs_zlp) {
        needs_zlp = false;
        uint8_t empty[1] = {'\0'};
        usbd_ep_write_packet(usbd_dev, ep, empty, 0);
    }
}

/* Receive data from the host */
static void bulk_out(usbd_device *usbd_dev, uint8_t ep) {
    uint8_t buf[USB_BULK_MAX_PACKET_SIZE];
    uint16_t len = usbd_ep_read_packet(usbd_dev, ep, (void*)buf, sizeof(buf));
    // TODO: flow control
    if (len > 0 && (bulk_out_callback != NULL)) {
        bulk_out_callback(buf, len);
    }
}

static void bulk_set_config(usbd_device* usbd_dev, uint16_t wValue) {
    (void)wValue;

    // OUT (host-to-device)
    usbd_ep_setup(usbd_dev, ENDP_BULK_OUT, USB_ENDPOINT_ATTR_BULK, 64,
                  bulk_out);

    // IN (device-to-host)
    usbd_ep_setup(usbd_dev, ENDP_BULK_IN, USB_ENDPOINT_ATTR_BULK, 64,
                  bulk_in);
}

void bulk_setup(usbd_device* usbd_dev,
               HostOutFunction report_recv_cb) {
    bulk_usbd_dev = usbd_dev;
    bulk_out_callback = report_recv_cb;

    cmp_usb_register_set_config_callback(bulk_set_config);
}

bool bulk_send_report(const uint8_t* report, size_t len) {
    if (len == 64) {
        needs_zlp = true;
    }
    uint16_t sent = usbd_ep_write_packet(bulk_usbd_dev, ENDP_BULK_IN,
                                         (const void*)report,
                                         (uint16_t)len);
    if (sent != 0) {
        return true;
    }
    return false;
}

#endif
