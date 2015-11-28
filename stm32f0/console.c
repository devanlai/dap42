#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

#include "console.h"

void console_setup(uint32_t baudrate) {
    /* Enable UART clock */
    rcc_periph_clock_enable(RCC_USART2);

    /* Setup GPIO pins for UART2 */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
    gpio_set_af(GPIOA, GPIO_AF1, GPIO2 | GPIO3);

    usart_set_baudrate(CONSOLE_USART, baudrate);
    usart_set_databits(CONSOLE_USART, 8);
    usart_set_parity(CONSOLE_USART, USART_PARITY_NONE);
    usart_set_stopbits(CONSOLE_USART, USART_CR2_STOP_1_0BIT);
    usart_set_mode(CONSOLE_USART, USART_MODE_TX_RX);
    usart_set_flow_control(CONSOLE_USART, USART_FLOWCONTROL_NONE);

    usart_enable(CONSOLE_USART);
}


void usart2_isr(void) {

}
