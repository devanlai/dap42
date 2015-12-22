## Copyright (c) 2015, Devan Lai
##
## Permission to use, copy, modify, and/or distribute this software
## for any purpose with or without fee is hereby granted, provided
## that the above copyright notice and this permission notice
## appear in all copies.
##
## THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
## WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
## WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
## AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
## CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
## LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
## NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
## CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

BINARY = DAP42
OPENCM3_DIR = ../libopencm3
LDSCRIPT = stm32f042x6.ld

DFU_UTIL = dfu-util
DFUSE_VID_PID := 0483:df11

SRCS := $(wildcard *.c)
SRCS += $(wildcard DAP/*.c)
SRCS += $(wildcard USB/*.c)
SRCS += $(wildcard DFU/*.c)
OBJS += $(SRCS:.c=.o)
DEPS  = $(SRCS:.c=.d)

.DEFAULT_GOAL := $(BINARY).bin

%.dfuse-flash: %.bin
	$(DFU_UTIL) -d $(DFUSE_VID_PID) -a 0 -s 0x08000000:leave -D $(<)

dfuse-flash: $(BINARY).dfuse-flash

clean::
	@rm -f $(OBJS)
	@rm -f $(DEPS)

include libopencm3.target.mk

size: $(OBJS) $(BINARY).elf
	@$(PREFIX)-size $(OBJS) $(BINARY).elf

OBJS := $(sort $(OBJS))
