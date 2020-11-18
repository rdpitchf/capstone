
#include "mbed.h"
#include "GUIDE_CAMERA.cpp"
#include "GUIDE_DEFINE_REGISTERS.cpp"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
static DigitalOut led(LED1);

char inputBuffer[1];

// MUST INCLUDE mbed-os/storage/filesystem/ WHEN COMPILING
FileHandle *mbed::mbed_override_console(int fd){
    return &serial_port;
}

int main(void){
  printf("Mbed OS version %d.%d.%d\r\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

  CAMERA camera(&serial_port);

  printf("---------- Camera Registers ----------\r\n");
  camera.ResetCameraRegisters();
  printf("---------- ResetCameraRegisters Completed ----------\r\n");
  camera.ReadRegisters();
  printf("-------------------------\r\n");
  camera.InitializeOV7670Camera();
  printf("FINISHED INITIALIZING CAMERA \r\n \r\n");


  // setup();
  printf("camera Completed\r\n");
  while(1){
    serial_port.read(inputBuffer, 1);
    switch (inputBuffer[0]){
      case 'i':
        led = 1;
        camera.SetupOV7670ForQQVGAYUV();
        led = 0;
        break;

      case 's':
        led = 1;
        camera.TakePhoto();
        // ThisThread::sleep_for(1000ms);
        led = 0;
        break;

      case 'o':
        led = 1;
        camera.SetupCameraGain();
        camera.SetCameraSaturationControl();
        // ThisThread::sleep_for(1000ms);
        led = 0;
        break;

      case 'p':
        led = 1;
        printf("Hello World\r\n");
        ThisThread::sleep_for(1000ms);
        led = 0;
        break;
    }
  }
}
