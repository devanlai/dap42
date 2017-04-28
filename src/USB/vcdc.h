/*
 * Copyright (c) 2016, Devan Lai
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

#ifndef VCDC_H_INCLUDED
#define VCDC_H_INCLUDED

#include "usb_common.h"
#include "cdc_defs.h"

extern const struct cdc_acm_functional_descriptors vcdc_acm_functional_descriptors;

extern void vcdc_app_setup(usbd_device* usbd_dev,
                           GenericCallback vcdc_tx_cb,
                           GenericCallback vcdc_rx_cb);
extern bool vcdc_app_update(void);
extern size_t vcdc_recv_buffered(uint8_t* data, size_t max_bytes);
extern size_t vcdc_send_buffered(const uint8_t* data, size_t num_bytes);

extern size_t vcdc_send_buffer_space(void);

extern void vcdc_putchar(const char c);
extern void vcdc_print(const char* s);
extern void vcdc_println(const char* s);
extern void vcdc_print_hex(uint32_t x);
extern void vcdc_print_hex_byte(uint8_t x);
extern void vcdc_print_hex_nibble(uint8_t x);

#endif
