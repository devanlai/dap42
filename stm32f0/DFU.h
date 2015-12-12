#ifndef DFU_H_INCLUDED
#define DFU_H_INCLUDED

extern void DFU_reset_and_jump_to_bootloader(void) __attribute__ ((noreturn));
extern void DFU_maybe_jump_to_bootloader(void);

#endif
