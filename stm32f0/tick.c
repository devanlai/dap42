#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>

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
