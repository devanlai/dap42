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
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>

#include "target.h"
#include "config.h"

static void writel(uint32_t address, uint32_t value) {
    *(volatile uint32_t *)address = value;
}

static void writeh(uint32_t address, uint16_t value) {
    *(volatile uint16_t *)address = value;
}

/* Reconfigure processor settings */
void cpu_setup(void) {
    /* Ensure FLASH_OBR_BOOT_SEL is clear, because BOOT0 is
     * shorted to GND on this board.
     */
    if (FLASH_OBR & FLASH_OBR_BOOT_SEL) {
        /* Sequence adapted from:
         * https://iwasz.pl/electronics/2020-07-29-stm32f042-option-bytes-linux/
         */

        /* Unlock flash bytes */
        writel(0x40022004, 0x45670123);
        writel(0x40022004, 0xCDEF89AB);

        /* Unlock flash option bytes */
        writel(0x40022008, 0x45670123);
        writel(0x40022008, 0xCDEF89AB);

        /* Set the option to "erase" */
        writel(0x40022010, 0x00000220);
        /* Set the "start" bit */
        writel(0x40022010, 0x00000260);
        flash_wait_for_last_operation();
        writel(0x40022010, 0x00000210);

        /* Write the actual values */
        writeh(0x1ffff800, 0x55AA);
        writeh(0x1ffff802, 0x807f);

        /* The new values will take effect after the next power cycle */
    }
}

/* Set STM32 to 48 MHz. */
void clock_setup(void) {
    rcc_clock_setup_in_hsi48_out_48mhz();

    // Trim from USB sync frame
    crs_autotrim_usb_enable();
    rcc_set_usbclk_source(RCC_HSI48);
}

void gpio_setup(void) {
    /* Enable GPIOA and GPIOB clocks. */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
}

void target_console_init(void) {
    /* Enable UART clock */
    rcc_periph_clock_enable(CONSOLE_USART_CLOCK);

    /* Setup GPIO pins for UART2 */
    gpio_mode_setup(CONSOLE_USART_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, CONSOLE_USART_GPIO_PINS);
    gpio_set_af(CONSOLE_USART_GPIO_PORT, CONSOLE_USART_GPIO_AF, CONSOLE_USART_GPIO_PINS);
}

void led_num(uint8_t value) {
    (void)value;
}
