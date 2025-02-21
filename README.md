# CSCI 5991 - Independent Study Repository
Student: Adam Arnett
Professor: Jack Kolb

## Topic/Purpose
The purpose of this independent study is to become acquainted with the Zephyr RTOS. This is motivated by general interest in learning more about embedded systems, specifically about RTOSes as opposed to bare metal programming. Additionally, many medical device companies are moving from bare metal programming to using an RTOS in their devices due to the increasing complexity of said devices, so learning about RTOSes would be useful professionally as well.

## Structure
The independent study will be organized by week, with each week having a specific topic. Some important topics such as schedulers may be split over multiple weeks. Normally, each week will correspond to one chapter in the textbook (see below) Each week should involve at least eight hours of work, since the course is two credits.

## Equipment
To facilitate learning by testing and demonstrating programs, the Nordic Semiconductor nRF52840 development kit will be used. Zephyr RTOS Embedded C Programming by Andrew Eliasz will also be used as a textbook.

## Topics by week
Each week outlines the intended topic and goals.

### Week 0 (Jan 21st - 24th): Hello World
 - Goal: Setup Command line tools for zephyr/nRF52840, be able to program the board using west build and west flash
 - Reference: Chapter 3 in textbook

### Week 1 (Jan 25th - 31st): Development Environment, Tools, Basic GPIO
 - Goal: Setup development environment for programming zephyr applications on the nRF52840
 - Goal: Setup Nordic Semiconductor Power Profiler Kit (PPK2) for use with the nRF52840 and test several example programs with it
 - Reference: Chapter 3 in textbook, PPK2 Manual
   - https://docs-be.nordicsemi.com/bundle/ug_ppk2/attach/PPK2_User_Guide_v1.0.1.pdf?_LANG=enus
 - Deliverable: Shell script to semi-automatically setup a zephyr development environment on an ubuntu based(?) linux distro
 - Deliverable: Markdown document detailing how to use the PPK2 and the power consumption of different sample programs
 - Deliverable: C program that can be flashed to the nRf52840 that uses the onboard LEDs and buttons to do something interesting

### Week 2 (Feb 1st - 7th): Multithreading
 - Goal: Understand how multithreading works in zephyr
 - Goal: Understand how memory protection/sharing works in zephyr
 - Goal: Investigate how the zephyr kernel communicates with a processor's MPU
 - Reference: Chapter 4 in textbook, ARM documentation for last goal
   - https://developer.arm.com/documentation/100166/0001/Memory-Protection-Unit
 - Deliverable: C program that can be flashed to the nRF52840 that utilizes three or more threads and:
   - Each thread prints a message to the console, saying which thread it is
   - Each thread has a "password" (possibly stored in thread custom data?) that should be kept secret from other threads
   - Thread passwords will be shared and included in printed messages upon user input (button press)

### Week 3 (Feb 8th - 14th): SPI
  - Note - this is being written retroactively. Unfortunate personal life events had me very sad and unable to get much done this week. I also still need to push this stuff to the git repo, but it's on my old computer.
  - Goal: Get an SPI (serial peripheral interface) bus working
  - Goal: Better understand devicetree by achieving the first goal
  - Deliverable: C program that can be flashed to the nRF52840 that...
   - Communicates in some way using an SPI bus
   - The SPI bus should utilize the Zephyr Generic SPI driver

### Week 4 (Feb 15th - 21st)

### Week 5 (Feb 22nd - 28th)

### Week 6 (Mar 1st - 7th)

### Week 7 (Mar 8th - 14th): Spring Break

### Week 8 (Mar 15th - 21st)

### Week 9 (Mar 22nd - 28th):

### Week 10 (Mar 29th - Apr 4th)

### Week 11 (Apr 5th - 11th)

### Week 12 (Apr 12th - 18th)

### Week 13 (Apr 19th - 25th)

### Week 14 (Apr 26th - May 2nd)

### Week 15 (May 3rd - 9th)

