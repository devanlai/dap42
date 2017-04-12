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

#include <stdio.h>
#include <string.h>

#include "USB/vcdc.h"
#include "retarget.h"
#include "slcan.h"

CanMode slcan_mode;
uint32_t slcan_baudrate;

static bool parse_hex_digits(const char* input, uint8_t num_digits, uint32_t* value_out) {
    bool success = true;
    uint32_t value = 0;

    uint8_t i;
    for (i=0; i < num_digits; i++) {
        uint32_t nibble = 0;
        if (input[i] >= '0' && input[i] <= '9') {
            nibble = 0x0 + (input[i] - '0');
        } else if (input[i] >= 'a' && input[i] <= 'f') {
            nibble = 0xA + (input[i] - 'a');
        } else if (input[i] >= 'A' && input[i] <= 'F') {
            nibble = 0xA + (input[i] - 'A');
        } else {
            success = false;
            break;
        }
        uint8_t offset = 4*(num_digits-i-1);
        value |= (nibble << offset);
    }

    if (success) {
        *value_out = value;
    }

    return success;
}

static bool parse_hex_values(const char* input, uint8_t num_values, uint8_t* values_out) {
    uint8_t i;
    for (i=0; i < num_values; i++) {
        uint32_t value;
        if (parse_hex_digits(input, 2, &value)) {
            values_out[i] = (uint8_t)value;
        } else {
            return false;
        }
        input += 2;
    }

    return true;
}

static bool parse_dec_digit(const char* input, uint8_t* value_out) {
    if (input[0] >= '0' && input[0] <= '9') {
        *value_out = 0 + (input[0] - '0');
        return true;
    } else {
        return false;
    }
}

static bool slcan_process_config_command(const char* command, size_t len) {
    bool success = false;

    // Validate command length
    if (command[0] == 'M' || command[0] == 'm') {
        if (len != 9) {
            return false;
        }
    } else if (command[0] == 's') {
        if (!((len == 5) || (len == 7))) {
            return false;
        }
    } else if (command[0] == 'S' || command[0] == 'Z') {
        if (len != 2) {
            return false;
        }
    } else if (len != 1) {
        return false;
    }

    if (slcan_mode != MODE_RESET && command[0] != 'C') {
        return false;
    } else if (command[0] == 'C' && slcan_mode == MODE_RESET) {
        return false;
    }

    switch (command[0]) {
        case 'S': {
            switch (command[1]) {
                case '3':
                    slcan_baudrate = 100000;
                    success = true;
                    break;
                case '4':
                    slcan_baudrate = 125000;
                    success = true;
                    break;
                case '5':
                    slcan_baudrate = 250000;
                    success = true;
                    break;
                case '6':
                    slcan_baudrate = 500000;
                    success = true;
                    break;
                case '8':
                    slcan_baudrate = 1000000;
                    success = true;
                    break;
                default:
                    success = false;
                    break;
            }
            break;
        }
        case 'O': {
            slcan_mode = MODE_NORMAL;
            success = can_reconfigure(slcan_baudrate, slcan_mode);
            break;
        }
        case 'L': {
            slcan_mode = MODE_SILENT;
            success = can_reconfigure(slcan_baudrate, slcan_mode);
            break;
        }
        case 'l': {
            slcan_mode = MODE_TEST_SILENT;
            success = can_reconfigure(slcan_baudrate, slcan_mode);
            break;
        }
        case 'C': {
            slcan_mode = MODE_RESET;
            success = can_reconfigure(slcan_baudrate, slcan_mode);
            break;
        }
        // Dummy commands for compatibility
        case 's': {
            // TODO: implement direct BTR control
            success = true;
            break;
        }
        case 'M':
        case 'm': {
            // TODO: implement filtering
            success = true;
            break;
        }
        case 'Z': {
            // TODO: implement timestamping
            success = true;
            break;
        }
        default: {
            success = false;
            break;
        }
    }

    return success;
}

static bool slcan_process_transmit_command(const char* command, size_t len) {
    bool success = false;

    if (slcan_mode == MODE_RESET || slcan_mode == MODE_SILENT) {
        return false;
    }

    bool valid_message = false;
    CAN_Message msg;
    if (command[0] == 't' || command[0] == 'T') {
        msg.type = CANData;
        msg.format = (command[0] == 't') ? CANStandard : CANExtended;
        size_t id_len = msg.format == CANStandard ? 3 : 8;
        if ((len >= id_len + 2) &&
            parse_hex_digits(&command[1], id_len, &msg.id) &&
            parse_dec_digit(&command[id_len + 1], &msg.len)) {
            if ((len == id_len + 2 + 2*msg.len) &&
                (msg.len <= 8) &&
                parse_hex_values(&command[id_len + 2], msg.len, msg.data)) {
                valid_message = true;
            }
        }
    } else if (command[0] == 'r' || command[0] == 'R') {
        msg.type = CANRemote;
        msg.format = (command[0] == 'r') ? CANStandard : CANExtended;
        size_t id_len = msg.format == CANStandard ? 3 : 8;
        if ((len == id_len + 2) &&
            parse_hex_digits(&command[1], id_len, &msg.id) &&
            parse_dec_digit(&command[id_len + 1], &msg.len)) {
            if (msg.len <= 8) {
                valid_message = true;
            }
        }
    }

    if (valid_message) {
        if (command[0] == 'r' || command[0] == 't') {
            vcdc_putchar('z');
        } else {
            vcdc_putchar('Z');
        }
        success = can_write(&msg);
    }

    return success;
}

static bool slcan_process_diagnostic_command(const char* command, size_t len) {
    bool success = false;

    // Validate command length
    if (command[0] == 'W') {
        if (len != 5) {
            return false;
        }
    } else {
        if (len != 1) {
            return false;
        }
    }

    switch (command[0]) {
        case 'V': {
            // Hardware and firmware version
            success = true;
            vcdc_print("V1111");
            break;
        }
        case 'v': {
            // Firmware version
            success = true;
            vcdc_print("v11");
            break;
        }
        case 'N': {
            // Serial number
            success = true;
            vcdc_print("C254");
            break;
        }
        case 'F': {
            // Internal status register
            success = true;
            vcdc_print("F00");
            break;
        }
        case 'W': {
            // Ignore the MCP2515 register write
            success = true;
            break;
        }
        default: {
            success = false;
            break;
        }
    }

    return success;
}

bool slcan_exec_command(const char* command, size_t len) {
    bool success = false;

    switch (command[0]) {
        // Configuration commands
        case 'S':
        case 'O':
        case 'L':
        case 'l':
        case 'C':
        case 's':
        case 'M':
        case 'm':
        case 'Z': {
            success = slcan_process_config_command(command, len);
            break;
        }
        // Transmission commands
        case 't':
        case 'T':
        case 'r':
        case 'R': {
            success = slcan_process_transmit_command(command, len);
            break;
        }
        // Diagnostic commands
        case 'V':
        case 'v':
        case 'N':
        case 'F':
        case 'W': {
            success = slcan_process_diagnostic_command(command, len);
            break;
        }
        default: {
            success = false;
            break;
        }
    }

    return success;
}

bool slcan_output_messages(void) {
    if (slcan_mode == MODE_RESET) {
        return false;
    }
    bool read = false;
    CAN_Message msg;
    while (can_read_buffer(&msg)) {
        read = true;
        if (msg.format == CANStandard) {
            vcdc_print(msg.type == CANData ? "t" : "r");
            vcdc_print_hex_nibble((uint8_t)(msg.id >> 8));
            vcdc_print_hex_byte((uint8_t)(msg.id & 0xFF));
            vcdc_putchar('0' + msg.len);
        } else {
            vcdc_print(msg.type == CANData ? "T" : "R");
            vcdc_print_hex(msg.id);
            vcdc_putchar('0' + msg.len);
        }
        uint8_t i = 0;
        for (i=0; i < msg.len; i++) {
            vcdc_print_hex_byte(msg.data[i]);
        }
        vcdc_putchar('\r');
    }

    return read;
}

void slcan_app_setup(uint32_t baudrate, CanMode mode) {
    slcan_mode = mode;
    slcan_baudrate = baudrate;
    can_setup(baudrate, mode);
}

bool slcan_app_update(void) {
    bool active = false;

    static char command_buffer[SLCAN_MAX_MESSAGE_LEN+1];
    static size_t command_len = 0;
    static bool overflow = false;

    while (command_len < sizeof(command_buffer)) {
        if (vcdc_recv_buffered((uint8_t*)&command_buffer[command_len], 1)) {
            if (command_buffer[command_len] == '\r') {
                if (overflow) {
                    // Reject the command and exit overflow
                    command_len = 0;
                    overflow = false;
                    vcdc_putchar(SLCAN_ERROR);
                    active = true;
                } else {
                    // Process the command
                    command_buffer[command_len] = '\0';
                    bool success = slcan_exec_command(command_buffer, command_len);
                    command_len = 0;
                    if (success) {
                        vcdc_putchar(SLCAN_OK);
                    } else {
                        vcdc_putchar(SLCAN_ERROR);
                    }
                    active = true;
                }
                break;
            } else if (command_buffer[command_len] == '\n' && command_len == 0) {
                // Ignore line-feed after carriage-return
                continue;
            } else if (overflow) {
                // Ignore everything until the end of the command
                continue;
            } else {
                // Add a character to the command
                command_len++;
            }
        } else {
            break;
        }
    }

    if (command_len >= sizeof(command_buffer)) {
        overflow = true;
        command_len = 0;
    }

    if (slcan_output_messages()) {
        active = true;
    }


    return active;
}
