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

#ifndef WINUSB_H_INCLUDED
#define WINUSB_H_INCLUDED

#include "winusb_defs.h"

/* Arbitrary, but must be equivalent to the last character in
   the special OS descriptor string */
#define WINUSB_MS_VENDOR_CODE 0x21

#define WINUSB_WINDOWS_VERSION_8_1 0x06030000

extern const struct winusb_platform_descriptor winusb_platform_capability_descriptor;

struct winusb_extended_property_device_interface_guid_descriptor {
    uint32_t dwLength;
    uint16_t bcdVersion;
    uint16_t wIndex;
    uint16_t wCount;
    uint32_t dwSize;
    uint32_t dwPropertyDataType;
    uint16_t wPropertyNameLength;
    const uint16_t bPropertyName[21];
    uint32_t dwPropertyDataLength;
    const uint16_t bPropertyData[];
} __attribute__((packed));

struct winusb_20_device_interface_guids_descriptor {
    uint16_t wLength;
    uint16_t wDescriptorType;
    uint16_t wPropertyDataType;
    uint16_t wPropertyNameLength;
    const uint16_t PropertyName[21];
    uint16_t wPropertyDataLength;
    const uint16_t PropertyData[40];
} __attribute__((packed));

struct winusb_20_device_interface_guid_descriptor {
    uint16_t wLength;
    uint16_t wDescriptorType;
    uint16_t wPropertyDataType;
    uint16_t wPropertyNameLength;
    const uint16_t PropertyName[20];
    uint16_t wPropertyDataLength;
    const uint16_t PropertyData[39];
} __attribute__((packed));

struct winusb_simple_descriptor_set {
    struct winusb_descriptor_set_header header;
    struct winusb_function_subset_header function;
    struct winusb_20_compatible_id_feature_descriptor wcid;
    struct winusb_20_device_interface_guids_descriptor guids;
} __attribute__((packed));

extern void winusb_setup(usbd_device* usbd_dev);

#endif
