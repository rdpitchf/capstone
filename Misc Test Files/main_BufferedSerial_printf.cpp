
#include "mbed.h"

static BufferedSerial serial_port(USBTX, USBRX, 921600);
static DigitalOut led(LED1);

// MUST INCLUDE mbed-os/storage/filesystem/ WHEN COMPILING
FileHandle *mbed::mbed_override_console(int fd){
    return &serial_port;
}

int main(void){

    printf("Mbed OS version %d.%d.%d\r\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

    int counter = 0;
    while(1){
      counter++;
      printf("Counter: %d\r\n", counter);
      led = !led;
      ThisThread::sleep_for(1000);
    }
}
