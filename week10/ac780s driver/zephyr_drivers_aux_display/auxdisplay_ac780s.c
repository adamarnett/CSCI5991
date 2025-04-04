/* Copyright (c) 2025 Adam Arnett <adamarnett@proton.me>
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT ac780s

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>


#define AC780S_FUNC_SET 0x38        // 0b 0011 1000
#define AC780S_FUNC_SET_DELAY 100   // 100 microseconds
//#define AC780S_DISP_ON 0x0C         // 0b 0000 1100
#define AC780S_DISP_ON_DELAY 100    // 100 microseconds
#define AC780S_ENTRY_MODE 0x06      // 0b 0000 0111
#define AC780S_ENTRY_MODE_DELAY 100 // 100 microseconds
#define AC780S_RET_HOME 0x02        // 0b 0000 0010
#define AC780S_CLEAR_DISP 0x01      // 0b 0000 0001

#define AC780S_CTL_WRITE_INST 0x00  // 0b 1100 0000
#define AC780S_CTL_WRITE_DISP 0x40  // 0b 1100 0000

#define AC780S_CUSTOM_CHAR_HEIGHT 8
#define AC780S_CUSTOM_CHAR_WIDTH 5
#define AC780S_CUSTOM_CHAR_MAX_COUNT 0

/*
 * bit to set in the display on/off command to indicate the display should be powered on
 */
#define AC780S_DISPLAY_ON_OFF_POWER_BIT BIT(2)

/*
 * bit to set in the display on/off command to indicate the cursor should be on
 */
#define AC780S_DISPLAY_ON_OFF_CURSOR_BIT BIT(1)

/*
 * bit to set in the display on/off command to indicate the display should be blinking
 * TODO (above) display blinking or cursor blinking?
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

enum font_type {
    FONT_5X8_DOTS,
    FONT_5X11_DOTS
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

//enum auxdisplay_ac780s_special_command {
//    AC780S_ENTRY_MODE_SET = 0x04,
//    AC780S_DISPLAY_ON_OFF = 0x08,
//    AC780S_CURSOR_DISPLAY_SHIFT = 0x10,
//    AC780S_FUNCTION_SET = 0x20,
//    AC780S_CGRAM_ADDRESS_SET = 0x40,
//    AC780S_DDGRAM_ADDRESS_SET = 0x80
//};

// TODO figure out what in here can only be changed on init
// if can't be changed after display is on, move to 
// devicetree to set in init function
struct auxdisplay_ac780s_data {
	bool power;         // disp on/off
	bool cursor;        // disp on/off
	bool blinking;      // disp on/off
    bool increment;     // entry mode
    enum font_type font;// func set
    uint8_t lines;      // func set
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

    // AC780S_CTL_WRITE_INST = 0x00
    // TODO clarify this
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

//static int
//auxdisplay_ac780s_send_special_command(const struct device *dev,
//				       const enum auxdisplay_ac780s_special_command command) {
//
//}


static int auxdisplay_ac780s_send_display_state(const struct device *dev,
    const struct auxdisplay_ac780s_data *data) {

}

static int auxdisplay_ac780s_display_on(const struct device *dev) {

    // TODO --> lots of very similar code between this, display off, cursor
    // enabled, blinking enabled functions... perhaps make another and just
    // have each of these set the thing in dev->data and another common 
    // function react by sending the actual display_on_off command? 
    // yes this is the way but I want the more striaghtforward way to work first...

    struct auxdisplay_ac780s_data *data = dev->data;

    data->power = true;

    // set the power bit to on
    uint8_t command = (AC780S_DISPLAY_ON_OFF | AC780S_DISPLAY_ON_OFF_POWER_BIT);

    // if cursor or blinking enabled, set the respective bits to 1
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

static int auxdisplay_ac780s_display_off(const struct device *dev) {

    struct auxdisplay_ac780s_data *data = dev->data;

    data->power = false;

    // do not set power bit to 1
    uint8_t command = AC780S_DISPLAY_ON_OFF;

    // if cursor or blinking enabled, set the respective bits to 1
    // TODO --> does this matter? Why would it if the display is getting shut off?
    // is there a situation in which it could matter?
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

static int auxdisplay_ac780s_cursor_set_enabled(const struct device *dev, bool enable) {

    struct auxdisplay_ac780s_data *data = dev->data;

    data->cursor = enable;

    uint8_t command = AC780S_DISPLAY_ON_OFF;

    // if cursor or blinking enabled, set the respective bits to 1
    if (data->power) {
        command |= AC780S_DISPLAY_ON_OFF_POWER_BIT;
    }
    if (enable) {
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

static int auxdisplay_ac780s_position_blinking_set_enabled(const struct device *dev, bool enable) {
    
    struct auxdisplay_ac780s_data *data = dev->data;

    data->blinking = enable;

    uint8_t command = AC780S_DISPLAY_ON_OFF;

    // if cursor or blinking enabled, set the respective bits to 1
    if (data->power) {
        command |= AC780S_DISPLAY_ON_OFF_POWER_BIT;
    }
    if (data->cursor) {
        command |= AC780S_DISPLAY_ON_OFF_CURSOR_BIT;
    }
    if (enable) {
        command |= AC780S_DISPLAY_ON_OFF_BLINKING_BIT;
    }

    int err = auxdisplay_ac780s_send_command(
        dev,
        command
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
	    const uint16_t cols = capabilities.columns;
	    const uint16_t rows = capabilities.rows;

        // to accomadate displays of various sizes rows in controller RAM are
        // always 20 bytes long, even if the display only as 8 columns.
        // they are also mapped "every other" so the number of characters to 
        // advance can't be calculated with something like (rows*20)+cols, so
        // instead we have a little lookup table
        uint8_t rowStartAddr[4] = {0x00, 0x28, 0x14, 0x42};

        // for error checking
        int err = 0;

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
            uint8_t command = (AC780S_DDGRAM_ADDRESS_SET | dest);

            err = auxdisplay_ac780s_send_command(
                dev,
                command
            );

        }

        //if (!err) {
        //    // TODO --> update x & y position in data?
        //}

        return err;

}

static int auxdisplay_ac780s_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y) {
    
    const struct auxdisplay_ac780s_data *data = dev->data;

    // TODO --> does dev->data->cursor_x work?
    *x = data->cursor_x;
    *y = data->cursor_y;

    return 0;
}

static int auxdisplay_ac780s_capabilities_get(const struct device *dev,
    struct auxdisplay_capabilities *capabilities) {

    const struct auxdisplay_ac780s_config *config = dev->config;
    
    void* err = memcpy(capabilities, &config->capabilities, sizeof(capabilities));

    if (err != capabilities) {
        // TODO --> is EFAULT (bad address) ok here?
        // or is EINVAL (invalid argument) ok?
        return -EINVAL;
    }
    
    return 0;
}

static int auxdisplay_ac780s_clear(const struct device *dev) {

    //const struct auxdisplay_ac780s_config *config = dev->config;

    int err = auxdisplay_ac780s_send_command(
        dev,
        AC780S_CLEAR_DISPLAY
    );

    return err;

}

static int auxdisplay_ac780s_custom_character_set(const struct device *dev,
    struct auxdisplay_character *character) {

}

static void auxdisplay_ac780s_advance_current_position(const struct device *dev) {

}

static int auxdisplay_ac780s_write(const struct device *dev, const uint8_t *text, uint16_t len) {

    const struct auxdisplay_ac780s_config *config = dev->config;

    //AC780S_CTL_WRITE_DISP or something = 0x40
    uint8_t buf[2] = {0x40, 0x00};

    int err = 0;

    // unfortunately have to write one uint8_t at a time, since a control byte must
    // precede the data to indicate it should be written to RAM.
    // technically the control byte only needs to be sent once, but I don't see a 
    // way to insert that into the i2c transmission before the entirety of the buffer
    for (uint16_t i = 0; i < len; i++) {
        // get next uint8_t
        buf[1] = text[i];

        // send transmission
        err = i2c_write_dt(
            &config->bus,
            buf,
            sizeof(buf)
        );

        if (err) {
           return err;
        }

        // TODO --> increment cursor x and/or y?

        // wait for write data to RAM command to execute
        k_usleep(100);
    }


    /*uint8_t cmds[] = {
        AC780S_CTL_WRITE_DISP,
        0x59, // Y
        0x41, // A
        0x59, // Y
        0x21, // !
        //0x10, // 
        //0x3A, // :
        //0x29, // )
        //0x10, //  
    };

    uint8_t cmdsII[] = {
        AC780S_CTL_WRITE_INST,
        //(AC780S_DDRAM_ADDRESS_SET | 0x28)
        (0x80 | 0x28)
    };
    
    uint8_t cmdsIII[] = {
        AC780S_CTL_WRITE_DISP,
        0x4F, // 0
        0x5F, // _
        0x6F, // o
        0x10, //  
        0x4F, // 0
        0x5F, // _
        0x6F, // o
        0x10, //  
        0x4F, // 0
        0x5F, // _
        0x6F, // o
        0x10, //  
        0x4F, // 0
        0x5F, // _
        0x6F, // o
        0x10, //  
        0x21, // !
        0x21, // !
        0x21, // !
        0x21, // !
        0x21, // !
    };

    i2c_write_dt(
        &config->bus,
        cmds,
        sizeof(cmds)
    );

    k_usleep(100);

    //uint8_t writeMe = 0x00;
//
    //while (1) {
//
    //    uint8_t cmdsIV[] = {
    //        AC780S_CTL_WRITE_DISP,
    //        writeMe
    //    };
//
    //    i2c_write_dt(
    //        &config->bus,
    //        cmdsIV,
    //        sizeof(cmdsIV)
    //    );
//
    //    printk("Writing [%d]", writeMe);
//
    //    writeMe++;
//
    //    k_msleep(500);
    //}

    i2c_write_dt(
        &config->bus,
        cmdsII,
        sizeof(cmdsII)
    );

    k_usleep(100);

    i2c_write_dt(
        &config->bus,
        cmdsIII,
        sizeof(cmdsIII)
    );*/

    return err;

}

static int auxdisplay_ac780s_init(const struct device *dev) {

    const struct auxdisplay_ac780s_config *config = dev->config;


    // TODO pull probably everything out of here lol
    // or maybe not? perhaps the initialization procedure in the datasheet
    // needs to be adhered to 100% of the time and is not an example?


	if (!device_is_ready(config->bus.bus)) {
		return -ENODEV;
	}

    //uint8_t cmdf[] = {
    //        AC780S_CTL_WRITE_INST,
    //        AC780S_FUNC_SET
    //};
    //i2c_write_dt(
    //        &config->bus,
    //        cmdf,
    //        sizeof(cmdf)
    //);
    //k_msleep(150);
    uint8_t command = AC780S_FUNCTION_SET;
    int err = auxdisplay_ac780s_send_command(
        dev,
        (command |= (AC780S_FUNCTION_SET_DATA_LENGTH_BIT | AC780S_FUNCTION_SET_DISPLAY_LINES_BIT))
    );

    if (err) {
        return err;
    }

    //cmdf[1] = AC780S_DISP_ON;
    command = AC780S_DISPLAY_ON_OFF;
    err = auxdisplay_ac780s_send_command(
        dev,
        (command |= (AC780S_DISPLAY_ON_OFF_POWER_BIT))
    );

    if (err) {
        return err;
    }
    //i2c_write_dt(
    //    &config->bus,
    //    cmdf,
    //    sizeof(cmdf)
    //);
    //k_msleep(150);

    //cmdf[1] = AC780S_ENTRY_MODE;
    //i2c_write_dt(
    //    &config->bus,
    //    cmdf,
    //    sizeof(cmdf)
    //);
    //k_msleep(150);

    command = AC780S_ENTRY_MODE_SET;
    err = auxdisplay_ac780s_send_command(
        dev,
        (command |= (AC780S_ENTRY_MODE_SET_INCREMENT_BIT))
    );

    if (err) {
        return err;
    }

    //cmdf[1] = AC780S_CLEAR_DISP;
    //i2c_write_dt(
    //    &config->bus,
    //    cmdf,
    //    sizeof(cmdf)
    //);
    //k_msleep(150);

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


/*
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣤⣤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣾⣿⣿⣿⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⣿⣿⣿⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⢀⣾⣿⣿⣿⣿⣷⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⣿⣿⣿⣿⣿⡀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⣾⣿⣿⣿⣿⣿⣿⣧⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⣿⣿⣿⣿⣿⣿⣧⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⣼⣿⣿⣿⣿⣿⣿⣿⣿⣇⠀⠀⠀⠀⠀⠀⠀⢀⣿⣿⣿⣿⣿⣿⣿⣿⡆⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⢰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡄⠀⠀⠀⠀⠀⠀⣼⣿⣿⣿⣿⣿⣿⣿⣿⣷⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⢀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣧⠀⠀⠀⠀⠀⢰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡄⠀⠀⠀⢀⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣇⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⢀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠈⣿⣿⣿⣿⣿⣿⠟⠉⠀⠀⠀⠙⢿⣿⣿⣿⣿⣿⣿⣿⡿⠋⠀⠀⠙⢻⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⠃⠀⠀⠀⠀⣠⣄⠀⢻⣿⣿⣿⣿⣿⡿⠀⣠⣄⠀⠀⠀⢻⣿⣿⣏⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⣾⣿⣿⣿⣿⠀⠀⠀⠀⠰⣿⣿⠀⢸⣿⣿⣿⣿⣿⡇⠀⣿⣿⡇⠀⠀⢸⣿⣿⣿⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣄⠀⠀⠀⠀⠙⠃⠀⣼⣿⣿⣿⣿⣿⣇⠀⠙⠛⠁⠀⠀⣼⣿⣿⣿⡇⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣷⣤⣄⣀⣠⣤⣾⣿⣿⣿⣿⣽⣿⣿⣦⣄⣀⣀⣤⣾⣿⣿⣿⣿⠃⠀⠀⢀⣀⠀
⠰⡶⠶⠶⠶⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⠛⠉⠉⠙⠛⠋
⠀⠀⢀⣀⣠⣤⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠷⠶⠶⠶⢤⣤⣀
⠀⠛⠋⠉⠁⠀⣀⣴⡿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣯⣤⣀⡀⠀⠀⠀⠀⠘
⠀⠀⢀⣤⡶⠟⠉⠁⠀⠀⠉⠛⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠟⠉⠀⠀⠀⠉⠙⠳⠶⣄⡀⠀
⠀⠀⠙⠁⠀⠀⠀⠀⠀⠀⠀⠀⢰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠁⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⣸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⣰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⢰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⢀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⣸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀



*/