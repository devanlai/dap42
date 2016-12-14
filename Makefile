## Copyright (c) 2016, Devan Lai
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

# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q              := @
NULL           := 2>/dev/null
MAKE           := $(MAKE) --no-print-directory
endif
export V

BUILD_DIR      ?= ./build

all: DAP42.bin KITCHEN42.bin \
     DAP103.bin DAP103-DFU.bin \
     DAP103-NUCLEO-STBOOT.bin \
     BRAINv3.3.bin
clean:
	$(Q)$(RM) $(BUILD_DIR)/*.bin
	$(Q)$(MAKE) -C src/ clean

.PHONY = all clean

$(BUILD_DIR):
	$(Q)mkdir -p $(BUILD_DIR)

DAP42.bin: | $(BUILD_DIR)
	@printf "  BUILD $(@)\n"
	$(Q)$(MAKE) TARGET=STM32F042 -C src/ clean
	$(Q)$(MAKE) TARGET=STM32F042 -C src/
	$(Q)cp src/DAP42.bin $(BUILD_DIR)/$(@)

KITCHEN42.bin: | $(BUILD_DIR)
	@printf "  BUILD $(@)\n"
	$(Q)$(MAKE) TARGET=KITCHEN42 -C src/ clean
	$(Q)$(MAKE) TARGET=KITCHEN42 -C src/
	$(Q)cp src/DAP42.bin $(BUILD_DIR)/$(@)

DAP103.bin: | $(BUILD_DIR)
	@printf "  BUILD $(@)\n"
	$(Q)$(MAKE) TARGET=STM32F103 -C src/ clean
	$(Q)$(MAKE) TARGET=STM32F103 -C src/
	$(Q)cp src/DAP42.bin $(BUILD_DIR)/$(@)

DAP103-DFU.bin: | $(BUILD_DIR)
	@printf "  BUILD $(@)\n"
	$(Q)$(MAKE) TARGET=STM32F103-DFUBOOT -C src/ clean
	$(Q)$(MAKE) TARGET=STM32F103-DFUBOOT -C src/
	$(Q)cp src/DAP42.bin $(BUILD_DIR)/$(@)

BRAINv3.3.bin: | $(BUILD_DIR)
	@printf "  BUILD $(@)\n"
	$(Q)$(MAKE) TARGET=BRAINV3.3 -C src/ clean
	$(Q)$(MAKE) TARGET=BRAINV3.3 -C src/
	$(Q)cp src/DAP42.bin $(BUILD_DIR)/$(@)

DAP103-NUCLEO-STBOOT.bin: | $(BUILD_DIR)
	@printf "  BUILD $(@)\n"
	$(Q)$(MAKE) TARGET=STLINKV2-1-STBOOT -C src/ clean
	$(Q)$(MAKE) TARGET=STLINKV2-1-STBOOT -C src/
	$(Q)cp src/DAP42.bin $(BUILD_DIR)/$(@)
