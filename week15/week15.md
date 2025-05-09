# Week 15
I am so exhausted but also so set on finishing a driver I can contribute. This week I was supposed to finish the SGP30 driver and send an email. I did send the email. The SGP30 driver could likely use more testing and refinement, but seeing as how it's an obsolete part, I don't know if it would be accepted. I did however find a temperature sensor breakout board in my desk drawer at home, which utilizes an IC that is not obsolete, does not yet have a driver in the Zephyr repo, and is not compatible with any other drivers present in the Zephyr repo. So hopefully that would work! I ended up making a driver for that between last night and this morning. I have it reading temperatures, and the attribute stuff is written but not tested. I'm proud of myself for being able to get it done so quickly! At the beginning of the semester I didn't even know where to start on a driver and now I can get one functional (to a certain degree) in a matter of hours!  
  
### Important files 
 
 - ```/mcp9808_driver```
	 + Contains driver code, Kconfig definition, CMakeLists, etc for mcp9808 driver
 - ```/mcp9808_driver/zephyr_drivers_sensor_microchip/mcp9808/mcp9808.c```
	 + Actual code for mcp9808 driver
 - ```/mcp9808Main```
	 + Project used to test the mcp9808 driver
 - ```/mcp9808Main/src/main.c```
	 + Main file for mcp9808 test
 - ```/therm```
	 + Thermometer project with sgp30 integrated
 - ```/sgp30_driver```
	 + Contains driver code, Kconfig definition, CMakeLists, etc for sgp30 driver
 - ```/sgp30_driver/sgp30/sgp30.c```
	 + Driver code for sgp30, updated since last week
 - ```/sgp30_driver/sgp30/sgp30.h```
	 + Header file for sgp30 code, updated since last week
 - ```/co2eq_tvoc_working.png```
	 + Output of the thermometer project when SGP30 was up and running
	 + The CO2 concentration was normal, as indoor levels can range between 400 and 1000 ppm
	 + The VOCs were a little high, but I think the fact that I had a candle burning about an hour before testing it may have caused them to be above the normal limit of 1 ppm.