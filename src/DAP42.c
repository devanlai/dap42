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

#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/iwdg.h>

#include "config.h"
#include "target.h"

#include "USB/composite_usb_conf.h"
#include "USB/cdc.h"
#include "USB/dfu.h"

#include "DAP/app.h"
#include "DAP/CMSIS_DAP_config.h"
#include "DFU/DFU.h"

#include "tick.h"
#include "retarget.h"
#include "console.h"

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

static uint32_t usb_timer = 0;

static void on_host_tx(uint8_t* data, uint16_t len) {
    usb_timer = 1000;
    console_send_buffered(data, (size_t)len);
}

static void on_host_rx(uint8_t* data, uint16_t* len) {
    usb_timer = 1000;
    *len = (uint16_t)console_recv_buffered(data, USB_CDC_MAX_PACKET_SIZE);
}

static struct usb_cdc_line_coding current_line_coding = {
    .dwDTERate = DEFAULT_BAUDRATE,
    .bCharFormat = USB_CDC_1_STOP_BITS,
    .bParityType = USB_CDC_NO_PARITY,
    .bDataBits = 8
};

static bool on_set_line_coding(const struct usb_cdc_line_coding* line_coding) {
    uint32_t databits;
    if (line_coding->bDataBits == 7 || line_coding->bDataBits == 8) {
        databits = line_coding->bDataBits;
    } else if (line_coding->bDataBits == 0) {
        // Work-around for PuTTY on Windows
        databits = current_line_coding.bDataBits;
    } else {
        return false;
    }

    uint32_t stopbits;
    switch (line_coding->bCharFormat) {
        case USB_CDC_1_STOP_BITS:
            stopbits = USART_STOPBITS_1;
            break;
        case USB_CDC_2_STOP_BITS:
            stopbits = USART_STOPBITS_2;
            break;
        default:
            return false;
    }

    uint32_t parity;
    switch(line_coding->bParityType) {
        case USB_CDC_NO_PARITY:
            parity = USART_PARITY_NONE;
            break;
        case USB_CDC_ODD_PARITY:
            parity = USART_PARITY_ODD;
            break;
        case USB_CDC_EVEN_PARITY:
            parity = USART_PARITY_EVEN;
            break;
        default:
            return false;
    }

    console_reconfigure(line_coding->dwDTERate, databits, stopbits, parity);
    memcpy(&current_line_coding, (const void*)line_coding, sizeof(current_line_coding));

    if (line_coding->bDataBits == 0) {
        current_line_coding.bDataBits = databits;
    }
    return true;
}

static bool on_get_line_coding(struct usb_cdc_line_coding* line_coding) {
    memcpy(line_coding, (const void*)&current_line_coding, sizeof(current_line_coding));
    return true;
}

static bool do_reset_to_dfu = false;
static void on_dfu_request(void) {
    do_reset_to_dfu = true;
}

int main(void) {
    DFU_maybe_jump_to_bootloader();

    clock_setup();
    tick_setup(1000);
    gpio_setup();
    led_num(0);

    console_setup(DEFAULT_BAUDRATE);
    retarget(STDOUT_FILENO, CONSOLE_USART);
    retarget(STDERR_FILENO, CONSOLE_USART);

    led_num(1);

    {
        char serial[USB_SERIAL_NUM_LENGTH+1];
        desig_get_unique_id_as_string(serial, USB_SERIAL_NUM_LENGTH+1);
        cmp_set_usb_serial_number(serial);
    }

    usbd_device* usbd_dev = cmp_usb_setup();
    DAP_app_setup(usbd_dev, &on_dfu_request);
    cdc_setup(usbd_dev, &on_host_rx, &on_host_tx,
              NULL, &on_set_line_coding, &on_get_line_coding);
    dfu_setup(usbd_dev, &on_dfu_request);

    uint16_t cdc_len = 0;
    uint8_t cdc_buf[USB_CDC_MAX_PACKET_SIZE];

    tick_start();


    /* Enable the watchdog to enable DFU recovery from bad firmware images */
    iwdg_set_period_ms(1000);
    iwdg_start();

    while (1) {
        iwdg_reset();
        usbd_poll(usbd_dev);

        // Handle CDC
        if (cdc_len > 0) {
            if (cdc_send_data(cdc_buf, cdc_len)) {
                usb_timer = 1000;
                cdc_len = 0;
            }
        }

        if (cdc_len == 0) {
            cdc_len = (uint16_t)console_recv_buffered(cdc_buf, USB_CDC_MAX_PACKET_SIZE);
        }


        // Handle DAP
        bool dap_active = DAP_app_update();
        if (dap_active) {
            usb_timer = 1000;
        } else if (do_reset_to_dfu) {
            /* Blink 3 times to indicate reset */
            int x;
            for (x=0; x < 3; x++) {
                iwdg_reset();
                led_num(7);
                wait_ms(150);
                led_num(0);
                wait_ms(150);
                iwdg_reset();
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
