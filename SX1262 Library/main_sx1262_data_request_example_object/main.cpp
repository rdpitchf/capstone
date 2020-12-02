
#include "mbed.h"
#include "SX1262_RADIO.cpp"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
static DigitalOut led1(LED1);
static DigitalOut led2(LED2);
static DigitalOut led3(LED3);

char inputBuffer[1];

// MUST INCLUDE mbed-os/storage/filesystem/ WHEN COMPILING
FileHandle *mbed::mbed_override_console(int fd){
    return &serial_port;
}

int main(void){
  printf("int main(). \r\n");

  SX_RADIO sx_radio;

  bool is_trail_camera = 1;

  while(1){
    serial_port.read(inputBuffer, 1);
    switch (inputBuffer[0]){
      // Continuous receive for 15 seconds.
      case 'b':
        led1 = 1;
        if(is_trail_camera){
          uint8_t result = sx_radio.send_battery_status(1, 34);
          printf("send battery status result: %d \r\n", result);
        }
        led1 = 0;
        break;

      case 'd':
        led2 = 1;
        if(is_trail_camera){
          uint8_t result2 = sx_radio.send_data(0, 0, QQVGA_DATA_SIZE);
          printf("send_data result: %X \r\n", result2);
        }
        led2 = 0;
        break;

      case 'c':
        led2 = 1;
        if(!is_trail_camera){
          uint8_t result2 = sx_radio.ch_request_data(15);
          printf("ch_request_data result: %X \r\n", result2);
        }
        led2 = 0;
        break;

      case 's':
        led3 = 1;
        is_trail_camera = !is_trail_camera;
        ThisThread::sleep_for(1000);
        led3 = 0;
        break;

      case 'n':
        led3 = 1;
        serial_port.read(inputBuffer, 1);
        switch (inputBuffer[0]){
          // Continuous receive for 15 seconds.
          case 'v':
            sx_radio.number_of_bytes_to_send = VGA_DATA_SIZE;
            break;
          case 'b':
            sx_radio.number_of_bytes_to_send = QVGA_DATA_SIZE;
            break;
          case 'n':
            sx_radio.number_of_bytes_to_send = QQVGA_DATA_SIZE;
            break;
          case 'm':
            sx_radio.number_of_bytes_to_send = 900;
            break;
        }
        ThisThread::sleep_for(1000);
        led3 = 0;
        break;

      case 'p':
        led1 = 1;
        led2 = 1;
        led3 = 1;
        if(is_trail_camera){
          printf("Hello World Trail Camera Mode\r\n");
        }else{
          printf("Hello World Central Hub Mode\r\n");
        }
        ThisThread::sleep_for(1000);
        led1 = 0;
        led2 = 0;
        led3 = 0;
        break;
    }
  }
}
