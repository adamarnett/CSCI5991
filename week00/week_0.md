# Week 0

For this week, I installed everything necessary to start working with zephyr.

To start, I had to make sure python's pip was installed, and create a virtual environment to install west.

Once West was installed, I basically cloned the whole zephyr repo with:
```west init zephyr_0```
where zephyr_0 is the name of the folder I created.

I then needed to update it which took forever on my anemic quad core processor from like 2015.

Once it was updated, I had to install the SDK. There is a way to do this with west, but it had some sort of bug that broke it for me. I had to manually install it with:
```
cd ~
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/zephyr-sdk-0.17.0_linux-x86_64.tar.xz
wget -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/sha256.sum | shasum --check --ignore-missing
tar xvf zephyr-sdk-0.17.0_linux-x86_64.tar.xz
cd zephyr-sdk-0.17.0
./setup.sh
sudo cp ~/zephyr-sdk-0.17.0/sysroots/x86_64-pokysdk-linux/usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d
sudo udevadm control --reload
```


### blinky
The first example I programmed to the board was blinky, which simply flashes an LED on the board. This was done with:
``` west build -p always -b nrf52840dk/nrf52840 samples/basic/blinky ```
This was done from my zephyr directory, ~/zephyr_0.

#### -p
The '-p always' part forces a pristine build, which creates a new build directory, such that there are no artifacts or byproducts from previous builds infiltrate the new one. Using auto instead of always causes the west build command to attempt to detect some situations in which previous builds might affect the new one. Pristine builds are important when switching to a new board, because doing so may lead to errors if a new directory isn't used. Using the following will set every build to be pristine:
``` west config build.pristine always ```
Changing always in the above command to never will disable pristine builds and always reuse the same directory.

#### -b
The '-b nrf52840dk/nrf52840' part indicates what kind of board to build for. Tis must be specified down to a specific CPU cluster. One or more CPU clusters may be present within one SoC (system on a chip) and multiple SoCs may be present on one board. Some boards (including the nRF52840DK) can emulate others, so an emulator may be specified as well. By including 'nrf52840dk/nrf52840' in the full build command above, that specified the target as the nrf52840 SoC (which has one CPU cluster) on the nRF52840 dev kit board. Another option is 'nrf52840dk/nrf52811' which sets the target as the emulation of the nRF52811 IC by the nRF52840. This is an option, since a development kit does not exist for the nRF52811.

After running the command initially, I got an error that said I didn't have the 'elftools' module. Why would I want elf tools, dwarves are cooler. To fix, I first tried using apt to get the module, but it was actually a python package. This worked:
``` pip install pyelftools ```

After the image(?) was built, flashing the board was pretty easy:
``` west flash ```
I was missing another python dependency (intelhex) but that was installed the same way elftools was with pip.

In order to modify the program I just flashed to the board, I made changes to the file zephyr_0/zephyr/samples/basic/blinky/src/main.c.
Changing the SLEEP_TIME_MS constant changes the delay between state changes for the LED. It's the most basic program possible that actually does something so there's not much to change other than changing the delay.

