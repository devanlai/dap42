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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/pwr.h>

#include "backup.h"

void backup_write(enum BackupRegister reg, uint32_t value) {
    rcc_periph_clock_enable(RCC_PWR);
    pwr_disable_backup_domain_write_protect();
    RTC_BKPXR((int)reg) = value;
    pwr_enable_backup_domain_write_protect();
    rcc_periph_clock_disable(RCC_PWR);
}

uint32_t backup_read(enum BackupRegister reg) {
    return RTC_BKPXR((int)reg);
}

bool backup_check_reset_source(enum ResetSource src) {
    bool matches = false;
    switch (src) {
        case RST_SRC_LPWR: {
            matches = (RCC_CSR & RCC_CSR_LPWRRSTF) != 0;
            break;
        }
        case RST_SRC_WWDG: {
            matches = (RCC_CSR & RCC_CSR_WWDGRSTF) != 0;
            break;
        }
        case RST_SRC_IWDG: {
            matches = (RCC_CSR & RCC_CSR_IWDGRSTF) != 0;
            break;
        }
        case RST_SRC_SW: {
            matches = (RCC_CSR & RCC_CSR_SFTRSTF) != 0;
            break;
        }
        case RST_SRC_POR: {
            matches = (RCC_CSR & RCC_CSR_PORRSTF) != 0;
            break;
        }
        case RST_SRC_PIN: {
            /* Only true if no other reset flags set */
            const uint32_t mask = RCC_CSR_LPWRRSTF
                                | RCC_CSR_WWDGRSTF
                                | RCC_CSR_IWDGRSTF
                                | RCC_CSR_SFTRSTF
                                | RCC_CSR_PORRSTF
                                | RCC_CSR_PINRSTF;
            matches = (RCC_CSR & mask) == RCC_CSR_PINRSTF;
            break;
        }
    }

    return matches;
}

void backup_clear_reset_source(void) {
    RCC_CSR |= RCC_CSR_RMVF;
}
