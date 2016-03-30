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
#include <stdarg.h>

#include <libopencm3/usb/usbd.h>

#include "composite_usb_conf.h"
#include "mtp.h"
#include "mtp_structs.h"
#include "mtp_datasets.h"

#if MTP_AVAILABLE

static bool mtp_validate_command(const struct usb_ptp_command_block* command,
                                 int min_params, int max_params,
                                 ...) {
    va_list argp;
    va_start(argp, max_params);

    uint8_t num_params = (command->length - sizeof(*command)) / 4;

    int i;
    for (i=0; i < max_params; i++) {
        uint32_t* param = va_arg(argp, uint32_t*);
        if (i < num_params) {
            *param = command->parameters[i];
        } else {
            *param = 0;
        }
    }

    va_end(argp);

    return (min_params <= num_params && num_params <= max_params);
}

enum MTPState {
    RESET, IDLE, COMMAND, DATA_IN, DATA_OUT, RESPONSE, STALLED
};

static enum MTPState mtp_state = RESET;
static usbd_device* mtp_usbd_dev = NULL;
static bool mtp_enabled = false;
static uint8_t mtp_data_buffer[512] __attribute__ ((aligned (4)));
static size_t mtp_data_buffer_count;
static size_t mtp_data_buffer_offset;

static struct mtp_object_info_dataset mtp_active_object_info;
static uint8_t mtp_scratch_space[512] __attribute__ ((aligned (4)));
static bool valid_object_info = false;

/* User Callbacks */
/*
static StreamDataReadyCallback mtp_stream_data_ready;
static StreamDataOutCallback mtp_stream_data_out;
static StreamDataInCallback mtp_stream_data_in;
static StreamDataCompleteCallback mtp_stream_data_complete;
static HandleCommandCallback mtp_handle_command;
*/

static const struct mtp_object_info_dataset hello_world = {
    .storageID = 0x00010001,
    .objectFormat = 0x3004,
    .protectionStatus = 0x0001,
    .objectCompressedSize = 11,
    .thumbFormat = 0x3000,
    .parentObject = 0x00000000,
    .filename = "test.txt"
};

static const struct mtp_storage_info_dataset test_storage = {
    .storageType = 0x0001,
    .filesystemType = 0x0001,
    .accessCapability = 0x0001,
    .maxCapacity=128,
    .storageDescription="Test",
    .volumeIdentifier="DAP42TEST"
};

static const struct mtp_object_info_dataset ram_file = {
    .storageID = 0x00010002,
    .objectFormat = 0x3000,
    .protectionStatus = 0x0001,
    .objectCompressedSize = sizeof(mtp_scratch_space),
    .filename="RAM.bin"
};

static const struct mtp_storage_info_dataset ram_storage = {
    .storageType = 0x0003,
    .filesystemType = 0x0001,
    .accessCapability = 0x0000,
    .maxCapacity=sizeof(mtp_scratch_space),
    .freeSpaceInBytes=sizeof(mtp_scratch_space),
    .freeSpaceInObjects=1,
    .storageDescription="RW",
    .volumeIdentifier="DAP42RAM",
    
};

struct store_entry {
    uint32_t storageID;
    const struct mtp_storage_info_dataset* dataset;
};

const struct store_entry store_table[] = {
    { 0x00010001U, &test_storage },
    { 0x00010002U, &ram_storage },
};

struct object_entry {
    uint32_t object_handle;
    const struct mtp_object_info_dataset* object_info;
    const uint8_t* object_data;
};

struct object_entry object_table[] = {
    { 0x00000001, &hello_world, (const uint8_t*)"Hello World" },
    { 0x00000002, &ram_file, mtp_scratch_space },
};

static bool validate_storage_id(uint32_t storage_id) {
    size_t i;
    for (i=0; i < sizeof(store_table)/sizeof(store_table[0]); i++) {
        if (storage_id == store_table[i].storageID) {
            return true;
        }
    }
    return false;
}

static enum ResponseCode mtp_handle_send_object_info_request(const struct usb_ptp_command_block* command,
                                                             struct mtp_raw_object_info_dataset* object_info) {
    enum ResponseCode status = RSP_General_Error;
    uint32_t storage_id = 0;
    uint32_t parent_handle = 0;

    if (mtp_validate_command(command, 0, 2, &storage_id, &parent_handle)) {
        if (storage_id == 0x00000000 || storage_id == 0x00010002U) {
            // TODO: string data
            memcpy((uint8_t*)(&mtp_active_object_info), object_info, sizeof(*object_info));
            mtp_active_object_info.storageID = 0x00010002U;
            valid_object_info = true;
            status = RSP_OK;
        } else if (storage_id == 0x00010001U) {
            status = RSP_Store_ReadOnly;
            valid_object_info = false;
        } else {
            status = RSP_Invalid_StorageID;
            valid_object_info = false;
        }
    } else {
        status = RSP_Parameter_Not_Supported;
    }
    return status;
}
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

    size_t bytes_to_move = sizeof(mtp_data_buffer) - mtp_data_buffer_offset;
    if (bytes_to_move > len) {
        bytes_to_move = len;
    }
    memcpy(&mtp_data_buffer[mtp_data_buffer_offset], data, bytes_to_move);

    mtp_data_buffer_offset += bytes_to_move;
    mtp_data_buffer_count += bytes_to_move;
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
    // Validate SendObjectInfo requests after the dataset is received
    if (command->code == OPR_SendObjectInfo) {
        struct mtp_raw_object_info_dataset* object_info;
        object_info = (struct mtp_raw_object_info_dataset*)mtp_data_buffer;
        response->code = mtp_handle_send_object_info_request(command, object_info);
    } else if (command->code == OPR_SendObject) {
        if (valid_object_info) {
            valid_object_info = false;
            size_t count = mtp_data_buffer_count;
            if (count > sizeof(mtp_scratch_space)) {
                count = sizeof(mtp_scratch_space);
            }
            memcpy(mtp_scratch_space, mtp_data_buffer, count);
            response->code = RSP_OK;
        } else {
            response->code = RSP_No_Valid_ObjectInfo;
        }
    }
    return true;
}

static CommandConfig mtp_handle_command(struct usb_ptp_command_block* command,
                                        struct usb_ptp_response_block* response) {
    CommandConfig config;
    config.data_length = 0;
    config.has_data_stage = false;
    config.data_stage_out = false;
    mtp_data_buffer_offset = 0;
    mtp_data_buffer_count = 0;
    switch (command->code) {
        case OPR_GetDeviceInfo: {
            response->code = RSP_OK;
            config.has_data_stage = true;
            config.data_length = sizeof(mtp_device_info);
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
            const uint32_t count = sizeof(store_table)/sizeof(store_table[0]);
            struct storage_id_array {
                uint32_t count;
                uint32_t ids[0];
            };

            struct storage_id_array* arr =
                (struct storage_id_array*)(mtp_data_buffer);
            arr->count = count;
            uint32_t i;
            for (i=0; i < count; i++) {
                arr->ids[i] = store_table[i].storageID;
            }
            config.data_length = sizeof(struct storage_id_array) + count*sizeof(uint32_t);
            mtp_data_buffer_count = config.data_length;
            break;
        }
        case OPR_GetStorageInfo: {
            config.has_data_stage = true;

            uint32_t storage_id = 0;
            if (!mtp_validate_command(command, 1, 1, &storage_id)) {
                response->code = RSP_Parameter_Not_Supported;
            } else if (storage_id == 0) {
                response->code = RSP_Invalid_StorageID;
            } else {
                const uint32_t count = sizeof(store_table)/sizeof(store_table[0]);
                uint32_t i;
                for (i=0; i < count; i++) {
                    if (store_table[i].storageID == storage_id) {
                        config.data_length = mtp_build_storage_info(store_table[i].dataset, mtp_data_buffer);
                        mtp_data_buffer_count = config.data_length;
                        response->code = RSP_OK;
                        break;
                    }
                }
            }
            break;
        }
        case OPR_GetNumObjects: {
            uint32_t storage_id = 0;
            uint32_t object_format_code = 0;
            uint32_t assoc_object_handle = 0;
            if (!mtp_validate_command(command, 1, 3,
                                      &storage_id,
                                      &object_format_code,
                                      &assoc_object_handle)) {
                response->code = RSP_Parameter_Not_Supported;
            } else if (object_format_code != 0) {
                response->code = RSP_Specification_By_Format_Unsupported;
            } else if (assoc_object_handle != 0) {
                response->code = RSP_Parameter_Not_Supported;
            } else {
                if ((storage_id == 0xFFFFFFFFU) || validate_storage_id(storage_id)) {
                    uint32_t i = 0;
                    uint32_t count = 0;
                    for (i=0; i < sizeof(object_table)/sizeof(object_table[0]); i++) {
                        if ((storage_id == 0xFFFFFFFFU) || (storage_id == object_table[i].object_info->storageID)) {
                            count++;
                        }
                    }
                    response->code = RSP_OK;
                    response->length = sizeof(*response) + 4;
                    response->parameters[0] = count;
                } else {
                    response->code = RSP_Invalid_StorageID;
                    response->length = sizeof(*response) + 4;
                    response->parameters[0] = 0;
                }
            }

            break;
        }
        case OPR_GetObjectHandles: {
            config.has_data_stage = true;
            
            uint32_t storage_id = 0;
            uint32_t object_format_code = 0;
            uint32_t assoc_object_handle = 0;
            if (!mtp_validate_command(command, 1, 3,
                                      &storage_id,
                                      &object_format_code,
                                      &assoc_object_handle)) {
                response->code = RSP_Parameter_Not_Supported;
            } else if (object_format_code != 0) {
                response->code = RSP_Specification_By_Format_Unsupported;
            } else if (assoc_object_handle != 0 && assoc_object_handle != 0xFFFFFFFFU) {
                response->code = RSP_Parameter_Not_Supported;
            } else {
                struct object_handle_array {
                    uint32_t count;
                    uint32_t handles[0];
                };

                struct object_handle_array* arr =
                    (struct object_handle_array*)(mtp_data_buffer);

                if ((storage_id == 0xFFFFFFFFU) || validate_storage_id(storage_id)) {
                    uint32_t i = 0;
                    arr->count = 0;
                    for (i=0; i < sizeof(object_table)/sizeof(object_table[0]); i++) {
                        if ((storage_id == 0xFFFFFFFFU) || (storage_id == object_table[i].object_info->storageID)) {
                            arr->handles[arr->count] = object_table[i].object_handle;
                            arr->count++;
                        }
                    }
                    response->code = RSP_OK;
                    mtp_data_buffer_count = 4+4*arr->count;
                    config.data_length = mtp_data_buffer_count;
                } else {
                    response->code = RSP_Invalid_StorageID;
                    arr->count = 0;
                    mtp_data_buffer_count = 4;
                    config.data_length = 4;
                }
            }
            break;
        }
        case OPR_GetObjectInfo: {
            config.has_data_stage = true;
            uint32_t object_handle = 0;
            if (mtp_validate_command(command, 1, 1, &object_handle)) {
                bool found = false;
                uint32_t i = 0;
                for (i=0; i < sizeof(object_table)/sizeof(object_table[0]); i++) {
                    if (object_table[i].object_handle == object_handle) {
                        found = true;
                        config.data_length = mtp_build_object_info(object_table[i].object_info, mtp_data_buffer);
                        mtp_data_buffer_count = config.data_length;
                        break;
                    }
                }
                if (found) {
                    response->code = RSP_OK;
                } else {
                    response->code = RSP_Invalid_ObjectHandle;
                }
            } else {
                response->code = RSP_Parameter_Not_Supported;
            }
            break;
        }
        case OPR_GetObject: {
            config.has_data_stage = true;
            uint32_t object_handle = 0;
            if (mtp_validate_command(command, 1, 1, &object_handle)) {
                bool found = false;
                uint32_t i = 0;
                for (i=0; i < sizeof(object_table)/sizeof(object_table[0]); i++) {
                    if (object_table[i].object_handle == object_handle) {
                        found = true;
                        mtp_data_buffer_count = object_table[i].object_info->objectCompressedSize;
                        memcpy(mtp_data_buffer, object_table[i].object_data, mtp_data_buffer_count);
                        config.data_length = mtp_data_buffer_count;
                        break;
                    }
                }
                if (found) {
                    response->code = RSP_OK;
                } else {
                    response->code = RSP_Invalid_ObjectHandle;
                }
            } else {
                response->code = RSP_Parameter_Not_Supported;
            }
            break;
        }
        case OPR_SendObjectInfo: {
            config.has_data_stage = true;
            config.data_stage_out = true;
            // Validation is deferred until after the data stage
            break;
        }
        case OPR_SendObject: {
            config.has_data_stage = true;
            config.data_stage_out = true;
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
    size_t i = 0;
    const char* X = "0123456789ABCDE\n";
    while (i < sizeof(mtp_scratch_space)) {
        strncpy((char*)(mtp_scratch_space+i), X, sizeof(mtp_scratch_space)-i);
        i += strlen(X);
    }
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
