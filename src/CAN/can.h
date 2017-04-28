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

#ifndef CAN_H
#define CAN_H

#include "can_helper.h"

#define CAN_RX_BUFFER_SIZE 16

extern bool can_setup(uint32_t baudrate, CanMode mode);
extern bool can_reconfigure(uint32_t baudrate, CanMode mode);
extern bool can_read(CAN_Message* msg);
extern bool can_read_buffer(CAN_Message* msg);

extern bool can_write(CAN_Message* msg);

extern bool can_rx_buffer_empty(void);
extern bool can_rx_buffer_full(void);
extern void can_rx_buffer_put(const CAN_Message* msg);
extern void can_rx_buffer_get(CAN_Message* msg);
extern CAN_Message* can_rx_buffer_peek(void);
extern void can_rx_buffer_pop(void);

#endif
