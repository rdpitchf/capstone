
// https://os.mbed.com/questions/82586/SDBlockDeviceh-not-found/
// https://github.com/ARMmbed/mbed-os-example-filesystem
// https://os.mbed.com/docs/mbed-os/v6.4/apis/sdblockdevice.html

#include "mbed.h"
#include "SD_CARD.cpp"
#include "GUIDE_camera.cpp"
#include "GUIDE_DEFINE_REGISTERS.cpp"

static BufferedSerial serial_port(USBTX, USBRX, 115200);

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

Picture_return_t take_a_picture(MICRO_SD_CARD *sd_card, CAMERA *camera, std::string filename, uint8_t * buffer, uint32_t buffer_max_size){
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

void take_photo(){

  MICRO_SD_CARD sd_card;
  CAMERA camera(&serial_port);
  camera.SetupOV7670ForQQVGAYUV();

  if(!sd_card.mount_sd_card()){
    printf("Setup Failed. \r\n");
  }

  std::string filename = "/fs/testing_file";
  Picture_return_t result = take_a_picture(&sd_card, &camera, filename, original_buffer, BUFFER_MAX_SIZE);
  printf("Status %d, Filename %s. \r\n", result.status, result.filename);

  if(!sd_card.unmount_sd_card()){
    printf("File unmount failed. \r\n");
  }
}

// Entry point for the example
int main() {
  printf("int main(). \r\n");
  take_photo();
  printf("PHOTO COMPLETED. \r\n");

}
