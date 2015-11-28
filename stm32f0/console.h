#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>
#include <libopencm3/stm32/usart.h>

#define CONSOLE_USART USART2
#define CONSOLE_TX_BUFFER_SIZE 128
#define CONSOLE_RX_BUFFER_SIZE 128

extern void console_setup(uint32_t baudrate);

extern inline void console_send_blocking(uint8_t data) {
    usart_send_blocking(CONSOLE_USART, data);
}

extern inline uint8_t console_recv_blocking(void) {
    return usart_recv_blocking(CONSOLE_USART);
}

extern size_t console_send_buffered(const uint8_t* data, size_t num_bytes);
extern size_t console_recv_buffered(uint8_t* data, size_t max_bytes);

extern void console_set_echo(bool enable);

#endif
