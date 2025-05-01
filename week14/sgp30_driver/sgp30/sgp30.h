

#ifndef ZEPHYR_DRIVERS_SENSOR_SGP30_SGP30_H_
#define ZEPHYR_DRIVERS_SENSOR_SGP30_SGP30_H_

#include <zephyr/device.h>


enum sgp30_command {
	SGP30_CMD_IAQ_INIT                    =  0x2003,
    SGP30_CMD_MEASURE_RAW	              =  0x2050,
    SGP30_CMD_MEASURE_TEST	              =  0x2032,
    SGP30_CMD_MEASURE_IAQ                 =  0x2008,
    SGP30_CMD_GET_IAQ_BASELINE            =  0x2015,
    SGP30_CMD_SET_IAQ_BASELINE            =  0x201E,
    SGP30_CMD_SET_ABSOLUTE_HUMIDITY       =  0x2061,
    SGP30_CMD_GET_FEATURE_SET             =  0x202F,
    SGP30_CMD_GET_TVOC_INCEPTIVE_BASELINE =  0x20B3,
    SGP30_CMD_SET_TVOC_BASELINE           =  0x2077,
    SGP30_CMD_GET_SERIAL_ID               =  0x3682
};

#define SGP30_TEST_OK		0xD400

#define SGP30_WAIT_MS               10
#define SGP30_WAIT_MEASURE_IAQ_MS   12
#define SGP30_WAIT_MEASURE_RAW_MS   25
#define SGP30_WAIT_MEASURE_TEST_MS  220

/*
 * CRC parameters were taken from the
 * "Checksum Calculation" section of the datasheet.
 */
#define SGP30_CRC_POLY		0x31
#define SGP30_CRC_INIT		0xFF


// value range of data collected
#define SGP30_MIN_TVOC      0
#define SGP30_MAX_TVOC      60000
#define SGP30_MIN_CO2EQ     400
#define SGP30_MAX_CO2EQ     60000
#define SGP30_MIN_ETOH      0
#define SGP30_MAX_ETOH      1000
#define SGP30_MIN_H2        0
#define SGP30_MAX_H2        1000

// value range of humidity compensation parameter
#define SGP30_MIN_ABSOLUTE_HUMIDITY 0
#define SGP30_MAX_ABSOLUTE_HUMIDITY 256000 

struct sgp30_config {
	struct i2c_dt_spec bus;
    // true if self test enabled during init
	bool selftest;
};

struct sgp30_data {
	uint16_t raw_etoh;
    uint16_t raw_h2;
    uint16_t tvoc;
    uint16_t co2eq;
    // humidity compensation parameter, 0 if compensation disabled
    uint32_t humidityCompensation;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SGP30_SGP30_H_ */
