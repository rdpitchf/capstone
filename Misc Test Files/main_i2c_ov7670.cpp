
#include "mbed.h"

#define OV7670_WRITE (0x42)
#define OV7670_READ  (0x43)
#define OV7670_WRITEWAIT (20)
#define OV7670_NOACK (0)
#define OV7670_REGMAX (201)
#define OV7670_I2CFREQ (100000)
#define REG_PID         0x0a    /* Product ID MSB */

static BufferedSerial serial_port(USBTX, USBRX, 921600);
static DigitalOut led(LED1);

static DigitalOut sccb_e(PC_6);

I2C i2c(PB_9, PB_8);

// Define functions.
int ReadReg(int addr);

// MUST INCLUDE mbed-os/storage/filesystem/ WHEN COMPILING
FileHandle *mbed::mbed_override_console(int fd){
    return &serial_port;
}

int main(void){

    printf("Mbed OS version %d.%d.%d\r\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
    sccb_e = 1;
    int counter = 0;
    while(1){
      counter++;
      printf("Counter: %d\r\n", counter);
      led = !led;
      ThisThread::sleep_for(1000);
      int data = ReadReg(REG_PID);
      printf("Data: %d\r\n", data);
    }
}

// read from camera
int ReadReg(int addr){
    int data;
    sccb_e = 0;
    i2c.start();
    i2c.write(OV7670_WRITE);
    wait_us(OV7670_WRITEWAIT);
    i2c.write(addr);
    i2c.stop();
    sccb_e = 1;
    wait_us(OV7670_WRITEWAIT);

    sccb_e = 0;
    i2c.start();
    i2c.write(OV7670_READ);
    wait_us(OV7670_WRITEWAIT);
    data = i2c.read(OV7670_NOACK);
    i2c.stop();
    sccb_e = 1;
    return data;
}
