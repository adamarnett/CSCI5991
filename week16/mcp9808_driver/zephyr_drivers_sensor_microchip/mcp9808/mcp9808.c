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
#include <zephyr/sys/byteorder.h>
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
enum mcp9808_resolution_bits {
    MCP9808_RES_0_5     = 0x00,
    MCP9808_RES_0_25    = 0x01,
    MCP9808_RES_0_125   = 0x02,
    MCP9808_RES_0_0625  = 0x03
};

// options for hysteresis bits
enum mcp9808_hysteresis {
    MCP9808_HYST_0      = 0x0000,
    MCP9808_HYST_1_5    = 0x0000 | MCP9808_CONF_BIT_HYST_L,
    MCP9808_HYST_3_0    = 0x0000 | MCP9808_CONF_BIT_HYST_U,
    MCP9808_HYST_6_0    = 0x0000 | (MCP9808_CONF_BIT_HYST_U | MCP9808_CONF_BIT_HYST_L)
};

// data struct holds value of each register in device
struct mcp9808_data {
    uint16_t reg_config;
    uint16_t reg_t_upper;
    uint16_t reg_t_lower;
    uint16_t reg_t_crit;
	uint16_t reg_t_amb;
    uint16_t reg_man_id;
    uint16_t reg_dev_id;
    // resolution register is only 8 bits
    uint8_t reg_res;
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
    MCP9808_REG_RES     = 0x08
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

#define MCP9808_THRESH_BIT_SIGN BIT(12)

// function to update a register's corresponding value in the dev->data struct
static int mcp9808_reg_get(
    const struct device *dev, 
    enum mcp9808_register reg
) {
    const struct mcp9808_config *config = dev->config;
    struct mcp9808_data *data = dev->data;

    uint8_t tx_buf[1] = {reg};
    uint8_t rx_buf[2] = {0,0};

    // write to register pointer then read the pointed to register
    // note that the resolution register is only 8 bits
    int err = i2c_write_read_dt(
        &config->bus,
        tx_buf,
        sizeof(tx_buf),
        rx_buf,
        // read 16 bits if not resolution register, else read 8 bits
        ((reg != MCP9808_REG_RES) ? sizeof(rx_buf) : sizeof(rx_buf[0]))
    );
    if (err) {
        return err;
    }

    uint16_t rxed_data = (rx_buf[0] << 8) + rx_buf[1];

    switch (reg) {
        case MCP9808_REG_CONFIG:
            data->reg_config = rxed_data;
            break;
        case MCP9808_REG_T_UPPER:
            data->reg_t_upper = rxed_data;
            break;
        case MCP9808_REG_T_LOWER:
            data->reg_t_lower = rxed_data;
            break;
        case MCP9808_REG_T_CRIT:
            data->reg_t_crit = rxed_data;
            break;
        case MCP9808_REG_T_AMB:
            data->reg_t_amb = rxed_data;
            break;
        case MCP9808_REG_MAN_ID:
            data->reg_man_id = rxed_data;
            break;
        case MCP9808_REG_DEV_ID:
            data->reg_dev_id = rxed_data;
            break;
        case MCP9808_REG_RES:
            // resolution handled differently due to 8 bit size of register
            if(!IN_RANGE(rx_buf[0], 0, 3)){
                return -EIO;
            }
            data->reg_res = (enum mcp9808_resolution_bits)rx_buf[0];
            break;
        default:
            return -EINVAL;
    }

    LOG_HEXDUMP_INF(data->reg_config, sizeof(data->reg_config), "CONFIG: ");
    LOG_HEXDUMP_INF(data->reg_t_upper, sizeof(data->reg_t_upper), "UPPER: ");
    LOG_HEXDUMP_INF(data->reg_t_lower, sizeof(data->reg_t_lower), "LOWER: ");
    LOG_HEXDUMP_INF(data->reg_res, sizeof(data->reg_res), "RESOLUTION: ");

    return 0;
}

// function to update a device register, reg, with whatever value exists for it in dev->data
static int mcp9808_reg_set(
    const struct device *dev,
    enum mcp9808_register reg
) {
    const struct mcp9808_config *config = dev->config;
    struct mcp9808_data *data = dev->data;

    uint8_t tx_buf[3] = {(uint8_t)reg, 0, 0};

    switch (reg) {
        case MCP9808_REG_CONFIG:
            sys_put_be16(data->reg_config, tx_buf+1);
            break;
        case MCP9808_REG_T_UPPER:
            sys_put_be16(data->reg_t_upper, tx_buf+1);
            break;
        case MCP9808_REG_T_LOWER:
            sys_put_be16(data->reg_t_lower, tx_buf+1);
            break;
        case MCP9808_REG_T_CRIT:
            sys_put_be16(data->reg_t_crit, tx_buf+1);
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
        case MCP9808_REG_RES:
            // resolution register is only 8 bits
            tx_buf[1] = (uint8_t)data->reg_res;
            break;
        default:
            return -EINVAL;
    }

    LOG_INF("BEFORE SET");
    mcp9808_reg_get(dev, MCP9808_REG_CONFIG);
    mcp9808_reg_get(dev, MCP9808_REG_T_UPPER);
    mcp9808_reg_get(dev, MCP9808_REG_T_LOWER);
    mcp9808_reg_get(dev, MCP9808_REG_RES);

    int err = i2c_write_dt(
        &config->bus,
        tx_buf,
        // write register pointer and data, 24 bits if not resolution register, else 16
        ((reg != MCP9808_REG_RES) ? sizeof(tx_buf) : (sizeof(tx_buf[0])*2))
    );
    if (err) {
        LOG_INF("MCP9808_REG_SET I2C ERR: %d\n", err);
        return err;
    }

    LOG_INF("AFTER SET");
    mcp9808_reg_get(dev, MCP9808_REG_CONFIG);
    mcp9808_reg_get(dev, MCP9808_REG_T_UPPER);
    mcp9808_reg_get(dev, MCP9808_REG_T_LOWER);
    mcp9808_reg_get(dev, MCP9808_REG_RES);

    return 0;
}

// convert a sensor_val to the format expected by mcp9808 for the upper,
// lower, and critical threshold values; returns raw value that can be
// written to one of the temp limit registers
static uint16_t mcp9808_convert_to_thresh(const struct sensor_value *val) {
    // check value of integer part, if out of range or parts disagree on sign, return error
    if (!IN_RANGE(val->val1, -255, 255) || (val->val1 * val->val2 < 0)) {
        // TODO: figure out better way to indicate error?
        return 0xE000;
    }

    // set sign bit if value is negative
    uint16_t ret = (val->val1 < 0) ? MCP9808_THRESH_BIT_SIGN : 0;

    // get uint8_t representation of integer part (handle sign later)
    uint8_t int_part = (uint8_t)abs(val->val1);

    // TODO: surely there's a better way than this...
    // get left two most digits of val->val2
    int32_t val2 = (uint8_t)abs(val->val2);
    // get the two left most digits from it's decimal form
    while (val2 >= 99) {
        val2 = val2/10;
    }
    
    // values can be represented in fractional part of thresh are .00, .25, .50, and .75
    uint8_t frac_part = DIV_ROUND_CLOSEST(CLAMP(val2, 0, 99), 25);
    // if rounding up to next number
    if (frac_part == 4) {
        // increment and check for overflow
        if (int_part++ == 0) {
            // TODO: is this the desired behavior?
            //       or throw an error if (255.87 < sensor_val < 256)?
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

// convert the contents of the ambient temperature register (raw_temp) to
// a sensor_value struct
static int mcp9808_convert_from_ambient_temp(
    uint16_t raw_temp,
    struct sensor_value *val
) {

    // convert lowest bits 3 through 0 to fractional part of sensor_value
    val->val2 = 0;
    for(int i = 0; i < 4; i++) {
        if (raw_temp & BIT(i)) {
            val->val2 += (1000000 >> (4-i));
        }
    }

    // convert bits 11 through 4 to integer part of sensor_value
    val->val1 = 0;
    for (int i = 0; i < 8; i++) {
        if (raw_temp & BIT(i+4)) {
            val->val1 += (1 << i);
        }
    }

    // if sign bit is set
    if (raw_temp & MCP9808_AMB_BIT_SIGN) {
        val->val1 *= -1;
        val->val2 *= -1;
    }

    return 0;
}

static int mcp9808_attr_set(
    const struct device *dev,
    enum sensor_channel chan,
    enum sensor_attribute attr,
    const struct sensor_value *val
) {

    if (chan != SENSOR_CHAN_AMBIENT_TEMP && chan != SENSOR_CHAN_ALL) {
        return -EINVAL;
    }

    // currently implemented:
    // SENSOR_ATTR_RESOLUTION       --> resolution
    // SENSOR_ATTR_HYSTERESIS       --> t hyst
    // SENSOR_ATTR_UPPER_THRESH     --> t upper
    // SENSOR_ATTR_LOWER_THRESH     --> t lower
    // SENSOR_ATTR_ALERT            --> alert
    // SENSOR_ATTR_CONFIGURATION    --> shutdown
    // not yet implemented:
    // shutdown mode
    // t crit (critical temp threshold)
    // crit lock (lock t crit bit)
    // win lock (lock t upper & lower bit)
    // alert selection (select for alerts from all thresholds or just t crit)
    // alert polarity (TODO: implement as dt property???)
    // alert mode (comparator vs interrupt)
    if (
        attr != SENSOR_ATTR_RESOLUTION &&   // resolution reg
        attr != SENSOR_ATTR_HYSTERESIS &&   // bits 9 & 10 in config reg
        attr != SENSOR_ATTR_UPPER_THRESH && // upper alert temp reg
        attr != SENSOR_ATTR_LOWER_THRESH && // lower alert temp reg
        attr != SENSOR_ATTR_ALERT &&        // bit 3 in config reg
        attr != SENSOR_ATTR_CONFIGURATION   // shutdown, bit 8 in config reg
    ) {
        return -EINVAL;
    }

    struct mcp9808_data *data = dev->data;
    int err = 0;
    uint16_t raw_value = 0xE000;

    switch (attr)
    {
    case SENSOR_ATTR_RESOLUTION:
        // four valid options for resolution: 0.5, 0.25, 0.125, 0.0625
        // val->val1 should be zero in all cases
        if (val->val1) {
            return -EINVAL;
        }
        if (val->val2 == 500000) {
            data->reg_res = MCP9808_RES_0_5;
        } else if (val->val2 == 250000) {
            data->reg_res = MCP9808_RES_0_25;
        } else if (val->val2 == 125000) {
            data->reg_res = MCP9808_RES_0_125;
        } else if (val->val2 == 62500) {
            data->reg_res = MCP9808_RES_0_0625;
        } else {
            return -EINVAL;
        }
        err = mcp9808_reg_set(dev, MCP9808_REG_RES);
        if (err) {
            return err;
        }
        break;
    case SENSOR_ATTR_HYSTERESIS:
        // four valid options for hysteresis value: 0.0, 1.5, 3.0, 6.0
        if (val->val1 == 0 && val->val2 == 0) {
            raw_value = MCP9808_HYST_0;
        } else if (val->val1 == 1 && val->val2 == 500000) {
            raw_value = MCP9808_HYST_1_5;
        } else if (val->val1 == 3 && val->val2 == 0) {
            raw_value = MCP9808_HYST_3_0;
        } else if (val->val1 == 6 && val->val2 == 0) {
            raw_value = MCP9808_HYST_6_0;
        } else {
            return -EINVAL;
        }
        data->reg_config &= ~(MCP9808_CONF_BIT_HYST_U | MCP9808_CONF_BIT_HYST_L);
        data->reg_config |= (enum mcp9808_hysteresis)raw_value;
        err = mcp9808_reg_set(dev, MCP9808_REG_CONFIG);
        if (err) {
            return err;
        }
        break;
    case SENSOR_ATTR_UPPER_THRESH:
        raw_value = mcp9808_convert_to_thresh(val);
        if (raw_value == 0xE000){
            return -EINVAL;
        } else {
            data->reg_t_upper = raw_value;
            err = mcp9808_reg_set(dev, MCP9808_REG_T_UPPER);
            if (err) {
                return err;
            }
        }
        break;
    case SENSOR_ATTR_LOWER_THRESH:
        raw_value = mcp9808_convert_to_thresh(val);
        if (raw_value == 0xE000){
            return -EINVAL;
        } else {
            data->reg_t_lower = raw_value;
            err = mcp9808_reg_set(dev, MCP9808_REG_T_LOWER);
            if (err) {
                return err;
            }
        }
        break;
    case SENSOR_ATTR_ALERT:
        if (val->val1 || val->val2) {
            data->reg_config |= MCP9808_CONF_BIT_ALERT_CTRL;
        } else {
            data->reg_config &= ~MCP9808_CONF_BIT_ALERT_CTRL;
        }
        err = mcp9808_reg_set(dev, MCP9808_REG_CONFIG);
        if (err) {
            return err;
        }
        break;
    case SENSOR_ATTR_CONFIGURATION:
        if (val->val1 || val->val2) {
            data->reg_config |= MCP9808_CONF_BIT_SHDN;
        } else {
            data->reg_config &= ~MCP9808_CONF_BIT_SHDN;
        }
        err = mcp9808_reg_set(dev, MCP9808_REG_CONFIG);
        if (err) {
            return err;
        }
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static int mcp9808_sample_fetch(
    const struct device *dev, 
    enum sensor_channel chan
) {
    // update dev->data->reg_t_amb with reg_get
    return mcp9808_reg_get(dev, MCP9808_REG_T_AMB);
}

static int mcp9808_channel_get(const struct device *dev,
    enum sensor_channel chan,
    struct sensor_value *val
) {
    struct mcp9808_data *data = dev->data;

    if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
        return -ENOTSUP;
    }

    // convert value in dev->data->reg_t_amb and store it in val
    return mcp9808_convert_from_ambient_temp(data->reg_t_amb, val);
    
}

static int mcp9808_init(const struct device *dev) {
    struct mcp9808_data *data = dev->data;

    for (int i = 1; i < 9; i++) {
        mcp9808_reg_get(dev, i);
    }

    // initialize to power up defaults described in datasheet
    data->reg_config = 0;
    data->reg_t_amb = 0;
    data->reg_t_crit = 0;
    data->reg_t_upper = 0;
    data->reg_t_lower = 0;
    data->reg_man_id = 0x54;
    data->reg_dev_id = 0x0400;
    data->reg_res = MCP9808_RES_0_0625;

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
