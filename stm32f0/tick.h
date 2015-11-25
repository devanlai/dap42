#ifndef TICK_H_INCLUDED
#define TICK_H_INCLUDED

extern bool tick_setup(uint32_t tick_freq_hz);
extern void tick_start(void);
extern void tick_stop(void);

extern volatile uint32_t __ticks;

extern inline uint32_t get_ticks(void) {
    return __ticks;
}

#endif
