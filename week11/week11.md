# Week 11

## Notes
On 2200mAh 3.7v lipo battery, nRF power source switched to lipo, backlight and sht41 unplugged, display powers on but does not acknowledge any I2C communication... strange. Checked the voltage and on the logic supply wire it's 4.99V. Maybe not enough current?

... Likely not enough current. I'm not certain but according to a discussion on Nordic Semiconductor's website, the maximum output of the VDD is 25mA. Perhaps that's somehow higher when connected to the computer? If it wasn't then none of the LCDs should ever have worked (the lowest current requirement of them are 60 mA).

In order to measure currents that aren't totally overshadowed by the power of an LCD display, perhaps I should have the nRF52840 connect to an arduino that runs an LCD? Or perhaps I could use a mosfet to totally cut power to the LCD under certain conditions? Or maybe I should go back to the 7-segment? I will go to the ECE depot and ask them for advice. My EE knowledge is limited - for now!