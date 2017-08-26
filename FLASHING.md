# Flashing instructions
## Overview
Depending on the board, there are a few different options for flashing firmware.

### SWD
If you already have a working debugger (not the one you're flashing!), you can always use that debugger to connect to the SWD pins on your board and reflash it. You may need an adapter PCB or jumpers to connect to the SWD pins.

### On-chip USB bootloader
The STM32F042 series has an on-chip USB [DFuSe](http://dfu-util.sourceforge.net/dfuse.html) bootloader. As it's part of the chip's ROM, it can't be overwritten, so it's almost always accessible. It will automatically run if the chip is blank or if the `BOOT0` pin is pulled high.
Once the main firmware has been flashed, it can retrigger the on-chip bootloader from USB to facilitate firmware updates without needing to touch the `BOOT0` pin.

### Off-chip USB bootloader
The STM32F10x series used in [STLinks](https://wiki.paparazziuav.org/wiki/STLink#Clones) does not have a built-in USB bootloader. [dapboot](https://github.com/devanlai/dapboot) is a companion USB DFU bootloader that can be flashed onto STM32F10x chips.
dapboot can be flashed with another debugger or using a USB-serial adapter with the on-chip serial bootloader.
Once dapboot has been flashed, the main firmware and any future firmware updates can be uploaded entirely over USB.

## DAP42 (native hardware, standalone debugger)
If you have freshly soldered board, you can verify that the chip and the USB circuitry are all working by plugging it into your computer with a USB cable.

If you haven't tried flashing the chip yet, you can just plug it and the on-chip USB bootloader should show up as a device named "STM32  BOOTLOADER" with vid/pid `0483:df11`. On Windows, it is normal for it to report that it could not find a driver - we'll cover installing a driver below.

If your OS doesn't detect the device at all, or it reports that the device is malfunctioning, you may have flashed broken firmware onto the chip or the board hardware might be bad. 

If you previously tried flashing it and messed up, you can force the bootloader to run by holding down switch `S1` while plugging in the cable.

Once you've verified that the hardware is working and can connect over USB, you can flash the main DAP42 firmware onto it.

### dfu-util
[dfu-util](http://dfu-util.sourceforge.net/) is a cross-platform open-source command-line utility that understands the USB DFU protocol.
It can handle discovering USB DFU devices, starting the bootloader from USB, and reading/writing firmware.

On Windows, if you plan to use `dfu-util`, you will need to use [Zadig](http://zadig.akeo.ie/) to install the WinUSB or libusb driver for the bootloader.

Let's start by verifying that `dfu-util` can detect the board:

    user@host:~/dap42/src$ dfu-util --list
    dfu-util 0.8
    
    Copyright 2005-2009 Weston Schmidt, Harald Welte and OpenMoko Inc.
    Copyright 2010-2014 Tormod Volden and Stefan Schmidt
    This program is Free Software and has ABSOLUTELY NO WARRANTY
    Please report bugs to http://sourceforge.net/p/dfu-util/tickets/
    
    Found DFU: [0483:df11] ver=2200, devnum=100, cfg=1, intf=0, alt=1, name="@Option Bytes  /0x1FFFF800/01*016 e", serial="FFFFFFFEFFFF"
    Found DFU: [0483:df11] ver=2200, devnum=100, cfg=1, intf=0, alt=0, name="@Internal Flash  /0x08000000/032*0001Kg", serial="FFFFFFFEFFFF"

If the chip is blank or you otherwise forced it into the bootloader, there should be two DFU interfaces - the one that we want is "Internal Flash".
If you're upgrading from a previous version of the debugger firmware, you might see one interface instead:

    Found Runtime: [1209:da42] ver=0120, devnum=3, cfg=1, intf=3, alt=0, name="DAP42 DFU", serial="383037201943425635001600"

If you're on Linux and you get something like `Cannot open DFU device 0483:df11` you may need to fix up the device permissions or use `sudo`.

You can use the `make dfuse-flash` target to flash the firmware via the bootloader:  

    user@host:~/dap42/src$ make dfuse-flash
    dfu-util -d 1209:da42,0483:df11  -a 0 -s 0x08000000:leave -D DAP42.bin
    dfu-util 0.8
    dfu-util: Invalid DFU suffix signature
    dfu-util: A valid DFU suffix will be required in a future dfu-util release!!!
    Opening DFU capable USB device...
    ID 0483:df11
    Run-time device DFU version 011a
    Claiming USB DFU Interface...
    Setting Alternate Setting #0 ...
    Determining device status: state = dfuERROR, status = 10
    dfuERROR, clearing status
    Determining device status: state = dfuIDLE, status = 0
    dfuIDLE, continuing
    DFU mode device DFU version 011a
    Device returned transfer size 2048
    DfuSe interface name: "Internal Flash  "
    Downloading to address = 0x08000000, size = 15764
    Download	[=========================] 100%        15764 bytes
    Download done.
    File downloaded successfully
    Transitioning to dfuMANIFEST state

At this point, you should have a functional debugger. On Windows, you may need to install drivers for the USB-serial interface and the USB DFU interface (again), but the CMSIS-DAP interface should work.

### STMicro's DfuSe tools
If you want to use the official STMicro Windows driver for the bootloader, you must use STMicro's official tools:

http://www.st.com/en/development-tools/stsw-stm32080.html

### SWD
The DAP42 board has two standard 2x5 0.05" pitch ARM SWD debug headers. The header off to the side of the board labelled `IF` is the one you need when flashing the debugger. The debugger can receive 3.3V power over the debug ribbon cable, but it is best to plug the debugger in over USB to provide power.

You can use the `make flash` target to flash the firmware using [openocd](http://www.openocd.net/).

The default rules are tight-lipped when something goes wrong:

    user@host:~/dap42/src$ make flash
      FLASH   DAP42.elf
    rules.mk:207: recipe for target 'DAP42.flash' failed
    make: *** [DAP42.flash] Error 255

To get more useful output when something goes wrong, pass in `V=1`

    user@host:~/dap42/src$ make V=1 flash
      FLASH   DAP42.elf
    openocd -f interface/cmsis-dap.cfg \
            -f target/stm32f0x.cfg \
            -c "program DAP42.elf verify reset exit" \

    Open On-Chip Debugger 0.9.0 (2017-03-07-13:28)
    Licensed under GNU GPL v2
    For bug reports, read
            http://openocd.org/doc/doxygen/bugs.html
    Info : only one transport option; autoselect 'swd'
    adapter speed: 1000 kHz
    adapter speed: 1000 kHz
    adapter_nsrst_delay: 100
    none separate
    cortex_m reset_config sysresetreq
    Error: unable to find CMSIS-DAP device
    Error: No Valid JTAG Interface Configured.
    rules.mk:207: recipe for target 'DAP42.flash' failed
    make: *** [DAP42.flash] Error 255

The default settings assume you're using a CMSIS-DAP debugger. If you have a different debugger, you can override the `OOCD_INTERFACE` variable:

    make OOCD_INTERFACE=interface/stlink-v2.cfg flash

## BRAINV3.3 (Embedded debugger on LPC1549 development board)  
The firmware for the embedded debugger on the BRAINv3.3 has a few differences:

You must pass in `TARGET=BRAINV3.3` to all make commands to ensure the correct firmware is built. 
Alternatively, create a file named `local.mk` in the `src/` directory and put it in that file.

If you've previously built firmware for a different target, make sure to run `make clean` first.

There is no dedicated button for `BOOT0` - instead, the reset button for the LPC1549 can be held down while plugging in the USB cable. This check is done in firmware at boot, so it may work even when the core debugger functionality is broken. However, it will not work if you flash entirely wrong firmware.

There is no SWD debug header for the debugger - if you brick it, recovery will be very difficult

To use CAN, the `BOOT0` function must be disabled since it's shared with the `CANRX` pin. Otherwise the CAN transceiver will almost always force the bootloader to run on powerup.

You can write to the option bytes to disable `BOOT0` using dfu-util:

    dfu-util -d 1209:da42,0483:df11 -a 1 -D ../option-bytes/no_boot_pin.options.bin -s 0x1FFFF800

Note that dfu-util will report 0% downloaded, but it actually succeeds. As of the time of writing, the `:will-reset` option is not yet in an official release, but after version 0.9.0, it should be possible to do:

    dfu-util -d 1209:da42,0483:df11 -a 1 -D ../option-bytes/no_boot_pin.options.bin -s 0x1FFFF800:will-reset

To restore the option bytes to the factory default, use `../option-bytes/factory_default.options.bin`

## DAP103 (STLink/v2 clone dongle)
### From pre-built images
The recommended way to flash an STLink/v2 dongle is to use a combined binary that contains both the bootloader and debugger firmware in a single file. Pre-built combined images can be found on the [releases](https://github.com/devanlai/dap42/releases) page.

To flash a pre-built binary onto an STLink/v2:

    openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg \
        -c "init; reset halt" \
        -c "stm32f1x unlock 0; reset halt" \
        -c "flash erase_sector 0 0 last" \
        -c "program DAP103-dapboot-combined-stlink.bin verify reset exit 0x08000000"

The above command will remove the default read protection on the STLink and flash the combined bootloader+debugger image.

Subsequent updates can be done via dfu-util:

    dfu-util -d 1209:da42,1209:db42 -D DAP103-DFU-stlink.bin

To do firmware updates on Windows, you may need to use Zadig to install WinUSB/libusb drivers for both the application DFU interface and the bootloader. On Linux, you may need to adjust permissions or use `sudo` to access the device.

### From source
To build the DAP103 firmware, you need to set either `TARGET=STM32F103` or `TARGET=STM32F103-DFUBOOT` depending on whether you want to use the bootloader or not. This must be passed into all make commands.

Alternatively, you can create a file named `local.mk` in the `src/` directory and put it in that file.

If you've previously built firmware for a different target, make sure to run `make clean` first.

Note: the SWD pin arrangement on STLink clones varies, even among dongles with identical case markings. Make sure that the pinout you're using matches the actual PCB you have.

To flash directly without a bootloader:

    make clean
    make TARGET=STM32F103
    make TARGET=STM32F103 flash

To load onto a device with the [dapboot](https://github.com/devanlai/dapboot) DFU bootloader installed:

    make clean
    make TARGET=STM32F103-DFUBOOT
    make TARGET=STM32F103-DFUBOOT dfu-flash

