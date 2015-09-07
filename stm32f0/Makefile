##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

BINARY = DAP
OPENCM3_DIR = ../libopencm3
LDSCRIPT = stm32f042x6.ld

DFU_UTIL = dfu-util

SRCS := $(wildcard *.c)
OBJS += $(SRCS:.c=.o)

.DEFAULT_GOAL := $(BINARY).bin

%.dfuse-flash: %.bin
	$(DFU_UTIL) -a 0 -s 0x8000000 -D $(<)

dfuse-flash: $(BINARY).dfuse-flash

include libopencm3.target.mk

OBJS := $(sort $(OBJS))
