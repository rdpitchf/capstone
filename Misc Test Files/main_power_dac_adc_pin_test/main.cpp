
#include "mbed.h"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
static DigitalOut led1(LED1);
static DigitalOut led2(LED2);
static DigitalOut led3(LED3);



static PinName i2c2_sda(PB_11);
static PinName i2c2_scl(PB_10);
static DigitalInOut pin3(PE_10);
static DigitalInOut pin4(PE_7);
static DigitalInOut pin5(PE_8);

static DigitalInOut pin6(PE_0);
static DigitalInOut pin7(PA_0);
static DigitalInOut pin8(PE_14);
static DigitalInOut pin9(PE_12);
static DigitalInOut pin10(PB_0);
static DigitalInOut pin11(PE_15);
static DigitalInOut pin12(PA_2);
static DigitalInOut pin13(PA_1);
static DigitalInOut pin14(PC_2);
static DigitalInOut pin15(PB_1);
static DigitalInOut pin1(PB_4);
static DigitalInOut pin2(PA_4);

static AnalogIn pinA(PC_1);


// PINS ALREADY USED
// static PinName mosi(PA_7);
// static PinName miso(PA_6);
// static PinName sclk(PA_5);
// static PinName nss(PD_14);
// static PinName reset(PG_0);
// static PinName dio1(PG_1);
// static PinName busy(PF_9);
// static PinName freq_select(PC_5);
// static PinName device_select(PC_4);
// static PinName crystal_select(PE_3);
// static PinName ant_switch(PE_6);
// static DigitalOut rrst(PF_12);
// static DigitalOut wrst(PD_15);
// static DigitalOut rck(PD_14);
// static DigitalOut wr(PA_7);
// static DigitalIn d0(PD_9);
// static DigitalIn d1(PD_8);
// static DigitalIn d2(PF_15);
// static DigitalIn d3(PE_13);
// static DigitalIn d4(PF_14);
// static DigitalIn d5(PE_11);
// static DigitalIn d6(PE_9);
// static DigitalIn d7(PF_13);
// static DigitalOut scl(PB_8);
// static DigitalInOut sda(PB_9);

char inputBuffer[1];

// MUST INCLUDE mbed-os/storage/filesystem/ WHEN COMPILING
FileHandle *mbed::mbed_override_console(int fd){
    return &serial_port;
}

int main(void){
  printf("int main(). \r\n");

  I2C i2c(i2c2_sda,i2c2_scl);
  i2c.frequency(1000000);
  i2c.start();
  i2c.write(0x78);
  i2c.stop();


  while(1){
    serial_port.read(inputBuffer, 1);
    switch (inputBuffer[0]){
      // Continuous receive for 15 seconds.
      case 'o':
        led1 = 1;
        pin1.output();
        pin2.output();
        pin3.output();
        pin4.output();
        pin5.output();
        pin6.output();
        pin7.output();
        pin8.output();
        pin9.output();
        pin10.output();
        pin11.output();
        pin12.output();
        pin13.output();
        pin14.output();
        pin15.output();
        ThisThread::sleep_for(1000);
        led1 = 0;
        break;

      case 'A':
        led1 = 1;
        printf("A: %d \r\n", pinA.read());
        ThisThread::sleep_for(1000);
        led1 = 0;
        break;

      case '1':
        led2 = 1;
        pin1.write(1);
        pin2.write(1);
        pin3.write(1);
        pin4.write(1);
        pin5.write(1);
        pin6.write(1);
        pin7.write(1);
        pin8.write(1);
        pin9.write(1);
        pin10.write(1);
        pin11.write(1);
        pin12.write(1);
        pin13.write(1);
        pin14.write(1);
        pin15.write(1);
        ThisThread::sleep_for(1000);
        led2 = 0;
        break;

      case '0':
        led1 = 1;
        led2 = 1;
        led3 = 1;
        pin1.write(0);
        pin2.write(0);
        pin3.write(0);
        pin4.write(0);
        pin5.write(0);
        pin6.write(0);
        pin7.write(0);
        pin8.write(0);
        pin9.write(0);
        pin10.write(0);
        pin11.write(0);
        pin12.write(0);
        pin13.write(0);
        pin14.write(0);
        pin15.write(0);
        ThisThread::sleep_for(1000);
        led1 = 0;
        led2 = 0;
        led3 = 0;
        break;

      case 'd':
        led3 = 1;
        led3 = 1;
        pin1.mode(OpenDrain);
        pin2.mode(OpenDrain);
        pin3.mode(OpenDrain);
        pin4.mode(OpenDrain);
        pin5.mode(OpenDrain);
        pin6.mode(OpenDrain);
        pin7.mode(OpenDrain);
        pin8.mode(OpenDrain);
        pin9.mode(OpenDrain);
        pin10.mode(OpenDrain);
        pin11.mode(OpenDrain);
        pin12.mode(OpenDrain);
        pin13.mode(OpenDrain);
        pin14.mode(OpenDrain);
        pin15.mode(OpenDrain);
        ThisThread::sleep_for(1000);
        led3 = 0;
        break;

      case 'n':
        led1 = 1;
        led2 = 1;
        pin1.mode(PullNone);
        pin2.mode(PullNone);
        pin3.mode(PullNone);
        pin4.mode(PullNone);
        pin5.mode(PullNone);
        pin6.mode(PullNone);
        pin7.mode(PullNone);
        pin8.mode(PullNone);
        pin9.mode(PullNone);
        pin10.mode(PullNone);
        pin11.mode(PullNone);
        pin12.mode(PullNone);
        pin13.mode(PullNone);
        pin14.mode(PullNone);
        pin15.mode(PullNone);
        ThisThread::sleep_for(1000);
        led1 = 0;
        led2 = 0;
        break;

      case 'i':
        led1 = 1;
        led3 = 1;
        pin1.input();
        pin2.input();
        pin3.input();
        pin4.input();
        pin5.input();
        pin6.input();
        pin7.input();
        pin8.input();
        pin9.input();
        pin10.input();
        pin11.input();
        pin12.input();
        pin13.input();
        pin14.input();
        pin15.input();
        ThisThread::sleep_for(1000);
        led1 = 0;
        led3 = 0;
        break;

      case 'r':
        led2 = 1;
        led3 = 1;
        printf("%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, \r\n",
        pin1.read(), pin2.read(), pin3.read(), pin4.read(), pin5.read(), pin6.read(),
        pin7.read(), pin8.read(), pin9.read(), pin10.read(), pin11.read(), pin12.read(),
        pin13.read(), pin14.read(), pin15.read() );

        ThisThread::sleep_for(1000);
        led2 = 0;
        led3 = 0;
        break;

      case 'p':
        led1 = 1;
        led2 = 1;
        led3 = 1;
        printf("Hello World\r\n");
        ThisThread::sleep_for(1000);
        led1 = 0;
        led2 = 0;
        led3 = 0;
        break;
    }
  }
}
