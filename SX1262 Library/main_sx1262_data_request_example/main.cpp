
#include "mbed.h"
#include "SX126X_LoRaRadio.h"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
static DigitalOut led1(LED1);
static DigitalOut led2(LED2);
static DigitalOut led3(LED3);

char inputBuffer[1];

// MUST INCLUDE mbed-os/storage/filesystem/ WHEN COMPILING
FileHandle *mbed::mbed_override_console(int fd){
    return &serial_port;
}

static PinName mosi(SPI_MOSI);
static PinName miso(SPI_MISO);
static PinName sclk(SPI_SCK);
static PinName nss(SPI_CS);
static PinName reset(PG_0);
static PinName dio1(PG_1);
static PinName busy(PF_9);
static PinName freq_select(PC_5);
static PinName device_select(PC_4);
static PinName crystal_select(PE_3);
static PinName ant_switch(PE_6);

static SX126X_LoRaRadio radio(mosi, miso, sclk, nss, reset, dio1, busy, freq_select, device_select, crystal_select, ant_switch);
bool using_lora = false;

typedef struct{
    bool rxDone;
    bool rxError;
    bool txDone;
    bool rxTimeout;
    bool txTimeout;
}RadioFlags_t;

typedef enum{
    SEND_PACKET,
    WAIT_SEND_DONE,
    RECEIVE_PACKET,
    WAIT_RECEIVE_DONE,
    INIT,
    LISTEN_ON_CHANNEL,
    WAIT_LISTEN_ON_CHANNEL,
}request_states_t;

RadioFlags_t radioFlags = {
    .rxDone = false,
    .rxError = false,
    .txDone = false,
    .rxTimeout = false,
    .txTimeout = false,
};

// Define packet sizes.
#define REQUEST_PACKET_SIZE 5
#define REQUEST_GRANT_PACKET_SIZE 4
#define DATA_PACKET_SIZE 255
#define DATA_ACK_PACKET_SIZE 3
// Define resolution sizes.
#define VGA_DATA_SIZE 640*480*2
#define QVGA_DATA_SIZE 320*240*2
#define QQVGA_DATA_SIZE 160*120*2
// Radio identification number. (uint16_t)
uint16_t radio_id = 0x10AF;
// Define Data packet type:
#define DATA_FULL 1
#define DATA_HALF_FULL 2
#define DATA_EMPTY 3
// Retry limits:
#define REQUEST_TX_RETRY_MAX 3
#define REQUEST_RX_RETRY_MAX 3
#define DATA_RETRY_MAX 3
// Masks:
#define REQUEST_GRANT_RX_PACKET_CALIBRATION_MASK 0x6




// ERROR and SUCCESS Codes:
#define REQUEST_ERROR_1 0x1 // Error: Channel was already busy
#define REQUEST_ERROR_2 0x2 // Error: Tx Timeout
#define REQUEST_ERROR_3 0x3 // Error: Rx Timeout
#define REQUEST_GRANT_ERROR_1 0x1 // Error: Rx timeout.
#define REQUEST_GRANT_ERROR_2 0x2 // Error: Tx timeout.

#define REQUEST_SUCCESS 0xF // Success: Successful request.
#define REQUEST_GRANT_SUCCESS 0xF // Success: Tx Successful

#define DATA_TX_SUCCESS 0x00
#define DATA_RX_SUCCESS 0x00
#define DATA_RX_ERROR_1 0x01
#define DATA_RX_ERROR_2 0x02
#define DATA_RX_ERROR_3 0x03
#define DATA_RX_ERROR_4 0x04
#define DATA_RX_ERROR_5 0x05

#define DATA_STATUS_SUCCESS 0
#define DATA_STATUS_TIMEOUT_ERROR 1
#define DATA_RX_FAILED_TIMEOUT 1
#define DATA_TRANSFER_SUCCESSFUL 2
#define DATA_ACK_TIMEOUT 0
#define DATA_ACK_ERROR 1
#define DATA_ACK_SUCCESS 2
#define DATA_SUCCESS 3





// FYI: ch stands for central_hub and tc stands for trail_camera.
volatile request_states_t tc_request_state = INIT;
volatile request_states_t ch_request_state = INIT;
volatile request_states_t tc_data_state = INIT;
volatile request_states_t ch_data_state = INIT;

// Request Packets.
uint8_t tc_request_tx_packet[REQUEST_PACKET_SIZE] {0};
uint8_t ch_request_rx_packet[REQUEST_PACKET_SIZE] {0};
// Request Grant Packets.
uint8_t tc_request_grant_rx_packet[REQUEST_GRANT_PACKET_SIZE] {0};
uint8_t ch_request_grant_tx_packet[REQUEST_GRANT_PACKET_SIZE] {0};
// Data Packets.
uint8_t tc_data_tx_packet[DATA_PACKET_SIZE] {0};
uint8_t ch_data_rx_packet[DATA_PACKET_SIZE] {0};
// Data Acknowledgement Packets.
uint8_t tc_data_ack_rx_packet[DATA_ACK_PACKET_SIZE] {0};
uint8_t ch_data_ack_tx_packet[DATA_ACK_PACKET_SIZE] {0};
// RSSI:
int16_t tc_request_grant_rx_rssi = 0;
int16_t ch_request_rx_rssi = 0;
// SNR:
int8_t tc_request_grant_rx_snr = 0;
int8_t ch_request_rx_snr = 0;




// Add in buffers in order to pass in pointers to add the data.
// This will require functions like set_data() and print_data()/save_data()
// This method is used because we are limited to 640KB of SRAM
// And one VGA picture is 614.4KB
static uint8_t tc_tx_data_buffer[DATA_PACKET_SIZE-3] {0};
static uint8_t ch_rx_data_buffer[DATA_PACKET_SIZE-3] {0};

// Set to the max size
static uint8_t ch_rx_data[VGA_DATA_SIZE] {0};
uint16_t ch_rx_data_size = 0;

// uint8_t frame_vga[VGA_DATA_SIZE] {0};
// uint8_t frame_qvga[QVGA_DATA_SIZE] {0};
uint8_t frame_qqvga[1] {0};

uint16_t rx_data_size = 0;
uint8_t rx_data_data = 0;
uint8_t rx_data_status = 0;





radio_modems_t modem;
uint32_t rx_bandwidth;
uint32_t tx_bandwidth;
uint32_t rx_datarate;
uint32_t tx_datarate;
uint8_t rx_coderate;
uint8_t tx_coderate;
uint16_t rx_preamble_len;
uint16_t tx_preamble_len;
bool rx_fix_len;
bool tx_fix_len;
bool rx_crc_on;
bool tx_crc_on;
bool rx_freq_hop_on;
bool tx_freq_hop_on;
bool rx_iq_inverted;
bool tx_iq_inverted;
uint32_t rx_bandwidth_afc;
uint16_t rx_symb_timeout;
uint8_t rx_payload_len;
uint8_t rx_hop_period;
bool rx_rx_continuous;
int8_t tx_power;
uint8_t tx_hop_period;
uint32_t tx_timeout_val;
uint32_t tx_fdev;

void set_rx_settings_lora(uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint32_t bandwidth_afc, uint16_t preamble_len, uint16_t symb_timeout, bool fix_len, uint8_t payload_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, bool rx_continuous);
void set_tx_settings_lora(int8_t power, uint32_t fdev, uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint16_t preamble_len, bool fix_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, uint32_t timeout);
void set_rx_settings_fsk(uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint32_t bandwidth_afc, uint16_t preamble_len, uint16_t symb_timeout, bool fix_len, uint8_t payload_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, bool rx_continuous);
void set_tx_settings_fsk(int8_t power, uint32_t fdev, uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint16_t preamble_len, bool fix_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, uint32_t timeout);
void set_radio_configuration_default(bool lora_or_fsk);

uint8_t send_request_packet(uint8_t battery_enable, uint8_t battery_status, uint32_t size, uint8_t *data_input);
uint8_t send_battery_status(uint8_t battery_enable, uint8_t battery_status);
uint8_t send_data(uint8_t battery_enable, uint8_t battery_status, uint32_t size,  uint8_t *data_input);
void calibrate_radio();
uint8_t ch_request_data(uint8_t timeout_seconds, uint8_t *data_output);
uint8_t tc_transmit_data(uint8_t transmit_interval, uint32_t size, uint8_t *data_input);
uint8_t ch_recevie_data(uint16_t device_id, uint16_t data_size, uint8_t *data_output);

uint8_t send_battery_status(uint8_t battery_enable, uint8_t battery_status){
  return send_request_packet(battery_enable, battery_status, 0, NULL);
}
uint8_t send_data(uint8_t battery_enable, uint8_t battery_status, uint32_t size, uint8_t *data_input){
  return send_request_packet(battery_enable, battery_status, size, data_input);
}
void calibrate_radio(){
  // Calibration process.
  printf("calibrate_radio()\r\n");
}

uint8_t send_request_packet(uint8_t battery_enable, uint8_t battery_status, uint32_t size, uint8_t *data_input){
  uint8_t send_request_status = 0x0;
  bool exit = false;

  tc_request_state = INIT;

  uint8_t tc_request_tx_packet_upper = (radio_id >> 8);
  uint8_t tc_request_tx_packet_lower = (radio_id & 0x00FF);
  uint8_t rx_retry_count = 0;
  uint8_t tx_retry_count = 0;

  // Reset all flags.
  radioFlags.txDone = false;
  radioFlags.rxDone = false;
  radioFlags.rxError = false;
  radioFlags.rxTimeout = false;
  radioFlags.txTimeout = false;

  while(1){
    switch(tc_request_state){
      case INIT:{
        // Set default radio configuration.
        set_radio_configuration_default(true);
        // Set the battery level in the battery packet.
        tc_request_tx_packet[0] = tc_request_tx_packet_upper;
        tc_request_tx_packet[1] = tc_request_tx_packet_lower;
        tc_request_tx_packet[2] = battery_enable;
        tc_request_tx_packet[3] = battery_status;
        if(battery_enable == 0){
          if(size == VGA_DATA_SIZE){
            tc_request_tx_packet[4] = 1;
          }else if(size == QVGA_DATA_SIZE){
            tc_request_tx_packet[4] = 2;
          }else if(size == QQVGA_DATA_SIZE){
            tc_request_tx_packet[4] = 3;
          }
        }else{
          tc_request_tx_packet[4] = 0;
        }
        // Check the channel before transmitting.
        tc_request_state = LISTEN_ON_CHANNEL;
        break;
      }
      case LISTEN_ON_CHANNEL:{
        radio.set_rx_timeout_us(5000000);
        // Potentially receive a packet over the radio
        radio.receive();
        // Wait for radio response.
        tc_request_state = WAIT_LISTEN_ON_CHANNEL;
        break;
      }
      case WAIT_LISTEN_ON_CHANNEL:{
        if(radioFlags.rxDone == true){
          // Reset flag.
          radioFlags.rxDone = false;
          // Something is already transmitting on the channel. Exit with error:
          send_request_status = REQUEST_ERROR_1;
          exit = true;
          // Reset State
          tc_request_state = INIT;
        }else if(radioFlags.rxTimeout == true){
          // Reset flag.
          radioFlags.rxTimeout = false;
          // Central Hub is potentially available. Start transmission process.
          tc_request_state = SEND_PACKET;
        }else if(radioFlags.rxError == true){
          // Reset flag.
          radioFlags.rxError = false;
          // Central Hub is potentially available. Start transmission process.
          tc_request_state = SEND_PACKET;
        }
        break;
      }
      case SEND_PACKET:{
        // Send packet over the radio.
        radio.send(tc_request_tx_packet, REQUEST_PACKET_SIZE);
        // Wait for radio response.
        tc_request_state = WAIT_SEND_DONE;
        break;
      }
      case WAIT_SEND_DONE:{
        if(radioFlags.txDone){
          // Reset flag.
          radioFlags.txDone = false;
          // Start receive mode
          tc_request_state = RECEIVE_PACKET;
        }
        else if(radioFlags.txTimeout){
          // Reset flag.
          radioFlags.txTimeout = false;
          // Retry if max is not reached
          if(tx_retry_count < REQUEST_TX_RETRY_MAX){
            tx_retry_count++;
            tc_request_state = SEND_PACKET;
          }else{
            // Exit with error:
            send_request_status = REQUEST_ERROR_2;
            exit = true;
            // Reset State
            tc_request_state = INIT;
          }
        }
        break;
      }
      case RECEIVE_PACKET:{
        radio.set_rx_timeout_us(2000000);
        // Receive packet over the radio
        radio.receive();
        // Wait for radio response.
        tc_request_state = WAIT_RECEIVE_DONE;
        break;
      }
      case WAIT_RECEIVE_DONE:{
        if(radioFlags.rxDone == true){
          // Reset flag.
          radioFlags.rxDone = false;
          // If Central Hub is talking to this radio.
          if(((tc_request_grant_rx_packet[0] << 8) | tc_request_grant_rx_packet[1]) == radio_id){
            // If required, preform calibration testing.
            if(tc_request_grant_rx_packet[2] == REQUEST_GRANT_RX_PACKET_CALIBRATION_MASK){
              calibrate_radio();
            }
            // If transmitting data
            if(battery_enable == 0){
              uint8_t transmit_interval = tc_request_grant_rx_packet[3];

              send_request_status = tc_transmit_data(transmit_interval, size, data_input);
              exit = true;
              // Reset State
              tc_request_state = INIT;

            }else{
              send_request_status = REQUEST_SUCCESS;
              exit = true;
              // Reset State
              tc_request_state = INIT;
            }
          }else{
            // Central Hub is messaging a different radio.
            send_request_status = REQUEST_ERROR_1;
            exit = true;
            // Reset State
            tc_request_state = INIT;
          }
        }else if(radioFlags.rxTimeout == true){
          // Reset flag.
          radioFlags.rxTimeout = false;
          // Retry if max is not reached
          if(rx_retry_count < REQUEST_RX_RETRY_MAX){
            rx_retry_count++;
            tc_request_state = SEND_PACKET;
          }else{
            send_request_status = REQUEST_ERROR_3;
            exit = true;
            // Reset State
            tc_request_state = INIT;
          }
        }else if(radioFlags.rxError == true){
          // Reset flag.
          radioFlags.rxError = false;
          // Retry if max is not reached
          if(rx_retry_count < REQUEST_RX_RETRY_MAX){
            rx_retry_count++;
            tc_request_state = SEND_PACKET;
          }else{
            send_request_status = REQUEST_ERROR_3;
            exit = true;
            // Reset State
            tc_request_state = INIT;
          }
        }
        break;
      }
    }
    if(exit){
      break;
    }
  }
  return send_request_status;
}
uint8_t ch_request_data(uint8_t timeout_seconds, uint8_t *data_output){

  uint8_t send_request_grant_status = 0x0;
  bool exit = false;

  ch_request_state = INIT;

  // Reset all flags.
  radioFlags.txDone = false;
  radioFlags.rxDone = false;
  radioFlags.rxError = false;
  radioFlags.rxTimeout = false;
  radioFlags.txTimeout = false;

  uint16_t tc_id = 0;
  uint8_t tc_battery_enable = 0;
  uint8_t tc_battery_status = 0;
  uint8_t data_size = 0;

  while(1){
    switch(ch_request_state){
      case INIT:{
        // Set default radio configuration.
        set_radio_configuration_default(true);

        radio.set_rx_timeout_us(11000000);

        radio.receive();

        ch_request_state = WAIT_RECEIVE_DONE;
        break;
      }
      case WAIT_RECEIVE_DONE:{
        if(radioFlags.rxDone == true){
          // Reset flag.
          radioFlags.rxDone = false;
          // Extract data elements.
          tc_id = (ch_request_rx_packet[0] << 8) | ch_request_rx_packet[1];
          tc_battery_enable = ch_request_rx_packet[2];
          tc_battery_status = ch_request_rx_packet[3];
          data_size = ch_request_rx_packet[4];

          // Set request grant packet to send.
          ch_request_grant_tx_packet[0] = tc_id >> 8;
          ch_request_grant_tx_packet[1] = tc_id & 0x00FF;
          // Arbitrary statement, just to have diversity when testing.
          if(ch_request_rx_snr < 8){
            ch_request_grant_tx_packet[2] = REQUEST_GRANT_RX_PACKET_CALIBRATION_MASK;
          }else{
            ch_request_grant_tx_packet[2] = 0x0;
          }
          // Set the interval (number of packets) for data transmission.
          if(tc_battery_enable == 0){
            if(data_size == VGA_DATA_SIZE){
              ch_request_grant_tx_packet[3] = 10;
            }else if(data_size == QVGA_DATA_SIZE){
              ch_request_grant_tx_packet[3] = 10;
            }else if(data_size == QQVGA_DATA_SIZE){
              ch_request_grant_tx_packet[3] = 10;
            }
          }
          printf("Received Battery Status from device %d and has value of %d \r\n", tc_id, tc_battery_status);
          ch_request_state = SEND_PACKET;
        }else if(radioFlags.rxTimeout == true){
          // Reset flag.
          radioFlags.rxTimeout = false;
          // End after timeout.
          send_request_grant_status = REQUEST_GRANT_ERROR_1;
          exit = true;
          // Reset State
          ch_request_state = INIT;
        }else if(radioFlags.rxError == true){
          // Reset flag.
          radioFlags.rxError = false;
          // End after timeout.
          send_request_grant_status = REQUEST_GRANT_ERROR_1;
          exit = true;
          // Reset State
          ch_request_state = INIT;
        }
        break;
      }
      case SEND_PACKET:{
        // Delay so the trail camera has enough time to go into receiver mode.
        wait_us(1000000);
        // Send packet over the radio.
        radio.send(ch_request_grant_tx_packet, REQUEST_GRANT_PACKET_SIZE);
        // Wait for radio response.
        ch_request_state = WAIT_SEND_DONE;
        break;
      }
      case WAIT_SEND_DONE:{
        if(radioFlags.txDone){
          // Reset flag.
          radioFlags.txDone = false;
          if(tc_battery_enable == 0){
            // Start receiving data from the trail camera.
            rx_data_status = ch_recevie_data(tc_id, data_size, data_output);
          }
          // Exit with success.
          send_request_grant_status = REQUEST_GRANT_SUCCESS;
          exit = true;
          // Reset State
          ch_request_state = INIT;
        }
        else if(radioFlags.txTimeout){
          // Reset flag.
          radioFlags.txTimeout = false;
          // Exit with error:
          send_request_grant_status = REQUEST_GRANT_ERROR_2;
          // Reset State
          ch_request_state = INIT;
          exit = true;

        }
        break;
      }
    }
    if(exit){
      break;
    }
  }
  return send_request_grant_status;
}
uint8_t tc_transmit_data(uint8_t transmit_interval, uint32_t size, uint8_t *data_input){
  printf("tc_transmit_data \r\n");

  uint32_t bytes_left = size;
  uint8_t interval_counter = 0;
  uint32_t index = 0;
  bool exit = false;
  bool send_data_complete = false;

  uint8_t retry_counter = 0;
  uint16_t tc_id = 0;
  tc_data_state = SEND_PACKET;

  while(1){
    switch(tc_data_state){
      case SEND_PACKET:{
          if(interval_counter++ < transmit_interval){
            // ******* SEND PACKET *******

            // Set buffer size in radio.
            radio.set_max_payload_length(modem, DATA_PACKET_SIZE);
            // Send Device ID:
            tc_data_tx_packet[0] = (radio_id >> 8);
            tc_data_tx_packet[1] = (radio_id & 0x00FF);

            // Copy over elements from data_input into transmit buffer:
            if((index + DATA_PACKET_SIZE-2) < size){
              for(uint8_t i = 0; i < DATA_PACKET_SIZE-3; i++){
                tc_data_tx_packet[i+3] = data_input[index + i];
              }
              // Valid packet.
              tc_data_tx_packet[2] = 0;
            }else if(index >= size){
              send_data_complete = true;
              // Set unused values to 0xFF
              for(uint8_t i = 0; i < DATA_PACKET_SIZE-3; i++){
                tc_data_tx_packet[i+3] = 0xFF;
              }
              // Invalid packet.
              tc_data_tx_packet[2] = 1;
            }else{
              send_data_complete = true;
              // Have packet full of data.
              for(uint8_t i = 0; i < (index - size); i++){
                tc_data_tx_packet[i+3] = data_input[index + i];
              }
              // Set unused values to 0xFF
              for(uint8_t i = (index - size); i < DATA_PACKET_SIZE-3; i++){
                tc_data_tx_packet[i+3] = 0xFF;
              }
              // Valid packet.
              tc_data_tx_packet[2] = 2;
            }
            index += DATA_PACKET_SIZE-3;
            bytes_left -= DATA_PACKET_SIZE-3;

            // Send packet over the radio.
            radio.send(tc_data_tx_packet, REQUEST_PACKET_SIZE);
            // Wait for radio response.
            tc_data_state = WAIT_SEND_DONE;
          }else{
            // ******* READ PACKET *******
            // Listen for acknowledgement packet.
            tc_data_state = RECEIVE_PACKET;
          }
        break;
      }
      case RECEIVE_PACKET:{
        //
        radio.set_rx_timeout_us(1000000);
        // Set buffer size in radio.
        radio.set_max_payload_length(modem, DATA_ACK_PACKET_SIZE);
        // Receive packet over the radio
        radio.receive();
        // Wait for radio response.
        tc_data_state = WAIT_RECEIVE_DONE;
        break;
      }
      case WAIT_RECEIVE_DONE:{
        if(radioFlags.rxDone == true){
          // Reset flag.
          radioFlags.rxDone = false;
          // Process received packet.

          tc_id = (tc_data_ack_rx_packet[0] << 8) | tc_data_ack_rx_packet[1];
          if(tc_id != radio_id){
            return DATA_RX_ERROR_4;
          }

          switch(tc_data_ack_rx_packet[2]){
            // Resend burst.
            case DATA_ACK_TIMEOUT:{}
            case DATA_ACK_ERROR:{
              // Reset counters.
              if(retry_counter++ < DATA_RETRY_MAX){
                interval_counter = 0;
                index -= DATA_PACKET_SIZE-3;
                bytes_left += DATA_PACKET_SIZE-3;
                tc_data_state = SEND_PACKET;
              }else{
                return DATA_RX_ERROR_5;
              }
              break;
            }
            case DATA_ACK_SUCCESS:{
              retry_counter = 0;
              // Send next burst if there is more to send.
              if(!send_data_complete){
                interval_counter = 0;
                tc_data_state = SEND_PACKET;
              }else{
                return DATA_SUCCESS;
              }
              break;
            }
          }

        }else if(radioFlags.rxTimeout == true){
          // Reset flag.
          radioFlags.rxTimeout = false;
          // Did not receive acknowledgement. Link Terminated. (Do not send potentially duplicate data)
          return DATA_RX_ERROR_3;

        }else if(radioFlags.rxError == true){
          // Reset flag.
          radioFlags.rxError = false;
          // Link Terminated. (Do not send potentially duplicate data)
          return DATA_RX_ERROR_2;
        }
        break;
      }
      case WAIT_SEND_DONE:{
        if(radioFlags.txDone){
          // Reset flag.
          radioFlags.txDone = false;
          // Send Again
          tc_data_state = SEND_PACKET;
          // Add in a delay for the centralhub to process data.
          wait_us(1000);
        }
        else if(radioFlags.txTimeout){
          // Reset flag.
          radioFlags.txTimeout = false;
          // Resend packet over the radio.
          radio.send(tc_data_tx_packet, REQUEST_PACKET_SIZE);
          // Wait for radio response.
          tc_data_state = WAIT_SEND_DONE;
        }
        break;
      }
    }
  }
  return DATA_SUCCESS;
}
uint8_t ch_recevie_data(uint16_t device_id, uint16_t data_size, uint8_t *data_output){
  printf("tc_transmit_data \r\n");

  uint32_t bytes_left = data_size;
  uint8_t interval_counter = 0;
  uint32_t index = 0;
  bool exit = false;
  bool receive_data_complete = false;

  uint8_t retry_counter = 0;
  ch_rx_data_size = data_size;
  ch_data_state = RECEIVE_PACKET;
  uint16_t tc_id = 0;
  uint8_t data_ack_status = 0;


  if(data_size == 1){
    ch_rx_data_size = VGA_DATA_SIZE;
  }else if(data_size == 2){
    ch_rx_data_size = QVGA_DATA_SIZE;
  }else if(data_size == 3){
    ch_rx_data_size = QQVGA_DATA_SIZE;
  }

  uint8_t transmit_interval = 0;
  if(data_size == VGA_DATA_SIZE){
    transmit_interval = 10;
  }else if(data_size == QVGA_DATA_SIZE){
    transmit_interval = 10;
  }else if(data_size == QQVGA_DATA_SIZE){
    transmit_interval = 10;
  }

  while(1){
    switch(ch_data_state){
      case SEND_PACKET:{
        // Set buffer size in radio.
        radio.set_max_payload_length(modem, DATA_ACK_PACKET_SIZE);
        // Send Device ID:
        ch_data_ack_tx_packet[0] = (device_id >> 8);
        ch_data_ack_tx_packet[1] = (device_id & 0x00FF);
        // Send Status:
        ch_data_ack_tx_packet[2] = data_ack_status;

        // Send packet over the radio.
        radio.send(ch_data_ack_tx_packet, DATA_ACK_PACKET_SIZE);
        // Wait for radio response.
        ch_data_state = WAIT_SEND_DONE;

        break;
      }
      case RECEIVE_PACKET:{
        //
        radio.set_rx_timeout_us(1000000);
        // Set buffer size in radio.
        radio.set_max_payload_length(modem, DATA_PACKET_SIZE);
        // Receive packet over the radio
        radio.receive();
        // Wait for radio response.
        ch_data_state = WAIT_RECEIVE_DONE;
        break;
      }
      case WAIT_RECEIVE_DONE:{
        if(radioFlags.rxDone == true){
          // Reset flag.
          radioFlags.rxDone = false;

          // Process received packet.
          tc_id = (ch_data_rx_packet[0] << 8) | ch_data_rx_packet[1];
          if(tc_id != device_id){
            return DATA_RX_ERROR_4;
          }

          switch(ch_data_rx_packet[2]){
            // Resend burst.
            case DATA_FULL:{
              for(uint8_t i = 3; i < DATA_PACKET_SIZE; i++){
                data_output[index++] = ch_data_rx_packet[i];
              }
              break;
            }
            case DATA_HALF_FULL:{
              // Reset counters.
              for(uint8_t i = 3; i < (ch_rx_data_size - DATA_PACKET_SIZE) + 3; i++){
                data_output[index++] = ch_data_rx_packet[i];
              }
              receive_data_complete = true;
              break;
            }
            case DATA_EMPTY:{
              receive_data_complete = true;
              break;
            }
          }
          if(interval_counter++ == transmit_interval){
            // Send success ack
            data_ack_status = DATA_STATUS_SUCCESS;
            interval_counter = 0;
            ch_data_state = SEND_PACKET;
          }else{
            // Recieve next packet.
            ch_data_state = RECEIVE_PACKET;
          }

        }else if( (radioFlags.rxTimeout == true) || (radioFlags.rxError == true) ){
          // Reset flag.
          radioFlags.rxTimeout = false;
          // Send timeout/error ack when it is suppose to be sent.
          if(retry_counter++ < DATA_RETRY_MAX){
            data_ack_status = DATA_STATUS_TIMEOUT_ERROR;
            interval_counter = 0;
            // index = index - (interval_counter*(DATA_PACKET_SIZE-3));
            ch_data_state = SEND_PACKET;
          }else{
            return DATA_RX_FAILED_TIMEOUT;
          }
        }
        break;
      }
      case WAIT_SEND_DONE:{
        if(radioFlags.txDone){
          // Reset flag.
          radioFlags.txDone = false;
          // Receive Again
          if(!receive_data_complete){
            ch_data_state = RECEIVE_PACKET;
          }else{
            return DATA_TRANSFER_SUCCESSFUL;
          }
        }
        else if(radioFlags.txTimeout){
          // Reset flag.
          radioFlags.txTimeout = false;
          // Resend packet over the radio.
          radio.send(ch_data_ack_tx_packet, DATA_ACK_PACKET_SIZE);
          // Wait for radio response.
          ch_data_state = WAIT_SEND_DONE;
        }
        break;
      }
    }
  }
  return DATA_SUCCESS;
}


void On_tx_done(void){
  printf("tx_done. \r\n");
  radioFlags.txDone = true;
}
void On_rx_done(const uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr){
  printf("rx_done. \r\n");
  radioFlags.rxDone = true;
  printf("RSSI: %d SNR: %d Size: %d. \r\n", rssi, snr, size);

  switch(tc_request_state){
    case WAIT_RECEIVE_DONE:{
      // Copy over data elements from Central Hub.
      for(uint8_t i = 0; i < REQUEST_GRANT_PACKET_SIZE; i++){
        tc_request_grant_rx_packet[i] = payload[i];
      }
      // Save the rssi and the snr in case it is needed later.
      tc_request_grant_rx_rssi = rssi;
      tc_request_grant_rx_snr = snr;
      break;
    }
  }
  switch(ch_request_state){
    case WAIT_RECEIVE_DONE:{
      // Copy over data elements from Central Hub.
      for(uint8_t i = 0; i < REQUEST_PACKET_SIZE; i++){
        ch_request_rx_packet[i] = payload[i];
      }
      // Save the rssi and the snr in case it is needed later.
      ch_request_rx_rssi = rssi;
      ch_request_rx_snr = snr;
      break;
    }
  }
  switch(tc_data_state){
    case WAIT_RECEIVE_DONE:{
      // Copy over data elements from Central Hub.
      for(uint8_t i = 0; i < DATA_ACK_PACKET_SIZE; i++){
        tc_data_ack_rx_packet[i] = payload[i];
      }
      break;
    }
  }
  switch(ch_data_state){
    case WAIT_RECEIVE_DONE:{
      // Copy over data elements from Central Hub.
      for(uint8_t i = 0; i < DATA_PACKET_SIZE; i++){
        ch_data_rx_packet[i] = payload[i];
      }
      break;
    }
  }



}
void On_tx_timeout(void){
  printf("tx_timeout. \r\n");
  radioFlags.txTimeout = true;
}
void On_rx_timeout(void){
  printf("rx_timeout. \r\n");
  radioFlags.rxTimeout = true;
}
void On_rx_error(void){
  printf("rx_error. \r\n");
  radioFlags.rxError = true;
}
void On_fhss_change_channel(uint8_t current_channel){
  printf("fhss_change_channel. \r\n");
}
void On_cad_done(bool channel_busy){
  printf("cad_done. \r\n");
}

static radio_events_t radio_events = {
  .tx_done = &On_tx_done,
  .tx_timeout = &On_tx_timeout,
  .rx_done = &On_rx_done,
  .rx_timeout = &On_rx_timeout,
  .rx_error = &On_rx_error,
  .fhss_change_channel = &On_fhss_change_channel,
  .cad_done = &On_cad_done,
};

void set_rx_settings_lora(uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint32_t bandwidth_afc, uint16_t preamble_len, uint16_t symb_timeout, bool fix_len, uint8_t payload_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, bool rx_continuous){
  modem = MODEM_LORA;
  rx_bandwidth = bandwidth;
  rx_datarate = datarate;
  rx_coderate = coderate;
  rx_preamble_len = preamble_len;
  rx_fix_len = fix_len;
  rx_crc_on = crc_on;
  rx_freq_hop_on = freq_hop_on;
  rx_iq_inverted = iq_inverted;

  rx_bandwidth_afc = 0;
  rx_symb_timeout = symb_timeout;
  rx_payload_len = payload_len;
  rx_hop_period = hop_period;
  rx_rx_continuous = rx_continuous;
}
void set_tx_settings_lora(int8_t power, uint32_t fdev, uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint16_t preamble_len, bool fix_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, uint32_t timeout){
  modem = MODEM_LORA;
  tx_bandwidth = bandwidth;
  tx_datarate = datarate;
  tx_coderate = coderate;
  tx_preamble_len = preamble_len;
  tx_fix_len = fix_len;
  tx_crc_on = crc_on;
  tx_freq_hop_on = freq_hop_on;
  tx_iq_inverted = iq_inverted;

  tx_power = power;
  tx_hop_period = hop_period;
  tx_timeout_val = timeout;
  tx_fdev = 0;
}
void set_rx_settings_fsk(uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint32_t bandwidth_afc, uint16_t preamble_len, uint16_t symb_timeout, bool fix_len, uint8_t payload_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, bool rx_continuous){
  modem = MODEM_FSK;
  rx_bandwidth = bandwidth;
  rx_datarate = datarate;
  rx_coderate = 0;
  rx_preamble_len = 0;
  rx_fix_len = fix_len;
  rx_crc_on = crc_on;
  rx_freq_hop_on = false;
  rx_iq_inverted = false;

  rx_bandwidth_afc = bandwidth_afc;
  rx_symb_timeout = symb_timeout;
  rx_payload_len = payload_len;
  rx_hop_period = 0;
  rx_rx_continuous = rx_continuous;
}
void set_tx_settings_fsk(int8_t power, uint32_t fdev, uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint16_t preamble_len, bool fix_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, uint32_t timeout){
  modem = MODEM_FSK;
  tx_bandwidth = 0;
  tx_datarate = datarate;
  tx_coderate = 0;
  tx_preamble_len = preamble_len;
  tx_fix_len = fix_len;
  tx_crc_on = crc_on;
  tx_freq_hop_on = false;
  tx_iq_inverted = false;

  tx_power = power;
  tx_hop_period = 0;
  tx_timeout_val = timeout;
  tx_fdev = fdev;
}
void set_radio_configuration_default(bool lora_or_fsk){
  using_lora = lora_or_fsk;

  if(using_lora == 1){
    set_rx_settings_lora(2, 10, 1, 0, 12, 1, false, 4, true, true, 1, false, false);
    set_tx_settings_lora(14, 0, 2, 10, 1, 12, false, true, true, 1, false, 1000);
    radio.set_rx_timeout_us(10000000);
    radio.set_rx_config(modem, rx_bandwidth, rx_datarate, rx_coderate, rx_bandwidth_afc, rx_preamble_len, rx_symb_timeout, rx_fix_len, rx_payload_len, rx_crc_on, rx_freq_hop_on, rx_hop_period, rx_iq_inverted, rx_rx_continuous);
    radio.set_tx_config(modem, tx_power, tx_fdev, tx_bandwidth, tx_datarate, tx_coderate, tx_preamble_len, tx_fix_len, tx_crc_on, tx_freq_hop_on, tx_hop_period, tx_iq_inverted, tx_timeout_val);

    radio.set_max_payload_length(modem, 4);
    radio.set_public_network(false);
    radio.set_channel(915000000);

  }else if(using_lora == 0){
    set_rx_settings_fsk(50000, 300000, 0, 50000, 0, 10, true, 255, true, false, 0, false, 0);
    set_tx_settings_fsk(14, 5000, 0, 30000, 0, 1, true, true, false, 0, false, 1000);
    radio.set_rx_timeout_us(10000000);
    radio.set_rx_config(modem, rx_bandwidth, rx_datarate, rx_coderate, rx_bandwidth_afc, rx_preamble_len, rx_symb_timeout, rx_fix_len, rx_payload_len, rx_crc_on, rx_freq_hop_on, rx_hop_period, rx_iq_inverted, rx_rx_continuous);
    radio.set_tx_config(modem, tx_power, tx_fdev, tx_bandwidth, tx_datarate, tx_coderate, tx_preamble_len, tx_fix_len, tx_crc_on, tx_freq_hop_on, tx_hop_period, tx_iq_inverted, tx_timeout_val);

    radio.set_max_payload_length(modem, 255);
    radio.set_channel(915000000);

  }else{
    printf("Error. 'set_radio_configuration' Input not defined correctly.\r\n");
  }
}

int main(void){
  printf("int main(). \r\n");
  bool is_trail_camera = 1;

  radio.radio_reset();
  radio.init_radio(&radio_events);

  while(1){
    serial_port.read(inputBuffer, 1);
    switch (inputBuffer[0]){
      // Continuous receive for 15 seconds.
      case 'b':
        led1 = 1;
        if(is_trail_camera){
          uint8_t result = send_battery_status(1, 34);
          printf("send battery status result: %d \r\n", result);
        }
        led1 = 0;
        break;

      case 'd':
        led2 = 1;
        if(is_trail_camera){
          uint8_t result2 = send_data(0, 0, QQVGA_DATA_SIZE, frame_qqvga);
          printf("send_data result: %d \r\n", result2);
        }
        led2 = 0;
        break;

      case 'c':
        led2 = 1;
        if(!is_trail_camera){
          uint8_t result2 = ch_request_data(15, ch_rx_data);
          printf("ch_request_data result: %d \r\n", result2);
        }
        led2 = 0;
        break;

      case 's':
        led3 = 1;
        is_trail_camera = !is_trail_camera;
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
