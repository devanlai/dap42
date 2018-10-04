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

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#include "console.h"
#include "target.h"

void console_setup(uint32_t baudrate) {
    /* Setup GPIO */
    target_console_init();

    usart_set_baudrate(CONSOLE_USART, baudrate);
    usart_set_databits(CONSOLE_USART, 8);
    usart_set_parity(CONSOLE_USART, USART_PARITY_NONE);
    usart_set_stopbits(CONSOLE_USART, USART_STOPBITS_1);

    // Only enable TX for logging by default
    usart_set_mode(CONSOLE_USART, CONSOLE_USART_MODE & ~USART_MODE_RX);
    usart_set_flow_control(CONSOLE_USART, USART_FLOWCONTROL_NONE);

    usart_enable(CONSOLE_USART);
    nvic_enable_irq(CONSOLE_USART_NVIC_LINE);
    rcc_periph_clock_enable(CONSOLE_RX_DMA_CLOCK);
}

void console_tx_buffer_clear(void);
void console_rx_buffer_clear(void);

#define IS_POW_OF_TWO(X) (((X) & ((X)-1)) == 0)
_Static_assert(IS_POW_OF_TWO(CONSOLE_TX_BUFFER_SIZE),
               "Unmasked circular buffer size must be a power of two");
_Static_assert(CONSOLE_TX_BUFFER_SIZE <= UINT16_MAX/2,
               "Buffer size too big for unmasked circular buffer");

static volatile uint8_t console_tx_buffer[CONSOLE_TX_BUFFER_SIZE];
static volatile uint8_t console_rx_buffer[CONSOLE_RX_BUFFER_SIZE];

static volatile uint16_t console_tx_head = 0;
static volatile uint16_t console_tx_tail = 0;

static uint16_t console_rx_head = 0;

void console_reconfigure(uint32_t baudrate, uint32_t databits, uint32_t stopbits,
                         uint32_t parity) {
    // Disable the UART and clear buffers
    usart_disable(CONSOLE_USART);

    usart_disable_rx_dma(CONSOLE_USART);
    usart_disable_tx_interrupt(CONSOLE_USART);
    nvic_disable_irq(CONSOLE_USART_NVIC_LINE);

    console_tx_buffer_clear();
    console_rx_buffer_clear();

    if (parity != USART_PARITY_NONE) {
        /* usart_set_databits counts parity bits as "data" bits */
        databits += 1;
    }

    usart_set_baudrate(CONSOLE_USART, baudrate);
    usart_set_databits(CONSOLE_USART, databits);
    usart_set_stopbits(CONSOLE_USART, stopbits);
    usart_set_parity(CONSOLE_USART, parity);
    usart_set_mode(CONSOLE_USART, CONSOLE_USART_MODE);

    dma_channel_reset(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL);

    // Configure RX DMA...
    dma_set_peripheral_address(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL, (uint32_t)&USART_RDR(CONSOLE_USART));
    dma_set_memory_address(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL, (uint32_t)console_rx_buffer);
    dma_set_number_of_data(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL, CONSOLE_RX_BUFFER_SIZE);
    dma_set_read_from_peripheral(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL);
    dma_enable_memory_increment_mode(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL);
    dma_set_peripheral_size(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL, DMA_CCR_PSIZE_8BIT);
    dma_set_memory_size(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL, DMA_CCR_MSIZE_8BIT);
    dma_set_priority(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL, DMA_CCR_PL_HIGH);
    dma_enable_circular_mode(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL);

    dma_enable_channel(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL);

    usart_enable_rx_dma(CONSOLE_USART);
    nvic_enable_irq(CONSOLE_USART_NVIC_LINE);

    // Re-enable the UART with the new settings
    usart_enable(CONSOLE_USART);
}

static bool console_tx_buffer_empty(void) {
    return console_tx_head == console_tx_tail;
}

static bool console_tx_buffer_full(void) {
    return (uint16_t)(console_tx_tail - console_tx_head) == CONSOLE_TX_BUFFER_SIZE;
}

static void console_tx_buffer_put(uint8_t data) {
    console_tx_buffer[console_tx_tail % CONSOLE_TX_BUFFER_SIZE] = data;
    console_tx_tail++;
}

static uint8_t console_tx_buffer_get(void) {
    uint8_t data = console_tx_buffer[console_tx_head % CONSOLE_TX_BUFFER_SIZE];
    console_tx_head++;
    return data;
}

void console_tx_buffer_clear(void) {
    console_tx_head = 0;
    console_tx_tail = 0;
}

size_t console_send_buffer_space(void) {
    return CONSOLE_TX_BUFFER_SIZE - (uint16_t)(console_tx_tail - console_tx_head);
}

static bool console_rx_buffer_empty(void) {
    if (DMA_CCR(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL) & DMA_CCR_EN) {
        uint16_t console_rx_tail = (CONSOLE_RX_BUFFER_SIZE - DMA_CNDTR(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL)) % CONSOLE_RX_BUFFER_SIZE;
        return console_rx_tail == console_rx_head;
    } else {
        return true;
    }
}

static uint8_t console_rx_buffer_get(void) {
    uint8_t data = console_rx_buffer[console_rx_head];
    console_rx_head = (console_rx_head + 1) % CONSOLE_RX_BUFFER_SIZE;
    return data;
}

void console_rx_buffer_clear(void) {
    console_rx_head = 0;
    dma_disable_channel(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL);
}

size_t console_send_buffered(const uint8_t* data, size_t num_bytes) {
    size_t bytes_written = 0;

    while (!console_tx_buffer_full() && (bytes_written < num_bytes)) {
        console_tx_buffer_put(data[bytes_written++]);
    }

    if (!console_tx_buffer_empty()) {
        usart_enable_tx_interrupt(CONSOLE_USART);
    }

    return bytes_written;
}

size_t console_recv_buffered(uint8_t* data, size_t max_bytes) {
    size_t bytes_read = 0;
    if (max_bytes == 1) {
        if (!console_rx_buffer_empty()) {
            *data = console_rx_buffer_get();
            bytes_read = 1;
        }
    } else if (!console_rx_buffer_empty()) {
        uint16_t console_rx_tail = (CONSOLE_RX_BUFFER_SIZE - DMA_CNDTR(CONSOLE_RX_DMA_CONTROLLER, CONSOLE_RX_DMA_CHANNEL)) % CONSOLE_RX_BUFFER_SIZE;
        if (console_rx_head > console_rx_tail) {
            while (console_rx_head < CONSOLE_RX_BUFFER_SIZE && bytes_read < max_bytes) {
                data[bytes_read++] = console_rx_buffer[console_rx_head++];
            }
            if (console_rx_head == CONSOLE_RX_BUFFER_SIZE) {
                console_rx_head = 0;
            }
        }

        if ((bytes_read < max_bytes) && (console_rx_head < console_rx_tail)) {
            while (console_rx_head < console_rx_tail && bytes_read < max_bytes) {
                data[bytes_read++] = console_rx_buffer[console_rx_head++];
            }
            if (console_rx_head == CONSOLE_RX_BUFFER_SIZE) {
                console_rx_head = 0;
            }
        }
    }

    return bytes_read;
}

void console_send_blocking(uint8_t data) {
    usart_send_blocking(CONSOLE_USART, data);
}

uint8_t console_recv_blocking(void) {
    return usart_recv_blocking(CONSOLE_USART);
}

void CONSOLE_USART_IRQ_NAME(void) {
    if (usart_get_flag(CONSOLE_USART, USART_FLAG_TXE)) {
        if (!console_tx_buffer_empty()) {
            usart_word_t buffered_byte = console_tx_buffer_get();
            usart_send(CONSOLE_USART, buffered_byte);
        } else {
            usart_disable_tx_interrupt(CONSOLE_USART);
        }
    }
}
