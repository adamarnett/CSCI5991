

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

static int sgp30_write_command(const struct device *dev, sgp30_command cmd) {

	const struct sgp30_config *config = dev->config;
	uint8_t tx_buf[2];

	sys_put_be16(cmd, tx_buf);

	return i2c_write_dt(&config->bus, tx_buf, sizeof(tx_buf));
}

static int sgp30_attr_set(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	struct sgp30_data *data = dev->data;

    if ((enum sensor_attribute_sgp30)attr != SENSOR_ATTR_SGP30_ABSOLUTE_HUMIDITY) {
        return -ENOTSUP;
    }

    uint8_t tx_buf[3] = {0,0,0};


    // NOTE TODO LOOK HERE PLEASE
    // anywhere data is read or write might need modification, because I just noticed
    // on the datasheet that there is no stop condition between sending the command
    // and sending the data or starting the read, so different zephyr functions will
    // be needed (like i2c_write_read_dt, or a modification of how i2c_write is called)


	/*
	 * Temperature and RH conversion to ticks as explained in datasheet
	 * in section "I2C commands"
	 */

	switch ((enum sensor_attribute_sgp30)attr) {
	case SENSOR_ATTR_SGP30_TEMPERATURE:
	{
		uint16_t t_ticks;
		int16_t tmp;

		tmp = (int16_t)CLAMP(val->val1, SGP30_COMP_MIN_T, SGP30_COMP_MAX_T);
		/* adding +87 to avoid most rounding errors through truncation */
		t_ticks = (uint16_t)((((tmp + 45) * 65535) + 87) / 175);
		sys_put_be16(t_ticks, data->t_param);
		data->t_param[2] = sgp30_compute_crc(t_ticks);
	}
		break;
	case SENSOR_ATTR_SGP30_HUMIDITY:
	{
		uint16_t rh_ticks;
		uint8_t tmp;

		tmp = (uint8_t)CLAMP(val->val1, SGP30_COMP_MIN_RH, SGP30_COMP_MAX_RH);
		/* adding +50 to eliminate rounding errors through truncation */
		rh_ticks = (uint16_t)(((tmp * 65535U) + 50U) / 100U);
		sys_put_be16(rh_ticks, data->rh_param);
		data->rh_param[2] = sgp30_compute_crc(rh_ticks);
	}
		break;
	default:
		return -ENOTSUP;
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

	k_sleep(SGP30_WAIT_MEASURE_TEST_MS);

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

	if (raw_sample != SGP30_TEST_OK) {
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
    raw_sample = sys_get_be16(rx_buf{3});
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
            val->val1 = data->raw_etoh;
	        val->val2 = 0;
            break;
        case SENSOR_CHAN_CO2:
            val->val1 = data->co2eq;
	        val->val2 = 0;
            break;
        case SENSOR_CHAN_VOC:
            val->val1 = data->tvoc;
	        val->val2 = 0;
            break;
    }

	return 0;
}


static int sgp30_init(const struct device *dev)
{
	const struct sgp30_config *config = dev->config;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("Bus not ready.");
		return -ENODEV;
	}

	if (config->selftest) {
		int rc = sgp30_selftest(dev);

		if (rc < 0) {
			LOG_ERR("Selftest failed!");
			return rc;
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

#define SGP30_INIT(n)						\
	static struct sgp30_data sgp30_data_##n;		\
								\
	static const struct sgp30_config sgp30_config_##n = {	\
		.bus = I2C_DT_SPEC_INST_GET(n),			\
		.selftest = DT_INST_PROP(n, enable_selftest),	\
	};							\
								\
	PM_DEVICE_DT_INST_DEFINE(n, sgp30_pm_action);		\
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
