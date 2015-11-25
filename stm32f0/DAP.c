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
#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/desig.h>

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>
#include <libopencm3/stm32/st_usbfs.h>

#include <libopencm3/stm32/usart.h>

#include "usb_conf.h"
#include "CMSIS_DAP_config.h"
#include "CMSIS_DAP.h"

#include "tick.h"
#include "retarget.h"

#include <string.h>

void led_num(uint8_t x);

/* Set STM32 to 48 MHz. */
static void clock_setup(void) {
    rcc_clock_setup_in_hsi48_out_48mhz();

    // Trim from USB sync frame
    crs_autotrim_usb_enable();
    rcc_set_usbclk_source(HSI48);
}

static inline uint32_t millis(void) {
    return get_ticks();
}

static inline void wait_ms(uint32_t duration_ms) {
    uint32_t now = millis();
    uint32_t end = now + duration_ms;
    if (end < now) {
        end = 0xFFFFFFFFU - end;
        while (millis() >= now) {
            __asm__("NOP");
        }
    }

    while (millis() < end) {
        __asm__("NOP");
    }
}

void print_hex(uint32_t x) {
    uint8_t i;
    for (i=8; i > 0; i--) {
        uint8_t nibble = (x >> ((i-1) * 4)) & 0xF;
        char nibble_char;
        if (nibble < 10) {
            nibble_char = '0' + nibble;
        } else {
            nibble_char = 'A' + (nibble - 10);
        }

        putchar(nibble_char);
    }
}

static inline void print(const char* s) {
    while (*s != '\0') {
        putchar(*s++);
    }
}

static inline void println(const char* s) {
    puts(s);
}

static void gpio_setup(void) {
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

static void button_setup(void) {
    /* Enable GPIOB clock. */
    rcc_periph_clock_enable(RCC_GPIOB);

    /* Set PB8 to an input */
    gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO8);
}

static void uart_setup(uint32_t baudrate) {
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
void led_num(uint8_t x) {
    if (x & 0x4) {
        gpio_clear(GPIOA, GPIO0);
    } else {
        gpio_set(GPIOA, GPIO0);
    }
    if (x & 0x2) {
        gpio_clear(GPIOA, GPIO1);
    } else {
        gpio_set(GPIOA, GPIO1);
    }
    if (x & 0x1) {
        gpio_clear(GPIOA, GPIO4);
    } else {
        gpio_set(GPIOA, GPIO4);
    }
}

static uint8_t request_buffers[DAP_PACKET_SIZE][DAP_PACKET_QUEUE_SIZE];

static uint8_t response_buffers[DAP_PACKET_SIZE][DAP_PACKET_QUEUE_SIZE];
static uint16_t response_lengths[DAP_PACKET_QUEUE_SIZE];

static uint8_t inbox_tail;
static uint8_t process_head;
static uint8_t outbox_head;

static uint32_t usb_timer = 0;

static void on_receive_report(uint8_t* data, uint16_t len) {
    usb_timer = 1000;
    memcpy((void*)request_buffers[inbox_tail], (const void*)data, len);
    inbox_tail = (inbox_tail + 1) % DAP_PACKET_QUEUE_SIZE;
    
}

static void on_send_report(uint8_t* data, uint16_t* len) {
    usb_timer = 1000;
    if (outbox_head != process_head) {
        memcpy((void*)data, (const void*)response_buffers[outbox_head], response_lengths[outbox_head]);
        *len = response_lengths[outbox_head];

        outbox_head = (outbox_head + 1) % DAP_PACKET_QUEUE_SIZE;
    } else {
        *len = 0;
    }
}

int main(void) {
    clock_setup();
    tick_setup(1000);
    button_setup();
    gpio_setup();
    led_num(0);

    uart_setup(115200);
    retarget(STDOUT_FILENO, USART2);
    retarget(STDERR_FILENO, USART2);

    led_num(1);
    
    DAP_Setup();

    tick_start();
    /*
    {
        char serial[USB_SERIAL_NUM_LENGTH+1];
        desig_get_unique_id_as_string(serial, USB_SERIAL_NUM_LENGTH+1);
        set_usb_serial_number(serial);
    }

    usbd_device* usbd_dev = hid_setup(&on_receive_report, &on_send_report);
    while (1) {
        usbd_poll(usbd_dev);
        if (process_head != inbox_tail) {
            uint32_t len = DAP_ProcessCommand(request_buffers[process_head],
                                              response_buffers[process_head]);
            response_lengths[process_head] = (uint16_t)len;
            process_head = (process_head + 1) % DAP_PACKET_QUEUE_SIZE;
        }

        // TODO: push reports to the endpoint
        if (outbox_head != process_head) {
            usbd_ep_write_packet(usbd_dev, 0x81, (const void*)response_buffers[outbox_head], response_lengths[outbox_head]);

            outbox_head = (outbox_head + 1) % DAP_PACKET_QUEUE_SIZE;
        }

        if (usb_timer > 0) {
            usb_timer--;
            LED_ACTIVITY_OUT(1);
        } else {
            LED_ACTIVITY_OUT(0);
        }
    }

    */

    int i = 0;
    while (1) {
        LED_ACTIVITY_OUT(1);

        print("Hello ");
        print_hex(i);
        println("");

        i++;
        LED_ACTIVITY_OUT(0);
        wait_ms(1000);
    }
    return 0;
}
