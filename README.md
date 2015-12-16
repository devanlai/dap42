# dap42
The dap42 project is an open-source firmware and [hardware design](https://github.com/devanlai/dap42-hardware) for an inexpensive, minimalist CMSIS-DAP based debug probe based on the [STM32F042F6](http://www.st.com/web/catalog/mmc/FM141/SC1169/SS1574/LN1823/PF259617) in a TSSOP-20 package.

This project is currently in alpha - features generally do 80% of what's needed, but the last 20% is generally untested.

## Current features
### Firmware
* [Serial Wire Debug](http://www.arm.com/products/system-ip/debug-trace/coresight-soc-components/serial-wire-debug.php) (SWD) access over [CMSIS-DAP 1.0](http://www.arm.com/products/processors/cortex-m/cortex-microcontroller-software-interface-standard.php) HID interface (currently only works with [OpenOCD](http://openocd.org))
* CDC-ACM USB-serial bridge
* Can initiate the on-chip [DFuSe](http://dfu-util.sourceforge.net/dfuse.html) bootloader from user code.

### Hardware
Discounting resistors and capacitors, the debug probe has a very low BOM count:
* 1x STM32F042F6
* 2x 0.05" 2x5 SWD headers, one for debugging the target, one for debugging the probe (optional)
* 1x mini-USB type B connector
* 1x 5V -> 3.3V linear regulator (MIC5504-3.3YM5)
* 2x 3mm momentary pushbuttons for reset and bootloader (optional)
* 3x LEDs (optional, but fun to look at)

In single quantities from official distributors, the STM32F042F6 can be bought for $2-$3 and is very easy to hand-solder.

In production use, with the probe integrated onto the target board, the BOM count can be further reduced:
* 1x STM32F042F6 (or STM32F042F4 with the current build size)
* 1x mini-usb type B connector
* 1x 5V -> 3.3V linear regulator

Because the STM32F042 enables the DFU bootloader when it hasn't been flashed and the dap42 firmware supports launching the bootloader from user code, it is possible to install or upgrade the dap42 firmware [dfu-util](http://dfu-util.sourceforge.net/) without ever needing to use the bootloader pin or an external debugger.

## Usage
The dap42 firmware has been tested with gdb and OpenOCD on STM32F042 (of course), STM32F103, and LPC11C14 targets.

For OpenOCD to configure and use the dap42, the CMSIS-DAP interface must be selected and the dap42's VID/PID pair must be specified.
Currently, the firmware uses the [pid.codes](http://pid.codes/) [reserved test PID 1209/0001](http://pid.codes/1209/0001/).
The VID/PID must be changed to an appropriately allocated pair before distributing hardware with the dap42 firmware.

An example of the extra configuration for OpenOCD is given below:

    interface cmsis-dap
    cmsis_dap_vid_pid 0x1209 0x0001

## Planned features
### Firmware
* CMSIS-DAP 1.10 support
 * Command queueing (command level, not packet level)
 * [Serial Wire Output](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0314h/Chdfgefg.html) (SWO) trace support
* [Serial Line CAN](http://lxr.free-electrons.com/source/drivers/net/can/slcan.c) (SLCAN) interface - Silent mode, RX only.
* [Media Transfer Protocol](https://en.wikipedia.org/wiki/Media_Transfer_Protocol) (MTP) interface or [Mass Storage Device](https://en.wikipedia.org/wiki/USB_mass_storage_device_class) (MSD) interface for drag-n-drop target firmware flashing

### Hardware
* Simultaneous USB-serial bridge and SWO trace using an STM32F042K6 in an LQFP-32 package with both UARTs pinned out.
* ESD protection
* Level-shifting/protective tri-state buffer between the probe and target SWD port.

## Licensing
The dap42 source code is tentatively planned to be released under the two-clause BSD license, pending further interpretation of [libopencm3](http://libopencm3.org/wiki/Main_Page)'s linking exception.
