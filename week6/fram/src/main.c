#include <zephyr/kernel.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/sys/printk.h>
#include <endian.h>

//uint32_t swap_endianness_u32(uint32_t value) {
//  return ((value >> 24) & 0x000000FF) |
//         ((value >> 8) & 0x0000FF00) |
//         ((value << 8) & 0x00FF0000) |
//         ((value << 24) & 0xFF000000);
//}
//
//uint16_t swap_endianness_u16(uint16_t value) {
//        return ((value >> 8) & 0x00FF) |
//               ((value << 8) & 0xFF00);
//}


const struct device *const serLcd_dev = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c0), serlcd_72));

// i2c1_dev --> use generic i2c driver for MB85RC256V FRAM chip
/*

Write format:
I2C address word                S = start condition
┌─┬────┬──┬──┬──┬─┬───┬───      A2,A1,A0 = pin states of A2,1,0 on MB85RC256V
│S│1010│A2│A1│A0│0│ACK│...      0 = 0 for write, 1 for read
└─┴────┴──┴──┴──┴─┴───┴───      ACK = ackowledge from MB85RC256V
FRAM write address                      
───┬────────┬───┬────────┬───┬───       Addr MSB = MSB of 16-bit write address
...│Addr MSB│ACK│Addr LSB│ACK│...       Addr MSB = LSB of 16-bit write address
───┴────────┴───┴────────┴───┴───       
Data byte (Writing one byte)            
───┬─────────┬───┬─┐               P = stop condition
...│Sent Byte│ACK│P│               
───┴─────────┴───┴─┘               
Data bytes (Writing many bytes)    
───┬─────────┬───┬───────────────┬───┬─┐        More data bytes can be any amount
...│Sent Byte│ACK│More sent bytes│ACK│P│        and does not need to be split by ACKs
───┴─────────┴───┴───────────────┴───┴─┘        

Read format:
 I2C address word                                   
 ┌─┬────┬──┬──┬──┬─┬───┬───                         
 │S│1010│A2│A1│A0│0│ACK│...                         
 └─┴────┴──┴──┴──┴─┴───┴───                         
 FRAM read address                                  
 ───┬────────┬───┬────────┬───┬───                  
 ...│Addr MSB│ACK│Addr LSB│ACK│...                  
 ───┴────────┴───┴────────┴───┴───                  
 I2C address word (again with 1 for R/W bit)        
 ───┬─┬────┬──┬──┬──┬─┬───┬───                      
 ...│S│1010│A2│A1│A0│1│ACK│...                      
 ───┴─┴────┴──┴──┴──┴─┴───┴───                      
 Data byte (reading one byte)                       
 ───┬─────────┬────┬─┐            NACK = not acknowledge from controller
 ...│Read Byte│NACK│P│                              
 ───┴─────────┴────┴─┘                              
 Data bytes (reading many bytes)                    
 ───┬─────────┬─────┬─────────┬───┬─────────┬────┬─┐    ACK-C = acknowledge from controller
 ...│Read Byte│ACK-C│Read Byte│...│Read Byte│NACK│P│
 ───┴─────────┴─────┴─────────┴───┴─────────┴────┴─┘

Note that when writing or reading sequentially, there is no limit to how much you can 
write or read. Once the maximum address (15 bit, so 0x7FFF or 0b0111111111111111 or 0d32767) 
is reached, it will roll back to 0x0000.

*/

const struct device *i2c1_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));


// Address word is 1010<A2><A1><A0><R/W> where A0-2 are the state of the
// A0-2 pins on the physical device. Pull down resistors internal to
// the fram chip make A0-2 all low by default. R/W is 0 for write and
// is 1 for read.
uint16_t framAddrWrite = 0x50; // 0b01010000
uint16_t framAddrRead = 0x51; // 0b01010001


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

// print a 64 byte wide region of fram starting at startAddr
int printFram_64B(uint16_t startAddr) {

        uint8_t txBuf[] = {
                (uint8_t)(startAddr >> 8),
                (uint8_t)(startAddr%256)
        };

        uint8_t rxBuf[64] = {0xFF};

        // read 64 bytes from startAddr (txBuf[0:2])
        int status = i2c_write_read(
                i2c1_dev,
                0x50,
                txBuf,
                2*sizeof(uint8_t),
                rxBuf,
                64*sizeof(uint8_t)
        );

        if (status) {
                printk("Failed to read data to print. Error [%d]\n", status);
                return status;
        }

        // check if everything got set to zero
        printk("\nDATA READ:\n[ ADDR ] [ BIN DATA      ][HEX ]\n");
        for (uint8_t i = 0; i < 64; i++) {
                printk("[0x%04X] [", (startAddr + i));
                for (int j = 7; j >= 1; j--) {
                        printk("%d ", ((rxBuf[i] >> j)&1) );
                }
                
                printk("%d][0x%02X]\n", (rxBuf[i]&1), rxBuf[i]);
        }

        // return success
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


int main(void)
{

        if (!device_is_ready(serLcd_dev)) {
                printk("AUX NOT READY TO BE PASSED :(\n");
		return 0;
	}

        if (auxdisplay_cursor_set_enabled(serLcd_dev, true)) {
                printk("CURSOR NOT ENABLED :(\n");
	}

        if ( !device_is_ready(i2c1_dev)) {
                printk("GENERIC I2C NOT READY :(\n");
                return 0;
        }

        int err = eraseFram_64B(0x0000);
        printk("erase result: [%d]\n", err);

        err = eraseFram_64B(0x0040);
        printk("erase result: [%d]\n", err);

        err = eraseFram_64B(0x0080);
        printk("erase result: [%d]\n", err);

        struct uartData testy;
        testy.time = 69;
        testy.cps = 69;
        testy.cpm = 69;
        testy.usph = 69;
        testy.speed = 'F';

        uint8_t testy_2[13] = {0xFF};

        struct uartData testy_3;
        testy_3.time = 0;
        testy_3.cps = 0;
        testy_3.cpm = 0;
        testy_3.usph = 0;
        testy_3.speed = 0;

        writeFram(0x0000, testy);

        readFram(0x0000, &testy_3);

        printFram_64B(0x0000);

        printk("testy time [%d]\n", testy.time);
        printk("testy cps [%d]\n", testy.cps);

        printk("testy_3 time [%d]\n", testy_3.time);
        printk("testy_3 cps [%d]\n", testy_3.cps);
        printk("testy_3 cpm [%d]\n", testy_3.cpm);
        printk("testy_3 usph [%d]\n", testy_3.usph);
        printk("testy_3 speed [%c]\n", testy_3.speed);

        return 0;
}
