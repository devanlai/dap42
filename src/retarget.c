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

#include <errno.h>
#include <stdio.h>

#include <libopencm3/stm32/usart.h>

#include "retarget.h"
#include "console.h"
#include "USB/vcdc.h"

static uint32_t usart_stdout = NO_USART;
static uint32_t usart_stderr = NO_USART;
static uint32_t usart_stdin  = NO_USART;

void retarget(int file, uint32_t usart) {
    if (file == STDOUT_FILENO) {
        usart_stdout = usart;
    } else if (file == STDERR_FILENO) {
        usart_stderr = usart;
    } else if (file == STDIN_FILENO) {
        usart_stdin = usart;
    }
}

#if !SEMIHOSTING

int _write(int file, char *ptr, int len) {
    int sent;

    uint32_t usart = NO_USART;
    if (file == STDOUT_FILENO) {
        usart = usart_stdout;
    } else if (file == STDERR_FILENO) {
        usart = usart_stderr;
    }

    if (usart == CONSOLE_USART) {
        sent = console_send_buffered((uint8_t*)ptr, (size_t)len);
        return sent;
    } else if (usart == VIRTUAL_USART) {
        sent = vcdc_send_buffered((uint8_t*)ptr, (size_t)len);
        return sent;
    }

    errno = EIO;
    return -1;
}

#endif

void print_hex_nibble(uint8_t x) {
    uint8_t nibble = x & 0x0F;
    char nibble_char;
    if (nibble < 10) {
        nibble_char = '0' + nibble;
    } else {
        nibble_char = 'A' + (nibble - 10);
    }
    putchar(nibble_char);
}

void print_hex_byte(uint8_t x) {
    print_hex_nibble(x >> 4);
    print_hex_nibble(x);
}

void print_hex(uint32_t x) {
    uint8_t i;
    for (i=8; i > 0; i--) {
        uint8_t nibble = (x >> ((i-1) * 4)) & 0xF;
        char nibble_char;
        if (nibble < 10) {
            nibble_char = '0' + nibble;
        } else {
            nibble_char = 'A' + (nibble - 10);
        }

        putchar(nibble_char);
    }
}

void print(const char* s) {
    while (*s != '\0') {
        putchar(*s++);
    }
}

void println(const char* s) {
    while (*s != '\0') {
        putchar(*s++);
    }
    putchar('\r');
    putchar('\n');
}
