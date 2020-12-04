
// https://os.mbed.com/questions/82586/SDBlockDeviceh-not-found/
// https://github.com/ARMmbed/mbed-os-example-filesystem
// https://os.mbed.com/docs/mbed-os/v6.4/apis/sdblockdevice.html

#include "mbed.h"
#ifndef INCLUDEDD
#define INCLUDEDD
#include "SD_CARD.cpp"
#endif
#include "GUIDE_CAMERA.cpp"
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

#define BUFFER_MAX_SIZE 101
static uint8_t original_buffer[BUFFER_MAX_SIZE]{0};

typedef struct{
    uint8_t status;
    std::string filename;
}Picture_return_t;

Picture_return_t take_a_picture(Micro_sd_card *sd_card, Camera *camera, std::string filename, uint8_t * buffer, uint32_t buffer_max_size);
uint8_t send_picture(Micro_sd_card *sd_card, Sx_radio *sx_radio, std::string filename);
void take_photo(Micro_sd_card *sd_card, Camera *camera, Sx_radio *sx_radio);

// Entry point for the example
int main() {
  printf("int main(). \r\n");
  bool is_trail_camera = 1;

  Micro_sd_card sd_card;
  Sx_radio sx_radio(&sd_card, &serial_port);

  while(1){
    serial_port.read(inputBuffer, 1);
    switch (inputBuffer[0]){
      case 'd':
        led2 = 1;
        if(is_trail_camera){
          Camera camera;
          take_photo(&sd_card, &camera, &sx_radio);
        }
        led2 = 0;
        break;

      case 'c':
        while(true){
          led2 = 1;
          if(!is_trail_camera){
              uint8_t result2 = sx_radio.ch_request_data(15);
              printf("ch_request_data result: %X \r\n", result2);
          }
          led2 = 0;
        }
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
  printf("PHOTO COMPLETED. \r\n");

}

Picture_return_t take_a_picture(Micro_sd_card *sd_card, Camera *camera, std::string filename, uint8_t * buffer, uint32_t buffer_max_size){

  Picture_return_t output{
    .status = 0,
    .filename = filename + ".yuv",
  };
  // Image is stored in FIFO Buffer
  camera->TakePhoto();
  std::string filename_temp = filename;
  std::string new_filename = filename;
  char * name = &filename[0];

  // Always find a save file name.
  while(!sd_card->file_exists(name)){
    filename_temp = filename_temp + "_1";
    new_filename = filename_temp + ".yuv";
    name = &new_filename[0];
  }
  output.filename = new_filename;
  if(sd_card->create_file(name)){
    camera->prepare_fifo_data();
    uint32_t buffer_size = camera->get_data(buffer, buffer_max_size);
    while(buffer_size == buffer_max_size){
      bool status = sd_card->write_data_to_file(buffer, buffer_size);
      buffer_size = camera->get_data(buffer, buffer_max_size);
    }
    bool status = sd_card->write_data_to_file(buffer, buffer_size);

    if(!sd_card->close_file()){
      output.status = 1;
      printf("Could not close the file. \r\n");
    }
  }else{
    output.status = 2;
    printf("Could not create the file. \r\n");
  }
  return output;
}

uint8_t send_picture(Micro_sd_card *sd_card, Sx_radio *sx_radio, std::string filename){
  uint8_t result = 0;

  if(sd_card->open_file(&filename[0])){
    printf("send_data start. \r\n");
    result = sx_radio->send_data(0, 0, QQVGA_DATA_SIZE);
    printf("send_data end. \r\n");
    if(!sd_card->close_file()){
      result = 1;
      printf("Could not close the file. \r\n");
    }
  }else{
    printf("Could not open the file. \r\n");
    result = 2;
  }
  return result;

}
void take_photo(Micro_sd_card *sd_card, Camera *camera, Sx_radio *sx_radio){


  camera->SetupOV7670ForQQVGAYUV();

  if(!sd_card->mount_sd_card()){
    printf("Setup Failed. \r\n");
  }

  std::string filename = "/fs/testing_file";
  Picture_return_t results = take_a_picture(sd_card, camera, filename, original_buffer, BUFFER_MAX_SIZE);
  printf("Capture Status %d. \r\n", results.status);

  uint8_t result = send_picture(sd_card, sx_radio, results.filename.c_str());
  printf(results.filename.c_str());
  printf("\r\n");
  printf("Transmit Status %d. \r\n",result);


  if(!sd_card->unmount_sd_card()){
    printf("File unmount failed. \r\n");
  }
}
