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

#ifndef COMPOSITE_USB_CONF_H_INCLUDED
#define COMPOSITE_USB_CONF_H_INCLUDED

#include "usb_common.h"
#include "config.h"

#define USB_CDC_MAX_PACKET_SIZE 64
#define USB_VCDC_MAX_PACKET_SIZE 64
#define USB_HID_MAX_PACKET_SIZE 64
#define USB_SERIAL_NUM_LENGTH   24

enum {
    ENDP_CONTROL_OUT = 0x00,
    ENDP_HID_REPORT_OUT,
#if CDC_AVAILABLE
    ENDP_CDC_DATA_OUT,
#endif
#if VCDC_AVAILABLE
    ENDP_VCDC_DATA_OUT,
#endif

    HIGHEST_OUT_ENDPOINT
};

enum {
    ENDP_CONTROL_IN = 0x80,
    ENDP_HID_REPORT_IN,
#if CDC_AVAILABLE
    ENDP_CDC_DATA_IN,
    ENDP_CDC_COMM_IN,
#endif
#if VCDC_AVAILABLE
    ENDP_VCDC_DATA_IN,
    ENDP_VCDC_COMM_IN,
#endif

    HIGHEST_IN_ENDPOINT,
};

enum {
    INTF_HID,
#if CDC_AVAILABLE
    INTF_CDC_COMM,
    INTF_CDC_DATA,
#endif
#if VCDC_AVAILABLE
    INTF_VCDC_COMM,
    INTF_VCDC_DATA,
#endif
#if DFU_AVAILABLE
    INTF_DFU,
#endif
};

enum {
    STR_NONE = 0,
    STR_MANUFACTURER,
    STR_PRODUCT,
    STR_HID_INTF = STR_PRODUCT,
    STR_SERIAL,
#if CDC_AVAILABLE
    STR_CDC_INTF_ASSOC_DESC,
    STR_CDC_CONTROL_INTF,
    STR_CDC_DATA_INTF,
#endif
#if VCDC_AVAILABLE
    STR_VCDC_INTF_ASSOC_DESC,
    STR_VCDC_CONTROL_INTF,
    STR_VCDC_DATA_INTF,
#endif
#if DFU_AVAILABLE
    STR_DFU_INTF,
#endif
};

#define USB_MAX_CONTROL_CLASS_CALLBACKS 8
#define USB_MAX_SET_CONFIG_CALLBACKS    8
#define USB_MAX_RESET_CALLBACKS 8

extern void cmp_set_usb_serial_number(const char* serial);
extern usbd_device* cmp_usb_setup(void);
extern bool cmp_usb_configured(void);
extern void cmp_usb_register_control_class_callback(uint16_t interface,
                                                    usbd_control_callback callback);
extern void cmp_usb_register_set_config_callback(usbd_set_config_callback callback);
extern void cmp_usb_register_reset_callback(GenericCallback callback);

#endif
