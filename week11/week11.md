# Week 11

## Notes
On 2200mAh 3.7v lipo battery, nRF power source switched to lipo, backlight and sht41 unplugged, display powers on but does not acknowledge any I2C communication... strange. Checked the voltage and on the logic supply wire it's 4.99V. Maybe not enough current?

... Likely not enough current. I'm not certain but according to a discussion on Nordic Semiconductor's website, the maximum output of the VDD is 25mA. Perhaps that's somehow higher when connected to the computer? If it wasn't then none of the LCDs should ever have worked (the lowest current requirement of them are 60 mA).

In order to measure currents that aren't totally overshadowed by the power of an LCD display, perhaps I should have the nRF52840 connect to an arduino that runs an LCD? Or perhaps I could use a mosfet to totally cut power to the LCD under certain conditions? Or maybe I should go back to the 7-segment? I will go to the ECE depot and ask them for advice. My EE knowledge is limited - for now!

Update: after going to the ECE depot we got it to work by using an external 5V boost converter to power the board over micro USB, while connecting the LCD backlight power supply directly to the battery. This implies that some aspect of the nRF52840DK is the limiting factor preventing the LCD from working. It may be that the internal 5V boost converter that the lipo power goes through can't supply enough current, or that the VDD pin can't supply enough current. In either case, what we did at the ECE depot proved that it is possible to power the whole thing on one small (150 mAh) lipo battery. It may just be that the power to the LCD needs to come directly from the battery. In this scenario, a logic level mosfet controlled by GPIO should be able to control this to turn the backlight on/off for power saving reasons. 

![Working on battery only](batteryPower.jpeg)