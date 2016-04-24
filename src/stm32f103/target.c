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
#include <libopencm3/stm32/gpio.h>

#include "target.h"
#include "config.h"

/* Set STM32 to 72 MHz. */
void clock_setup(void) {
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
}

void gpio_setup(void) {
    /*
      LED0, 1, 2 on PC13, PC14, PC15
      TX, RX (MCU-side) on PA2, PA3
      TGT_RST on PB6
      TGT_SWDIO, TGT_SWCLK on PB14, PB13
      TGT_SWO on PB11
    */

    /* Enable GPIOA, GPIOB, and GPIOC clocks. */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);


    /* Setup LEDs as open-drain outputs */
    const uint8_t mode = GPIO_MODE_OUTPUT_10_MHZ;
    const uint8_t conf = (LED_OPEN_DRAIN ? GPIO_CNF_OUTPUT_OPENDRAIN
                                         : GPIO_CNF_OUTPUT_PUSHPULL);
    gpio_set_mode(GPIOC, mode, conf, GPIO13 | GPIO14 | GPIO15);
}

void led_bit(uint8_t position, bool state) {
    uint32_t gpio = 0xFFFFFFFFU;
    if (position == 0) {
        gpio = GPIO13;
    } else if (position == 1) {
        gpio = GPIO14;
    } else if (position == 2) {
        gpio = GPIO15;
    }

    if (gpio != 0xFFFFFFFFU) {
        if (state ^ LED_OPEN_DRAIN) {
            gpio_set(GPIOC, gpio);
        } else {
            gpio_clear(GPIOC, gpio);
        }
    }
}

void led_num(uint8_t value) {
    if ((value & 0x4) ^ (LED_OPEN_DRAIN << 2)) {
        gpio_set(GPIOC, GPIO15);
    } else {
        gpio_clear(GPIOC, GPIO15);
    }
    if ((value & 0x2) ^ (LED_OPEN_DRAIN << 1)) {
        gpio_set(GPIOC, GPIO14);
    } else {
        gpio_clear(GPIOC, GPIO14);
    }
    if ((value & 0x1) ^ LED_OPEN_DRAIN) {
        gpio_set(GPIOC, GPIO13);
    } else {
        gpio_clear(GPIOC, GPIO13);
    }
}
