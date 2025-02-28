#include <zephyr/kernel.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <string.h>

// state enum
/*
┌────────────┐                       
│boot        │                       
└─────┬──────┘                       
      │                                                  
      ▼                              
┌────────────┐                       
│selectRW    │◄─────────────────────┐
└─┬──────────┘                      │
  │     ▲                           │
  │     │                           │
  │sw3  │sw1                        │
  │     │                           │
  ▼     │      sw3 &                │
┌───────┴────┐ read  ┌────────────┐ │
│selectAddr  ┼──────►│enterLen    │ │
└─┬──────────┘       └─┬───┬──────┘ │
  │     ▲   ▲          │   │        │
  │     │   └──────────┘   │sw3     │
  │sw3 &│sw1      sw1      │        │
  │write│                  │        │
  ▼     │                  ▼        │
┌───────┴────┐ sw3   ┌────────────┐ │
│enterData   ├──────►│io          ├─┘
└────────────┘       └────────────┘  
*/
enum sys_state {
        boot,           // booting up, buttons do nothing
        selectRW,       // selecting read or write option
        selectAddr,     // selecting address to get
        enterLen,       // entering len of data to get
        enterData,      // entering data to write
        io              // reading or writing from flash, buttons do nothing

};
// init state as boot
enum sys_state state = boot;

// read write enum
enum rw_state {
        read,
        write,
        notSelected
};
// init state as read
enum rw_state rw = notSelected;

// button pressed enum --> enable wait until user input
enum button_pressed {
        yes,
        no
};
// init state as no
enum button_pressed button = no;

// keep track of address offset to read/write
uint16_t ioAddr = 0;

// keep track of len of data to read
uint8_t readLen = 0;

// keep track of data to write
static uint8_t writeData = 0;

// flash
// define region to use (same as sample because I don't want to go putting
// random crap all over the flash)
#define SPI_FLASH_REGION_OFFSET 0xff000
// sector size of mx25r64
#define SPI_FLASH_SECTOR_SIZE 4096

#define SPI_FLASH_COMPAT nordic_qspi_nor

// get dt device for flash
const struct device *flash_dev = DEVICE_DT_GET_ONE(SPI_FLASH_COMPAT);

// init for flash
int initFlash() {
        if (!device_is_ready(flash_dev)) {
                printk("Flash device not ready\n");
                return 0;
        }

        return 1;
}

// read from flash
// limits area that can be read from, hides flash_dev
int readFlash(uint16_t localOffset, uint8_t len, uint8_t* dest) {
        // ensure compatible with flash hardware
        if (localOffset%4 != 0) {
                return -1;
        }
        // ensure compatible with flash hardware - again
        if (len%4 != 0) {
                return -1;
        }

        const void* destP = dest;
        const off_t offset = SPI_FLASH_REGION_OFFSET + localOffset;
        const size_t lenConst = sizeof(uint8_t)*len;

        return flash_read(
                flash_dev, 
                offset, 
                destP, 
                lenConst
        );
}

// write to flash
// limits area and amount that can be written to, hides flash_dev
int writeFlash(uint16_t localOffset, const uint8_t* data) {
        // ensure compatible with flash hardware
        if (localOffset%4 != 0) {
                return -1;
        }

        const void* dataP = data;
        const off_t offset = SPI_FLASH_REGION_OFFSET + localOffset;
        const size_t lenConst = sizeof(data);

        return flash_write(
                flash_dev,
                offset,
                dataP,
                lenConst
        );
}

// gpio
// define buttons
#define SW0_NODE DT_ALIAS(sw0)
#define SW1_NODE DT_ALIAS(sw1)
#define SW2_NODE DT_ALIAS(sw2)
#define SW3_NODE DT_ALIAS(sw3)

// get dt specs for buttons
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static const struct gpio_dt_spec sw1 = GPIO_DT_SPEC_GET_OR(SW1_NODE, gpios, {0});
static const struct gpio_dt_spec sw2 = GPIO_DT_SPEC_GET_OR(SW2_NODE, gpios, {0});
static const struct gpio_dt_spec sw3 = GPIO_DT_SPEC_GET_OR(SW3_NODE, gpios, {0});

// callback structs for buttons
static struct gpio_callback sw0_cb_data;
static struct gpio_callback sw1_cb_data;
static struct gpio_callback sw2_cb_data;
static struct gpio_callback sw3_cb_data;

// disable gpio interrupts
int disableGPIO() {
        if (
		gpio_pin_interrupt_configure_dt(&sw0,
			GPIO_INT_DISABLE) ||
		gpio_pin_interrupt_configure_dt(&sw1,
			GPIO_INT_DISABLE) ||
		gpio_pin_interrupt_configure_dt(&sw2,
			GPIO_INT_DISABLE) ||
		gpio_pin_interrupt_configure_dt(&sw3,
			GPIO_INT_DISABLE)
	) {
		return 0;
	}
        return 1;
}

// enable gpio interrupts
int enableGPIO() {
        if (
		gpio_pin_interrupt_configure_dt(&sw0,
			GPIO_INT_EDGE_FALLING) ||
		gpio_pin_interrupt_configure_dt(&sw1,
			GPIO_INT_EDGE_FALLING) ||
		gpio_pin_interrupt_configure_dt(&sw2,
			GPIO_INT_EDGE_FALLING) ||
		gpio_pin_interrupt_configure_dt(&sw3,
			GPIO_INT_EDGE_FALLING)
	) {
		return 0;
	}
        return 1;
}

// callback functions for buttons
void sw0_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
        //printk("BUTTON1/SW0\n");
	switch (state)
        {
                case boot:
                        break;
                case selectRW:
                        //printk("sw0 selectRW\n");
                        switch (rw)
                        {       
                                case read:
                                        //printk("sw0 selectRW read\n");
                                        rw = write;
                                        break;
                                case write:
                                        rw = read;
                                        //printk("sw0 selectRW write\n");
                                        break;
                                case notSelected:
                                        rw = write;
                                        //printk("sw0 selectRW notSelected\n");
                                        break;
                                default:
                                        break;
                        }
                        break;
                case selectAddr:
                        ioAddr+=4;
                        break;
                case enterData:
                        writeData++;
                        break;
                case enterLen:
                        readLen+=4;
                        break;
                case io:
                        break;
                default:
                        break;
        }
        button = yes;
}

void sw1_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	switch (state)
        {
                case boot:
                        break;
                case selectRW:
                        break;
                case selectAddr:
                        state = selectRW;
                        break;
                case enterData:
                        state = selectAddr;
                        break;
                case enterLen:
                        state = selectAddr;
                        break;
                case io:
                        break;
                default:
                        break;
        }
        button = yes;
}

void sw2_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	switch (state)
        {
                case boot:
                        break;
                case selectRW:
                        switch (rw)
                        {
                                case read:
                                        rw = write;
                                        break;
                                case write:
                                        rw = read;
                                        break;
                                case notSelected:
                                        rw = read;
                                        break;
                                default:
                                        break;
                        }
                        break;
                case selectAddr:
                        if (ioAddr != 0) {
                                ioAddr-=4;
                        }
                        break;
                case enterData:
                        writeData--;
                        break;
                case enterLen:
                        readLen-=4;
                        break;
                case io:
                        break;
                default:
                        break;
        }
        button = yes;
}

void sw3_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	switch (state)
        {
                case boot:
                        break;
                case selectRW:
                        state = selectAddr;
                        break;
                case selectAddr:
                        switch (rw)
                        {
                                case read:
                                        state = enterLen;
                                        break;
                                case write:
                                        state = enterData;
                                        break;
                                default:
                                        break;
                        }
                        break;
                case enterData:
                        state = io;
                        break;
                case enterLen:
                        state = io;
                        break;
                case io:
                        break;
                default:
                        break;
        }
        button = yes;
}

// init for gpio
int initGPIO() {
        if (
		!gpio_is_ready_dt(&sw0)  ||
		!gpio_is_ready_dt(&sw1)  ||
		!gpio_is_ready_dt(&sw2)  ||
		!gpio_is_ready_dt(&sw3)
	        ) {
                printk("GPIO not ready\n");
		return 0;
	}

        if (
		gpio_pin_configure_dt(&sw0, GPIO_INPUT) 		 ||
		gpio_pin_configure_dt(&sw1, GPIO_INPUT) 		 ||
		gpio_pin_configure_dt(&sw2, GPIO_INPUT) 		 ||
		gpio_pin_configure_dt(&sw3, GPIO_INPUT)
		) {
                printk("GPIO not configured\n");
		return 0;
	}

        enableGPIO();

        gpio_init_callback(&sw0_cb_data, sw0_pressed, BIT(sw0.pin));
	gpio_init_callback(&sw1_cb_data, sw1_pressed, BIT(sw1.pin));
	gpio_init_callback(&sw2_cb_data, sw2_pressed, BIT(sw2.pin));
	gpio_init_callback(&sw3_cb_data, sw3_pressed, BIT(sw3.pin));
        gpio_add_callback(sw0.port, &sw0_cb_data);
	gpio_add_callback(sw1.port, &sw1_cb_data);
	gpio_add_callback(sw2.port, &sw2_cb_data);
	gpio_add_callback(sw3.port, &sw3_cb_data);

        return 1;
}


int main(void)
{

        
        uint8_t writeDataBuf[4];

        // keep track of read data
        uint8_t readBuf[256];

        memset(writeDataBuf, 0, sizeof(uint8_t) * 4);
        memset(readBuf, 0, sizeof(uint8_t) * 256);

        // setup flash and gpio
        if (!(initFlash() && initGPIO())) {
                printk("Encountered an error during initialization!\n");
                return 0;
        }

        const uint8_t test[] = {0x32, 0x32, 0x32, 0x32};
        off_t edit = SPI_FLASH_REGION_OFFSET + 4;
        int err = flash_write(
                flash_dev, 
                edit, 
                test, 
                sizeof(test)
        );
        printk("err: [%d]\n", err);
        uint8_t testBuf[] = {0, 0, 0, 0};
        err = readFlash(0, 4, testBuf);
        printk("err: [%d]\n", err);
        for (int i = 0; i < 4; i++) {
                printk("testBuf[%d] = {%d}\n", i, testBuf[i]);
        }

        while (1) {
                switch (state)
                {
                        case boot:
                                // booting is done, so go to select read/write
                                state = selectRW;
                                // no button to press, so set it as such anyways
                                button = yes;
                                break;
                        case selectRW:
                                switch (rw)
                                {
                                        case read:
                                                printk("Select read or write: read\n");
                                                break;
                                        case write:
                                                printk("Select read or write: write\n");
                                                break;
                                        case notSelected:
                                                printk("Select read or write: Use button 1 & 3 to select\n");
                                        default:
                                                break;
                                }
                                button = no;
                                break;
                        case selectAddr:
                                switch (rw)
                                {
                                        case read:
                                                printk("Select an address to read: +0x%d\n", ioAddr);
                                                break;
                                        case write:
                                                printk("Select an address to write: +0x%d\n", ioAddr);
                                                break;
                                        case notSelected:
                                                printk("How did you get here?\n");
                                                state = selectRW;
                                        default:
                                                break;
                                }
                                button = no;
                                break;
                        case enterData:
                                printk("Select some data to enter: %d\n", writeData);
                                button = no;
                                break;
                        case enterLen:
                                printk("Select an amount of data to read: %d\n", readLen);
                                button = no;
                                break;
                        case io:
                                switch (rw)
                                {
                                        case read:
                                                memset(readBuf, 0, sizeof(uint8_t)*256);
                                                if (readFlash(ioAddr, readLen, readBuf)) {
                                                        printk("Data read unsuccessfully.\n");
                                                } else {
                                                        printk("Data read successfully.\n");
                                                        printk("Data read: [");
                                                        for (int i = 0; i < readLen; i++) {
                                                                printk("%d", readBuf[i]);
                                                        }
                                                        printk("]\n");
                                                }
                                                break;
                                        case write:
                                                memset(writeDataBuf, writeData, sizeof(uint8_t)*4);
                                                printk("data buf [");
                                                for (int i = 0; i < 4; i++) {
                                                        printk("%d", writeDataBuf[i]);
                                                }
                                                printk("]\n");
                                                if (writeFlash(ioAddr, writeDataBuf)) {
                                                        printk("Data written unsuccessfully.\n");
                                                } else {
                                                        printk("Data written successfully.\n");
                                                }
                                                break;
                                        case notSelected:
                                                printk("How did you get here?\n");
                                                state = selectRW;
                                        default:
                                                break;
                                }
                                state = selectRW;
                                button = yes;
                                break;
                        default:
                                state = selectRW;
                                button = no;
                                break;
                }
                
                //switch (state)
                //{
                //        case boot:
                //                //printk("boot\n");
                //                break;
                //        case selectRW:
                //                //printk("selectRW\n");
                //                break;
                //        case selectAddr:
                //                //printk("selecAddr\n");
                //                switch (rw)
                //                {
                //                        case read:
                //                                //printk("read\n");
                //                                break;
                //                        case write:
                //                                //printk("write\n");
                //                                break;
                //                        default:
                //                                break;
                //                }
                //                break;
                //        case enterData:
                //                //printk("enterData\n");
                //                break;
                //        case enterLen:
                //                //printk("io\n");
                //                break;
                //        case io:
                //                break;
                //        default:
                //                break;
                //}

                // allow interrupts from GPIO buttons to allow for update
                if (!enableGPIO()) {
                        printk("Encountered an error while enabling GPIO... Exiting.\n");
                        return 0;
                }
                // poll for update
                while (button == no) {
                        k_msleep(1000);
                }
                // disallow interrupts from GPIO buttons to make the above switch 
                // statement atomic in terms of button presses (like they can't 
                // interrupt it)
                if (!disableGPIO()) {
                        printk("Encountered an error while disabling GPIO... Exiting.\n");
                        return 0;
                }
        }

        // should never get here
        return 0;
}
