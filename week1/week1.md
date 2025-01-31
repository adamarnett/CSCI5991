## Week 1 - Development Environment, Tools, Basic GPIO


#### circle_blinky
Simple program for the nRF52840 that makes it's LEDs blink in a rapid clockwise fashion. Demonstrates some basic GPIO. See circle_blinky/src/main.c

#### button_blinky
Anoter simple program, but this one uses the buttons onboard the nRF to switch between different states for the LEDs. One that's just on, one that's just off, one that flashes all the LEDs, and one that flashes them in sequence.

#### Notes on the Power Profiler Kit II (PPK2)
In order to setup the PPK2 I had to download the desktop version of nRF Connect, and get the Power Profiler software from within that program. Once that was installed and running, I selected the connected PPK2, set it to ampere mode, and disabled power output. I originally had power output enabled, but when that was the case it just showed what seemed to be noise around 500 nA, which is not much higher than the minimum amount of current the manual says it can measure.
![physical setup](ppk2_setup.png)

I am very tired I will finish this in the morning I guess
