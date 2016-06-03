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

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>

#include "tick.h"

volatile uint32_t __ticks = 0;

void sys_tick_handler(void)
{
    __ticks++;
}

bool tick_setup(uint32_t tick_freq_hz) {
    bool success = false;

    if (systick_set_frequency(tick_freq_hz, rcc_ahb_frequency)) {
        systick_clear();
        systick_interrupt_enable();
        success = true;
    }

    return success;
}

void tick_start(void) {
    systick_counter_enable();
}

void tick_stop(void) {
    systick_counter_disable();
}

uint32_t get_ticks(void) {
    return __ticks;
}
