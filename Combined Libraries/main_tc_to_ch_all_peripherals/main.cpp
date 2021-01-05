
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
#include "millis.h"
#include "GPS.h"
#include <stdio.h>

#define FILE_OPEN_RETRY 10
#define FILE_CLOSE_RETRY 10
#define FILE_MOUNT_RETRY 10

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
void setup_gps(gps* myGPS, I2C* test);
void save_position_time_date(Micro_sd_card *sd_card, std::string filename);

// Entry point for the example
int main() {

  printf("int main(). \r\n");
  bool is_trail_camera = 1;

  Micro_sd_card sd_card;
  Sx_radio sx_radio(&sd_card, &serial_port);

  while(1){
    serial_port.read(inputBuffer, 1);
    switch (inputBuffer[0]){
      case 'd':{
        led2 = 1;
        if(is_trail_camera){
          Camera camera(false);
          take_photo(&sd_card, &camera, &sx_radio);
        }
        led2 = 0;
        break;
      }

      case 'c':{
        while(true){
          led2 = 1;
          if(!is_trail_camera){
              uint8_t result2 = sx_radio.ch_request_data(15);
              printf("ch_request_data result: %X \r\n", result2);
          }
          led2 = 0;
        }
        break;
      }
      case 'n':{
        led3 = 1;
        Camera camera(true);
        camera.set_to_night_mode();
        take_photo(&sd_card, &camera, &sx_radio);
        ThisThread::sleep_for(1000);
        led3 = 0;
        break;
      }
      case 's':{
        led3 = 1;
        is_trail_camera = !is_trail_camera;
        ThisThread::sleep_for(1000);
        led3 = 0;
        break;
      }

      case 'g':{
        led3 = 1;
        if(is_trail_camera){
          gps myGPS;
          I2C gpsi2c(PC_9,PC_0);
          setup_gps(&myGPS, &gpsi2c);
          position_time_date_t a = myGPS.position_time_date();
          printf("GPS: Lat: %ld Long: %ld Alt: %ld tow: %lu Date: %ld %d %d, %d %d %d\r\n", a.lat, a.lon, a.alt, a.tow, a.Year, a.Month, a.Day, a.Hour, a.Minute, a.Second);
        }
        ThisThread::sleep_for(1000);
        led3 = 0;
        break;
      }

      case 'q':{
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
      }

      case 'p':{
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
  printf("PHOTO COMPLETED. \r\n");

}

void setup_gps(gps* myGPS, I2C* test){
  Ticker flipper;
  flipper.attach_us(&ticker_SysTick_Handler, 1000);
  // myGPS.enableDebugging(false);
  // test.frequency(400000);

  //Connect to the Ublox module using Wire port
  if (myGPS->begin(*test, 0x42 << 1) != false){
    printf("Ublox GPS not detected at default I2C address. Please check wiring. Freezing.\r\n");
    while (1);
  }

  myGPS->setI2COutput(COM_TYPE_UBX, 0); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGPS->saveConfiguration(0); //Save the current settings to flash and BBR

  flipper.detach();
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

  bool sd_created = false;
  for(uint8_t i = 0; i < FILE_OPEN_RETRY; i++){
    if(sd_card->create_file(name)){
      sd_created = true;
      break;
    }
  }

  if(sd_created){
    camera->prepare_fifo_data();
    uint32_t buffer_size = camera->get_data(buffer, buffer_max_size);
    while(buffer_size == buffer_max_size){
      sd_card->write_data_to_file(buffer, buffer_size);
      buffer_size = camera->get_data(buffer, buffer_max_size);
    }
    sd_card->write_data_to_file(buffer, buffer_size);

    bool sd_close = false;
    for(uint8_t i = 0; i < FILE_CLOSE_RETRY; i++){
      if(sd_card->close_file()){
        sd_close = true;
        break;
      }
    }
    if(!sd_close){
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
  result = sx_radio->send_data(0, 0, QQVGA_DATA_SIZE, filename);
  if(!sd_card->close_file()){
    result = 1;
    printf("Could not close the file. \r\n");
  }
  return result;

}
void save_position_time_date(Micro_sd_card *sd_card, std::string filename){
  gps myGPS;
  I2C gpsi2c(PC_9,PC_0);
  setup_gps(&myGPS, &gpsi2c);
  position_time_date_t a = myGPS.position_time_date();
  while(a.Year == 0 || a.Month == 0 || a.Day == 0 || a.tow == 0 || a.lat == 0 || a.lon == 0 || a.alt == 0){
    a = myGPS.position_time_date();
    wait_us(25000);
  }
  printf("GPS: Lat: %ld Long: %ld Alt: %ld tow: %lu Date: %ld %d %d, %d %d %d\r\n", a.lat, a.lon, a.alt, a.tow, a.Year, a.Month, a.Day, a.Hour, a.Minute, a.Second);

  // Remove the .yuv from filename
  // Add .txt to filename
  std::string name_temp = filename + ".txt";
  char * name = &name_temp[0];

  bool sd_created = false;
  for(uint8_t i = 0; i < FILE_OPEN_RETRY; i++){
    if(sd_card->create_file(name)){
      sd_created = true;
      break;
    }
  }
  if(sd_created){
    // Write to sd card:
    uint8_t sd_card_buffer[4]{0};
    for(uint8_t i = 0; i < 4; i++){
      sd_card_buffer[i] = (a.lat >> (8*i)) & 0xFF;
    }
    sd_card->write_data_to_file(sd_card_buffer, 4);
    for(uint8_t i = 0; i < 4; i++){
      sd_card_buffer[i] = (a.lon >> (8*i)) & 0xFF;
    }
    sd_card->write_data_to_file(sd_card_buffer, 4);
    for(uint8_t i = 0; i < 4; i++){
      sd_card_buffer[i] = (a.alt >> (8*i)) & 0xFF;
    }
    sd_card->write_data_to_file(sd_card_buffer, 4);
    for(uint8_t i = 0; i < 4; i++){
      sd_card_buffer[i] = (a.tow >> (8*i)) & 0xFF;
    }
    sd_card->write_data_to_file(sd_card_buffer, 4);
    for(uint8_t i = 0; i < 2; i++){
      sd_card_buffer[i] = (a.Year >> (8*i)) & 0xFF;
    }
    sd_card->write_data_to_file(sd_card_buffer, 2);
    sd_card_buffer[0] = a.Month;
    sd_card->write_data_to_file(sd_card_buffer, 1);
    sd_card_buffer[0] = a.Day;
    sd_card->write_data_to_file(sd_card_buffer, 1);
    sd_card_buffer[0] = a.Hour;
    sd_card->write_data_to_file(sd_card_buffer, 1);
    sd_card_buffer[0] = a.Minute;
    sd_card->write_data_to_file(sd_card_buffer, 1);
    sd_card_buffer[0] = a.Second;
    sd_card->write_data_to_file(sd_card_buffer, 1);

    bool sd_close = false;
    for(uint8_t i = 0; i < FILE_CLOSE_RETRY; i++){
      if(sd_card->close_file()){
        sd_close = true;
        break;
      }
    }
    if(!sd_close){
      printf("Could not close the gps file. \r\n");
    }
  }else{
    printf("Could not create the gps file. \r\n");
  }
}
void take_photo(Micro_sd_card *sd_card, Camera *camera, Sx_radio *sx_radio){


  camera->SetupOV7670ForQQVGAYUV();

  bool sd_mount = false;
  for(uint8_t i = 0; i < FILE_MOUNT_RETRY; i++){
    if(sd_card->mount_sd_card()){
      sd_mount = true;
      break;
    }
  }
  if(!sd_mount){
    printf("Setup Failed. \r\n");
  }else{
    std::string filename = "/fs/testing_file";
    Picture_return_t results = take_a_picture(sd_card, camera, filename, original_buffer, BUFFER_MAX_SIZE);
    printf("Capture Status %d. \r\n", results.status);

    size_t lastindex = results.filename.find_last_of(".");
    std::string name = results.filename.substr(0, lastindex);

    save_position_time_date(sd_card, name);

    uint8_t result = send_picture(sd_card, sx_radio, name);
    printf("%s",name);
    printf("\r\n");
    printf("Transmit Status %d. \r\n",result);

    if(result == 22){
      std::string name_txt = name + ".txt";
      if(!sd_card->delete_file(name_txt)){
        printf("Could not delete: %s.txt\r\n", name_txt);
      }
      std::string name_yuv = name + ".yuv";
      if(!sd_card->delete_file(name_yuv)){
        printf("Could not delete: %s.yuv\r\n", name_yuv);
      }
    }

    sd_mount = false;
    for(uint8_t i = 0; i < FILE_MOUNT_RETRY; i++){
      if(sd_card->unmount_sd_card()){
        sd_mount = true;
        break;
      }
    }
    if(!sd_mount){
      printf("File unmount failed. \r\n");
    }
  }
}
