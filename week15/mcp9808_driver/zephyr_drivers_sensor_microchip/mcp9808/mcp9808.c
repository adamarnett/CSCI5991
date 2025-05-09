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
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcp9808, CONFIG_SENSOR_LOG_LEVEL);

struct mcp9808_config {
	struct i2c_dt_spec bus;
};

// bits in configuration register
enum mcp9808_config_bit {
    // bits 15 through 11 are unused
    MCP9808_CONF_BIT_HYST_U     = BIT(10),
    MCP9808_CONF_BIT_HYST_L     = BIT(9),
    MCP9808_CONF_BIT_SHDN       = BIT(8),
    MCP9808_CONF_BIT_CRIT_LOCK  = BIT(7),
    MCP9808_CONF_BIT_WIN_LOCK   = BIT(6),
    MCP9808_CONF_BIT_INT_CLEAR  = BIT(5),
    MCP9808_CONF_BIT_ALERT_STAT = BIT(4),
    MCP9808_CONF_BIT_ALERT_CTRL = BIT(3),
    MCP9808_CONF_BIT_ALERT_SEL  = BIT(2),
    MCP9808_CONF_BIT_ALERT_POL  = BIT(1),
    MCP9808_CONF_BIT_ALERT_MOD  = BIT(0)
};

// options for resolution register
enum mcp9808_resolution {
    MCP9808_RES_0_5     = 0x00,
    MCP9808_RES_0_25    = 0x01,
    MCP9808_RES_0_125   = 0x02,
    MCP9808_RES_0_0625  = 0x03
};

// options for hysteresis bits
enum mcp9808_hysteresis {
    MCP9808_HYST_0      = (~MCP9808_CONF_BIT_HYST_U | ~MCP9808_CONF_BIT_HYST_L),
    MCP9808_HYST_1_5    = (~MCP9808_CONF_BIT_HYST_U |  MCP9808_CONF_BIT_HYST_L),
    MCP9808_HYST_3_0    = ( MCP9808_CONF_BIT_HYST_U | ~MCP9808_CONF_BIT_HYST_L),
    MCP9808_HYST_6_0    = ( MCP9808_CONF_BIT_HYST_U |  MCP9808_CONF_BIT_HYST_L)
};

struct mcp9808_data {
    uint16_t config_reg;
    uint16_t alert_temp_upper;
    uint16_t alert_temp_lower;
    uint16_t critical_temp;
	uint16_t raw_ambient_temp;
    uint16_t manufacturer_id;
    uint16_t device_id;
    enum mcp9808_resolution resolution;
    
    bool shutdown;
};

// accesible registers within mcp9808
enum mcp9808_register {
    MCP9808_REG_CONFIG  = 0x01,
    MCP9808_REG_T_UPPER = 0x02,
    MCP9808_REG_T_LOWER = 0x03,
    MCP9808_REG_T_CRIT  = 0x04,
    MCP9808_REG_T_AMB   = 0x05,
    MCP9808_REG_MAN_ID  = 0x06,
    MCP9808_REG_DEV_ID  = 0x07,
    MCP9808_REG_RESOLU  = 0x08
};

// bits in ambient temperature register
enum mcp9808_amb_temp_bit {
    MCP9808_AMB_BIT_VS_T_CRIT   = BIT(15),
    MCP9808_AMB_BIT_VS_T_UPPER  = BIT(14),
    MCP9808_AMB_BIT_VS_T_LOWER  = BIT(13),
    MCP9808_AMB_BIT_SIGN        = BIT(12)
    // bits 11 through 0 represent ambient temp in celcius
    // 11-4 represent integer part
    // 3-0 represent fractional part
};

#define MCP9808_THRESH_INT_MAX UINT8_MAX
#define MCP9808_THRESH_INT_MIN 0
#define MCP9808_THRESH_FRAC_MAX 

static int mcp9808_reg_set(
    const struct device *dev,
    enum mcp9808_register reg
) {
    const struct mcp9808_config *config = dev->config;
    const struct mcp9808_data *data = dev->data;

    // buffer to hold value of register to write to
    uint8_t tx_reg_buf[1] = {(uint8_t)reg};
    // buffer to hold new value to write to a register (16 bit)
    uint8_t tx_val_buf[2] = {0,0};

    switch (reg) {
        case MCP9808_REG_CONFIG:
            sys_put_be16(data->config_reg, tx_val_buf);
            break;
        case MCP9808_REG_T_UPPER:
            sys_put_be16(data->alert_temp_upper, tx_val_buf);
            break;
        case MCP9808_REG_T_LOWER:
            sys_put_be16(data->alert_temp_lower, tx_val_buf);
            break;
        case MCP9808_REG_T_CRIT:
            sys_put_be16(data->critical_temp, tx_val_buf);
            break;
        case MCP9808_REG_T_AMB:
            // ambient temp is read only
            return -EACCES;
        case MCP9808_REG_MAN_ID:
            // manufacturer id is read only
            return -EACCES;
        case MCP9808_REG_DEV_ID:
            // manufacturer id is read only
            return -EACCES;
        case MCP9808_REG_RESOLU:
            tx_val_buf[0] = (uint8_t)data->resolution;
            break;
        default:
            return -EINVAL;
    }

    // write to register pointer to allow the next write to go to the
    // correct register
    int err = i2c_write_dt(
        &config->bus,
        tx_reg_buf,
        sizeof(tx_reg_buf)
    );
    if (err) {
        return;
    }
    
    // write entirely new value to 16 bit register
    // note that the resolution register is only 8 bits
    err = i2c_write_dt(
        &config->bus,
        tx_val_buf,
        ((reg != MCP9808_REG_RESOLU) ? sizeof(tx_val_buf) : sizeof(tx_val_buf[0]))
    );
    if (err) {
        return err;
    }

    return 0;
}

// convert a sensor_val to the format expected by mcp9808 for the upper,
// lower, and critical threshold values; returns raw value that can be
// written to one of the temp limit registers
static uint16_t mcp9808_convert_to_thresh(struct sensor_value *val) {
    if (!IN_RANGE(val->val1, -255, 255)) {
        // TODO: figure out better way to indicate error
        return 0xE000;
    }

    // set sign bit if value is negative
    uint16_t ret = (val->val1 < 0) ? 0x0100 : 0x0000;

    // TODO: double check that this cast is ok
    // get uint8_t representation of integer part
    uint8_t int_part = (uint8_t)abs(val->val1);

    // TODO: surely there's a better way than this...
    // get left two most digits of val->val2
    int32_t val2 = val->val2;
    // the fractional part really shouldn't be negative, but just in case:
    if (val2 < 0) {
        return -EINVAL;
    }
    // get the two left most digits from it's decimal form
    while (val2 > 99) {
        val2 = val2/10;
    }
    
    // values can be represented in fractional part of thresh are .00, .25, .50, and .75
    uint8_t frac_part = DIV_ROUND_CLOSEST(CLAMP(val2, 0, 99), 25);
    // if rounding up to next number
    if (frac_part == 4) {
        // increment and check for overflow
        if (int_part++ == 0) {
            // TODO: is this the desired behavior?
            //       or throw an error if 255.87 < sensor_val < 256?
            // round down to 255.75
            int_part = 255;
            frac_part = 3;
        }
        else {
            frac_part = 0;
        }
    }

    // put together returned value with int part at bits 11-4, frac part at bits 3-2
    ret |= ((((uint16_t)int_part) << 4) & 0x0FF0);
    ret |= ((frac_part << 2) & 0x000C);

    return ret;
}

static int mcp9808_attr_set(
    const struct device *dev,
    enum sensor_channel chan,
    enum sensor_attribute attr,
    const struct sensor_value *val
) {

    if (
        attr != SENSOR_ATTR_RESOLUTION &&   // resolution reg
        attr != SENSOR_ATTR_HYSTERESIS &&   // bits 9 & 10 in config reg
        attr != SENSOR_ATTR_UPPER_THRESH && // upper alert temp reg
        attr != SENSOR_ATTR_LOWER_THRESH && // lower alert temp reg
        attr != SENSOR_ATTR_ALERT           // bit 3 in config reg
    ) {
        return -EINVAL;
    }

    struct mcp9808_data *data = dev->data;

    switch (attr)
    {
    case SENSOR_ATTR_RESOLUTION:
        // TODO: verify that this is ok and defined behavior
        // casting the int to an enum in the else IS undefined if the int
        // doesn't match any of the enum options... but is it ok if we 
        // check the int beforehand?
        if (!IN_RANGE(val->val1, MCP9808_RES_0_5, MCP9808_RES_0_0625)) {
            return -EINVAL;
        }
        else {
            data->resolution = (enum mcp9808_resolution)val->val1;
        }
        break;
    case SENSOR_ATTR_HYSTERESIS:
        if (!IN_RANGE(val->val1, MCP9808_HYST_0, MCP9808_HYST_6_0)) {
            return -EINVAL;
        }
        else {
            data->config_reg &= ~(MCP9808_CONF_BIT_HYST_U | MCP9808_CONF_BIT_HYST_L);
            data->config_reg |= (enum mcp9808_hysteresis)val->val1;
        }
        break;
    case SENSOR_ATTR_UPPER_THRESH:
        uint16_t raw_value = mcp9808_convert_to_thresh(val);
        if (raw_value == 0xE000){
            return -EINVAL;
        }
        else {
            data->alert_temp_upper = raw_value;
        }
        break;
    case SENSOR_ATTR_LOWER_THRESH:
        uint16_t raw_value = mcp9808_convert_to_thresh(val);
        if (raw_value == 0xE000){
            return -EINVAL;
        }
        else {
            data->alert_temp_lower = raw_value;
        }
        break;
    case SENSOR_ATTR_ALERT:
        if (val->val1 || val->val2) {
            data->config_reg |= MCP9808_CONF_BIT_ALERT_CTRL;
        }
        else {
            data->config_reg &= ~MCP9808_CONF_BIT_ALERT_CTRL;
        }
        break;
    default:
        return -EINVAL;
    }


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

    // TODO: double check this, very confident it's wrong
    // get lowest 4 bits for fractional part
    val->val2 = (int32_t)(raw_temp & 0x000F);

    // right shift to get rid of fractional part
    raw_temp = raw_temp >> 4;

    // if sign bit is set
    if (raw_temp & MCP9808_AMB_BIT_SIGN) {
        // sign extension
        raw_temp |= 0xFE00;
    } else {
        // otherwise ensure only bits that hold numerical
        // data are set
        raw_temp &= 0x00FF;
    }
    
    // TODO: double check this
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
