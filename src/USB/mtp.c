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

#include "composite_usb_conf.h"
#include "mtp.h"
#include "mtp_datasets.h"

#if MTP_AVAILABLE

enum MTPState {
    RESET, IDLE, COMMAND, DATA_IN, DATA_OUT, RESPONSE, STALLED
};

static enum MTPState mtp_state = RESET;
static usbd_device* mtp_usbd_dev = NULL;
static bool mtp_enabled = false;
static uint8_t mtp_data_buffer[256] __attribute__ ((aligned (4)));
static size_t mtp_data_buffer_count;
static size_t mtp_data_buffer_offset;

/* User Callbacks */
/*
static StreamDataReadyCallback mtp_stream_data_ready;
static StreamDataOutCallback mtp_stream_data_out;
static StreamDataInCallback mtp_stream_data_in;
static StreamDataCompleteCallback mtp_stream_data_complete;
static HandleCommandCallback mtp_handle_command;
*/

static bool mtp_stream_data_ready(const struct usb_ptp_command_block* command) {
    (void)command;
    return true;
}

static bool mtp_stream_data_out(const struct usb_ptp_command_block* command,
                                const struct usb_ptp_data_block* data_header,
                                struct usb_ptp_response_block* response,
                                const uint8_t* data, size_t len) {
    (void)command;
    (void)data_header;
    (void)response;
    (void)data;
    (void)len;
    // Discard received data
    return true;
}

static size_t mtp_stream_data_in(const struct usb_ptp_command_block* command,
                                 struct usb_ptp_response_block* response,
                                 uint8_t* buffer, size_t len) {
    (void)command;
    (void)response;
    (void)buffer;
    (void)len;
    size_t bytes_to_move = mtp_data_buffer_count - mtp_data_buffer_offset;
    if (bytes_to_move > len) {
        bytes_to_move = len;
    }
    memcpy(buffer, &mtp_data_buffer[mtp_data_buffer_offset], bytes_to_move);
    mtp_data_buffer_offset += bytes_to_move;
    return bytes_to_move;
}

static bool mtp_stream_data_complete(const struct usb_ptp_command_block* command,
                                     struct usb_ptp_response_block* response) {
    (void)command;
    (void)response;
    // Ignore end-of-data-phase notification
    return true;
}

static CommandConfig mtp_handle_command(struct usb_ptp_command_block* command,
                                        struct usb_ptp_response_block* response) {
    CommandConfig config;
    config.data_length = 0;
    config.has_data_stage = false;
    config.data_stage_out = false;
    switch (command->code) {
        case OPR_GetDeviceInfo: {
            response->code = RSP_OK;
            config.has_data_stage = true;
            config.data_length = sizeof(mtp_device_info);
            mtp_data_buffer_offset = 0;
            mtp_data_buffer_count = sizeof(mtp_device_info);
            memcpy(mtp_data_buffer, mtp_device_info, sizeof(mtp_device_info));
            break;
        }
        case OPR_OpenSession: {
            response->code = RSP_OK;
            break;
        }
        case OPR_CloseSession: {
            response->code = RSP_OK;
            break;
        }
        case OPR_GetStorageIDs: {
            response->code = RSP_OK;
            config.has_data_stage = true;
            config.data_length = 8;
            struct storage_id_array {
                uint32_t count;
                uint32_t ids[0];
            };

            struct storage_id_array* arr = (struct storage_id_array*)(mtp_data_buffer);
            arr->count = 1;
            arr->ids[0] = 0x00010001U;
            mtp_data_buffer_count = 8;
            mtp_data_buffer_offset = 0;
            break;
        }
        case OPR_GetStorageInfo: {
            response->code = RSP_OK;
            config.has_data_stage = true;
            config.data_length = sizeof(mtp_test_store_info);
            memcpy(mtp_data_buffer, mtp_test_store_info, sizeof(mtp_test_store_info));
            mtp_data_buffer_count = sizeof(mtp_test_store_info);
            mtp_data_buffer_offset = 0;
            break;
        }
        default: {
            response->code = RSP_Operation_Not_Supported;
            break;
        }
    }

    return config;
}

static int mtp_control_class_request(usbd_device *usbd_dev,
                                     struct usb_setup_data *req,
                                     uint8_t **buf, uint16_t *len,
                                     usbd_control_complete_callback* complete) {
    (void)complete;
    (void)usbd_dev;

    if (req->wIndex != INTF_MTP) {
        return USBD_REQ_NEXT_CALLBACK;
    }

    int status;
    switch (req->bRequest) {
        case USB_PTP_REQ_CANCEL_REQ: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
        case USB_PTP_REQ_GET_EXTENDED_EVENT_DATA: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
        case USB_PTP_REQ_DEVICE_RESET: {
            mtp_reset();
            status = USBD_REQ_HANDLED;
            break;
        }
        case USB_PTP_REQ_GET_DEVICE_STATUS: {
            struct usb_ptp_get_device_status_request* resp;
            resp = (struct usb_ptp_get_device_status_request*)(*buf);
            if (mtp_state == STALLED) {
                bool out_stalled = usbd_ep_stall_get(usbd_dev, ENDP_MTP_DATA_OUT) != 0;
                bool in_stalled = usbd_ep_stall_get(usbd_dev, ENDP_MTP_DATA_IN) != 0;

                uint32_t* params = (uint32_t*)(resp->payload);

                if (in_stalled && out_stalled) {
                    resp->code = RSP_Transaction_Cancelled;
                    resp->length = sizeof(*resp) + 2 * 4;
                    params[0] = ENDP_MTP_DATA_OUT;
                    params[1] = ENDP_MTP_DATA_IN;
                } else if (in_stalled || out_stalled) {
                    resp->code = RSP_Transaction_Cancelled;
                    resp->length = sizeof(*resp) + 1 * 4;
                    params[0] = in_stalled ? ENDP_MTP_DATA_IN
                                           : ENDP_MTP_DATA_OUT;
                } else {
                    resp->length = sizeof(*resp);
                    resp->code = RSP_OK;
                }
            } else if (mtp_state == RESET) {
                resp->length = sizeof(*resp);
                resp->code = RSP_Device_Busy;
            } else {
                resp->length = sizeof(*resp);
                resp->code = RSP_OK;
            }

            *len = resp->length;
            status = USBD_REQ_HANDLED;
            break;
        }
        default: {
            status = USBD_REQ_NEXT_CALLBACK;
            break;
        }
    }

    return status;
}

static void mtp_set_config(usbd_device *usbd_dev, uint16_t wValue) {
    (void)wValue;

    usbd_ep_setup(usbd_dev, ENDP_MTP_DATA_OUT, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    usbd_ep_setup(usbd_dev, ENDP_MTP_DATA_IN, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    usbd_ep_setup(usbd_dev, ENDP_MTP_EVENT_IN, USB_ENDPOINT_ATTR_INTERRUPT, 64, NULL);

    cmp_usb_register_control_class_callback(INTF_MTP, mtp_control_class_request);

    mtp_enabled = true;
}

void mtp_setup(usbd_device* usbd_dev) {
    mtp_usbd_dev = usbd_dev;

    cmp_usb_register_set_config_callback(mtp_set_config);
}

bool mtp_send_event(struct usb_ptp_async_event* event) {
    uint16_t sent = usbd_ep_write_packet(mtp_usbd_dev, ENDP_MTP_EVENT_IN,
                                         (const void*)(event),
                                         (uint16_t)(event->length));
    return (sent != 0);
}

static size_t mtp_read_data_out(uint8_t* buf, size_t len) {
    uint16_t read = usbd_ep_read_packet(mtp_usbd_dev, ENDP_MTP_DATA_OUT,
                                        (void*)buf, (uint16_t)len);
    return (size_t)read;
}

static size_t mtp_write_data_in(const uint8_t* buf, size_t len) {
    uint16_t written = usbd_ep_write_packet(mtp_usbd_dev, ENDP_MTP_DATA_IN,
                                        (void*)buf, (uint16_t)len);
    return (size_t)written;
}

static uint32_t timeout;

bool mtp_update(void) {
    static struct mtp_transaction txn;
    bool active = false;

    static enum MTPState old_state = RESET;

    if (!mtp_enabled) {
        return false;
    }

    while (1) {
        if (mtp_state == RESET) {
            mtp_state = IDLE;
        }

        if (mtp_state == IDLE) {
            txn.buffer_len = mtp_read_data_out(txn.buffer, sizeof(txn.buffer));
            if (txn.buffer_len > 0) {
                active = true;

                /* Validate that the command is well-formed */
                if (txn.buffer_len < sizeof(struct usb_ptp_command_block)) {
                    /* Must contain at least the block header */
                    mtp_state = STALLED;
                    break;
                }

                struct usb_ptp_command_block* command;
                command = (struct usb_ptp_command_block*)(txn.buffer);

                if (command->type != BLK_Command) {
                    /* Wrong block type */
                    mtp_state = STALLED;
                    break;
                }

                if (txn.buffer_len != command->length) {
                    /* Only support one command <=> one packet */
                    mtp_state = STALLED;
                    break;
                }

                uint32_t num_params = (txn.buffer_len >= 12) ? (txn.buffer_len - 12)/4 : 0;
                if (num_params > MTP_MAX_COMMAND_PARAMETERS) {
                    /* PIMA 15740 only has 5 parameters max */
                    mtp_state = STALLED;
                    break;
                }

                memcpy((void*)(&txn.command), txn.buffer, txn.buffer_len);
                mtp_state = COMMAND;
                txn.response.header.type = BLK_Response;
                txn.response.header.code = RSP_Undefined;
                txn.response.header.transactionID = txn.command.header.transactionID;
                txn.response.header.length = sizeof(txn.response.header);
            } else {
                break;
            }
        }

        if (mtp_state == COMMAND) {
            timeout = 1000;
            txn.buffer_len = 0;
            txn.data_header_packet_done = false;
            txn.data_bytes_transferred = 0;
            CommandConfig config = mtp_handle_command(&txn.command.header, &txn.response.header);
            if (config.has_data_stage) {
                mtp_state = config.data_stage_out ? DATA_OUT : DATA_IN;
                txn.data.header.length = config.data_length + sizeof(txn.data.header);
                txn.data.header.type = BLK_Data;
                txn.data.header.code = txn.command.header.code;
                txn.data.header.transactionID = txn.command.header.transactionID;
            } else {
                mtp_state = RESPONSE;
            }
        }

        if (mtp_state == DATA_IN) {
            if (!txn.data_header_packet_done) {
                if (mtp_write_data_in((uint8_t*)&txn.data.header, sizeof(txn.data.header)) != 0) {

                    txn.data_header_packet_done = true;
                    txn.data_bytes_transferred += sizeof(txn.data.header);
                    active = true;
                    break;
                }
            } else if (txn.data_bytes_transferred >= txn.data.header.length) {
                mtp_stream_data_complete(&txn.command.header,
                                         &txn.response.header);
                mtp_state = RESPONSE;
            } else if (mtp_stream_data_ready(&txn.command.header)) {
                if (txn.buffer_len == 0) {
                    size_t bytes_to_move = txn.data.header.length - txn.data_bytes_transferred;
                    if (bytes_to_move > sizeof(txn.buffer)) {
                        bytes_to_move = sizeof(txn.buffer);
                    }
                    txn.buffer_len = mtp_stream_data_in(&txn.command.header,
                                                        &txn.response.header,
                                                        txn.buffer, bytes_to_move);
                }

                if (txn.buffer_len > 0) {
                    if (mtp_write_data_in(txn.buffer, txn.buffer_len) != 0) {
                        txn.data_bytes_transferred += txn.buffer_len;
                        txn.buffer_len = 0;
                        active = true;
                        break;
                    }
                }
            } else {
                break;
            }
        } else if (mtp_state == DATA_OUT) {
            if (!txn.data_header_packet_done) {
                txn.buffer_len = mtp_read_data_out(txn.buffer, sizeof(txn.buffer));
                txn.data_bytes_transferred += txn.buffer_len;
                if (txn.buffer_len == sizeof(txn.data.header)) {
                    active = true;
                    txn.data_header_packet_done = true;
                    memcpy((void*)(&txn.data.header), txn.buffer, txn.buffer_len);
                } else if (txn.buffer_len > 0) {
                    active = true;
                    mtp_state = STALLED;
                } else {
                    break;
                }
            } else if (txn.data_bytes_transferred >= txn.data.header.length) {
                mtp_stream_data_complete(&txn.command.header, &txn.response.header);
                mtp_state = RESPONSE;
            } else if (mtp_stream_data_ready(&txn.command.header)) {
                txn.buffer_len = mtp_read_data_out(txn.buffer, sizeof(txn.buffer));
                txn.data_bytes_transferred += txn.buffer_len;

                if (txn.buffer_len == 0) {
                    // Can't tell if ZLP or no data, so try a few times
                    if (timeout > 0) {
                        timeout--;
                        break;
                    } else {
                        mtp_stream_data_complete(&txn.command.header, &txn.response.header);
                        active = true;
                        mtp_state = RESPONSE;
                    }
                } else if (txn.buffer_len > 0) {
                    active = true;
                    timeout = 1000;
                    if (txn.buffer_len < USB_MTP_MAX_PACKET_SIZE) {
                        mtp_stream_data_complete(&txn.command.header, &txn.response.header);
                        mtp_state = RESPONSE;
                    } else {
                        mtp_stream_data_out(&txn.command.header,
                                            &txn.data.header,
                                            &txn.response.header,
                                            txn.buffer, txn.buffer_len);
                    }
                }
            } else {
                break;
            }
        }

        if (mtp_state == RESPONSE) {
            size_t len = (size_t)(txn.response.header.length);
            if (mtp_write_data_in(txn.response.data, len) != 0) {
                mtp_state = IDLE;
                active = true;
            } else {
                break;
            }
        }

        if (mtp_state == STALLED) {
            bool out_stalled = usbd_ep_stall_get(mtp_usbd_dev, ENDP_MTP_DATA_OUT) != 0;
            bool in_stalled = usbd_ep_stall_get(mtp_usbd_dev, ENDP_MTP_DATA_IN) != 0;

            if (!out_stalled && !in_stalled) {
                /* Exit stall status when both endpoints unstalled */
                mtp_state = RESET;
            }
            break;
        }
    }

    if (mtp_state != old_state) {
        if (mtp_state == STALLED) {
            /* Stall endponts when entering the STALLED state */
            usbd_ep_stall_set(mtp_usbd_dev, ENDP_MTP_DATA_OUT, 1);
            usbd_ep_stall_set(mtp_usbd_dev, ENDP_MTP_DATA_IN, 1);
        } else if (old_state == STALLED) {
            /* Unstall endpoints when leaving the STALLED state */
            usbd_ep_stall_set(mtp_usbd_dev, ENDP_MTP_DATA_OUT, 1);
            usbd_ep_stall_set(mtp_usbd_dev, ENDP_MTP_DATA_IN, 1);
        }
    }

    /* Remember the last state we processed so that the state can be safely
       updated from outside of mtp_update */
    old_state = mtp_state;

    return active;
}

void mtp_reset(void) {
    /* Invalidate any outgoing transfers immediately */
    usbd_ep_nak_set(mtp_usbd_dev, ENDP_MTP_DATA_IN, 1);

    /* Enter the RESET state, which transitions into the IDLE state
       after the next update */
    mtp_state = RESET;
}

#endif
