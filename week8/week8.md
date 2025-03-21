
This week has been a bit rough. 
 
I was sick at the beginning of the week, and have not had access to my ADHD medication because my pharmacy has been out of stock. 

I worked on the geiger counter for a bit, and have come to hate asynchronous serial communication. I was able to get the display to be a little bit more responsive, but then broke it somehow and hadn't made any commits along the way (I couldn't restore it to it's narrowly better state). I am going to blame the counter to a certain degree for how strangely it outputs data. The fact that it only sends data roughly every one second (956ms according to saleae) massively limits the ability for the display to look like it's updating quickly. Part of it is likely on me for not being able to respond to these quick enough, but even best case scenario it won't look very good. Kinda want to shift gears to something else because this is driving me up a wall. I'd rather eat the uarnium ore I got to test it than continue trying to force this to work. (I also hate how physically clunky it is!!!)

Better paths for the remainder of the semester would include creating and contributing a driver or two to zephyr, porting zephyr to a new board, or trying to fix an issue posted on the zephyr github.

I made a document called driver.md with steps on how to create a driver. I've started by reading the datasheet for the IC that controls the Sparkfun Quad Alphanumeric display and getting it to work with an Arduino. It's a little weird because if I'm reading the data sheet right, one of the I2C lines has to be pulled high for a duration to wake up the IC, and a normal I2C start condition won't accomplish this. The behavior of the I2C lines on the Arduino is strange, as the datasheet says the default address is 0X70, but the writes are to address 0X00 and 0X34. Very odd, but I believe I can figure it out! My current theory is that I missed something like additional bits in the address transmission like how the FRAM chip has extra pins that can change the addresss.

Speaking of the FRAM chip, the reason why it was reading out data backwards was because I was reading into an array of uint8 variables. Since the packed struct was written to the chip from a little endian board, the least significant byte of the uint32 at the beginning of the struct was the first byte written. then the next three bytes in the uint32 followed in increasing significance, then the rest of the struct. By reading back each byte into it's own uint8, no effort was made by the compiler to reorganize anything, because it didn't matter. An easier way to "fix" the reversed bytes rather than using the shifting and masking I was originally doing is simply to cast the index of the array corresponding to the start of something bigger than a uint8 to a pointer of the correct type and dereference that. For example, at the beginning of the struct is a uint32. To get it from the array of read data is easy: 
```
*((uint32_t*)(&rxBuf[0]))
```

