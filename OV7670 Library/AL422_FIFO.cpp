
#include "mbed.h"

static DigitalOut rrst(PF_12);
static DigitalOut wrst(PD_15);
static DigitalOut rck(PD_14);
static DigitalOut wr(PA_7);

static DigitalIn d0(PD_9);
static DigitalIn d1(PD_8);
static DigitalIn d2(PF_15);
static DigitalIn d3(PE_13);
static DigitalIn d4(PF_14);
static DigitalIn d5(PE_11);
static DigitalIn d6(PE_9);
static DigitalIn d7(PF_13);

class AL422_FIFO{
  public:
    // Initialize and set output pins for control. Set D0-7 pins to input.
    AL422_FIFO(){
      rrst = 1;
      wrst = 1;
      wr = 1;

      d0.mode(PullNone);
      d1.mode(PullNone);
      d2.mode(PullNone);
      d3.mode(PullNone);
      d4.mode(PullNone);
      d5.mode(PullNone);
      d6.mode(PullNone);
      d7.mode(PullNone);
    }

    void readReset(){
      rrst = 0;
      wait_us(1);

      // Add clock cycle
      rck = 1;
      wait_us(1);
      rck = 0;
      wait_us(1);

      rrst = 1;
    }
    void writeReset(){
      wrst = 0;
      wait_us(1);

      // Add a clock cycle
      rck = 1;
      wait_us(1);
      rck = 0;
      wait_us(1);

      wrst = 1;
    }

    void writeEnable(){
      wr = 0;
      wait_us(1);
    }
    void writeDisable(){
      wr = 1;
      wait_us(1);
    }

    uint8_t readByte(){
      rck = 1;
      // wait_us(1);
      uint8_t data_byte = d0 | (d1 << 1) | (d2 << 2) | (d3 << 3) | (d4 << 4) | (d5 << 5) | (d6 << 6) |  (d7 << 7);
      rck = 0;
      // wait_us(1);
      return data_byte;
    }
    void skipByte(){
      rck = 1;
      wait_us(1);
      rck = 1;
      wait_us(1);
      rck = 0;
      wait_us(1);
      rck = 0;
    }
    void readBytes(uint8_t *buffer, int count){
      for(int i = 0; i < count; i++){
        buffer[i] = readByte();
      }
    }
};
