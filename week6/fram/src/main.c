#include <zephyr/kernel.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/sys/printk.h>


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

int writeFramByte(uint8_t addrMsb, uint8_t addrLsb, uint8_t* data) {

        uint8_t writeBuf[] = {addrMsb, addrLsb, 0x45};

        return i2c_write(i2c1_dev, writeBuf, sizeof(writeBuf), 0x120);
        
}

int readFram(uint8_t addrMsb, uint8_t addrLsb, uint8_t* data, uint8_t readLen) {

        uint8_t writeBuf[] = {addrMsb, addrLsb};

        return i2c_write_read(
                i2c1_dev, 
                framAddrWrite,
                writeBuf, 
                sizeof(writeBuf), 
                data,
                readLen
        );
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

        uint8_t data0[] = {0x45};
        uint8_t data1[] = {0xFF};
        uint8_t txBuf[] = {0x00,0x00,0x45};

        printk("Before read/write data0: [%d], data1: [%d]\n", data0[0], data1[0]);

        int status = 200;

        //status = writeFramByte(0x00,0x00,data0);

        status = i2c_write(
                i2c1_dev,
                txBuf,
                sizeof(txBuf),
                0b01010000
        );

        printk("Status after write: [%d]\n", status);

        k_msleep(10);

        //status = readFram(
        //        0x00,
        //        0x00,
        //        data1,
        //        sizeof(uint8_t)
        //);

        status = i2c_write_read(
                i2c1_dev,
                0b01010000, //0x50,
                txBuf,
                2,
                data1,
                1
        );

        printk("Status after read: [%d]\n", status);

        printk("After read/write data0: [%d], data1: [%d]\n", data0[0], data1[0]);


        return 0;
}
