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

/*
    Portions of this file are derived from:

    Dapper Mime - an open-source CMSIS-DAP implementation
    HAL for STM32F0xx2
    this file is used by the mbed CMSIS-DAP routines

    Copyright (C) 2014 Peter Lawrence

    Permission is hereby granted, free of charge, to any person obtaining a 
    copy of this software and associated documentation files (the "Software"), 
    to deal in the Software without restriction, including without limitation 
    the rights to use, copy, modify, merge, publish, distribute, sublicense, 
    and/or sell copies of the Software, and to permit persons to whom the 
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in 
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
    DEALINGS IN THE SOFTWARE.
*/

#ifndef __DAP_HAL_H__
#define __DAP_HAL_H__

#include <libopencm3/stm32/gpio.h>
#include "DAP/CMSIS_DAP_config.h"
#include "tick.h"
#include <libopencm3/cm3/systick.h>
#include <libopencmsis/core_cm3.h>


/*
 * TIMESTAMP SUPPORT
 */

// Get current timestamp value, in micro-seconds:
// Use SYSTICK value and timer ticks, assume 1ms counter.
static __inline uint32_t TIMESTAMP_GET (void) {
  uint32_t reload = STK_RVR;
  uint32_t current = STK_CVR;
  uint32_t tick = get_ticks();
  return tick * 1000U + (reload - current) / (CPU_CLOCK / TIMESTAMP_CLOCK);
}

/*
SWD functionality
*/

static __inline void PORT_SWD_SETUP (void)
{
    GPIO_BSRR(SWDIO_GPIO_PORT) = SWDIO_GPIO_PIN;
    GPIO_BSRR(SWCLK_GPIO_PORT) = SWCLK_GPIO_PIN;
  
    gpio_set_output_options(SWDIO_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, SWDIO_GPIO_PIN);
    gpio_set_output_options(SWCLK_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, SWCLK_GPIO_PIN);

    gpio_mode_setup(SWDIO_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SWDIO_GPIO_PIN);
    gpio_mode_setup(SWCLK_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SWCLK_GPIO_PIN);
}

static __inline void PORT_OFF (void)
{
    GPIO_BRR(SWDIO_GPIO_PORT) = SWDIO_GPIO_PIN;
    GPIO_BRR(SWCLK_GPIO_PORT) = SWCLK_GPIO_PIN;
    gpio_mode_setup(SWDIO_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, SWDIO_GPIO_PIN);
    gpio_mode_setup(SWCLK_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, SWCLK_GPIO_PIN);
}

static __inline void PIN_SWCLK_TCK_SET (void)
{
    GPIO_BSRR(SWCLK_GPIO_PORT) = SWCLK_GPIO_PIN;
}

static __inline void PIN_SWCLK_TCK_CLR (void)
{
    GPIO_BRR(SWCLK_GPIO_PORT) = SWCLK_GPIO_PIN;
}

static __inline uint32_t PIN_SWDIO_TMS_IN  (void)
{
    return (GPIO_IDR(SWDIO_GPIO_PORT) & SWDIO_GPIO_PIN) ? 0x1 : 0x0;
}

static __inline void PIN_SWDIO_TMS_SET (void)
{
    GPIO_BSRR(SWDIO_GPIO_PORT) = SWDIO_GPIO_PIN;
}

static __inline void PIN_SWDIO_TMS_CLR (void)
{
    GPIO_BRR(SWDIO_GPIO_PORT) = SWDIO_GPIO_PIN;
}

static __inline uint32_t PIN_SWDIO_IN (void)
{
    return (GPIO_IDR(SWDIO_GPIO_PORT) & SWDIO_GPIO_PIN) ? 0x1 : 0x0;
}

static __inline void PIN_SWDIO_OUT (uint32_t bit)
{
    if (bit & 1) {
        GPIO_BSRR(SWDIO_GPIO_PORT) = SWDIO_GPIO_PIN;
    } else {
        GPIO_BRR(SWDIO_GPIO_PORT) = SWDIO_GPIO_PIN;
    }
}

static __inline void     PIN_SWDIO_OUT_ENABLE  (void)
{
    
    GPIO_MODER(SWDIO_GPIO_PORT) &= ~( (0x3 << (SWDIO_GPIO_PIN_NUM * 2)) );
    GPIO_MODER(SWDIO_GPIO_PORT) |=  ( (0x1 << (SWDIO_GPIO_PIN_NUM * 2)) );
}

static __inline void     PIN_SWDIO_OUT_DISABLE (void)
{
    GPIO_MODER(SWDIO_GPIO_PORT) &= ~( (0x3 << (SWDIO_GPIO_PIN_NUM * 2)) );
}

/*
  JTAG-only functionality (not used in this application)
*/

static __inline void PORT_JTAG_SETUP (void) {}

static __inline uint32_t PIN_TDI_IN  (void) {  return 0; }

static __inline void     PIN_TDI_OUT (uint32_t bit) { (void)bit; }

static __inline uint32_t PIN_TDO_IN (void) {  return 0; }

static __inline uint32_t PIN_nTRST_IN (void) {  return 0; }

static __inline void     PIN_nTRST_OUT  (uint32_t bit) { (void)bit; }

/*
  other functionality not applicable to this application
*/
static __inline uint32_t PIN_SWCLK_TCK_IN  (void) {
	return (GPIO_IDR(SWCLK_GPIO_PORT) & SWCLK_GPIO_PIN) ? 0x1 : 0x0;
}

static __inline uint32_t PIN_nRESET_IN  (void) {
#if defined(nRESET_GPIO_PORT) && defined(nRESET_GPIO_PIN)
	return (GPIO_IDR(nRESET_GPIO_PORT) & nRESET_GPIO_PIN) ? 0x1 : 0x0;
#else
    return 1;
#endif
}

static __inline void PIN_nRESET_OUT (uint32_t bit) {
#if defined(nRESET_GPIO_PORT) && defined(nRESET_GPIO_PIN)
    if (bit & 0x1) {
        GPIO_BSRR(nRESET_GPIO_PORT) = nRESET_GPIO_PIN;
    } else {
        GPIO_BRR(nRESET_GPIO_PORT) = nRESET_GPIO_PIN;
    }
#endif
}

static __inline void LED_CONNECTED_OUT (uint32_t bit) {
#if defined(LED_CON_GPIO_PORT) && defined(LED_CON_GPIO_PIN)
    if ((bit & 0x1) ^ LED_OPEN_DRAIN) {
        gpio_set(LED_CON_GPIO_PORT, LED_CON_GPIO_PIN);
    } else {
        gpio_clear(LED_CON_GPIO_PORT, LED_CON_GPIO_PIN);
    }
#endif
}

static __inline void LED_RUNNING_OUT (uint32_t bit) {
#if defined(LED_RUN_GPIO_PORT) && defined(LED_RUN_GPIO_PIN)
    if ((bit & 0x1) ^ LED_OPEN_DRAIN) {
        gpio_set(LED_RUN_GPIO_PORT, LED_RUN_GPIO_PIN);
    } else {
        gpio_clear(LED_RUN_GPIO_PORT, LED_RUN_GPIO_PIN);
    }
#endif
}

static __inline void LED_ACTIVITY_OUT (uint32_t bit) {
#if defined(LED_ACT_GPIO_PORT) && defined(LED_ACT_GPIO_PIN)
    if ((bit & 0x1) ^ LED_OPEN_DRAIN) {
        gpio_set(LED_ACT_GPIO_PORT, LED_ACT_GPIO_PIN);
    } else {
        gpio_clear(LED_ACT_GPIO_PORT, LED_ACT_GPIO_PIN);
    }
#endif
}

static __inline void DAP_SETUP (void) {
    LED_ACTIVITY_OUT(0);
    LED_RUNNING_OUT(0);
    LED_CONNECTED_OUT(0);

#if defined(nRESET_GPIO_PORT) && defined(nRESET_GPIO_PIN)
    // Configure nRESET as an open-drain output
    GPIO_BSRR(nRESET_GPIO_PORT) = nRESET_GPIO_PIN;

    gpio_set_output_options(nRESET_GPIO_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_LOW, nRESET_GPIO_PIN);

    gpio_mode_setup(nRESET_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, nRESET_GPIO_PIN);
#endif
}

static __inline uint32_t RESET_TARGET (void) { return 0; }

#endif
