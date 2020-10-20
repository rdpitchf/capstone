
#include "mbed.h"
#include "CAMERA.cpp"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
static DigitalOut led(LED1);
CAMERA camera;
char inputBuffer[1];

// MUST INCLUDE mbed-os/storage/filesystem/ WHEN COMPILING
FileHandle *mbed::mbed_override_console(int fd){
    return &serial_port;
}

#define VSYNC PA_6
DigitalInOut vsync(VSYNC);

const int XRES = 160;
const int YRES = 120;
const int BYTES_PER_PIXEL = 2;
const int frameSize = XRES * YRES * BYTES_PER_PIXEL;
uint8_t frame[frameSize];


int main(void){
  // printf("Mbed OS version %d.%d.%d\r\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

  // setup();
  // printf("Setup Completed\r\n");
  int counter = 0;
  while(1){
    serial_port.read(inputBuffer, 1);
    switch (inputBuffer[0]){
      case 'i':
        led = 1;
        camera.QQVGARGB565();
        vsync.input();
        led = 0;
        break;


        case 's':
          led = 1;
          while(!vsync.read());
          while(vsync.read());
          camera.prepareCapture();
          camera.startCapture();
          while(!vsync.read());
          camera.stopCapture();

          camera.readFrame(frame, XRES, YRES, BYTES_PER_PIXEL);
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
