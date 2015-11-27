#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "console.h"

void console_setup(uint32_t baudrate) {
    /* Enable UART clock */
    rcc_periph_clock_enable(RCC_USART2);

    /* Setup GPIO pins for UART2 */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
    gpio_set_af(GPIOA, GPIO_AF1, GPIO2 | GPIO3);

    usart_set_baudrate(USART2, baudrate);
    usart_set_databits(USART2, 8);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_stopbits(USART2, USART_CR2_STOP_1_0BIT);
    usart_set_mode(USART2, USART_MODE_TX_RX);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

    usart_enable(USART2);
}


void usart2_handler(void) {

}
