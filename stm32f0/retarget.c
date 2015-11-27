#include <errno.h>
#include <stdio.h>

#include <libopencm3/stm32/usart.h>

#include "retarget.h"
#include "console.h"

static uint32_t usart_stdout = NO_USART;
static uint32_t usart_stderr = NO_USART;

void retarget(int file, uint32_t usart) {
    if (file == STDOUT_FILENO) {
        usart_stdout = usart;
    } else if (file == STDERR_FILENO) {
        usart_stderr = usart;
    }
}

int _write(int file, char *ptr, int len)
{
    int i;

    uint32_t usart = 0;
    if (file == STDOUT_FILENO) {
        usart = usart_stdout;
    } else if (file == STDERR_FILENO) {
        usart = usart_stderr;
    }

    if (usart == CONSOLE_USART) {
        for (i = 0; i < len; i++) {
            console_send_blocking(ptr[i]);
        }
        return i;
    }

    errno = EIO;
    return -1;
}
