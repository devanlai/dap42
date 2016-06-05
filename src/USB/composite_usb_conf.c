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
#include <stdint.h>
#include <string.h>

#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/hid.h>
#include <libopencm3/usb/dfu.h>

#include "composite_usb_conf.h"
#include "usb_setup.h"

#include "hid_defs.h"
#include "misc_defs.h"

#include "hid.h"
#include "dfu.h"
#include "cdc.h"
#include "vcdc.h"

#include "config.h"

static const struct usb_device_descriptor dev = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_MISCELLANEOUS_DEVICE,
    .bDeviceSubClass = USB_MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = USB_MISC_PROTOCOL_INTERFACE_ASSOCIATION_DESCRIPTOR,
    .bMaxPacketSize0 = 64,
    .idVendor = 0x1209,
    .idProduct = 0xDA42,
    .bcdDevice = 0x0100,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

#if CDC_AVAILABLE

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


#endif

#if VCDC_AVAILABLE

/*
 * This notification endpoint isn't implemented. According to CDC spec it's
 * optional, but its absence causes a NULL pointer dereference in the
 * Linux cdc_acm driver.
 */
static const struct usb_endpoint_descriptor vcomm_endpoints[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = ENDP_VCDC_COMM_IN,
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        .wMaxPacketSize = 16,
        .bInterval = 1,
    }
};

static const struct usb_endpoint_descriptor vdata_endpoints[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = ENDP_VCDC_DATA_OUT,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = USB_VCDC_MAX_PACKET_SIZE,
        .bInterval = 1,
    },
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = ENDP_VCDC_DATA_IN,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = USB_VCDC_MAX_PACKET_SIZE,
        .bInterval = 1,
    }
};

#endif

#if CDC_AVAILABLE

static const struct usb_iface_assoc_descriptor iface_assoc = {
    .bLength = USB_DT_INTERFACE_ASSOCIATION_SIZE,
    .bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
    .bFirstInterface = INTF_CDC_COMM,
    .bInterfaceCount = 2,
    .bFunctionClass = USB_CLASS_CDC,
    .bFunctionSubClass = USB_CDC_SUBCLASS_ACM,
    .bFunctionProtocol = USB_CDC_PROTOCOL_NONE,
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
    .bInterfaceProtocol = USB_CDC_PROTOCOL_NONE,
    .iInterface = 5,

    .endpoint = comm_endpoints,

    .extra = &cdc_acm_functional_descriptors,
    .extralen = sizeof(cdc_acm_functional_descriptors)
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
    .iInterface = 6,

    .endpoint = data_endpoints,
};

#endif

#if VCDC_AVAILABLE

static const struct usb_iface_assoc_descriptor viface_assoc = {
    .bLength = USB_DT_INTERFACE_ASSOCIATION_SIZE,
    .bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
    .bFirstInterface = INTF_VCDC_COMM,
    .bInterfaceCount = 2,
    .bFunctionClass = USB_CLASS_CDC,
    .bFunctionSubClass = USB_CDC_SUBCLASS_ACM,
    .bFunctionProtocol = USB_CDC_PROTOCOL_NONE,
    .iFunction = 4,
};

static const struct usb_interface_descriptor vcomm_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = INTF_VCDC_COMM,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_CDC,
    .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
    .bInterfaceProtocol = USB_CDC_PROTOCOL_NONE,
    .iInterface = 8,

    .endpoint = vcomm_endpoints,

    .extra = &vcdc_acm_functional_descriptors,
    .extralen = sizeof(vcdc_acm_functional_descriptors)
};

static const struct usb_interface_descriptor vdata_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = INTF_VCDC_DATA,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 9,

    .endpoint = vdata_endpoints,
};

#endif

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

#if DFU_AVAILABLE

static const struct usb_interface_descriptor dfu_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = INTF_DFU,
    .bAlternateSetting = 0,
    .bNumEndpoints = 0,
    .bInterfaceClass = 0xFE,
    .bInterfaceSubClass = 1,
    .bInterfaceProtocol = 1,
    .iInterface = 7,

    .endpoint = NULL,

    .extra = &dfu_function,
    .extralen = sizeof(dfu_function),
};

#endif

static const struct usb_interface interfaces[] = {
    /* HID interface */
    {
        .num_altsetting = 1,
        .altsetting = &hid_iface,
    },
#if CDC_AVAILABLE
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
    },
#endif
#if VCDC_AVAILABLE
    /* Virtual CDC Control Interface */
    {
        .num_altsetting = 1,
        .altsetting = &vcomm_iface,
        .iface_assoc = &viface_assoc
    },
    /* Virtual CDC Data Interface */
    {
        .num_altsetting = 1,
        .altsetting = &vdata_iface,
    },
#endif
#if DFU_AVAILABLE
    /* DFU interface */
    {
        .num_altsetting = 1,
        .altsetting = &dfu_iface,
    }
#endif
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
    (PRODUCT_NAME " CMSIS-DAP"),
    serial_number,
    (PRODUCT_NAME " Composite CDC HID"),
    "CDC Control",
    "CDC Data",
    (PRODUCT_NAME " DFU"),
    "SLCAN CDC Control",
    "SLCAN CDC Data",
};

void cmp_set_usb_serial_number(const char* serial) {
    serial_number[0] = '\0';
    if (serial) {
        strncpy(serial_number, serial, USB_SERIAL_NUM_LENGTH);
        serial_number[USB_SERIAL_NUM_LENGTH] = '\0';
    }
}

/* Buffer to be used for control requests. */
static uint8_t usbd_control_buffer[256] __attribute__ ((aligned (2)));

/* Configuration status */
static bool configured = false;

bool cmp_usb_configured(void) {
    return configured;
}

static void cmp_usb_handle_reset(void) {
    configured = false;
}

/* Class-specific control request handlers */
struct callback_entry {
    usbd_control_callback callback;
    uint16_t interface;
};

static struct callback_entry control_class_callbacks[USB_MAX_CONTROL_CLASS_CALLBACKS];
static uint8_t num_control_class_callbacks;

/* Config setup handlers */
static usbd_set_config_callback set_config_callbacks[USB_MAX_SET_CONFIG_CALLBACKS];
static uint8_t num_set_config_callbacks;

void cmp_usb_register_control_class_callback(uint16_t interface,
                                             usbd_control_callback callback) {
    if (num_control_class_callbacks < USB_MAX_CONTROL_CLASS_CALLBACKS) {
        control_class_callbacks[num_control_class_callbacks].interface = interface;
        control_class_callbacks[num_control_class_callbacks].callback = callback;
        num_control_class_callbacks++;
    }
}

static int cmp_usb_dispatch_control_class_request(usbd_device *usbd_dev,
                                                  struct usb_setup_data *req,
                                                  uint8_t **buf, uint16_t *len,
                                                  usbd_control_complete_callback* complete) {

    int result = USBD_REQ_NEXT_CALLBACK;

    uint8_t i;
    uint16_t interface = req->wIndex;
    for (i=0; i < num_control_class_callbacks; i++) {
        if (interface == control_class_callbacks[i].interface) {
            usbd_control_callback callback = control_class_callbacks[i].callback;
            result = callback(usbd_dev, req, buf, len, complete);
            if (result == USBD_REQ_HANDLED || result == USBD_REQ_NOTSUPP) {
                break;
            }
        }
    }

    return result;
}

void cmp_usb_register_set_config_callback(usbd_set_config_callback callback) {
    if (callback && num_set_config_callbacks < USB_MAX_SET_CONFIG_CALLBACKS) {
        set_config_callbacks[num_set_config_callbacks++] = callback;
    }
}

static void cmp_usb_set_config(usbd_device* usbd_dev, uint16_t wValue) {
    uint8_t i;
    /* Remove existing callbacks, to be re-registered by
       set-config callbacks below */
    for (i=0; i < USB_MAX_CONTROL_CLASS_CALLBACKS; i++) {
        control_class_callbacks[i].interface = 0;
        control_class_callbacks[i].callback = NULL;
    }

    num_control_class_callbacks = 0;

    /* Register our class-specific control request dispatcher */
    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        cmp_usb_dispatch_control_class_request);

    /* Record that we're configured */
    configured = true;

    /* Run registered setup handlers */
    for (i=0; i < num_set_config_callbacks; i++) {
        if (set_config_callbacks[i]) {
            set_config_callbacks[i](usbd_dev, wValue);
        }
    }
}

usbd_device* cmp_usb_setup(void) {
    int num_strings = sizeof(usb_strings)/sizeof(const char*);

    const usbd_driver* driver = target_usb_init();
    usbd_device* usbd_dev = usbd_init(driver, &dev, &config,
                                      usb_strings, num_strings,
                                      usbd_control_buffer, sizeof(usbd_control_buffer));
    usbd_register_set_config_callback(usbd_dev, cmp_usb_set_config);
    usbd_register_reset_callback(usbd_dev, cmp_usb_handle_reset);
    return usbd_dev;
}
