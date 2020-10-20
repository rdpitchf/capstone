
#include "mbed.h"
#include "CAMERA.cpp"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
static DigitalOut led(LED1);

char inputBuffer[1];

// MUST INCLUDE mbed-os/storage/filesystem/ WHEN COMPILING
FileHandle *mbed::mbed_override_console(int fd){
    return &serial_port;
}

#define VSYNC PA_6
DigitalInOut vsync(VSYNC);

const int frameSize = 640*480*2;
uint8_t frame[frameSize];

int main(void){
  // printf("Hello Start Program. \r\n");
  CAMERA camera;
  // printf("camera. \r\n");
  vsync.input();
  int counter = 0;
  while(1){
    serial_port.read(inputBuffer, 1);
    switch (inputBuffer[0]){
        case 's':
          led = 1;
          while(!vsync.read());
          while(vsync.read());
          camera.prepareCapture();
          camera.startCapture();
          while(!vsync.read());
          camera.stopCapture();

          // for(int a = 0; a < 128; a++){
          //   camera.take_vga_picture(frame);
          //   serial_port.write(frame, sizeof(frame));
          // }
          camera.take_vga_picture(frame);
          serial_port.write(frame, sizeof(frame));

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
