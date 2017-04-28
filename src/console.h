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

#ifndef CONSOLE_H_INCLUDED
#define CONSOLE_H_INCLUDED

#include <stddef.h>
#include <libopencm3/stm32/usart.h>

#include "config.h"

extern void console_setup(uint32_t baudrate);
extern void console_reconfigure(uint32_t baudrate, uint32_t databits,
                                uint32_t stopbits, uint32_t parity);

extern void console_send_blocking(uint8_t data);
extern uint8_t console_recv_blocking(void);
extern size_t console_send_buffered(const uint8_t* data, size_t num_bytes);
extern size_t console_recv_buffered(uint8_t* data, size_t max_bytes);
extern size_t console_send_buffer_space(void);

#endif
