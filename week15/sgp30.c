

#define DT_DRV_COMPAT sensirion_sgp30

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include <zephyr/drivers/sensor/sgp30.h>
#include "sgp30.h"

LOG_MODULE_REGISTER(SGP30, CONFIG_SENSOR_LOG_LEVEL);

static uint8_t sgp30_compute_crc(uint16_t value)
{
	uint8_t buf[2];

	sys_put_be16(value, buf);

	return crc8(buf, 2, SGP30_CRC_POLY, SGP30_CRC_INIT, false);
}

static int sgp30_write_command(const struct device *dev, enum sgp30_command cmd) {

	const struct sgp30_config *config = dev->config;
    struct sgp30_data *data = dev->data;
	uint8_t tx_buf[2];

	sys_put_be16(cmd, tx_buf);

    int err = i2c_write_dt(&config->bus, tx_buf, sizeof(tx_buf));

    // if initializing iaq, need to take measurements once every second
    // to ensure compensation algorithms in sensor work correctly, so
    // we start a timer that will trigger the setting of the iaqInitialized
    // bool to false
    if (cmd == SGP30_CMD_IAQ_INIT && !err) {
        data->iaqInitialized = true;
        k_timer_start(
            &data->sgp30_iaq_timer,
            K_MSEC(1000),
            K_MSEC(1000)
        );
    }
    // return error if trying to get iaq before initializing it
    else if (cmd == SGP30_CMD_MEASURE_IAQ && !data->iaqInitialized) {
        return -EIO;
    }

	return err;
}

static int sgp30_attr_set(
                const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	struct sgp30_data *data = dev->data;
    int32_t temp_val1 = val->val1;
    int32_t temp_val2 = val->val2;

    if (
        chan != SENSOR_CHAN_GAS_RES &&
        chan != SENSOR_CHAN_VOC &&
        chan != SENSOR_CHAN_CO2
    ) {
        return -ENOTSUP;
    }

    // error checking --> SGP30 expects fixed-point 8.8 bit numbers
    if (temp_val1 < 0 || temp_val2 < 0) {
        return -EINVAL;
    }
    while (temp_val1 > INT8_MAX) {
        temp_val1 = temp_val1/10;
    }
    while (temp_val2 > INT8_MAX) {
        temp_val2 = temp_val1/10;
    }

    uint16_t raw_values = (((uint8_t)temp_val1) << 8) + (uint8_t)temp_val2;

    switch (chan) {
        case SENSOR_CHAN_GAS_RES:
            data->humidity_compensation = raw_values;
            break;
        case SENSOR_CHAN_CO2:
            data->co2eq_baseline = raw_values;
            break;
        case SENSOR_CHAN_VOC:
            data->tvoc_baseline = raw_values;
            break;
    }

    return 0;
}

static int sgp30_calibrate(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr)
{
    const struct sgp30_config *config = dev->config;
	struct sgp30_data *data = dev->data;
    int err = 0;

    if (
        chan != SENSOR_CHAN_GAS_RES &&
        chan != SENSOR_CHAN_VOC &&
        chan != SENSOR_CHAN_CO2
    ) {
        return -ENOTSUP;
    }

    // setting absolute humidity needs 2 bytes for command, 2 for parameter, and 1 for crc
    // setting iaq baseline needs 2 bytes for command, 2*2=4 for parameters, 2*1=2 for crcs
    // in either case, the parameter is a fixed 8.8 bit number where the first 8 bits represent
    // the integer and the second 8 bits represent the fracitonal part
    // when setting the iaq baseline, seperate parameters for tvoc and co2eq must be sent.
    // the parameter that comes after the command in iaq baseline looks like this, with each
    // box representing one byte:
    //┌────┬────┬────┬────┬────┬────┐
    //│tvoc│tvoc│tvoc│co2 │co2 │co2 │
    //│int │frac│crc │int │frac│crc │
    //└────┴────┴────┴────┴────┴────┘
    uint8_t tx_buf[9] = {0,0,0,0,0,0,0,0,0};

    switch (chan) {
        case SENSOR_CHAN_GAS_RES:
            sys_put_be16(SGP30_CMD_SET_ABSOLUTE_HUMIDITY, tx_buf);
            sys_put_be16(data->humidity_compensation, tx_buf+2);
            tx_buf[4] = sgp30_compute_crc(data->humidity_compensation);
            err = i2c_write_dt(
                &config->bus,
                tx_buf,
                sizeof(uint8_t)*5
            );
            break;
        case SENSOR_CHAN_CO2:
        case SENSOR_CHAN_VOC:
            if (data->iaqInitialized) {
                return -EIO;
            }
            sys_put_be16(SGP30_CMD_SET_IAQ_BASELINE, tx_buf);
            sys_put_be16(data->tvoc_baseline, tx_buf+2);
            tx_buf[4] = sgp30_compute_crc(data->tvoc_baseline);
            sys_put_be16(data->co2eq_baseline, tx_buf+5);
            tx_buf[7] = sgp30_compute_crc(data->co2eq_baseline);
            err = i2c_write_dt(
                &config->bus,
                tx_buf,
                sizeof(uint8_t)*8
            );
            break;
        default:
            return -ENOTSUP;
    }
    if (err) {
        return err;
    }

	return 0;
}

static int sgp30_selftest(const struct device *dev)
{
	const struct sgp30_config *config = dev->config;
	uint8_t rx_buf[3];
    uint16_t test_result = 0;
	int err = 0;

	err = sgp30_write_command(dev, SGP30_CMD_MEASURE_TEST);
	if (err < 0) {
		LOG_ERR("Failed to start selftest!");
		return err;
	}

	k_sleep(K_MSEC(SGP30_WAIT_MEASURE_TEST_MS));

	err = i2c_read_dt(&config->bus, rx_buf, sizeof(rx_buf));
	if (err < 0) {
		LOG_ERR("Failed to read data sample.");
		return err;
	}

	test_result = sys_get_be16(rx_buf);
	if (sgp30_compute_crc(test_result) != rx_buf[2]) {
		LOG_ERR("Received invalid CRC from selftest.");
		return -EIO;
	}

	if (test_result != SGP30_TEST_OK) {
		LOG_ERR("Selftest failed.");
		return -EIO;
	}

	return 0;
}

static int sgp30_sample_fetch(const struct device *dev,
			       enum sensor_channel chan) {

    const struct sgp30_config *config = dev->config;
    struct sgp30_data *data = dev->data;
    uint8_t tx_buf[2] = {0,0};
    uint8_t rx_buf[6] = {0,0,0,0,0,0};
    uint16_t raw_sample = 0;
    int err = 0;
    
    if (
        chan != SENSOR_CHAN_GAS_RES &&
        chan != SENSOR_CHAN_VOC &&
        chan != SENSOR_CHAN_CO2 &&
        chan != SENSOR_CHAN_ALL
    ) {
        return -ENOTSUP;
    }
    
    // if getting all channels, don't need to call with both
    // VOC and CO2 because either will get the other since they
    // get read out by the same command
    if (chan == SENSOR_CHAN_ALL) {
        err = sgp30_sample_fetch(dev, SENSOR_CHAN_GAS_RES);
        if (err) {
            return err;
        }
        return sgp30_sample_fetch(dev, SENSOR_CHAN_CO2);
    }

    // if first iaq measurement, send the init command
    if (
        (!data->iaqInitialized) &&
        (chan == SENSOR_CHAN_CO2 || chan == SENSOR_CHAN_VOC)
    ) {
        err = sgp30_calibrate(
            dev,
            SENSOR_CHAN_VOC,
            SENSOR_ATTR_CALIBRATION
        );
        if (err) {
            return err;
        }
        err = sgp30_write_command(dev, SGP30_CMD_IAQ_INIT);
        if (err) {
            return err;
        }
    }

    // send measurement command to get back H2 and ETOH or CO2EQ and TVOC
    sys_put_be16(
        (chan == SENSOR_CHAN_GAS_RES ? SGP30_CMD_MEASURE_RAW : SGP30_CMD_MEASURE_IAQ),
        tx_buf
    );
    err = i2c_write_dt(&config->bus, tx_buf, sizeof(tx_buf));
    if (err) {
        LOG_ERR("I2C write failed while fetching measurement");
        return err;
    }

    // wait for measurement to be taken by device
    k_msleep(
        (chan == SENSOR_CHAN_GAS_RES ? SGP30_WAIT_MEASURE_RAW_MS : SGP30_WAIT_MEASURE_IAQ_MS)
    );

    // read back 6 bytes of data:
    // RAW: <H2 MSB>, <H2 LSB>, CRC, <ETOH MSB>, <ETOH LSB>, CRC
    // IAQ: <CO2EQ MSB>, <CO2EQ LSB>, CRC, <TVOC MSB>, <TVOC LSB>, CRC
    err = i2c_read_dt(&config->bus, rx_buf, sizeof(rx_buf));
    if (err) {
        LOG_ERR("I2C read failed while fetching measurement");
        return err;
    }

    // check CRC for H2/CO2EQ measurement, update data if it looks good
    // this is for H2/CO2EQ because it is read out first (index 0 in rx_buf)
    raw_sample = sys_get_be16(rx_buf);
	if (sgp30_compute_crc(raw_sample) != rx_buf[2]) {
		LOG_ERR("Invalid CRC8 for data sample.");
		return -EIO;
	} else {
        if (chan == SENSOR_CHAN_GAS_RES) {
            data->raw_h2 = raw_sample;
        } else {
            data->co2eq = raw_sample;
        }
    }

    // do the same as above but for ETOH/TVOC, which is at index 3 in rx_buf
    raw_sample = sys_get_be16(rx_buf+3);
	if (sgp30_compute_crc(raw_sample) != rx_buf[5]) {
		LOG_ERR("Invalid CRC8 for data sample.");
		return -EIO;
	} else {
        if (chan == SENSOR_CHAN_GAS_RES) {
            data->raw_etoh = raw_sample;
        } else {
            data->tvoc = raw_sample;
        }
    }

    printk("data->humid [%d], data->voc [%d]\n", data->humidity_compensation, data->tvoc_baseline);

	return 0;
}

static int sgp30_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct sgp30_data *data = dev->data;

	if (
        chan != SENSOR_CHAN_GAS_RES &&
        chan != SENSOR_CHAN_VOC &&
        chan != SENSOR_CHAN_CO2 
    ) {
        return -ENOTSUP;
    }

    switch (chan) {
        case SENSOR_CHAN_GAS_RES:
            val->val1 = data->raw_h2;
	        val->val2 = data->raw_etoh;
            break;
        case SENSOR_CHAN_CO2:
            val->val1 = data->co2eq;
	        val->val2 = 0;
            break;
        case SENSOR_CHAN_VOC:
            val->val1 = data->tvoc;
	        val->val2 = 0;
            break;
        default:
            return -ENOTSUP;
    }

	return 0;
}

extern void sgp30_iaq_timer_expiration(struct k_timer *timer_id) {
    // does it need a soft reset or just re-init after timer expires?

    // get pointer to user_data assigned in sgp30_init function
    // user_data is a bool representing if iaq has been initialized
    void* user_data = k_timer_user_data_get(timer_id);

    // set data->iaqInitialized to false, since timer expired
    user_data = false;

}

static int sgp30_init(const struct device *dev)
{
	const struct sgp30_config *config = dev->config;
    struct sgp30_data *data = dev->data;

    // initialize timer for tracking repeated iaq measurements
    // needed because measurements must be taken at least every one second
    // otherwise a soft reset(?) and re-issue of the iaq_init command
    // will be necessary to get accurate data
    k_timer_init(
        &data->sgp30_iaq_timer,
        sgp30_iaq_timer_expiration,
        NULL // no stop function needed right now
    );
    // give the timer access to the iaqInitialized bool so that the expiration
    // function can set this to false if the timer expires
    k_timer_user_data_set(
        &data->sgp30_iaq_timer,
        &data->iaqInitialized
    );

    // humidity compensation value set to 0 (no compensation) by default
    data->humidity_compensation=0;
    // iaq not initialized by default since that would necessitate a measurement
    // to be taken every second going forward
    data->iaqInitialized=false;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("Bus not ready.");
		return -ENODEV;
	}

	if (config->selftest) {
		int err = sgp30_selftest(dev);

		if (err < 0) {
			LOG_ERR("Selftest failed!");

			return err;
		}
		LOG_DBG("Selftest succeeded!");
	}

	return 0;
}

static DEVICE_API(sensor, sgp30_api) = {
	.sample_fetch = sgp30_sample_fetch,
	.channel_get = sgp30_channel_get,
	.attr_set = sgp30_attr_set,
};

// define timer, data struct, config struct, and sensor instance
    //static struct k_timer sgp30_iaq_timer_##n;  
    //                                        
    //K_TIMER_DEFINE(sgp30_iaq_timer_##n, sgp30_iaq_timer_expiration, NULL); 
#define SGP30_INIT(n)						\
	static struct sgp30_data sgp30_data_##n;		\
								\
	static const struct sgp30_config sgp30_config_##n = {	\
		.bus = I2C_DT_SPEC_INST_GET(n),			\
		.selftest = DT_INST_PROP(n, enable_selftest),	\
	};							\
								\
	SENSOR_DEVICE_DT_INST_DEFINE(n,				\
			      sgp30_init,			\
			      PM_DEVICE_DT_INST_GET(n),	\
			      &sgp30_data_##n,			\
			      &sgp30_config_##n,		\
			      POST_KERNEL,			\
			      CONFIG_SENSOR_INIT_PRIORITY,	\
			      &sgp30_api);

DT_INST_FOREACH_STATUS_OKAY(SGP30_INIT)
