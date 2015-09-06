/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>,
 * Copyright (C) 2010 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

/* Set STM32 to 48 MHz. */
static void clock_setup(void)
{
	rcc_clock_setup_in_hsi48_out_48mhz();
}

static void gpio_setup(void)
{
    /*
	  Button on PB8
      LED0, 1, 2 on PA0, PA1, PA4
      TX, RX (MCU-side) on PA2, PA3
      TGT_RST on PB1
      TGT_SWDIO, TGT_SWCLK on PA5, PA6
      TGT_SWO on PA7
    */

	/* Enable GPIOA and GPIOB clocks. */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);


    /* Setup LEDs as open-drain outputs */
    gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_LOW,
                            GPIO0 | GPIO1 | GPIO4);

    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    GPIO0 | GPIO1 | GPIO4);
}

static void button_setup(void)
{
    /* Enable GPIOB clock. */
    rcc_periph_clock_enable(RCC_GPIOB);

    /* Set PB8 to an input */
    gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO8);
}

int main(void)
{
    int i;

    clock_setup();
    button_setup();
    gpio_setup();


    gpio_set(GPIOA, GPIO0 | GPIO1 | GPIO4);

	int delay;
    while (1) {
	  if (gpio_get(GPIOB, GPIO8)) {
            delay = 1200000;
		} else {
            delay = 600000;
		}
		
        gpio_toggle(GPIOA, GPIO0);
        for (i = 0; i < delay; i++) {
            __asm__("nop");
        }
		gpio_toggle(GPIOA, GPIO0);
        gpio_toggle(GPIOA, GPIO1);
        for (i = 0; i < delay; i++) {
            __asm__("nop");
        }
		gpio_toggle(GPIOA, GPIO1);
        gpio_toggle(GPIOA, GPIO4);
        for (i = 0; i < delay; i++) {
            __asm__("nop");
        }
		gpio_toggle(GPIOA, GPIO4);
    }

    return 0;
}
