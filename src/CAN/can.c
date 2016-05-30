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

#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

#include <string.h>

#include "config.h"
#include "can.h"

#if CAN_RX_AVAILABLE

static volatile CAN_Message can_rx_buffer[CAN_RX_BUFFER_SIZE];
static uint8_t can_rx_head = 0;
static uint8_t can_rx_tail = 0;

bool can_rx_buffer_empty(void) {
    return can_rx_head == can_rx_tail;
}

bool can_rx_buffer_full(void) {
    return can_rx_head == ((can_rx_tail + 1) % CAN_RX_BUFFER_SIZE);
}

static volatile const CAN_Message* can_rx_buffer_peek(void) {
    return &can_rx_buffer[can_rx_head];
}

static void can_rx_buffer_pop(void) {
    can_rx_head = (can_rx_head + 1) % CAN_RX_BUFFER_SIZE;
}

static volatile CAN_Message* can_rx_buffer_tail(void) {
    return &can_rx_buffer[can_rx_tail];
}

static void can_rx_buffer_extend(void) {
    can_rx_tail = (can_rx_tail + 1) % CAN_RX_BUFFER_SIZE;
}

void can_rx_buffer_put(const CAN_Message* msg) {
    memcpy((void*)&can_rx_buffer[can_rx_tail], (const void*)msg, sizeof(CAN_Message));
    can_rx_tail = (can_rx_tail + 1) % CAN_RX_BUFFER_SIZE;
}

void can_rx_buffer_get(CAN_Message* msg) {
    memcpy((void*)msg, (const void*)&can_rx_buffer[can_rx_head], sizeof(CAN_Message));
    can_rx_head = (can_rx_head + 1) % CAN_RX_BUFFER_SIZE;
}

bool can_reconfigure(uint32_t baudrate, CanMode mode) {
    /* Set appropriate bit timing */
    uint32_t sjw = CAN_BTR_SJW_1TQ;
    uint32_t ts1;
    uint32_t ts2;
    uint32_t brp;

    if (baudrate == 1000000) {
        brp = 3;
        ts1 = CAN_BTR_TS1_11TQ;
        ts2 = CAN_BTR_TS2_4TQ;
    } else if (baudrate == 500000) {
        brp = 6;
        ts1 = CAN_BTR_TS1_11TQ;
        ts2 = CAN_BTR_TS2_4TQ;
    } else if (baudrate == 250000) {
        brp = 12;
        ts1 = CAN_BTR_TS1_11TQ;
        ts2 = CAN_BTR_TS2_4TQ;
    } else if (baudrate == 125000) {
        brp = 24;
        ts1 = CAN_BTR_TS1_11TQ;
        ts2 = CAN_BTR_TS2_4TQ;
    } else if (baudrate == 100000) {
        brp = 30;
        ts1 = CAN_BTR_TS1_11TQ;
        ts2 = CAN_BTR_TS2_4TQ;
    } else {
        return false;
    }

    can_reset(CAN);

    bool loopback = (mode == MODE_TEST_LOCAL || mode == MODE_TEST_SILENT);
    bool silent = (mode == MODE_SILENT || mode == MODE_TEST_SILENT);

    if (mode == MODE_TEST_GLOBAL) {
        return false;
    }

    /* CAN cell init. */
    if (can_init(CAN,
                 false,           /* TTCM: Time triggered comm mode? */
                 true,            /* ABOM: Automatic bus-off management? */
                 false,           /* AWUM: Automatic wakeup mode? */
                 false,           /* NART: No automatic retransmission? */
                 true,            /* RFLM: Receive FIFO locked mode? */
                 false,           /* TXFP: Transmit FIFO priority? */
                 sjw,
                 ts1,
                 ts2,
                 brp,/* BRP+1: Baud rate prescaler */
                 loopback,/* loopback mode */
                 silent))/* silent mode */
    {
        return false;
    }

    return true;
}

bool can_setup(uint32_t baudrate, CanMode mode) {
    /* Enable CAN clock */
    rcc_periph_clock_enable(RCC_CAN);

    /* Setup CANRX */
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8);
    gpio_set_af(GPIOB, GPIO_AF4, GPIO8);

#if CAN_TX_AVAILABLE
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
    gpio_set_af(GPIOB, GPIO_AF4, GPIO9);
#endif

    if (can_reconfigure(baudrate, mode)) {
        /* CAN filter 0 init. */
        can_filter_id_mask_32bit_init(CAN,
                                      0,     /* Filter ID */
                                      0,     /* CAN ID */
                                      0,     /* CAN ID mask */
                                      0,     /* FIFO assignment (here: FIFO0) */
                                      true); /* Enable the filter. */
        /* Enable CAN RX interrupt. */
        //can_enable_irq(CAN, CAN_IER_FMPIE0);
        //nvic_enable_irq(NVIC_CEC_CAN_IRQ);
        return true;
    }

    return false;
}

bool can_read(CAN_Message* msg) {
    bool success = false;

    uint8_t fifo = 0xFF;
    if ((CAN_RF0R(CAN) & CAN_RF0R_FMP0_MASK) != 0) {
        fifo = 0;
    } else if ((CAN_RF1R(CAN) & CAN_RF1R_FMP1_MASK) != 0) {
        fifo = 1;
    }

    if (fifo != 0xFF) {
        uint32_t fmi;
        bool ext, rtr;
        can_receive(CAN, fifo, true, &msg->id, &ext, &rtr, &fmi, &msg->len, msg->data);

        msg->format = ext ? CANExtended : CANStandard;
        msg->type = rtr ? CANRemote : CANData;
        success = true;
    }

    return success;
}

bool can_read_buffer(CAN_Message* msg) {
    bool success = false;
    if (!can_rx_buffer_empty()) {
        can_rx_buffer_get(msg);
        success = true;
    }

    return success;
}

/*
void cec_can_isr(void) {
    while (!can_rx_buffer_full()) {
        CAN_Message msg;
        bool read = can_read(&msg);
        if (!read) {
            break;
        }
        can_rx_buffer_put(&msg);
    }
}
*/

#endif
