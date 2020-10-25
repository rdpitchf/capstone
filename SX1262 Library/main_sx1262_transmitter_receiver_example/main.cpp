
#include "mbed.h"
#include "SX126X_LoRaRadio.h"

#define DEVICE_SPI 1
#define MBED_LORA_RADIO_DRV_SX126X_LORARADIO_H_


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

// SPI_MOSI    = D11,
// SPI_MISO    = D12,
// SPI_SCK     = D13,
// SPI_CS      = D10,

// static PinName mosi(PD_4);
// static PinName miso(PD_3);
// static PinName sclk(PD_1);
// static PinName nss(PD_0);
static PinName reset(PG_0);
static PinName dio1(PG_1);
static PinName busy(PF_9);
static PinName freq_select(PC_5);
static PinName device_select(PC_4);
static PinName crystal_select(PE_3);
static PinName ant_switch(PE_6);

static SX126X_LoRaRadio radio(mosi, miso, sclk, nss, reset, dio1, busy, freq_select, device_select, crystal_select, ant_switch);
bool using_lora = false;

void On_tx_done(void){
  printf("tx_done. \r\n");
}
void On_rx_done(const uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr){
  printf("rx_done. \r\n");
  printf("RSSI: %d SNR: %d Size: %d. \r\n", rssi, snr, size);
  // uint8_t *get_data = payload;
  uint8_t valid = 0;
  uint8_t invalid = 0;
  for(int i = 0; i < size; i++){
    if(payload[i] == 72){
      valid++;
    }else{
      invalid++;
    }
  }
  printf("valid: %d invalid: %d Total: %d. \r\n", valid, invalid, (valid+invalid));
  for(int i = 0; i < size; i++){
    printf("%d,", payload[i]);
  }
  printf("\r\n");
}
void On_tx_timeout(void){
  printf("tx_timeout. \r\n");
}
void On_rx_timeout(void){
  printf("rx_timeout. \r\n");
}
void On_rx_error(void){
  printf("rx_error. \r\n");
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

void set_rx_settings_lora(uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint32_t bandwidth_afc, uint16_t preamble_len, uint16_t symb_timeout, bool fix_len, uint8_t payload_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, bool rx_continuous);
void set_tx_settings_lora(int8_t power, uint32_t fdev, uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint16_t preamble_len, bool fix_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, uint32_t timeout);
void set_rx_settings_fsk(uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint32_t bandwidth_afc, uint16_t preamble_len, uint16_t symb_timeout, bool fix_len, uint8_t payload_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, bool rx_continuous);
void set_tx_settings_fsk(int8_t power, uint32_t fdev, uint32_t bandwidth, uint32_t datarate, uint8_t coderate, uint16_t preamble_len, bool fix_len, bool crc_on, bool freq_hop_on, uint8_t hop_period, bool iq_inverted, uint32_t timeout);
void set_radio_configuration_default(bool lora_or_fsk);

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
    set_rx_settings_lora(2, 10, 1, 0, 12, 1, true, 255, false, true, 1, false, true);
    set_tx_settings_lora(14, 0, 2, 10, 1, 12, true, false, true, 1, false, 10000);
    radio.set_rx_config(modem, rx_bandwidth, rx_datarate, rx_coderate, rx_bandwidth_afc, rx_preamble_len, rx_symb_timeout, rx_fix_len, rx_payload_len, rx_crc_on, rx_freq_hop_on, rx_hop_period, rx_iq_inverted, rx_rx_continuous);
    radio.set_tx_config(modem, tx_power, tx_fdev, tx_bandwidth, tx_datarate, tx_coderate, tx_preamble_len, tx_fix_len, tx_crc_on, tx_freq_hop_on, tx_hop_period, tx_iq_inverted, tx_timeout_val);

    radio.set_max_payload_length(modem, 255);
    radio.set_public_network(false);
    radio.set_channel(915000000);

  }else if(using_lora == 0){
    set_rx_settings_fsk(50000, 300000, 0, 50000, 0, 10, true, 255, true, false, 0, false, 0);
    set_tx_settings_fsk(14, 5000, 0, 30000, 0, 1, true, true, false, 0, false, 10000);
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
  bool is_recevier = 0;

  radio.radio_reset();
  radio.init_radio(&radio_events);
  set_radio_configuration_default(true);


  uint8_t buffer[255];
  for(int i = 0; i < 255; i++){
    buffer[i] = 'H';
  }

  while(1){
    serial_port.read(inputBuffer, 1);
    switch (inputBuffer[0]){
      // Continuous receive for 15 seconds.
      case 'r':
        led1 = 1;
        if(is_recevier){
          printf("Start Receive. \r\n");
          radio.receive();
          printf("Stop Receive. \r\n");
        }
        led1 = 0;
        break;

      case 's':
        led3 = 1;
        is_recevier = !is_recevier;
        ThisThread::sleep_for(1000);
        led3 = 0;
        break;
      // Continuous transmit for 15 seconds.
      case 't':
        led2 = 1;

        if(!is_recevier){
          printf("Start Transmission. \r\n");
          for(int i = 0; i < 1; i++){
            radio.send(buffer, 255);
          }
          printf("Stop Transmission. \r\n");
        }
        ThisThread::sleep_for(1000);
        led2 = 0;
        break;

      case 'p':
        led1 = 1;
        led2 = 1;
        if(is_recevier){
          printf("Hello World Receiver Mode\r\n");
        }else{
          printf("Hello World Transmitter Mode\r\n");
        }
        ThisThread::sleep_for(1000);
        led1 = 0;
        led2 = 0;
        break;
    }
  }
}
