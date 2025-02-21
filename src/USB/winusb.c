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

#include "usb_common.h"
#include "winusb.h"
#include "usb21_standard.h"

#include "composite_usb_conf.h"

#if WINUSB_AVAILABLE && (DFU_AVAILABLE || BULK_AVAILABLE)

static const struct winusb_simple_descriptor_set winusb_desc_set = {
    .header = {
            .wLength = sizeof(struct winusb_descriptor_set_header),
            .wDescriptorType = WINUSB_DT_MS_OS_20_SET_HEADER_DESCRIPTOR,
            .dwWindowsVersion = WINUSB_WINDOWS_VERSION_8_1,
            .wTotalLength = sizeof(winusb_desc_set),
        },
#if DFU_AVAILABLE
    .dfu = {
        .function = {
            .wLength = sizeof(struct winusb_function_subset_header),
            .wDescriptorType = WINUSB_DT_MS_OS_20_SUBSET_HEADER_FUNCTION,
            .bFirstInterface = INTF_DFU,
            .wSubsetLength = (sizeof(struct winusb_function_subset_header) +
                            sizeof(struct winusb_20_compatible_id_feature_descriptor) +
                            sizeof(struct winusb_20_device_interface_guids_descriptor)),
        },
        .wcid = {
            .wLength = 20,
            .wDescriptorType = WINUSB_DT_MS_OS_20_FEATURE_COMPATIBLE_ID,
            .compatibleId = "WINUSB",
            .subCompatibleId = ""
        },
        .guids = {
            .wLength = sizeof(struct winusb_20_device_interface_guids_descriptor),
            .wDescriptorType = WINUSB_DT_MS_OS_20_FEATURE_REG_PROPERTY,
            .wPropertyDataType = WINUSB_PROP_DATA_TYPE_REG_MULTI_SZ,
            .wPropertyNameLength = 42,
            .PropertyName = u"DeviceInterfaceGUIDs",
            .wPropertyDataLength = 80,
            .PropertyData = u"{DA103DEF-26A6-47D9-970E-B3D561A277FA}\0"
        },
    },
#endif
#if BULK_AVAILABLE
    .bulk = {
        .function = {
            .wLength = sizeof(struct winusb_function_subset_header),
            .wDescriptorType = WINUSB_DT_MS_OS_20_SUBSET_HEADER_FUNCTION,
            .bFirstInterface = INTF_BULK,
            .wSubsetLength = (sizeof(struct winusb_function_subset_header) +
                            sizeof(struct winusb_20_compatible_id_feature_descriptor) +
                            sizeof(struct winusb_20_device_interface_guids_descriptor)),
        },
        .wcid = {
            .wLength = 20,
            .wDescriptorType = WINUSB_DT_MS_OS_20_FEATURE_COMPATIBLE_ID,
            .compatibleId = "WINUSB",
            .subCompatibleId = ""
        },
        .guids = {
            .wLength = sizeof(struct winusb_20_device_interface_guids_descriptor),
            .wDescriptorType = WINUSB_DT_MS_OS_20_FEATURE_REG_PROPERTY,
            .wPropertyDataType = WINUSB_PROP_DATA_TYPE_REG_MULTI_SZ,
            .wPropertyNameLength = 42,
            .PropertyName = u"DeviceInterfaceGUIDs",
            .wPropertyDataLength = 80,
            .PropertyData = u"{DA103DEF-26A6-47D9-970E-B3D561A277FA}\0"
        },
    },
#endif
};

const struct winusb_platform_descriptor winusb_platform_capability_descriptor = {
    .bLength = (WINUSB_PLATFORM_DESCRIPTOR_HEADER_SIZE +
                1*WINUSB_DESCRIPTOR_SET_INFORMATION_SIZE),
    .bDescriptorType = USB_DT_DEVICE_CAPABILITY,
    .bDevCapabilityType = USB_DC_PLATFORM,
    .platformCapabilityUUID = WINUSB_OS_20_UUID,
    .descriptor_set_information = {
        {
            .dwWindowsVersion = WINUSB_WINDOWS_VERSION_8_1,
            .wMSOSDescriptorSetTotalLength = sizeof(winusb_desc_set),
            .bMS_VendorCode = WINUSB_MS_VENDOR_CODE,
            .bAltEnumCode = 0,
        },
    }
};

static enum usbd_request_return_codes winusb_control_vendor_request(usbd_device *usbd_dev,
                                         struct usb_setup_data *req,
                                         uint8_t **buf, uint16_t *len,
                                         usbd_control_complete_callback* complete) {
    (void)complete;
    (void)usbd_dev;

    if (req->bRequest != WINUSB_MS_VENDOR_CODE) {
        return USBD_REQ_NEXT_CALLBACK;
    }

    int status = USBD_REQ_NOTSUPP;
    if (req->wIndex == WINUSB_REQ_MS_OS_20_DESCRIPTOR_INDEX) {
        *buf = (uint8_t*)(&winusb_desc_set);
        if (*len > winusb_desc_set.header.wTotalLength) {
            *len = winusb_desc_set.header.wTotalLength;
        }
        status = USBD_REQ_HANDLED;
    } else {
        status = USBD_REQ_NOTSUPP;
    }

    return status;
}

static void winusb_set_config(usbd_device* usbd_dev, uint16_t wValue) {
    (void)wValue;
    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        winusb_control_vendor_request);
}

static const struct usb_device_capability_descriptor* capabilities[] = {
    (const struct usb_device_capability_descriptor*)&winusb_platform_capability_descriptor,
};

static const struct usb_bos_descriptor winusb_bos_descriptor = {
    .bLength = USB_DT_BOS_SIZE,
    .bDescriptorType = USB_DT_BOS,
    .wTotalLength = (USB_DT_BOS_SIZE +
                     WINUSB_PLATFORM_DESCRIPTOR_HEADER_SIZE +
                     1*WINUSB_DESCRIPTOR_SET_INFORMATION_SIZE),
    .bNumDeviceCaps = 1,
    .capabilities = capabilities
};

void winusb_setup(usbd_device* usbd_dev) {
    usb21_setup(usbd_dev, &winusb_bos_descriptor);
    usbd_register_set_config_callback(usbd_dev, winusb_set_config);

    /* Windows probes the compatible ID before setting the configuration,
       so also register the callback now */
    winusb_set_config(usbd_dev, 0x0000);
}

#endif
