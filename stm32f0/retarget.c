#include <errno.h>
#include <stdio.h>

#include <libopencm3/stm32/usart.h>

#include "retarget.h"
#include "console.h"

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
    }

    errno = EIO;
    return -1;
}
