# dap42
The dap42 project is an open-source firmware and [hardware design](https://github.com/devanlai/dap42-hardware) for an inexpensive, minimalist CMSIS-DAP based debug probe based on the [STM32F042F6](http://www.st.com/web/catalog/mmc/FM141/SC1169/SS1574/LN1823/PF259617) in a TSSOP-20 package.

This project is stable - it has been proven in the field by a few dozen users and all core functionality has been tested.

## Current features
### Firmware
* [Serial Wire Debug](http://www.arm.com/products/system-ip/debug-trace/coresight-soc-components/serial-wire-debug.php) (SWD) access over [CMSIS-DAP 1.0](http://www.arm.com/products/processors/cortex-m/cortex-microcontroller-software-interface-standard.php) HID interface (tested with [OpenOCD](http://openocd.org) and [LPCXpresso](https://www.lpcware.com/lpcxpresso))
* CDC-ACM USB-serial bridge
* [Device Firmware Upgrade](http://www.usb.org/developers/docs/devclass_docs/DFU_1.1.pdf) (DFU) over USB (detach-only, switches to on-chip [DFuSe](http://dfu-util.sourceforge.net/dfuse.html) bootloader).
* [Serial Line CAN](http://lxr.free-electrons.com/source/drivers/net/can/slcan.c) (SLCAN) interface - Silent mode, RX only.

## Flash instructions
The default method to upload new firmware is via [dfu-util](http://dfu-util.sourceforge.net/). The Makefile includes the `dfuse-flash` target to invoke dfu-util. dfu-util automatically detaches the dap42 firmware and uploads the firmware through the on-chip bootloader.

To flash via another debugger, use `make flash`.

For detailed flashing instructions, see [FLASHING.md](./FLASHING.md)

### STM32F103
The dap42 firmware can also target the STM32F103 chip.

To build firmware for STLink/v2 knockoff designs, use the `STM32F103` target or `STM32F103-DFUBOOT` target when using the [dapboot](https://github.com/devanlai/dapboot) bootloader. The CDC UART is connected to the `SWIM` pin (`PB11`) as an RX-only input.

To build firmware for the "bluepill" dev board, use the `STM32F103-BLUEPILL` or `STM32F103-BLUEPILL-DFUBOOT` targets.

The pin mapping is as follows:

| Signal | Pin  |
| ------ | ---- |
| SWDIO  | PB14 |
| SWCLK  | PB13 |
| RESET  | PB0  |
| TX     | PA2  |
| RX     | PA3  |

## Usage
### OpenOCD
The dap42 firmware has been tested with gdb and OpenOCD on STM32F042 (of course), STM32F103, and LPC11C14 targets.

In general, the probe can be used with OpenOCD just by specifying the cmsis-dap interface:

    openocd -f interface/cmsis-dap.cfg -f your_config.cfg -c "some inline command"

Example OpenOCD configurations can be found under the [openocd/](openocd/) folder.

### MCUXpresso
The default probe detection rules used by MCUXpresso (formerly LPCXpresso) will not auto-discover generic CMSIS-DAP probes.
To use the dap42 probe with MCUXpresso, you can modify the detection rules by editing `probetable.csv` in your MCUXpresso installation.

For MCUXpresso version 11.4 or newer, edit `probetable.csv` under `ide/plugins/com.mcuxpresso.tools.bin.<...>/binaries/Scripts`:

    0x1209, 0xDA42, 64, 1, 0, 0, 0, "", 0x0000, -1, -1

In older versions, the CSV format has one fewer entry at the end:

    0x1209, 0xDA42, 64, 1, 0, 0, 0, "", 0x0000, -1

### USB-serial
#### Windows
On Windows 10, the serial port works without requiring additional configuration.

For Windows XP through Windows 7, the [DAP42CDC.inf](drivers/DAP42CDC.inf) `.inf` file can be used to load the generic USB serial driver.

#### Mac OSX
On Mac OSX version 10.7 and later, the serial port should work without requiring any additional configuration.

#### Linux
On Linux, the serial port should be detected without requiring additional configuration.
Depending on your distro settings, the modem manager may attempt to grab the serial port and use it as a modem.
To prevent this, you can define a custom udev rule to ensure that the modem manager ignores the debugger.

    ATTRS{idVendor}=="1209" ATTRS{idProduct}=="da42", ENV{ID_MM_DEVICE_IGNORE}="1"

## Planned features
### Firmware
* CMSIS-DAP 1.10 support
 * Command queueing (command level, not packet level)
 * [Serial Wire Output](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0314h/Chdfgefg.html) (SWO) trace support
* CMSIS-DAP 2.0 support
 * Bulk endpoints for higher throughput
 * WebUSB compatibility

### Hardware
* Simultaneous USB-serial bridge and SWO trace using an STM32F042K6 in an LQFP-32 package with both UARTs pinned out.
* Level-shifting/protective tri-state buffer between the probe and target SWD port.

## Acknowledgements
The dap42 project was inspired by the [Dapper Mime](http://dappermime.sourceforge.net/) CMSIS-DAP proof-of-concept project.

The dap42 USB VID/PID pair is [1209/DA42](http://pid.codes/1209/DA42/), allocated through the [pid.codes](http://pid.codes/) open-source USB PID program.

## Licensing
All contents of the dap42 project are licensed under terms that are compatible with the terms of the GNU Lesser General Public License version 3.

Non-libopencm3 related portions of the dap42 project are licensed under the less restrictive ISC license, except where otherwise specified in the headers of specific files.

See the LICENSE file for full details.
