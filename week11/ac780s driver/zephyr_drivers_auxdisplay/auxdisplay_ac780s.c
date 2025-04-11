/* Copyright (c) 2025 Adam Arnett <adamarnett@proton.me>
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT ac780s

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>

#define AC780S_CUSTOM_CHAR_HEIGHT 8
#define AC780S_CUSTOM_CHAR_WIDTH 5
#define AC780S_CUSTOM_CHAR_MAX_COUNT 8

/*
 * bit to set in the display on/off command to indicate the display should be powered on
 */
#define AC780S_DISPLAY_ON_OFF_POWER_BIT BIT(2)

/*
 * bit to set in the display on/off command to indicate the cursor should be on
 */
#define AC780S_DISPLAY_ON_OFF_CURSOR_BIT BIT(1)

/*
 * bit to set in the display on/off command to indicate the cursor position should be blinking
 */
#define AC780S_DISPLAY_ON_OFF_BLINKING_BIT BIT(0)

/*
 * bit to set in the function set command to indicate data length (8-bit (1) vs 4-bit (0))
 */
#define AC780S_FUNCTION_SET_DATA_LENGTH_BIT BIT(4)

/*
 * bit to set in the function set command to indicate number of display lines (per row?)
 * TODO figure out exactly what this does 
 */
#define AC780S_FUNCTION_SET_DISPLAY_LINES_BIT BIT(3)

/*
 * bit to set in the function set command to indicate font type (5x10 dots (1) vs 5x8 dots (0))
 */
#define AC780S_FUNCTION_SET_FONT_TYPE_BIT BIT(2)

/*
 * bit to set in the entry mode set command to indicate incremet after write to display
 */
#define AC780S_ENTRY_MODE_SET_INCREMENT_BIT BIT(1)

/*
 * bit to set in the entry mode set command to indicate shifting of display after write to display
 */
#define AC780S_ENTRY_MODE_SET_SHIFT_DISPLAY_BIT BIT(0)

/*
 * bit to set in the cursor display shift command to indicate shifting of entire display
 */
#define AC780S_CURSOR_DISPLAY_SHIFT_DISPLAY_BIT BIT(3)

/*
 * bit to set in the cursor display shift command to indicate shifting to the right
 */
#define AC780S_CURSOR_DISPLAY_SHIFT_RIGHT_BIT BIT(2)

enum font_type {
    FONT_5X8_DOTS,
    FONT_5X10_DOTS
};

enum auxdisplay_ac780s_command {
    AC780S_CLEAR_DISPLAY = 0x01,
    AC780S_RETURN_HOME = 0x02,
    AC780S_ENTRY_MODE_SET = 0x04,
    AC780S_DISPLAY_ON_OFF = 0x08,
    AC780S_CURSOR_DISPLAY_SHIFT = 0x10,
    AC780S_FUNCTION_SET = 0x20,
    AC780S_CGRAM_ADDRESS_SET = 0x40,
    AC780S_DDGRAM_ADDRESS_SET = 0x80
};

// TODO figure out what in here can only be changed on init
// if can't be changed after display is on, move to 
// devicetree to set in init function
struct auxdisplay_ac780s_data {
	bool power;         
	bool cursor;        
	bool blinking;      
    bool increment;     
    enum font_type font;
    uint8_t lines;      
	uint16_t cursor_x;
	uint16_t cursor_y;
};

struct auxdisplay_ac780s_config {
	struct auxdisplay_capabilities capabilities;
	struct i2c_dt_spec bus;
	uint16_t command_delay_us; // TODO remove this
	uint16_t special_command_delay_us;
};

static int auxdisplay_ac780s_send_command(const struct device *dev,
   const enum auxdisplay_ac780s_command command) {

    const struct auxdisplay_ac780s_config *config = dev->config;

    // all commands must be preceded by a control byte (0x00) that
    // indicates a command will follow 
    uint8_t buf[2] = {0x00, command};

    int err = i2c_write_dt(
        &config->bus,
        buf,
        sizeof(buf)
    );

    // clear display and return home commands require a 10us wait
    // while all others require 100us
    if (command < 0x04) {
        k_usleep(10);
    } else {
        k_usleep(100);
    }

    return err;
}

static int auxdisplay_ac780s_send_display_state(const struct device *dev,
    const struct auxdisplay_ac780s_data *data) {

    uint8_t command = AC780S_DISPLAY_ON_OFF;

    if (data->power) {
        command |= AC780S_DISPLAY_ON_OFF_POWER_BIT;
    }
    if (data->cursor) {
        command |= AC780S_DISPLAY_ON_OFF_CURSOR_BIT;
    }
    if (data->blinking) {
        command |= AC780S_DISPLAY_ON_OFF_BLINKING_BIT;
    }

    int err = auxdisplay_ac780s_send_command(
        dev,
        command
    );
    
    return err;
}

static int auxdisplay_ac780s_display_on(const struct device *dev) {

    struct auxdisplay_ac780s_data *data = dev->data;

    data->power = true;

    int err = auxdisplay_ac780s_send_display_state(
        dev,
        data
    );

    return err;
}

static int auxdisplay_ac780s_display_off(const struct device *dev) {

    struct auxdisplay_ac780s_data *data = dev->data;

    data->power = false;

    int err = auxdisplay_ac780s_send_display_state(
        dev,
        data
    );

    return err;

}

static int auxdisplay_ac780s_cursor_set_enabled(const struct device *dev, bool enable) {

    struct auxdisplay_ac780s_data *data = dev->data;

    data->cursor = enable;

    int err = auxdisplay_ac780s_send_display_state(
        dev,
        data
    );

    return err;
}

static int auxdisplay_ac780s_position_blinking_set_enabled(const struct device *dev, bool enable) {
    
    struct auxdisplay_ac780s_data *data = dev->data;

    data->blinking = enable;

    int err = auxdisplay_ac780s_send_display_state(
        dev,
        data
    );

    return err;
}

static int auxdisplay_ac780s_cursor_position_set(const struct device *dev,
    enum auxdisplay_position type, int16_t x,
    int16_t y) {

        // get number of columns and rows had by the display to do the math
        // to correctly place the cursor
        const struct auxdisplay_ac780s_config *config = dev->config;
        const struct auxdisplay_capabilities capabilities = config->capabilities;
        struct auxdisplay_ac780s_data *data = dev->data;
	    const uint16_t cols = capabilities.columns;
	    const uint16_t rows = capabilities.rows;

        // to accomadate displays of various sizes rows in controller RAM are
        // always 20 bytes long, even if the display only as 8 columns.
        // they are also mapped "every other" so the number of characters to 
        // advance can't be calculated with something like (rows*20)+cols, so
        // instead we have a little lookup table
        uint8_t rowStartAddr[4] = {0x00, 0x28, 0x0E, 0x36};

        // for error checking
        int err = 0;

        // TODO finish the rest of the options
        // if moving from 0,0
        if (type == AUXDISPLAY_POSITION_ABSOLUTE) {
            // check if x & y are within display bounds
            if (
                (x < 0 || x >= cols) ||
                (y < 0 || y >= rows)
            ) {
                // return invalid arg error if they are not
                return -EINVAL;
            }

            // offset in display controller RAM
            uint8_t dest = rowStartAddr[y] + x;

            // use logical or to add destination address to address
            // set command
            uint8_t command = AC780S_DDGRAM_ADDRESS_SET;
            command |= dest;

            err = auxdisplay_ac780s_send_command(
                dev,
                command
            );

            if (err) {
                return err;
            }

            // can't shift multiple at once, have to send multiple shift commands
            for (int i =0; i < x; i++) {
                printk("iter [%d]\n", i);

                err = auxdisplay_ac780s_send_command(
                    dev,
                    (AC780S_CURSOR_DISPLAY_SHIFT | AC780S_CURSOR_DISPLAY_SHIFT_RIGHT_BIT)
                );

                if (err) {
                    return err;
                }
            }

        }

        if (!err) {
            data->cursor_x = x;
            data->cursor_y = y;
        }
    
        return err;

}

static int auxdisplay_ac780s_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y) {
    
    const struct auxdisplay_ac780s_data *data = dev->data;

    *x = data->cursor_x;
    *y = data->cursor_y;

    return 0;
}

static int auxdisplay_ac780s_capabilities_get(const struct device *dev,
    struct auxdisplay_capabilities *capabilities) {

    const struct auxdisplay_ac780s_config *config = dev->config;
    
    void* err = memcpy(capabilities, &config->capabilities, sizeof(*capabilities));

    if (err != capabilities) {
        return -EINVAL;
    }
    
    return 0;
}

static int auxdisplay_ac780s_clear(const struct device *dev) {

    int err = auxdisplay_ac780s_send_command(
        dev,
        AC780S_CLEAR_DISPLAY
    );

    return err;

}

static int auxdisplay_ac780s_custom_character_set(const struct device *dev,
    struct auxdisplay_character *character) {

    const struct auxdisplay_ac780s_config *config = dev->config;
    const uint8_t *character_data = character->data;
    uint8_t character_index = character->index;

    if (character_index >= AC780S_CUSTOM_CHAR_MAX_COUNT) {
        return -EINVAL;
    }

    // buffer for data to send to ac780s
    uint8_t new_char[AC780S_CUSTOM_CHAR_HEIGHT+1];
    memset(new_char,0,(AC780S_CUSTOM_CHAR_HEIGHT+1)*sizeof(uint8_t));
    // precede data with control byte to indicate that data will follow
    new_char[0] = 0x40;
    // put data from auxdisplay_character struct into buffer
    for (int i = 0; i < (AC780S_CUSTOM_CHAR_HEIGHT*AC780S_CUSTOM_CHAR_WIDTH); i++) {
        if (character_data[i] == 0xFF) {
            new_char[(i/5)+1] |= BIT(i%5);
        } else if (character_data[i] != 0x00) {
            return -EINVAL;
        }
    }

    // set CGRAM address
    int err = auxdisplay_ac780s_send_command(
        dev,
        AC780S_CGRAM_ADDRESS_SET | (character_index * 0x08)
    );
    if (err) {
        return err;
    }

    // write data for new char to CGRAM
    err = i2c_write_dt(
        &config->bus,
        new_char,
        sizeof(new_char)
    );

    // sleep to allow processing of write to RAM
    k_msleep(60);

    if (!err) {
        character->character_code = character_index;
    }
    
    return err;
}

static void auxdisplay_ac780s_advance_current_position(const struct device *dev) {

    const struct auxdisplay_ac780s_config *config = dev->config;
    struct auxdisplay_ac780s_data *data = dev->data;
    const struct auxdisplay_capabilities capabilities = config->capabilities;
	const uint16_t cols = capabilities.columns;
	const uint16_t rows = capabilities.rows;

    if ((data->cursor_x++) >= cols) {
        data->cursor_x = 0;
        if ((data->cursor_y++) >= rows) {
            data->cursor_y = 0;
        }
    }

}

static int auxdisplay_ac780s_write(const struct device *dev, const uint8_t *text, uint16_t len) {

    const struct auxdisplay_ac780s_config *config = dev->config;
    const struct auxdisplay_capabilities capabilities = config->capabilities;
    
    // precede data with control byte to indicate that data will follow (0x40)
    // 0x10 is a space - just a placeholder for now
    uint8_t buf[2] = {0x40, 0x10};

    int err = 0;
    int16_t x = 0 , y = 0;

    err = auxdisplay_ac780s_cursor_position_get(dev, &x, &y);
    if (err) {
        return err;
    }

    // if last command was to write to CGRAM, address counter in the 
    // ac780s will still be pointed there. Setting position here ensures
    // we're writing to the right place
    err = auxdisplay_ac780s_cursor_position_set(
        dev, 
        AUXDISPLAY_POSITION_ABSOLUTE,
        x,
        y
    );
    if (err) {
        return err;
    }

    // unfortunately have to write one uint8_t at a time, since a control byte must
    // precede the data to indicate it should be written to RAM.
    // technically the control byte only needs to be sent once, but I don't see a 
    // way to insert that into the i2c transmission before the entirety of the buffer
    for (uint16_t i = 0; i < len; i++) {
        // get next uint8_t
        buf[1] = text[i];

        printk("%X\n", buf[1]);

        // send transmission
        err = i2c_write_dt(
            &config->bus,
            buf,
            sizeof(buf)
        );
        if (err) {
           return err;
        }
        // wait for write data to RAM command to execute
        k_usleep(50);

        auxdisplay_ac780s_advance_current_position(dev);
        err = auxdisplay_ac780s_cursor_position_get(dev, &x, &y);
        if (err) {
            return err;
        }

        if (x >= (capabilities.columns)) {
            err = auxdisplay_ac780s_cursor_position_set(
                dev, 
                AUXDISPLAY_POSITION_ABSOLUTE,
                0,
                ((y+1)%capabilities.rows)
            );
            if (err) {
                return err;
            }
        }

    }

    return err;

}

static int auxdisplay_ac780s_init(const struct device *dev) {

    const struct auxdisplay_ac780s_config *config = dev->config;
    struct auxdisplay_ac780s_data *data = dev->data;

    data->cursor_x = 0;
    data->cursor_y = 0;

	if (!device_is_ready(config->bus.bus)) {
		return -ENODEV;
	}

    uint8_t command = AC780S_FUNCTION_SET;
    int err = auxdisplay_ac780s_send_command(
        dev,
        (command |= (AC780S_FUNCTION_SET_DATA_LENGTH_BIT | AC780S_FUNCTION_SET_DISPLAY_LINES_BIT))
    );
    if (err) {
        return err;
    }

    err = auxdisplay_ac780s_display_on(dev);
    if (err) {
        return err;
    }

    command = AC780S_ENTRY_MODE_SET;
    err = auxdisplay_ac780s_send_command(
        dev,
        (command |= (AC780S_ENTRY_MODE_SET_INCREMENT_BIT))
    );
    if (err) {
        return err;
    }

    err = auxdisplay_ac780s_clear(dev);

    return err;
}




static DEVICE_API(auxdisplay, auxdisplay_ac780s_auxdisplay_api) = {
	.display_on = auxdisplay_ac780s_display_on,
	.display_off = auxdisplay_ac780s_display_off,
	.cursor_set_enabled = auxdisplay_ac780s_cursor_set_enabled,
	.position_blinking_set_enabled = auxdisplay_ac780s_position_blinking_set_enabled,
	.cursor_position_set = auxdisplay_ac780s_cursor_position_set,
	.cursor_position_get = auxdisplay_ac780s_cursor_position_get,
	.capabilities_get = auxdisplay_ac780s_capabilities_get,
	.clear = auxdisplay_ac780s_clear,
	.custom_character_set = auxdisplay_ac780s_custom_character_set,
	.write = auxdisplay_ac780s_write,
};

// TODO --> using fonts that are 10 pixels(?) high will require modification here
// AND in the devicetree/ yaml stuff... may need to pick a font type at initialization?
#define AUXDISPLAY_AC780S_INST(inst)                                                               \
	static const struct auxdisplay_ac780s_config auxdisplay_ac780s_config_##inst = {           \
		.capabilities = {                                                                  \
				.columns = DT_INST_PROP(inst, columns),                            \
				.rows = DT_INST_PROP(inst, rows),                                  \
				.mode = 0,                                                         \
				.brightness.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,              \
				.brightness.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,              \
				.backlight.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,               \
				.backlight.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,               \
				.custom_characters = AC780S_CUSTOM_CHAR_MAX_COUNT,                 \
				.custom_character_width = AC780S_CUSTOM_CHAR_WIDTH,                \
				.custom_character_height = AC780S_CUSTOM_CHAR_HEIGHT,              \
			},                                                                         \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	static struct auxdisplay_ac780s_data auxdisplay_ac780s_data_##inst;                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &auxdisplay_ac780s_init, NULL, &auxdisplay_ac780s_data_##inst, \
			      &auxdisplay_ac780s_config_##inst, POST_KERNEL,                       \
			      CONFIG_AUXDISPLAY_INIT_PRIORITY, &auxdisplay_ac780s_auxdisplay_api);

DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_AC780S_INST)
