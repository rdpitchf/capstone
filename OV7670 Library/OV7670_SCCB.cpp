
#include "mbed.h"

static DigitalOut scl(PB_8);
static DigitalInOut sda(PB_9);
static double half_period_us = 1.25;
static bool tri_state = false;

#define ID_ADDRESS_WRITE 0x42
#define ID_ADDRESS_READ 0x43

class OV7670_SCCB{
  public:
    OV7670_SCCB(){
      if(tri_state){
        scl_HIGH();
        sda_LOW();
      }else{
        scl_HIGH();
        sda_HIGH();
      }
      sda.mode(PullUp);
    }

    static void scl_LOW(){
      scl = 0;
    }
    static void scl_HIGH(){
      scl = 1;
    }
    static void sda_LOW(){
      sda = 0;
    }
    static void sda_HIGH(){
      sda = 1;
    }

    static bool write_until_true(uint8_t subaddress, uint8_t data){
      while(read(subaddress) != data){
        write_3_phase(subaddress, data);
      }
      return true;
    }
    static bool write_3_phase(uint8_t subaddress, uint8_t data){
      bool success = true;
      // Send address
      start();
      if(!write_addresses(ID_ADDRESS_WRITE)){
        success = false;
      }

      if(!write_addresses(subaddress)){
        success = false;
      }

      if(!write_addresses(data)){
        success = false;
      }
      stop();

      return success;
    }
    static bool write_addresses(uint8_t address){
      bool success = false;

      // Transmit in order D7, D6, ... , D0
      for (uint8_t i = 0; i < 8; i++) {
        if ((address << i) & 0x80){
          sda_HIGH();
        }else{
          sda_LOW();
        }
        wait_us(half_period_us);
        scl_HIGH();
        wait_us(half_period_us);
        scl_LOW();
        wait_us(half_period_us);

      }

      // Read the Don't Care Bit from Slave
      sda.input();
      scl_HIGH();
      wait_us(half_period_us);
      if(!sda.read()){
        success = true;
      }else{
        // printf("Write Unsuccessful.\r\n");
      }
      scl_LOW();
      wait_us(half_period_us);
      sda.output();

      return success;
    }
    static uint8_t read(uint8_t subaddress){
      bool success = true;
      // Send address
      start();
      if(!write_addresses(ID_ADDRESS_WRITE)){
        success = false;
      }

      if(!write_addresses(subaddress)){
        success = false;
      }
      stop();

      // Send Data.
      start();
      if(!write_addresses(ID_ADDRESS_READ)){
        success = false;
      }

      uint8_t data = read_data();
      if(data == 0x00){
        success = false;
      }
      stop();

      if(!success){
        // printf("Read failed.\r\n");
      }

      return data;
    }
    static uint8_t read_data(){
      uint8_t data = 0;

      // Read incoming data
      sda.input();

      for (uint8_t i = 8; i > 0; i--) {
        scl_HIGH();
        wait_us(half_period_us);

        data = data << 1;
        if (sda.read())
          data++;

        scl_LOW();
        wait_us(half_period_us);
      }
      sda.output();

      return data;
    }

    static void start(){
      if(tri_state){
        start_tri_state();
      }else{
        start_normal();
      }
    }
    static void stop(){
      if(tri_state){
        stop_tri_state();
      }else{
        stop_normal();
      }
    }
    static void start_tri_state(){
      scl = 1;
      sda = 1;
      wait_us(half_period_us);
      sda = 0;
      wait_us(half_period_us);
      scl = 0;
      wait_us(half_period_us);
    }
    static void stop_tri_state(){
      scl = 1;
      sda = 0;
      wait_us(half_period_us);
      sda = 1;
      wait_us(half_period_us);
      sda = 0;
      wait_us(half_period_us);
    }
    static void start_normal(){
      scl = 1;
      sda = 0;
      wait_us(half_period_us);
      scl = 0;
      wait_us(half_period_us);
    }
    static void stop_normal(){
      scl = 1;
      sda = 0;
      wait_us(half_period_us);
      sda = 1;
      wait_us(half_period_us);
    }

};
