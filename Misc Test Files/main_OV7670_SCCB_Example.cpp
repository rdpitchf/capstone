
#include "mbed.h"
#include "OV7670_SCCB.cpp"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
static DigitalOut led(LED1);
OV7670_SCCB i2c_camera_object;
char inputBuffer[1];

// MUST INCLUDE mbed-os/storage/filesystem/ WHEN COMPILING
FileHandle *mbed::mbed_override_console(int fd){
    return &serial_port;
}

void read_sccb_g(){
  uint8_t add = 0x14;
  uint8_t val = 0;

  printf("ADD\tVAL\n");

  val = i2c_camera_object.read(add);
  printf("0x%02X\t0x%02X\n", add++, val);
}
void write3w_sccb_h(){
  uint8_t add = 0x14;
  uint8_t val = 0x01;
  printf("ADD\tVAL\n");
  if(i2c_camera_object.write_3_phase(add, val)){
    printf("SUCCESSFUL WRITE\r\n");
  }
}
void write3w_sccb_j(){
  uint8_t add = 0x14;
  uint8_t val = 0xC5;
  printf("ADD\tVAL\n");
  if(i2c_camera_object.write_3_phase(add, val)){
    printf("SUCCESSFUL WRITE\r\n");
  }
}

int main(void){
  printf("Mbed OS version %d.%d.%d\r\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

  // setup();
  printf("Setup Completed\r\n");
  int counter = 0;
  while(1){
    serial_port.read(inputBuffer, 1);
    switch (inputBuffer[0]){
      case 'j':
        led = 1;
        write3w_sccb_j();
        led = 0;
        break;
      case 'g':
        led = 1;
        read_sccb_g();
        led = 0;
        break;
      case 'h':
        led = 1;
        write3w_sccb_h();
        led = 0;
        break;
      case 'p':
        led = 1;
        printf("Hello World\r\n");
        ThisThread::sleep_for(1000);
        led = 0;
        break;
    }
  }
}
