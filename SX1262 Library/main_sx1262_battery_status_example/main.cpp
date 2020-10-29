
#include "mbed.h"
#include "SX126X_LoRaRadio.h"

// #define DEVICE_SPI 1
// #define MBED_LORA_RADIO_DRV_SX126X_LORARADIO_H_

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
//
// typedef enum{
//     SEND_PACKET,
//     WAIT_SEND_DONE,
//     RECEIVE_PACKET,
//     WAIT_RECEIVE_DONE,
//     INIT,
//     LISTEN_ON_CHANNEL,
//     WAIT_LISTEN_ON_CHANNEL,
// }central_hub_request_states_t;

// Define packet sizes.
#define REQUEST_PACKET_SIZE 4
#define REQUEST_GRANT_PACKET_SIZE 3

// Radio identification number. (uint16_t)
uint16_t radio_id = 0x10AF;


// ************** For Trail Camera **************
// Channel was already busy
#define REQUEST_ERROR_CODE_1 0x1
// Transmitter Timeout
#define REQUEST_ERROR_CODE_2 0x2
// Receiver Timeout
#define REQUEST_ERROR_CODE_3 0x3
// Successful request.
#define REQUEST_SUCCESS_CODE 0xF

#define REQUEST_TX_RETRY_MAX 3
#define REQUEST_RX_RETRY_MAX 3
#define REQUEST_GRANT_RX_PACKET_CALIBRATION_MASK 0x6

uint8_t request_tx_packet[REQUEST_PACKET_SIZE] {0};
uint8_t request_grant_rx_packet[REQUEST_GRANT_PACKET_SIZE] {0};
int16_t request_grant_rx_rssi = 0;
int8_t request_grant_rx_snr = 0;
volatile request_states_t request_state = INIT;
// ************** For Trail Camera **************



// ************** For Central Hub **************
// Rx timeout.
#define REQUEST_GRANT_ERROR_CODE_1 0x1
// Tx timeout.
#define REQUEST_GRANT_ERROR_CODE_2 0x2
// Tx Successful
#define REQUEST_GRANT_SUCCESS_CODE 0xF

uint8_t central_hub_request_rx_packet[REQUEST_PACKET_SIZE] {0};
uint8_t central_hub_request_grant_tx_packet[REQUEST_GRANT_PACKET_SIZE] {0};
int16_t central_hub_request_rx_rssi = 0;
int8_t central_hub_request_rx_snr = 0;
volatile request_states_t central_hub_request_state = INIT;
// ************** For Central Hub **************


RadioFlags_t radioFlags = {
    .rxDone = false,
    .rxError = false,
    .txDone = false,
    .rxTimeout = false,
    .txTimeout = false,
};

void set_rx_settings_lora(uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint32_t bandwidth_afc, uint16_t preamble_len, uint16_t symb_timeout, bool fix_len, uint8_t payload_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, bool rx_continuous);
void set_tx_settings_lora(int8_t power, uint32_t fdev, uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint16_t preamble_len, bool fix_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, uint32_t timeout);
void set_rx_settings_fsk(uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint32_t bandwidth_afc, uint16_t preamble_len, uint16_t symb_timeout, bool fix_len, uint8_t payload_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, bool rx_continuous);
void set_tx_settings_fsk(int8_t power, uint32_t fdev, uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint16_t preamble_len, bool fix_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, uint32_t timeout);
void set_radio_configuration_default(bool lora_or_fsk);

uint8_t send_request_packet(uint8_t battery_enable, uint8_t battery_status);
uint8_t send_battery_status(uint8_t battery_enable, uint8_t battery_status);
uint8_t send_data(uint8_t battery_enable, uint8_t battery_status, uint8_t *data_input, uint8_t size);
uint8_t calibrate_radio();
uint8_t central_hub_request_data(uint8_t timeout_seconds);

uint8_t send_battery_status(uint8_t battery_enable, uint8_t battery_status){
  return send_request_packet(battery_enable, battery_status);
}
uint8_t send_data(uint8_t battery_enable, uint8_t battery_status, uint8_t *data_input, uint8_t size){
  uint8_t request_output = send_request_packet(battery_enable, battery_status);
  if(request_output != REQUEST_SUCCESS_CODE){
    return request_output;
  }else{
    // Start sending data to the central Hub
    printf("Start Sending data. \r\n");
  }
}
uint8_t calibrate_radio(){
  // Calibration process.
  printf("calibrate_radio()\r\n");
  return REQUEST_SUCCESS_CODE;
}
uint8_t send_request_packet(uint8_t battery_enable, uint8_t battery_status){
  uint8_t send_request_status = 0x0;
  bool exit = false;

  request_state = INIT;

  uint8_t request_tx_packet_upper = (radio_id >> 8);
  uint8_t request_tx_packet_lower = (radio_id & 0x00FF);
  uint8_t rx_retry_count = 0;
  uint8_t tx_retry_count = 0;

  // Reset all flags.
  radioFlags.txDone = false;
  radioFlags.rxDone = false;
  radioFlags.rxError = false;
  radioFlags.rxTimeout = false;
  radioFlags.txTimeout = false;

  while(1){
    switch(request_state){
      case INIT:{
        // Set default radio configuration.
        set_radio_configuration_default(true);
        // Set the battery level in the battery packet.
        request_tx_packet[0] = request_tx_packet_upper;
        request_tx_packet[1] = request_tx_packet_lower;
        request_tx_packet[2] = battery_enable;
        request_tx_packet[3] = battery_status;
        // Check the channel before transmitting.
        request_state = LISTEN_ON_CHANNEL;
        break;
      }
      case LISTEN_ON_CHANNEL:{
        radio.set_rx_timeout_us(5000000);
        // Potentially receive a packet over the radio
        radio.receive();
        // Wait for radio response.
        request_state = WAIT_LISTEN_ON_CHANNEL;
        break;
      }
      case WAIT_LISTEN_ON_CHANNEL:{
        if(radioFlags.rxDone == true){
          // Reset flag.
          radioFlags.rxDone = false;
          // Something is already transmitting on the channel. Exit with error:
          send_request_status = REQUEST_ERROR_CODE_1;
          exit = true;
          // Reset State
          request_state = INIT;
        }
        else if(radioFlags.rxTimeout == true){
          // Reset flag.
          radioFlags.rxTimeout = false;
          // Central Hub is potentially available. Start transmission process.
          request_state = SEND_PACKET;
        }
        break;
      }
      case SEND_PACKET:{
        // Send packet over the radio.
        radio.send(request_tx_packet, REQUEST_PACKET_SIZE);
        // Wait for radio response.
        request_state = WAIT_SEND_DONE;
        break;
      }
      case WAIT_SEND_DONE:{
        if(radioFlags.txDone){
          // Reset flag.
          radioFlags.txDone = false;
          // Start receive mode
          request_state = RECEIVE_PACKET;
        }
        else if(radioFlags.txTimeout){
          // Reset flag.
          radioFlags.txTimeout = false;
          // Retry if max is not reached
          if(tx_retry_count < REQUEST_TX_RETRY_MAX){
            tx_retry_count++;
            request_state = SEND_PACKET;
          }else{
            // Exit with error:
            send_request_status = REQUEST_ERROR_CODE_2;
            exit = true;
            // Reset State
            request_state = INIT;
          }
        }
        break;
      }
      case RECEIVE_PACKET:{
        radio.set_rx_timeout_us(2000000);
        // Receive packet over the radio
        radio.receive();
        // Wait for radio response.
        request_state = WAIT_RECEIVE_DONE;
        break;
      }
      case WAIT_RECEIVE_DONE:{
        if(radioFlags.rxDone == true){
          // Reset flag.
          radioFlags.rxDone = false;
          // If Central Hub is talking to this radio.
          if(((request_grant_rx_packet[0] << 8) | request_grant_rx_packet[1]) == radio_id){
            // If required, preform calibration testing.
            if(request_grant_rx_packet[2] == REQUEST_GRANT_RX_PACKET_CALIBRATION_MASK){
              send_request_status = calibrate_radio();
              exit = true;
              // Reset State
              request_state = INIT;
            }else{
              send_request_status = REQUEST_SUCCESS_CODE;
              exit = true;
              // Reset State
              request_state = INIT;
            }
          }else{
            // Central Hub is messaging a different radio.
            send_request_status = REQUEST_ERROR_CODE_1;
            exit = true;
            // Reset State
            request_state = INIT;
          }
        }
        else if(radioFlags.rxTimeout == true){
          // Reset flag.
          radioFlags.rxTimeout = false;
          // Retry if max is not reached
          if(rx_retry_count < REQUEST_RX_RETRY_MAX){
            rx_retry_count++;
            request_state = SEND_PACKET;
          }else{
            send_request_status = REQUEST_ERROR_CODE_3;
            exit = true;
            // Reset State
            request_state = INIT;
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
uint8_t central_hub_request_data(uint8_t timeout_seconds){

  uint8_t send_request_grant_status = 0x0;
  bool exit = false;

  central_hub_request_state = INIT;

  // Reset all flags.
  radioFlags.txDone = false;
  radioFlags.rxDone = false;
  radioFlags.rxError = false;
  radioFlags.rxTimeout = false;
  radioFlags.txTimeout = false;

  uint16_t trail_camera_id = 0;
  uint8_t trail_camera_battery_enable = 0;
  uint8_t trail_camera_battery_status = 0;


  while(1){
    switch(central_hub_request_state){
      case INIT:{
        // Set default radio configuration.
        set_radio_configuration_default(true);

        radio.set_rx_timeout_us(11000000);

        radio.receive();

        central_hub_request_state = WAIT_RECEIVE_DONE;
        break;
      }
      case WAIT_RECEIVE_DONE:{
        if(radioFlags.rxDone == true){
          // Reset flag.
          radioFlags.rxDone = false;
          // Extract data elements.
          trail_camera_id = (central_hub_request_rx_packet[0] << 8) | central_hub_request_rx_packet[1];
          trail_camera_battery_enable = central_hub_request_rx_packet[2];
          trail_camera_battery_status = central_hub_request_rx_packet[3];

          // Set request grant packet to send.
          central_hub_request_grant_tx_packet[0] = trail_camera_id >> 8;
          central_hub_request_grant_tx_packet[1] = trail_camera_id & 0x00FF;
          // Arbitrary statement, just to have diversity when testing.
          if(central_hub_request_rx_snr < 8){
            central_hub_request_grant_tx_packet[2] = REQUEST_GRANT_RX_PACKET_CALIBRATION_MASK;
          }else{
            central_hub_request_grant_tx_packet[2] = 0x0;
          }
          printf("Received Battery Status from device %d and has value of %d \r\n", trail_camera_id, trail_camera_battery_status);
          central_hub_request_state = SEND_PACKET;
        }
        else if(radioFlags.rxTimeout == true){
          // Reset flag.
          radioFlags.rxTimeout = false;
          // End after timeout.
          send_request_grant_status = REQUEST_GRANT_ERROR_CODE_1;
          exit = true;
          // Reset State
          central_hub_request_state = INIT;
        }
        break;
      }
      case SEND_PACKET:{
        // Delay so the trail camera has enough time to go into receiver mode.
        wait_us(1000000);
        // Send packet over the radio.
        radio.send(central_hub_request_grant_tx_packet, REQUEST_GRANT_PACKET_SIZE);
        // Wait for radio response.
        central_hub_request_state = WAIT_SEND_DONE;
        break;
      }
      case WAIT_SEND_DONE:{
        if(radioFlags.txDone){
          // Reset flag.
          radioFlags.txDone = false;
          if(trail_camera_battery_enable == 1){
            // Start receiving data from the trail camera.
            printf("Start Receiving data. \r\n");
          }
          // Exit with success.
          send_request_grant_status = REQUEST_GRANT_SUCCESS_CODE;
          exit = true;
          // Reset State
          central_hub_request_state = INIT;
        }
        else if(radioFlags.txTimeout){
          // Reset flag.
          radioFlags.txTimeout = false;
          // Exit with error:
          send_request_grant_status = REQUEST_GRANT_ERROR_CODE_2;
          // Reset State
          central_hub_request_state = INIT;
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

void On_tx_done(void){
  printf("tx_done. \r\n");
  radioFlags.txDone = true;
}
void On_rx_done(const uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr){
  printf("rx_done. \r\n");
  radioFlags.rxDone = true;
  printf("RSSI: %d SNR: %d Size: %d. \r\n", rssi, snr, size);

  switch(request_state){
    case WAIT_RECEIVE_DONE:{
      // Copy over data elements from Central Hub.
      for(uint8_t i = 0; i < REQUEST_GRANT_PACKET_SIZE; i++){
        request_grant_rx_packet[i] = payload[i];
      }
      // Save the rssi and the snr in case it is needed later.
      request_grant_rx_rssi = rssi;
      request_grant_rx_snr = snr;
      break;
    }
  }
  switch(central_hub_request_state){
    case WAIT_RECEIVE_DONE:{
      // Copy over data elements from Central Hub.
      for(uint8_t i = 0; i < REQUEST_PACKET_SIZE; i++){
        central_hub_request_rx_packet[i] = payload[i];
      }
      // Save the rssi and the snr in case it is needed later.
      central_hub_request_rx_rssi = rssi;
      central_hub_request_rx_snr = snr;
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

      case 'c':
        led2 = 1;
        if(!is_trail_camera){
          uint8_t result2 = central_hub_request_data(15);
          printf("central_hub_request_data result: %d \r\n", result2);
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
