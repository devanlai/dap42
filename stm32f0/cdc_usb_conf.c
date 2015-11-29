/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
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

#include <stdlib.h>
#include "cdc_usb_conf.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/st_usbfs.h>
#include <libopencm3/stm32/syscfg.h>

#include <string.h>

extern void led_num(uint8_t x);

static const struct usb_device_descriptor dev = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_CDC,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = 0x1209,
    .idProduct = 0x0001,
    .bcdDevice = 0x0100,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec it's
 * optional, but its absence causes a NULL pointer dereference in the
 * Linux cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endpoints[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x83,
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        .wMaxPacketSize = 16,
        .bInterval = 1,
    }
};

static const struct usb_endpoint_descriptor data_endpoints[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x01,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = USB_CDC_MAX_PACKET_SIZE,
        .bInterval = 1,
    },
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x82,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = USB_CDC_MAX_PACKET_SIZE,
        .bInterval = 1,
    }
};

static const struct {
    struct usb_cdc_header_descriptor header;
    struct usb_cdc_call_management_descriptor call_mgmt;
    struct usb_cdc_acm_descriptor acm;
    struct usb_cdc_union_descriptor cdc_union;
} __attribute__ ((packed)) cdcacm_functional_descriptors = {
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
        .bDataInterface = 1,
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
        .bControlInterface = 0,
        .bSubordinateInterface0 = 1,
    }
};

static const struct usb_interface_descriptor comm_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_CDC,
    .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
    .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
    .iInterface = 0,

    .endpoint = comm_endpoints,

    .extra = &cdcacm_functional_descriptors,
    .extralen = sizeof(cdcacm_functional_descriptors)
};

static const struct usb_interface_descriptor data_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 1,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 0,

    .endpoint = data_endpoints,
};

static const struct usb_interface cdc_interfaces[] = {
    {
        .num_altsetting = 1,
        .altsetting = &comm_iface,
    }, {
        .num_altsetting = 1,
        .altsetting = &data_iface,
    }
};

static const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 2,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0xC0,
    .bMaxPower = 0x32,

    .interface = cdc_interfaces,
};

static char serial_number[USB_SERIAL_NUM_LENGTH+1] = "000000000000000000000000";

static const char *usb_strings[] = {
    "Devanarchy",
    "DAP42 CMSIS-DAP",
    serial_number,
    "huh?"
};

/* Buffer to be used for control requests. */
static uint8_t usbd_control_buffer[128];

void cdc_set_usb_serial_number(const char* serial) {
    serial_number[0] = '\0';
    if (serial) {
        strncpy(serial_number, serial, USB_SERIAL_NUM_LENGTH);
        serial_number[USB_SERIAL_NUM_LENGTH] = '\0';
    }
}

static int cdc_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
                               void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {
    (void)complete;
    (void)usbd_dev;

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

            (void)dtr;
            (void)rts;

            //glue_set_line_state_cb(dtr, rts);

            status = USBD_REQ_HANDLED;
            break;
        }
        case USB_CDC_REQ_SET_LINE_CODING: {
            struct usb_cdc_line_coding *coding;

            if (*len < sizeof(struct usb_cdc_line_coding)) {
                status = USBD_REQ_NOTSUPP;
                break;
            }

            coding = (struct usb_cdc_line_coding *)*buf;
            (void)coding;
            /*
            return glue_set_line_coding_cb(coding->dwDTERate,
                               coding->bDataBits,
                               coding->bParityType,
                               coding->bCharFormat);
            */
            status = USBD_REQ_HANDLED;
            break;
        }
        default: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
    }

    return status;
}

/* User callbacks */
static ReceiveDataFunction rx_callback = NULL;
static TransmitDataFunction tx_callback = NULL;

/* Receive data from the host */
static void cdc_bulk_data_out(usbd_device *usbd_dev, uint8_t ep) {
    uint8_t buf[USB_CDC_MAX_PACKET_SIZE];
    uint16_t len = usbd_ep_read_packet(usbd_dev, ep, (void*)buf, sizeof(buf));
    if (len > 0 && (rx_callback != NULL)) {
        rx_callback(buf, len);
    }
}

static void cdc_set_config(usbd_device *usbd_dev, uint16_t wValue) {
    (void)wValue;

    usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64,
                  cdc_bulk_data_out);
    usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        cdc_control_request);
}

usbd_device* cdc_setup(ReceiveDataFunction rx_cb, TransmitDataFunction tx_cb) {
    rx_callback = rx_cb;
    tx_callback = tx_cb;
    
    /* TODO: properly reset USB so that we re-enumerate correctly when from the
       USB DFU bootloader */
    rcc_periph_reset_pulse(RST_USB);

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_SYSCFG_COMP);

    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    GPIO11 | GPIO12);
    gpio_set_af(GPIOA, GPIO_AF2, GPIO11 | GPIO12);
    SYSCFG_CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;

    usbd_device* usbd_dev = usbd_init(&st_usbfs_v2_usb_driver, &dev, &config,
                                      usb_strings, 4,
                                      usbd_control_buffer, sizeof(usbd_control_buffer));
    usbd_register_set_config_callback(usbd_dev, cdc_set_config);
    
    return usbd_dev;
}
