#include "mbed.h"
#include <stdio.h>
#include <errno.h>
#include <functional>

#include "BlockDevice.h"
#include "SDBlockDevice.h"
#include "FATFileSystem.h"

class Micro_sd_card{
  public:
    SDBlockDevice *bd;
    FATFileSystem *fs;
    FILE *file;

    Micro_sd_card(){
      this->bd = new SDBlockDevice(A2,PC_2,PB_13,PB_12);
      this->fs = new FATFileSystem("fs");
    }
    ~Micro_sd_card(){
      delete this->bd;
      delete this->fs;
      this->bd = nullptr;
      this->fs = nullptr;
    }
    bool file_exists(char* filename){
      fflush(stdout);
      FILE *f;
      f = fopen(filename, "r+");

      if(!f){
        return true;
      }else{
        return false;
      }
    }
    bool create_file(const char* filename){
      // Create the numbers file if it doesn't exist
      fflush(stdout);
      this->file = nullptr;
      this->file = fopen(filename, "w+");
      if(this->file){
        return true;
      }else{
        return false;
      }
    }
    bool mount_sd_card(){
      // Mount SD Card:
      fflush(stdout);
      int err = this->fs->mount(this->bd);
      if(!err){
        return true;
      }else{
        return false;
      }
    }
    bool reformat_sd_card(){
      // Reformat if we can't mount the filesystem
      fflush(stdout);
      int err = this->fs->reformat(this->bd);
      if(!err){
        return true;
      }else{
        return false;
      }
    }
    bool write_data_to_file(uint8_t * data_buffer , size_t data_length){
      size_t data_written  = fwrite(data_buffer , sizeof(uint8_t), data_length, this->file);
      if(data_written == data_length){
        return true;
      }else{
        return false;
      }
    }
    bool close_file(){
      fflush(stdout);
      int err = fclose(this->file);
      if(!err){
        return true;
      }else{
        return false;
      }
    }
    bool open_file(char* filename){
      fflush(stdout);
      this->file = fopen(filename, "r+");
      if(this->file){
        return true;
      }else{
        return false;
      }
    }
    size_t read_data_from_file(uint8_t *buffer, size_t max_buffer_size){
      size_t bytes_read = fread(buffer, sizeof(uint8_t), max_buffer_size,this->file);
      return bytes_read;
    }
    bool end_of_file(){
      return feof(this->file);
    }
    bool unmount_sd_card(){
      fflush(stdout);
      int err = this->fs->unmount();
      if(!err){
        return true;
      }else{
        return false;
      }
    }
    bool delete_file(std::string filename){
      char * f_name = &filename[0];
      if(remove(f_name) != 0 ){
        return false;
      }else{
        return true;
      }
    }

};
