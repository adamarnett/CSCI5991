#include "fram.h"


// write a uartData struct to fram starting at address addr
int writeFram(uint16_t addr, struct uartData data) {

    struct framTx writeStruct;
    writeStruct.addrMsb = (uint8_t)(addr >> 8);
    writeStruct.addrLsb = (uint8_t)(addr%256);
    writeStruct.data = data;

    return i2c_write(
            i2c1_dev,
            (uint8_t*) &writeStruct,
            sizeof(writeStruct),
            0x50
    );
    
}

// read a uartData struct from fram at address startAddr to the
// struct pointed to by dataStruct
// does endianess conversion
int readFram(uint16_t startAddr, struct uartData* dataStruct) {
    uint8_t rxBuf[sizeof(struct uartData)] = {0xFF};

    uint8_t txBuf[] = {
            (uint8_t)(startAddr >> 8),
            (uint8_t)(startAddr%256)
    };

    int err = i2c_write_read(
            i2c1_dev, 
            0x50,
            txBuf, 
            sizeof(txBuf), 
            rxBuf,
            sizeof(rxBuf)
    );

    if (err) {
            return err;
    }

    dataStruct->time |= (((uint32_t)rxBuf[3]) << 24) | 
            (((uint32_t)rxBuf[2]) << 16) |
            (((uint32_t)rxBuf[1]) << 8) |
            ((uint32_t)rxBuf[0]);
    dataStruct->cps |= (((uint16_t)rxBuf[5]) << 8) |
            ((uint16_t)rxBuf[4]);
    dataStruct->cpm |= (((uint16_t)rxBuf[7]) << 8) |
            ((uint16_t)rxBuf[6]);
    dataStruct->usph |= (((uint32_t)rxBuf[11]) << 24) | 
            (((uint32_t)rxBuf[10]) << 16) |
            (((uint32_t)rxBuf[9]) << 8) |
            ((uint32_t)rxBuf[8]);
    dataStruct->speed = rxBuf[12];

    return 0;
}

// erase a 64 byte wide region of fram starting at startAddr
int eraseFram_64B(uint16_t startAddr) {
        
    // buffer w/ room for address and 64 bytes of zeros
    uint8_t txBuf[66] = {0x00};
    txBuf[0] = (uint8_t)(startAddr >> 8);
    txBuf[1] = (uint8_t)(startAddr%256);

    // buffer full of not zeros to read into
    uint8_t rxBuf[64] = {0xFF};

    // int to track return value of functions
    int status = 0;
    
    // write buffer full of zeros
    status = i2c_write(
            i2c1_dev,
            txBuf,
            sizeof(txBuf),
            0x50
    );

    if (status) {
            printk("Failed to write while attempting to erase data. Error [%d]\n", status);
            return status;
    }

    // read 64 bytes from startAddr (txBuf[0:2])
    status = i2c_write_read(
            i2c1_dev,
            0x50,
            txBuf,
            2*sizeof(uint8_t),
            rxBuf,
            64*sizeof(uint8_t)
    );

    if (status) {
            printk("Failed to validate erasure of data. Error [%d]\n", status);
            return status;
    }

    // check if everything got set to zero
    for (uint8_t i = 0; i < 64; i++) {
            // if rxBuf has anything but 0
            if (rxBuf[i]) {
                    // erase failed...
                    return 1;
            }
    }

    // return success
    return 0;

}

// init fram, returns non-zero on failure
int fram_init() {
    if ( !device_is_ready(i2c1_dev)) {
        printk("GENERIC I2C NOT READY :(\n");
        return 1;
    }
    return 0;
}
