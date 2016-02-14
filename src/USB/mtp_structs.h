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

#ifndef MTP_STRUCTS_H
#define MTP_STRUCTS_H

#include <stdint.h>

/* Structs used to build MTP datasets. Only some portions have
 * the same offsets as the final packed response.
 */

struct mtp_object_info_dataset {
    uint32_t storageID;
    uint16_t objectFormat;
    uint16_t protectionStatus;
    uint32_t objectCompressedSize;
    
    /* Only required for actual images */
    uint16_t thumbFormat;
    uint32_t thumbCompressedSize;
    uint32_t thumbPixWidth;
    uint32_t thumbPixHeight;
    uint32_t imagePixWidth;
    uint32_t imagePixHeight;
    uint32_t imageBitDepth;
    
    uint32_t parentObject;
    
    /* Only needed for folders */
    uint16_t associationType;
    uint32_t associationDescription;

    uint32_t sequenceNumber;

    const char* filename;
    const char* dateCreated;
    const char* dateModified;
    const char* keywords;
} __attribute__((packed));

extern size_t mtp_build_string(const char* str, uint8_t* buffer);
extern size_t mtp_build_object_info(const struct mtp_object_info_dataset* object_info, uint8_t* buffer);

#endif
