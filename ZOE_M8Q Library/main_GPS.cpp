
#include "mbed.h"
#include "millis.h"
// #include "SparkFun_Ublox_Arduino_Library.h"
#include "GPS.h"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
static DigitalOut led1(LED1);
static DigitalOut led2(LED2);
static DigitalOut led3(LED3);

char inputBuffer[1];

// MUST INCLUDE mbed-os/storage/filesystem/ WHEN COMPILING
FileHandle *mbed::mbed_override_console(int fd){
    return &serial_port;
}

gps myGPS;
I2C test(PC_9,PC_0);

void setup(){

  printf("SparkFun Ublox Example\r\n");

  // myGPS.enableDebugging(false);
  // test.frequency(400000);

  //Connect to the Ublox module using Wire port
  if (myGPS.begin(test, 0x42 << 1) != false){
    printf("Ublox GPS not detected at default I2C address. Please check wiring. Freezing.\r\n");
    while (1);
  }

  myGPS.setI2COutput(COM_TYPE_UBX, 0); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGPS.saveConfiguration(0); //Save the current settings to flash and BBR
}

void loop(){
  //Query module only every second. Doing it more often will just cause I2C traffic.
  //The module only responds when a new position is available
  printf("Starting Loop.\r\n");
  while(1){
    // int32_t latitude = myGPS.getLatitude();
    // int32_t longitude = myGPS.getLongitude();
    // int32_t altitude = myGPS.getAltitude();
    // uint32_t tow = myGPS.getTimeOfWeek();
    // uint16_t yy = myGPS.getYear();
    // uint8_t mm = myGPS.getMonth();
    // uint8_t dd = myGPS.getDay();
    // uint8_t hh = myGPS.getHour();
    // uint8_t m = myGPS.getMinute();
    // uint8_t ss = myGPS.getSecond();
    // printf("1 Lat: %d Long: %d Alt: %d tow: %u Date: %d %d %d, %d %d %d\r\n", latitude, longitude, altitude, tow, yy, mm, dd, hh, m, ss);

    position_time_date_t a = myGPS.position_time_date();
    printf("2 Lat: %d Long: %d Alt: %d tow: %u Date: %d %d %d, %d %d %d\r\n", a.lat, a.lon, a.alt, a.tow, a.Year, a.Month, a.Day, a.Hour, a.Minute, a.Second);

    ThisThread::sleep_for(1000);
  }
}

int main() {
  Ticker flipper;
  flipper.attach_us(&ticker_SysTick_Handler, 1000);
  printf("int main(). \r\n");
  setup();
  loop();
}
