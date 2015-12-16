/*
 * Copyright (c) 2015, Devan Lai
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

#include <stdio.h>
#include <string.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/syscfg.h>

#include "USB/composite_usb_conf.h"
#include "DAP/CMSIS_DAP_config.h"
#include "DAP/CMSIS_DAP.h"
#include "DFU/DFU.h"

#include "tick.h"
#include "retarget.h"
#include "console.h"


void led_num(uint8_t value);
void led_bit(uint8_t position, bool state);

/* Set STM32 to 48 MHz. */
static void clock_setup(void) {
    rcc_clock_setup_in_hsi48_out_48mhz();

    // Trim from USB sync frame
    crs_autotrim_usb_enable();
    rcc_set_usbclk_source(RCC_HSI48);
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

void led_bit(uint8_t position, bool state) {
    uint32_t gpio = 0xFFFFFFFFU;
    if (position == 0) {
        gpio = GPIO4;
    } else if (position == 1) {
        gpio = GPIO1;
    } else if (position == 2) {
        gpio = GPIO0;
    }

    if (gpio != 0xFFFFFFFFU) {
        if (state) {
            gpio_clear(GPIOA, gpio);
        } else {
            gpio_set(GPIOA, gpio);
        }
    }
}

void led_num(uint8_t value) {
    if (value & 0x4) {
        gpio_clear(GPIOA, GPIO0);
    } else {
        gpio_set(GPIOA, GPIO0);
    }
    if (value & 0x2) {
        gpio_clear(GPIOA, GPIO1);
    } else {
        gpio_set(GPIOA, GPIO1);
    }
    if (value & 0x1) {
        gpio_clear(GPIOA, GPIO4);
    } else {
        gpio_set(GPIOA, GPIO4);
    }
}

static uint32_t usb_timer = 0;

static void on_host_tx(uint8_t* data, uint16_t len) {
    usb_timer = 1000;
    console_send_buffered(data, (size_t)len);
}

static void on_host_rx(uint8_t* data, uint16_t* len) {
    usb_timer = 1000;
    *len = (uint16_t)console_recv_buffered(data, USB_CDC_MAX_PACKET_SIZE);
}

static uint8_t request_buffers[DAP_PACKET_SIZE][DAP_PACKET_QUEUE_SIZE];

static uint8_t response_buffers[DAP_PACKET_SIZE][DAP_PACKET_QUEUE_SIZE];
static uint16_t response_lengths[DAP_PACKET_QUEUE_SIZE];

static uint8_t inbox_tail;
static uint8_t process_head;
static uint8_t outbox_head;

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

static bool do_reset_to_dfu = false;
uint32_t DAP_ProcessVendorCommand(uint8_t* request, uint8_t* response) {
    if (request[0] == ID_DAP_Vendor31) {
        if (request[1] == 'D' && request[2] == 'F' && request[3] == 'U') {
            response[0] = request[0];
            response[1] = DAP_OK;
            do_reset_to_dfu = true;
            return 2;
        } else {
            response[0] = request[0];
            response[1] = DAP_ERROR;
            return 2;
        }
    }

    response[0] = ID_DAP_Invalid;
    return 1;
}

int main(void) {
    DFU_maybe_jump_to_bootloader();

    clock_setup();
    tick_setup(1000);
    button_setup();
    gpio_setup();
    led_num(0);

    console_setup(115200);
    retarget(STDOUT_FILENO, CONSOLE_USART);
    retarget(STDERR_FILENO, CONSOLE_USART);

    led_num(1);
    
    DAP_Setup();

    tick_start();

    {
        char serial[USB_SERIAL_NUM_LENGTH+1];
        desig_get_unique_id_as_string(serial, USB_SERIAL_NUM_LENGTH+1);
        cmp_set_usb_serial_number(serial);
    }

    uint16_t cdc_len = 0;
    uint8_t cdc_buf[USB_CDC_MAX_PACKET_SIZE];

    usbd_device* usbd_dev = cmp_setup(&on_send_report, &on_receive_report, &on_host_tx);

    while (1) {
        usbd_poll(usbd_dev);

        // Handle CDC
        if (cdc_len > 0) {
            uint16_t sent = usbd_ep_write_packet(usbd_dev, ENDP_CDC_DATA_IN,
                                                 (const void*)cdc_buf, cdc_len);
            if (sent == cdc_len) {
                usb_timer = 1000;
                cdc_len = 0;
            }
        }

        if (cdc_len == 0) {
            cdc_len = (uint16_t)console_recv_buffered(cdc_buf, USB_CDC_MAX_PACKET_SIZE);
        }

        // Handle DAP
        if (process_head != inbox_tail) {
            uint32_t len = DAP_ProcessCommand(request_buffers[process_head],
                                              response_buffers[process_head]);
            response_lengths[process_head] = (uint16_t)len;
            process_head = (process_head + 1) % DAP_PACKET_QUEUE_SIZE;
        }

        if (outbox_head != process_head) {
            uint16_t sent = usbd_ep_write_packet(usbd_dev, ENDP_HID_REPORT_IN,
                                                 (const void*)response_buffers[outbox_head],
                                                 response_lengths[outbox_head]);
            if (sent != 0) {
                outbox_head = (outbox_head + 1) % DAP_PACKET_QUEUE_SIZE;
            }
        }

        /* If resetting, wait until all pending requests are done */
        if (do_reset_to_dfu && (outbox_head == process_head)) {
            /* Blink 3 times to indicate reset */
            int x;
            for (x=0; x < 3; x++) {
                led_num(7);
                wait_ms(150);
                led_num(0);
                wait_ms(150);
            }

            DFU_reset_and_jump_to_bootloader();
        }

        if (usb_timer > 0) {
            usb_timer--;
            LED_ACTIVITY_OUT(1);
        } else {
            LED_ACTIVITY_OUT(0);
        }
    }

    return 0;
}
