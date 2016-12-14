/*
 * Copyright (c) 2016, Devan Lai
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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/gpio.h>

#include "target.h"
#include "config.h"

/* Reconfigure processor settings */
void cpu_setup(void) {

}

/* Set STM32 to 48 MHz. */
void clock_setup(void) {
    rcc_clock_setup_in_hsi48_out_48mhz();

    // Trim from USB sync frame
    crs_autotrim_usb_enable();
    rcc_set_usbclk_source(RCC_HSI48);
}

void gpio_setup(void) {
    /*
      LED0, 1, 2 on PA4, PA5, PC13
      TX, RX (MCU-side) on PA2, PA3
      TGT_RST on PB13
      TGT_SWDIO, TGT_SWCLK on PB15, PB14
      TGT_SWO on PA10
    */

    /* Enable GPIOA, GPIOB, and GPIOC clocks. */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);


    /* Setup LEDs as open-drain outputs */

    uint8_t otype = (LED_OPEN_DRAIN ? GPIO_OTYPE_OD
                                    : GPIO_OTYPE_PP);
    gpio_set_output_options(GPIOA, otype, GPIO_OSPEED_LOW,
                            GPIO4 | GPIO5);
    gpio_set_output_options(GPIOC, otype, GPIO_OSPEED_LOW,
                            GPIO13);

    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    GPIO4 | GPIO5);
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    GPIO13);
}

void target_console_init(void) {
    /* Enable UART clock */
    rcc_periph_clock_enable(CONSOLE_USART_CLOCK);

    /* Setup GPIO pins for UART2 */
    gpio_mode_setup(CONSOLE_USART_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, CONSOLE_USART_GPIO_PINS);
    gpio_set_af(CONSOLE_USART_GPIO_PORT, CONSOLE_USART_GPIO_AF, CONSOLE_USART_GPIO_PINS);
}

void led_bit(uint8_t position, bool state) {
    uint32_t gpio = 0xFFFFFFFFU;
    uint32_t port = 0xFFFFFFFFU;
    if (position == 0) {
        port = GPIOA;
        gpio = GPIO4;
    } else if (position == 1) {
        port = GPIOA;
        gpio = GPIO5;
    } else if (position == 2) {
        port = GPIOC;
        gpio = GPIO13;
    }

    if (gpio != 0xFFFFFFFFU && port != 0xFFFFFFFFU) {
        if (state ^ LED_OPEN_DRAIN) {
            gpio_set(port, gpio);
        } else {
            gpio_clear(port, gpio);
        }
    }
}

void led_num(uint8_t value) {
    if (((value & 0x4) != 0) ^ LED_OPEN_DRAIN) {
        gpio_set(GPIOC, GPIO13);
    } else {
        gpio_clear(GPIOC, GPIO13);
    }
    if (((value & 0x2) != 0) ^ LED_OPEN_DRAIN) {
        gpio_set(GPIOA, GPIO5);
    } else {
        gpio_clear(GPIOA, GPIO5);
    }
    if (((value & 0x1) != 0) ^ LED_OPEN_DRAIN) {
        gpio_set(GPIOA, GPIO4);
    } else {
        gpio_clear(GPIOA, GPIO4);
    }
}
