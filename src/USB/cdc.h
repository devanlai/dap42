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

#ifndef CDC_H_INCLUDED
#define CDC_H_INCLUDED

#include "usb_common.h"
#include <libopencm3/usb/cdc.h>
#include "cdc_defs.h"

typedef void (*SetControlLineStateFunction)(bool dtr, bool rts);

typedef bool (*SetLineCodingFunction)(const struct usb_cdc_line_coding* line_coding);
typedef bool (*GetLineCodingFunction)(struct usb_cdc_line_coding* line_coding);

extern void cdc_setup(usbd_device* usbd_dev,
                      HostInFunction cdc_tx_cb,
                      HostOutFunction cdc_rx_cb,
                      SetControlLineStateFunction set_control_line_state_cb,
                      SetLineCodingFunction set_line_coding_cb,
                      GetLineCodingFunction get_line_coding_cb);

extern bool cdc_send_data(const uint8_t* data, size_t len);

#endif
