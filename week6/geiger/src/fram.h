#include <zephyr/kernel.h>

#include <zephyr/drivers/i2c.h>


#ifndef FRAM_H
#define FRAM_H

static const struct device *i2c1_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

// Address word is 1010<A2><A1><A0><R/W> where A0-2 are the state of the
// A0-2 pins on the physical device. Pull down resistors internal to
// the fram chip make A0-2 all low by default. R/W is 0 for write and
// is 1 for read.
//uint16_t framAddrWrite = 0x50; // 0b01010000
//uint16_t framAddrRead = 0x51; // 0b01010001

// struct for storing data from uart
struct __attribute__((packed)) uartData {
    uint32_t time;  // time (since startup) measurement was saved
    uint16_t cps;   // counts per second
    uint16_t cpm;   // counts per minute
    uint32_t usph;  // micro sieverts per hour
    uint8_t speed;  // counter report speed, S, F, or I
};

// struct for data transmission to fram
struct __attribute__((packed)) framTx {
    uint8_t addrMsb;        // most significant byte of write to address, msb of addrMsb 
                            // should be zero since the fram chip has a 15-bit address space
    uint8_t addrLsb;        // least significant byte of write to address
    
    struct uartData data;   // data to be written
};

// write a uartData struct to fram starting at address addr
int writeFram(uint16_t addr, struct uartData data);

// read a uartData struct from fram at address startAddr to the
// struct pointed to by dataStruct
// does endianess conversion
int readFram(uint16_t startAddr, struct uartData* dataStruct);

// erase a 64B region of fram starting at the address startAddr
int eraseFram_64B(uint16_t startAddr);

// init fram, returns non-zero on failure
int fram_init();



#endif