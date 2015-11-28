#ifndef CONSOLE_H
#define CONSOLE_H

#include <libopencm3/stm32/usart.h>

#define CONSOLE_USART USART2

extern void console_setup(uint32_t baudrate);

extern inline void console_send_blocking(uint8_t data) {
    usart_send_blocking(CONSOLE_USART, data);
}

extern inline uint8_t console_recv_blocking(void) {
    return usart_recv_blocking(CONSOLE_USART);
}

#endif
