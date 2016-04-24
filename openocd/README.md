This directory contains sample scripts/configuration files for the dap42 that can be used with openocd

All example commands are given assuming that the current working directory is `dap42/stm32f0/`

Flashing a dap42 unit
---------------------

    openocd -f interface/<whatever-interface.cfg> -f openocd/flash-dap42.cfg

When flashing or debugging one dap42 unit with another dap42, you need to specify the serial number of the dap42 used as the debug interface to avoid ambiguity.

    openocd -f openocd/interface-dap42.cfg -c "cmsis_dap_serial 383037201943425635001600" -f openocd/flash-dap42.cfg

Debugging with gdb
------------------
`debug.cfg` contains generic settings suitable for using a dap42 unit to debug another target using openocd and gdb.

To start a remote server that gdb can connect to:

    openocd -f openocd/interface-dap42.cfg -f target/<whatever-target.cfg> -f openocd/debug.cfg

To launch gdb and connect to the openocd server:

    arm-none-eabi-gdb --tui --eval "target remote localhost:3333" <target-elf-file-here>

From within gdb, you can reset and halt the target

    (gdb) mon reset init
    target state: halted
    target halted due to debug-request, current mode: Thread
    xPSR: 0xc1000000 pc: 0x08002850 msp: 0x20001800

From there, you can setup breakpoints, step through code, etc.

    (gdb) break DAP42.c:157
    Breakpoint 1 at 0x8001008: file DAP42.c, line 157.
    (gdb) c
    Continuing.

    Breakpoint 1, main () at DAP42.c:157
