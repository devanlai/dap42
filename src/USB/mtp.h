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

#ifndef MTP_H_INCLUDED
#define MTP_H_INCLUDED

#include <stdlib.h>

#include <libopencm3/usb/usbd.h>

#include "composite_usb_conf.h"
#include "usb_common.h"
#include "mtp_defs.h"

#define MTP_MAX_COMMAND_PARAMETERS 5
#define MTP_MAX_RESPONSE_PARAMETERS 5
#define MTP_MAX_EVENT_PARAMETERS 3

struct mtp_transaction {
    union {
        struct usb_ptp_command_block header;
        uint8_t data[sizeof(struct usb_ptp_command_block) + 4 * MTP_MAX_COMMAND_PARAMETERS];
    } command;

    union {
        struct usb_ptp_data_block header;
        uint8_t data[sizeof(struct usb_ptp_data_block)];
    } data;

    union {
        struct usb_ptp_response_block header;
        uint8_t data[sizeof(struct usb_ptp_command_block) + 4 * MTP_MAX_RESPONSE_PARAMETERS];
    } response;

    uint8_t buffer[USB_MTP_MAX_PACKET_SIZE];
    size_t buffer_len;

    bool data_header_packet_done;
    uint32_t data_bytes_transferred;
};

typedef struct {
    uint32_t data_length;
    bool has_data_stage;
    bool data_stage_out;
} CommandConfig;

typedef CommandConfig (*HandleCommandCallback)(struct usb_ptp_command_block* command,
                                               struct usb_ptp_response_block* response);

/*
extern void mtp_setup(usbd_device* usbd_dev,
                      HandleCommandCallback handle_command);
*/

extern void mtp_setup(usbd_device* usbd_dev);

typedef bool (*StreamDataReadyCallback)(const struct usb_ptp_command_block* command);
typedef bool (*StreamDataOutCallback)(const struct usb_ptp_command_block* command,
                                      const struct usb_ptp_data_block* data_header,
                                      struct usb_ptp_response_block* response,
                                      const uint8_t* buffer, size_t len);
typedef size_t (*StreamDataInCallback)(const struct usb_ptp_command_block* command,
                                     struct usb_ptp_response_block* response,
                                       uint8_t* data, size_t len);

typedef void (*StreamDataCompleteCallback)(const struct usb_ptp_command_block* command,
                                           struct usb_ptp_response_block* response);

extern void mtp_set_data_ready_callback(StreamDataReadyCallback cb);
extern void mtp_set_data_out_callback(StreamDataOutCallback cb);
extern void mtp_set_data_in_callback(StreamDataInCallback cb);
extern void mtp_set_data_complete_callback(StreamDataCompleteCallback cb);

extern bool mtp_send_event(struct usb_ptp_async_event* event);

extern bool mtp_update(void);

extern void mtp_reset(void);

#endif
