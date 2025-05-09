

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP30_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP30_H_

#ifdef __cplusplus
extern "C" {
#endif

enum sensor_attribute_sgp30 {
	// Absolute humidity is in mg/m^3, between 0 and 256000
    // Writing 0 to absolute humidity disables humidity compensation
	SENSOR_ATTR_SGP30_ABSOLUTE_HUMIDITY

    // SENSOR_ATTR_SGP30_IAQ_BASELINE
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP30_H_ */
