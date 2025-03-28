
# Week 9
Quite proud of myself for getting the display to work through the auxdisplay driver! 
Sadly, the IC in the display already has a driver since it can be used to control LED matricies more generally, so I probably can't contribute what I wrote which is OK because the auxdisplay API probably is not the best spot for a segmented display like the Sparkfun quad alphanumeric. Many funcitons can't really be completed because the display doesn't have as many features as LCD dot matrix displays (like the Sparkfun serial LCD) that the API is intended for.

### ht16k33
Folder containing program to run the driver for the quad alphanumeric display. Mostly the same as other project folders. Called ht16k33 because the IC that controls the display is the VK16K33, which is very nearly identical to the ht16k33. 

### auxdisplay_quadalpha.c
C file implementing the auxdisplay API for the quad alphanumeric display. Belongs in a zephyr repo in: 
```
zephyr/drivers/auxdisplay
```
Implements functions like auxdisplay\_quadalpha\_write, defines certain commands for use in I2C, such as QUADALPHA\_BRIGHT\_MAX which sets the LEDs to their maximum brightness. Also uses DEVICE_API macro to declare the implemented API struct.

### sparkfun,quadalpha.yaml
yaml file to describe the display as a devicetree node. HEAVILY referenced sparkfun,serlcd.yaml as an example. Belongs in same directory as above file.

### Kconfig.quadalpha
Kconfig file to describe AUXDISPLAY_QUADALPHA as a kconfig option. Belongs in same directory as above file.

### Kconfig
Added:
```
source "drivers/auxdisplay/Kconfig.quadalpha"
```
to ensure quadalpha option is included in kconfig options for project. Belongs in same directory as above file.

### CMakeLists.txt
Added: 
```
zephyr_library_sources_ifdef(CONFIG_AUXDISPLAY_SERLCD		auxdisplay_quadalpha.c)
```
to ensure quadalpha driver included when building auxdisplay driver as a whole. Belongs in same directory as above file.

### boardPorting.md
Notes on the guide on how to port new boards to Zephyr. Basically a distilled all you need and nothing you don't version.
