#include <errno.h>
#include <stdio.h>

#include <libopencm3/stm32/usart.h>

#include "retarget.h"

static uint32_t uart_stdout = NO_UART;
static uint32_t uart_stderr = NO_UART;

void retarget(int file, uint32_t uart) {
    if (file == STDOUT_FILENO) {
        uart_stdout = uart;
    } else if (file == STDERR_FILENO) {
        uart_stderr = uart;
    }
}

int _write(int file, char *ptr, int len)
{
    int i;

    uint32_t uart = NO_UART;
    if (file == STDOUT_FILENO) {
        uart = uart_stdout;
    } else if (file == STDERR_FILENO) {
        uart = uart_stderr;
    }

    if (uart != NO_UART) {
        for (i = 0; i < len; i++) {
            usart_send_blocking(uart, ptr[i]);
        }
        return i;
    }

    errno = EIO;
    return -1;
}
