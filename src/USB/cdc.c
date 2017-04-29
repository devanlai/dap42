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

#include <stdlib.h>
#include <string.h>

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>

#include "composite_usb_conf.h"
#include "cdc.h"

#include "console.h"
#include "tick.h"

#if CDC_AVAILABLE

_Static_assert((CONSOLE_TX_BUFFER_SIZE >= USB_CDC_MAX_PACKET_SIZE),
               "TX buffer too small");

/* Descriptors */
const struct cdc_acm_functional_descriptors cdc_acm_functional_descriptors = {
    .header = {
        .bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
        .bcdCDC = 0x0110,
    },
    .call_mgmt = {
        .bFunctionLength =
        sizeof(struct usb_cdc_call_management_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
        .bmCapabilities = 0,
        .bDataInterface = INTF_CDC_DATA,
    },
    .acm = {
        .bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_ACM,
        .bmCapabilities = (1 << 1),
    },
    .cdc_union = {
        .bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_UNION,
        .bControlInterface = INTF_CDC_COMM,
        .bSubordinateInterface0 = INTF_CDC_DATA,
    }
};

/* User callbacks */
static HostOutFunction cdc_rx_callback = NULL;
static SetControlLineStateFunction cdc_set_control_line_state_callback = NULL;
static SetLineCodingFunction cdc_set_line_coding_callback = NULL;
static GetLineCodingFunction cdc_get_line_coding_callback = NULL;

static usbd_device* cdc_usbd_dev;

static void cdc_set_config(usbd_device *usbd_dev, uint16_t wValue);

void cdc_setup(usbd_device* usbd_dev,
               HostOutFunction cdc_rx_cb,
               SetControlLineStateFunction set_control_line_state_cb,
               SetLineCodingFunction set_line_coding_cb,
               GetLineCodingFunction get_line_coding_cb) {
    cdc_usbd_dev = usbd_dev;
    cdc_rx_callback = cdc_rx_cb;
    cdc_set_control_line_state_callback = set_control_line_state_cb,
    cdc_set_line_coding_callback = set_line_coding_cb;
    cdc_get_line_coding_callback = get_line_coding_cb;

    cmp_usb_register_set_config_callback(cdc_set_config);
}

/* Generic CDC-ACM functionality */

bool cdc_send_data(const uint8_t* data, size_t len) {
    if (!cmp_usb_configured()) {
        return false;
    }
    uint16_t sent = usbd_ep_write_packet(cdc_usbd_dev, ENDP_CDC_DATA_IN,
                                         (const void*)data,
                                         (uint16_t)len);
    return (sent != 0);
}

static int cdc_control_class_request(usbd_device *usbd_dev,
                                     struct usb_setup_data *req,
                                     uint8_t **buf, uint16_t *len,
                                     usbd_control_complete_callback* complete) {
    (void)complete;
    (void)usbd_dev;

    if (req->wIndex != INTF_CDC_DATA && req->wIndex != INTF_CDC_COMM) {
        return USBD_REQ_NEXT_CALLBACK;
    }
    int status = USBD_REQ_NOTSUPP;

    switch (req->bRequest) {
        case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
            /*
             * This Linux cdc_acm driver requires this to be implemented
             * even though it's optional in the CDC spec, and we don't
             * advertise it in the ACM functional descriptor.
             */

            bool dtr = (req->wValue & (1 << 0)) != 0;
            bool rts = (req->wValue & (1 << 1)) != 0;

            if (cdc_set_control_line_state_callback) {
                cdc_set_control_line_state_callback(dtr, rts);
            }

            status = USBD_REQ_HANDLED;
            break;
        }
        case USB_CDC_REQ_SET_LINE_CODING: {
            const struct usb_cdc_line_coding *coding;

            if (*len < sizeof(struct usb_cdc_line_coding)) {
                status = USBD_REQ_NOTSUPP;
            } else if (cdc_set_line_coding_callback) {
                coding = (const struct usb_cdc_line_coding *)(*buf);
                if (cdc_set_line_coding_callback(coding)) {
                    status = USBD_REQ_HANDLED;
                } else {
                    status = USBD_REQ_NOTSUPP;
                }
            } else {
                /* No callback - accept whatever is requested */
                status = USBD_REQ_HANDLED;
            }

            break;
        }
        case USB_CDC_REQ_GET_LINE_CODING: {
            struct usb_cdc_line_coding *coding;
            coding = (struct usb_cdc_line_coding*)(*buf);

            if (cdc_get_line_coding_callback) {
                if (cdc_get_line_coding_callback(coding)) {
                    *len = sizeof(struct usb_cdc_line_coding);
                    status = USBD_REQ_HANDLED;
                } else {
                    status = USBD_REQ_NOTSUPP;
                }
            } else {
                status = USBD_REQ_NOTSUPP;
            }
            break;
        }
        default: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
    }

    return status;
}

/* CDC-ACM RX flow control */
static bool cdc_rx_stalled = false;
static void cdc_set_nak(void) {
    if (!cdc_rx_stalled) {
        usbd_ep_nak_set(cdc_usbd_dev, ENDP_CDC_DATA_OUT, true);
        cdc_rx_stalled = true;
    }
}

static void cdc_clear_nak(void) {
    if (cdc_rx_stalled) {
        usbd_ep_nak_set(cdc_usbd_dev, ENDP_CDC_DATA_OUT, false);
        cdc_rx_stalled = false;
    }
}

/* Receive data from the host */
static void cdc_bulk_data_out(usbd_device *usbd_dev, uint8_t ep) {
    // Force NAK to prevent the USB controller from accepting a second
    // packet before we've processed this one to decide if have space
    // to accept more packets.
    cdc_set_nak();

    uint8_t buf[USB_CDC_MAX_PACKET_SIZE];
    uint16_t len = usbd_ep_read_packet(usbd_dev, ep, (void*)buf, sizeof(buf));
    bool accept_more_packets = true;
    if (len > 0 && (cdc_rx_callback != NULL)) {
        accept_more_packets = cdc_rx_callback(buf, len);
    }

    // Handle flow control
    if (accept_more_packets) {
        cdc_clear_nak();
    }
}

static void cdc_bulk_data_in(usbd_device *usbd_dev, uint8_t ep);
static void cdc_start_in_transfer(void);

static void cdc_set_config(usbd_device *usbd_dev, uint16_t wValue) {
    (void)wValue;

    usbd_ep_setup(usbd_dev, ENDP_CDC_DATA_OUT, USB_ENDPOINT_ATTR_BULK, 64,
                  cdc_bulk_data_out);
    usbd_ep_setup(usbd_dev, ENDP_CDC_DATA_IN, USB_ENDPOINT_ATTR_BULK, 64, cdc_bulk_data_in);
    usbd_ep_setup(usbd_dev, ENDP_CDC_COMM_IN, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

    cmp_usb_register_control_class_callback(INTF_CDC_DATA, cdc_control_class_request);
    cmp_usb_register_control_class_callback(INTF_CDC_COMM, cdc_control_class_request);
    cmp_usb_register_sof_callback(cdc_start_in_transfer);
}

/* CDC-ACM USB UART bridge functionality */

// User callbacks
static GenericCallback cdc_uart_rx_callback = NULL;
static GenericCallback cdc_uart_tx_callback = NULL;

static struct usb_cdc_line_coding current_line_coding = {
    .dwDTERate = DEFAULT_BAUDRATE,
    .bCharFormat = USB_CDC_1_STOP_BITS,
    .bParityType = USB_CDC_NO_PARITY,
    .bDataBits = 8
};

void cdc_uart_app_reset(void);

static bool cdc_uart_set_line_coding(const struct usb_cdc_line_coding* line_coding) {
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

    // Reset the output packet buffer
    cdc_uart_app_reset();

    console_reconfigure(line_coding->dwDTERate, databits, stopbits, parity);
    memcpy(&current_line_coding, (const void*)line_coding, sizeof(current_line_coding));

    if (line_coding->bDataBits == 0) {
        current_line_coding.bDataBits = databits;
    }
    return true;
}

static bool cdc_uart_get_line_coding(struct usb_cdc_line_coding* line_coding) {
    memcpy(line_coding, (const void*)&current_line_coding, sizeof(current_line_coding));
    return true;
}

static bool cdc_uart_on_host_tx(uint8_t* data, uint16_t len) {
    console_send_buffered(data, (size_t)len);
    if (cdc_uart_rx_callback) {
        cdc_uart_rx_callback();
    }

    return (console_send_buffer_space() >= USB_CDC_MAX_PACKET_SIZE);
}

static uint16_t packet_len = 0;
static uint8_t packet_buffer[USB_CDC_MAX_PACKET_SIZE];
static uint32_t packet_timeout = 0;
static uint32_t packet_timestamp = 0;
static bool need_zlp = false;

void cdc_uart_app_reset(void) {
    packet_len = 0;
    packet_timestamp = get_ticks();
    need_zlp = false;
    cdc_clear_nak();
}

void cdc_uart_app_setup(usbd_device* usbd_dev,
                   GenericCallback cdc_tx_cb,
                   GenericCallback cdc_rx_cb) {
    cdc_uart_tx_callback = cdc_tx_cb;
    cdc_uart_rx_callback = cdc_rx_cb;

    cdc_setup(usbd_dev,
              &cdc_uart_on_host_tx,
              NULL,
              &cdc_uart_set_line_coding, &cdc_uart_get_line_coding);
    cmp_usb_register_reset_callback(cdc_uart_app_reset);
}

void cdc_uart_app_set_timeout(uint32_t timeout_ms) {
    packet_timeout = timeout_ms;
}

static bool transfer_complete = false;
static void cdc_start_in_transfer(void) {
    transfer_complete = false;
    if (packet_len < USB_CDC_MAX_PACKET_SIZE) {
        uint16_t max_bytes = (USB_CDC_MAX_PACKET_SIZE- packet_len);
        packet_len += console_recv_buffered(&packet_buffer[packet_len], max_bytes);
    }

    if (packet_len > 0) {
        if (cdc_send_data(packet_buffer, packet_len)) {
            transfer_complete = (packet_len < USB_CDC_MAX_PACKET_SIZE);
            packet_len = console_recv_buffered(packet_buffer, USB_CDC_MAX_PACKET_SIZE);
            if (cdc_uart_tx_callback) {
                cdc_uart_tx_callback();
            }
        }
    } else {
        transfer_complete = true;
    }
}

static void cdc_bulk_data_in(usbd_device *usbd_dev, uint8_t ep) {
    (void)usbd_dev;
    (void)ep;

    if (packet_len < USB_CDC_MAX_PACKET_SIZE) {
        uint16_t max_bytes = (USB_CDC_MAX_PACKET_SIZE- packet_len);
        packet_len += console_recv_buffered(&packet_buffer[packet_len], max_bytes);
    }

    if (!transfer_complete) {
        cdc_send_data(packet_buffer, packet_len);
        if (packet_len < USB_CDC_MAX_PACKET_SIZE) {
            transfer_complete = true;
        }
        packet_len = 0;

        if (cdc_uart_tx_callback) {
            cdc_uart_tx_callback();
        }
    }
}

bool cdc_uart_app_update() {
    bool active = false;

    // Handle flow control for data received from the host
    if (console_send_buffer_space() >= USB_CDC_MAX_PACKET_SIZE) {
        cdc_clear_nak();
    }

    return active;
}

#endif
