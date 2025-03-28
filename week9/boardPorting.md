# Porting a new board to Zephyr

## Hierarchy & Terminology
Hardware is supported based on a hierarchical abstraction. Each board has one or more SoCs (system on a chip), each SoC can (but does not need to be part of) an SoC series which can be part of an SoC family, and each SoCmust have a type of CPU with a certain architecture. For example, the nrf52840dk/nrf52840 has the nRF52840 SoC, from the nRF52 SoC series, which is part of the Nordic nRF SoC family. The nRF52840 has an Arm Cortex-M4 processor with ARMv7-M architecture. 

The "board target" is the complete string provided to Zephyr build tools to compile and link the image to be flashed to a specific board. It consists of a board name, revision, SoC, CPU cluster, and variant. Board target can be generally represented as \[board name\]\[@revision\]\[/SoC\[/CPU cluster\]\[/variant\]\]. CPU clusters can be used to specify if an image should be flashed to a single core of a multi core SoC, like the Arm Cortex-M33. Variants can specify hardware differences or options for building. For example, some SoCs will have an "ns" variant, which denotes a non-secure build when the use of Trusted Firmware-M is an option.


## Steps in porting a board
### 1. Make sure the SoC is supported
Ensure that the SoC the board has is supported by Zephyr. This can be done by checking the list of [already supported boards and shields](https://docs.zephyrproject.org/latest/boards/index.html#boards). If it's not supported, there is a guide for porting new architecture.

### 2. Create a board directory
Find an existing board that uses the SoC your board uses. Zephyr suggests starting by copy/pasting it's board directory (zephyr/boards/\<VENDOR\>/\<BOARD NAME\>), renaming that to the name of the new board (lets say that's MCU1), and changing the contents of the new directory to match the hardware of MCU1. Having the new board directory within the appropriate vendor directory is mandatory if contributing to Zephyr, but optional if just working locally. The name of the new board directory does not need to match it's name. Multiple boards can be in one directory. 

The following files are mandatory: 
 - board.yml
 - MCU1\_\<qualifiers\>.dts
 - Kconfig.MCU1

#### board.yml
Describes high-level details of the board like name, SoC(s), variants, etc. CPU clusters are not described here, as they are inherited from the SoC's YAML file.

#### MCU1\_\<qualifiers\>.dts
Devicetree file for the board. Describes SoC, connectors, and hardware components like on-board LEDs, sensors, peripherals, etc.

#### Kconfig.MCU1
Software configuration for selection of SoC and other board settings when building.

The above files are the bare minimum for getting a board to work.

### 3. Test if it builds
Once the board directory is complete, you can test if it's correct by building the hello world sample with:
```
west build -b MCU1 samples/hello_world
```

### 4. Add flash support
To add support for west to be able to flash the board, the board.cmake file will need to be included in the board directory. This file will configure a "runner" for the board, which is a Zephyr-specific python class that wraps flash and debug tools. West's flash command is basically powered by runners. Examples can be referenced in the board porting guide or in extant board.cmake files. 

In the board.yml file, you can specify certain options for flashing, like if a board needs to be of a certain revision, or if a range of revisions is OK. If a board's revision is a string rather than a number or single letter, a custom board.cmake file will be necessary, since Zephyr doesn't natively support string revisions.










