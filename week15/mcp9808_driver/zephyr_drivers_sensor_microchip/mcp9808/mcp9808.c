/*
 * Copyright (c) 2025 Adam Arnett
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp9808

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcp9808, CONFIG_SENSOR_LOG_LEVEL);

struct mcp9808_config {
	struct i2c_dt_spec bus;
};

struct mcp9808_data {
	uint16_t raw_ambient_temp;
    uint16_t critical_temp;
    uint16_t alert_temp_upper;
    uint16_t alert_temp_lower;
    uint16_t config_reg;
    bool shutdown;
};

enum mcp9808_register {
    MCP9808_REG_CONFIG  = 0x01,
    MCP9808_REG_T_UPPER = 0x02,
    MCP9808_REG_T_LOWER = 0x03,
    MCP9808_REG_T_CRIT  = 0x04,
    MCP9808_REG_T_AMB   = 0x05
};

// bits in configuration register
enum mcp9808_config_bit {
    // bits 15 through 11 are unused
    MCP9808_CONFIG_T_HYST_M     = BIT(10),
    MCP9808_CONFIG_T_HYST_L     = BIT(9),
    MCP9808_CONFIG_SHDN         = BIT(8),
    MCP9808_CONFIG_CRIT_LOCK    = BIT(7),
    MCP9808_CONFIG_WIN_LOCK     = BIT(6),
    MCP9808_CONFIG_INT_CLEAR    = BIT(5),
    MCP9808_CONFIG_ALERT_STAT   = BIT(4),
    MCP9808_CONFIG_ALERT_CONT   = BIT(3),
    MCP9808_CONFIG_ALERT_SEL    = BIT(2),
    MCP9808_CONFIG_ALERT_POL    = BIT(1),
    MCP9808_CONFIG_ALERT_MOD    = BIT(0)
};

enum mcp9808_amb_temp_bit {
    MCP9808_AMB_TEMP_VS_T_CRIT  = BIT(15),
    MCP9808_AMB_TEMP_VS_T_UPPER = BIT(14),
    MCP9808_AMB_TEMP_VS_T_LOWER = BIT(13),
    MCP9808_AMB_TEMP_SIGN       = BIT(12)
    // bits 11 through 0 represent ambient temp in celcius
};

static int mcp9808_config_set(
    const struct device *dev, 
    enum mcp9808_config_bit bit
) {
    const struct mcp9808_config *config = dev->config;
    const struct mcp9808_data *data = dev->data;

    uint8_t tx_buf[1] = {bit};
    int err = i2c_write_dt(
        &config->bus,
        tx_buf,
        sizeof(tx_buf)
    );
    if (err) {
        return;
    }
    
    err = i2c_write_dt(
        &config->bus,
        &data->config_reg,
        sizof(data->config_reg)
    );
    if (err) {
        return err;
    }

    return 0;
}

static int mcp9808_attr_set(
    const struct device *dev,
    enum sensor_channel chan,
    enum sensor_attribute attr,
    const struct sensor_value *val
) {


    if (
        attr != SENSOR_ATTR_HYSTERESIS &&
        attr != SENSOR_ATTR_UPPER_THRESH &&
        attr != SENSOR_ATTR_LOWER_THRESH &&
        attr != SENSOR_ATTR_ALERT
    ) {
        return -EINVAL;
    }

    //switch (attr)
    //{
    //case SENSOR_ATTR_ALERT:
    //    
    //    break;
    //
    //default:
    //    break;
    //}


    return 0;
}

static int mcp9808_convert_ambient_temp(
    uint16_t raw_temp,
    struct sensor_value *val
) {

    // if haven't gotten measurement yet
    if (raw_temp == 0xE0) {
        return -EINVAL;
    }

    // get lowest 4 bits for fractional part
    val->val2 = (int32_t)(raw_temp & 0x000F);

    // right shift to get rid of fractional part
    raw_temp = raw_temp >> 4;

    // if sign bit is set
    if (raw_temp & MCP9808_AMB_TEMP_SIGN) {
        // sign extension
        raw_temp |= 0xFE00;
    } else {
        // otherwise ensure only bits that hold numerical
        // data are set
        raw_temp &= 0x00FF;
    }
    
    val->val1 = (int32_t)(raw_temp & 0xFF);

    return 0;
}

static int mcp9808_sample_fetch(
    const struct device *dev, 
    enum sensor_channel chan
) {
    const struct mcp9808_config *config = dev->config;
    struct mcp9808_data *data = dev->data;

    uint8_t tx_buf[1] = {MCP9808_REG_T_AMB};
    uint8_t rx_buf[2] = {0,0};

    // write 0x05 to register pointer
    int err = i2c_write_read_dt(
        &config->bus,
        tx_buf,
        sizeof(tx_buf),
        rx_buf,
        sizeof(rx_buf)
    );
    if (err) {
        return err;
    }

    data->raw_ambient_temp = (rx_buf[0] << 8) + rx_buf[1];

    return 0;
}

static int mcp9808_channel_get(const struct device *dev,
    enum sensor_channel chan,
    struct sensor_value *val
) {
    struct mcp9808_data *data = dev->data;

    if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
        return -ENOTSUP;
    }

    return mcp9808_convert_ambient_temp(data->raw_ambient_temp, val);
    
}

static int mcp9808_init(const struct device *dev) {
    struct mcp9808_data *data = dev->data;

    // set ambient temp to a value that's not possible to get
    // back from the sensor to indicate lack of fetch
    data->raw_ambient_temp = 0xE0;
    data->critical_temp = 0x00;
    data->alert_temp_upper = 0x00;
    data->alert_temp_lower = 0x00;
    data->hysteresis = 0x00;
    data->shutdown = false;

    return 0;
}

static DEVICE_API(sensor, mcp9808_api) = {
	.sample_fetch = mcp9808_sample_fetch,
	.channel_get = mcp9808_channel_get,
	.attr_set = mcp9808_attr_set,
};

#define MCP9808_INIT(n)						\
	static struct mcp9808_data mcp9808_data_##n;		\
								\
	static const struct mcp9808_config mcp9808_config_##n = {	\
		.bus = I2C_DT_SPEC_INST_GET(n),			\
	};							\
								\
	SENSOR_DEVICE_DT_INST_DEFINE(n,				\
                mcp9808_init,			\
                NULL,                       \
			    &mcp9808_data_##n,			\
			    &mcp9808_config_##n,		\
			    POST_KERNEL,			\
			    CONFIG_SENSOR_INIT_PRIORITY,	\
			    &mcp9808_api);

DT_INST_FOREACH_STATUS_OKAY(MCP9808_INIT)
