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

#include <stddef.h>
#include <string.h>

#include "mtp_structs.h"

size_t mtp_build_string(const char* str, uint8_t* buffer) {
    /* Number of UCS-16 characters, including the null-terminator */
    uint8_t num_chars = 0;

    const char* src = str;
    uint8_t* dest = buffer + 1;

    if (str != NULL && *str != '\0') {
        while (1) {
            *dest = *src;
            *(dest+1) = '\0';
            num_chars += 1;

            if (*src == '\0' || num_chars == 255) {
                break;
            } else {
                dest += 2;
                src  += 1;
            }
        }
    }

    buffer[0] = num_chars;
    return 2*num_chars+1;
}

size_t mtp_build_object_info(const struct mtp_object_info_dataset* object_info,
                             uint8_t* buffer) {
    size_t total = 0;
    
    size_t fixed_length_header_size =
        offsetof(struct mtp_object_info_dataset, filename);
    memcpy(buffer, object_info, fixed_length_header_size);
    buffer += fixed_length_header_size;
    total  += fixed_length_header_size;

    const char* strings[] = {
        object_info->filename,
        object_info->dateCreated,
        object_info->dateModified,
        object_info->keywords
    };

    size_t i;
    for (i=0; i < sizeof(strings)/sizeof(const char*); i++) {
        size_t string_size = mtp_build_string(strings[i], buffer);
        buffer += string_size;
        total  += string_size;
    }

    return total;
}
