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
