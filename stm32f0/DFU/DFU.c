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

#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/syscfg.h>

#include <libopencm3/stm32/flash.h>

#include "backup.h"
#include "DFU.h"

static const uint32_t CMD_BOOT_WITH_BOOT0_PIN  = 0x44465500UL;
static const uint32_t CMD_BOOT_WITH_NBOOT0_BIT = 0x44466621UL;

/* Start of bootloader region */
static const uint32_t BOOT_ADDR = 0x1FFFC400UL;

static inline void __set_MSP(uint32_t topOfMainStack)
{
    asm("msr msp, %0" : : "r" (topOfMainStack));
}

static void jump_to_bootloader(void) __attribute__ ((noreturn));

/* Sets up and jumps to the bootloader */
static void jump_to_bootloader(void) {
    uint32_t boot_stack_ptr = *(uint32_t*)(BOOT_ADDR);
    uint32_t dfu_reset_addr = *(uint32_t*)(BOOT_ADDR+4);

    void (*dfu_bootloader)(void) = (void (*))(dfu_reset_addr);

    /* Remap vector table to system memory */
    rcc_periph_clock_enable(RCC_SYSCFG_COMP);
    SYSCFG_CFGR1 = 0x1;

    /* Reset the stack pointer */
    __set_MSP(boot_stack_ptr);

    dfu_bootloader();
    while (1);
}

/* Writes a DFU command to the backup register and resets */
void DFU_reset_and_jump_to_bootloader(void) {
    /* Check if BOOT_SEL is set, which requires driving the BOOT0 pin*/
    if (FLASH_OBR & FLASH_OBR_BOOT_SEL) {
        backup_write(BKP0, CMD_BOOT_WITH_BOOT0_PIN);
    } else {
        backup_write(BKP0, CMD_BOOT_WITH_NBOOT0_BIT);
    }
    scb_reset_system();
}

/* Potentially diverts to the DFU bootloader should be called as early as possible */
void DFU_maybe_jump_to_bootloader(void) {
    uint32_t cmd = backup_read(BKP0);
    if (cmd == CMD_BOOT_WITH_BOOT0_PIN || cmd == CMD_BOOT_WITH_NBOOT0_BIT) {
        /* Clear the backup register */
        backup_write(BKP0, 0x0);

        if (cmd == CMD_BOOT_WITH_BOOT0_PIN) {
            /* If BOOT_SEL is set, drive the BOOT0 pin high so that the
               ROM bootloader will enter DFU mode */
            rcc_periph_clock_enable(RCC_GPIOB);
            gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8);
            gpio_set(GPIOB, GPIO8);
        }

        /* Jump to the ROM bootloader */
        jump_to_bootloader();
    }
}
