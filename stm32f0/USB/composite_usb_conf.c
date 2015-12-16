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
#include "composite_usb_conf.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/st_usbfs.h>
#include <libopencm3/stm32/syscfg.h>

#include <string.h>

#include "hid_defs.h"
#include "misc_defs.h"

static const struct usb_device_descriptor dev = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_MISCELLANEOUS_DEVICE,
    .bDeviceSubClass = USB_MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = USB_MISC_PROTOCOL_INTERFACE_ASSOCIATION_DESCRIPTOR,
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
        .bEndpointAddress = ENDP_CDC_COMM_IN,
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        .wMaxPacketSize = 16,
        .bInterval = 1,
    }
};

static const struct usb_endpoint_descriptor data_endpoints[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = ENDP_CDC_DATA_OUT,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = USB_CDC_MAX_PACKET_SIZE,
        .bInterval = 1,
    },
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = ENDP_CDC_DATA_IN,
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

static const struct usb_iface_assoc_descriptor iface_assoc = {
    .bLength = USB_DT_INTERFACE_ASSOCIATION_SIZE,
    .bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
    .bFirstInterface = INTF_CDC_COMM,
    .bInterfaceCount = 2,
    .bFunctionClass = USB_CLASS_CDC,
    .bFunctionSubClass = 0,
    .bFunctionProtocol = 0,
    .iFunction = 4,
};

static const struct usb_interface_descriptor comm_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = INTF_CDC_COMM,
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
    .bInterfaceNumber = INTF_CDC_DATA,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 0,

    .endpoint = data_endpoints,
};

static const uint8_t hid_report_descriptor[] = {
    0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,        // Usage (0x01)
    0xA1, 0x01,        // Collection (Application)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x40,        //   Report Count (64)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x40,        //   Report Count (64)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,        //   Report Count (1)
    0x09, 0x01,        //   Usage (0x01)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection

    // 33 bytes
};

static const struct {
    struct usb_hid_descriptor hid_descriptor;
    struct {
        uint8_t bReportDescriptorType;
        uint16_t wDescriptorLength;
    } __attribute__((packed)) hid_report;
} __attribute__((packed)) hid_function = {
    .hid_descriptor = {
        .bLength = sizeof(hid_function),
        .bDescriptorType = USB_DT_HID,
        .bcdHID = 0x0111,
        .bCountryCode = 0,
        .bNumDescriptors = 1,
    },
    .hid_report = {
        .bReportDescriptorType = USB_DT_REPORT,
        .wDescriptorLength = sizeof(hid_report_descriptor),
    },
};

static const struct usb_endpoint_descriptor hid_endpoints[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = ENDP_HID_REPORT_IN,
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        .wMaxPacketSize = USB_HID_MAX_PACKET_SIZE,
        .bInterval = 1,
    },
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = ENDP_HID_REPORT_OUT,
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        .wMaxPacketSize = USB_HID_MAX_PACKET_SIZE,
        .bInterval = 1,
    },
};

static const struct usb_interface_descriptor hid_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = INTF_HID,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_HID,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 2,

    .endpoint = hid_endpoints,

    .extra = &hid_function,
    .extralen = sizeof(hid_function),
};

static const struct usb_interface interfaces[] = {
    /* HID interface */
    {
        .num_altsetting = 1,
        .altsetting = &hid_iface,
    },
    /* CDC Control Interface */
    {
        .num_altsetting = 1,
        .altsetting = &comm_iface,
        .iface_assoc = &iface_assoc
    },
    /* CDC Data Interface */
    {
        .num_altsetting = 1,
        .altsetting = &data_iface,
    }
};

static const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = sizeof(interfaces)/sizeof(struct usb_interface),
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0xC0,
    .bMaxPower = 0x32,

    .interface = interfaces,
};

static char serial_number[USB_SERIAL_NUM_LENGTH+1] = "000000000000000000000000";

static const char *usb_strings[] = {
    "Devanarchy",
    "DAP42 CMSIS-DAP",
    serial_number,
    "DAP42 Composite CDC HID"
};

/* Buffer to be used for control requests. */
static uint8_t usbd_control_buffer[128] __attribute__ ((aligned (2)));

/* User callbacks */
static HostOutFunction cdc_rx_callback = NULL;
static HostInFunction cdc_tx_callback = NULL;
static HostOutFunction hid_report_out_callback = NULL;
static HostInFunction hid_report_in_callback = NULL;

void cmp_set_usb_serial_number(const char* serial) {
    serial_number[0] = '\0';
    if (serial) {
        strncpy(serial_number, serial, USB_SERIAL_NUM_LENGTH);
        serial_number[USB_SERIAL_NUM_LENGTH] = '\0';
    }
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

static int hid_control_standard_request(usbd_device *usbd_dev,
                                        struct usb_setup_data *req,
                                        uint8_t **buf, uint16_t *len,
                                        usbd_control_complete_callback* complete) {
    (void)complete;
    (void)usbd_dev;

    if (req->wIndex != INTF_HID) {
        return USBD_REQ_NEXT_CALLBACK;
    }

    int status = USBD_REQ_NOTSUPP;
    switch (req->bRequest) {
        case USB_REQ_GET_DESCRIPTOR: {
            uint8_t descriptorType = (uint8_t)((req->wValue >> 8) & 0xFF);
            uint8_t descriptorIndex = (uint8_t)(req->wValue & 0xFF);
            if (descriptorType == USB_DT_REPORT) {
                if (descriptorIndex == 0) {
                    *buf = (uint8_t *)hid_report_descriptor;
                    *len = sizeof(hid_report_descriptor);
                    status = USBD_REQ_HANDLED;
                }
            }
            break;
        }
        case USB_REQ_SET_DESCRIPTOR: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
        default: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
    }

    return status;
}

static int hid_control_class_request(usbd_device *usbd_dev,
                                     struct usb_setup_data *req,
                                     uint8_t **buf, uint16_t *len,
                                     usbd_control_complete_callback* complete) {
    (void)complete;
    (void)usbd_dev;

    if (req->wIndex != INTF_HID) {
        return USBD_REQ_NEXT_CALLBACK;
    }

    int status = USBD_REQ_NOTSUPP;
    switch (req->bRequest) {
        case USB_HID_REQ_GET_REPORT: {
            if ((hid_report_in_callback != NULL) && (*buf != NULL)) {
                hid_report_in_callback(*buf, len);
                status = USBD_REQ_HANDLED;
            }
            break;
        }
        case USB_HID_REQ_SET_REPORT: {
            if ((hid_report_out_callback != NULL) && (*len > 0))
            {
                hid_report_out_callback(*buf, *len);
                status = USBD_REQ_HANDLED;
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

/* Receive data from the host */
static void cdc_bulk_data_out(usbd_device *usbd_dev, uint8_t ep) {
    uint8_t buf[USB_CDC_MAX_PACKET_SIZE];
    uint16_t len = usbd_ep_read_packet(usbd_dev, ep, (void*)buf, sizeof(buf));
    if (len > 0 && (cdc_rx_callback != NULL)) {
        cdc_rx_callback(buf, len);
    }
}

/* Handle sending a report to the host */
static void hid_interrupt_in(usbd_device *usbd_dev, uint8_t ep) {
    if (hid_report_in_callback != NULL) {
        uint8_t buf[USB_HID_MAX_PACKET_SIZE];
        uint16_t len = 0;
        hid_report_in_callback(buf, &len);
        if (len > 0) {
            usbd_ep_write_packet(usbd_dev, ep, (const void*)buf, len);
        }
    }
}

/* Receive data from the host */
static void hid_interrupt_out(usbd_device *usbd_dev, uint8_t ep) {
    uint8_t buf[USB_HID_MAX_PACKET_SIZE];
    uint16_t len = usbd_ep_read_packet(usbd_dev, ep, (void*)buf, sizeof(buf));
    if (len > 0 && (hid_report_out_callback != NULL)) {
        hid_report_out_callback(buf, len);
    }
}

static void cmp_set_config(usbd_device *usbd_dev, uint16_t wValue) {
    (void)wValue;

    usbd_ep_setup(usbd_dev, ENDP_CDC_DATA_OUT, USB_ENDPOINT_ATTR_BULK, 64,
                  cdc_bulk_data_out);
    usbd_ep_setup(usbd_dev, ENDP_CDC_DATA_IN, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    usbd_ep_setup(usbd_dev, ENDP_CDC_COMM_IN, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

    usbd_ep_setup(usbd_dev, ENDP_HID_REPORT_OUT, USB_ENDPOINT_ATTR_INTERRUPT, 64,
                  &hid_interrupt_out);
    usbd_ep_setup(usbd_dev, ENDP_HID_REPORT_IN, USB_ENDPOINT_ATTR_INTERRUPT, 64,
                  &hid_interrupt_in);

    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        cdc_control_class_request);

    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        hid_control_standard_request);

    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        hid_control_class_request);
}

usbd_device* cmp_setup(HostInFunction report_send_cb, HostOutFunction report_recv_cb, HostOutFunction cdc_rx_cb) {
    hid_report_out_callback = report_recv_cb;
    hid_report_in_callback = report_send_cb;
    cdc_rx_callback = cdc_rx_cb;
    //cdc_tx_callback = tx_cb;

    rcc_periph_reset_pulse(RST_USB);

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_SYSCFG_COMP);

    /* Remap PA11 and PA12 for use as USB */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    GPIO11 | GPIO12);
    gpio_set_af(GPIOA, GPIO_AF2, GPIO11 | GPIO12);
    SYSCFG_CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;

    usbd_device* usbd_dev = usbd_init(&st_usbfs_v2_usb_driver, &dev, &config,
                                      usb_strings, 4,
                                      usbd_control_buffer, sizeof(usbd_control_buffer));
    usbd_register_set_config_callback(usbd_dev, cmp_set_config);

    return usbd_dev;
}
