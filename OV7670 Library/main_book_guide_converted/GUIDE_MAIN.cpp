
#include "mbed.h"
#include <string>
#include <algorithm>
#include <cctype>
#include "GUIDE_DEFINE_REGISTERS.cpp"
#include "OV7670_SCCB.cpp"

static int PhotoTakenCount = 0;
// Serial Input
const int BUFFERLENGTH = 255;
static char IncomingByte[BUFFERLENGTH]; // for incoming serial data
// VGA Default
static int PHOTO_WIDTH = 640;
static int PHOTO_HEIGHT = 480;
static int PHOTO_BYTES_PER_PIXEL = 2;
// Command and Parameter related strings
static string RawCommandLine = "";
static string Command = "QQVGA";
static string FPSParam = "ThirtyFPS";
static string AWBParam = "SAWB";
static string AECParam = "HistAEC";
static string YUVMatrixParam = "YUVMatrixOn";
static string DenoiseParam = "DenoiseNo";
static string EdgeParam = "EdgeNo";
static string ABLCParam = "AblcON";

enum ResolutionType
{
  VGA,
  VGAP,
  QVGA,
  QQVGA,
  None
};

static ResolutionType Resolution = None;

// Camera input/output pin connection to Arduino
static DigitalOut wrst(PD_15); // Output Write Pointer Reset
static DigitalOut rrst(PF_12); // Output Read Pointer Reset
static DigitalOut wen(PA_5); // Output Write Enable
static DigitalIn vsync(PA_6); // Input Vertical Sync marking frame capture
static DigitalOut rclk(PD_14); // Output FIFO buffer output clock
// set OE to low gnd
// FIFO Ram input pins
static DigitalIn d0(PD_9);
static DigitalIn d1(PD_8);
static DigitalIn d2(PF_15);
static DigitalIn d3(PE_13);
static DigitalIn d4(PF_14);
static DigitalIn d5(PE_11);
static DigitalIn d6(PE_9);
static DigitalIn d7(PF_13);
// I2C
#define OV7670_I2C_ADDRESS 0x21
#define I2C_ERROR_WRITING_START_ADDRESS 11
#define I2C_ERROR_WRITING_DATA 22
#define DATA_TOO_LONG 1 // data too long to fit in transmit buffer
#define NACK_ON_TRANSMIT_OF_ADDRESS 2 // received NACK on transmit of address
#define NACK_ON_TRANSMIT_OF_DATA 3 // received NACK on transmit of data
#define OTHER_ERROR 4 // other error
#define I2C_READ_START_ADDRESS_ERROR 33
#define I2C_READ_DATA_SIZE_MISMATCH_ERROR 44
// Use for serial communication
static BufferedSerial serial_port(USBTX, USBRX, 115200);
static char inputBuffer[1];

static OV7670_SCCB sccb;

void ResetCameraRegisters();
uint8_t OV7670WriteReg(uint8_t reg, uint8_t data);
uint8_t OV7670Write(uint8_t start, uint8_t pData, uint8_t size);
uint8_t ReadRegisterValue(uint8_t RegisterAddress);
string ParseI2CResult(uint8_t result);
void ReadRegisters();
void InitializeOV7670Camera();
void PulseLowEnabledPin(DigitalOut pin, uint8_t DurationMicroSecs);
void PulsePin(DigitalOut pin, uint8_t DurationMicroSecs);
void DisplayHelpMenu();
void DisplayHelpCommandsParams();
void DisplayCurrentCommand();
void ExecuteCommand(string Command);
void SetCameraFPSMode();
void SetupCameraFor30FPS();
void SetupCameraNightMode();
void SetCameraAEC();
void SetupCameraAverageBasedAECAGC();
void SetCameraHistogramBasedAECAGC();
void SetCameraSaturationControl();
void SetupCameraArrayControl();
void SetupCameraADCControl();
void SetupCameraABLC();
void SetupCameraAWB();
void SetupCameraSimpleAutomaticWhiteBalance();
void SetupCameraGain();
void SetupCameraAdvancedAutomaticWhiteBalance();
void SetupCameraAdvancedAutoWhiteBalanceConfig();
void SetupCameraDenoiseEdgeEnhancement();
void SetupCameraDenoise();
void SetupCameraEdgeEnhancement();
void SetupCameraUndocumentedRegisters();
void SetCameraColorMatrixYUV();
void TakePhoto();
unsigned long pulseIn(DigitalIn pin);
void CaptureOV7670Frame();
void ReadTransmitCapturedFrame();
string CreatePhotoFilename();
void CheckRemoveFile(string Filename);
uint8_t ConvertPinValueToByteValue(int PinValue, int PinPosition);
void CreatePhotoInfoFile();
string CreatePhotoInfoFilename();
string CreatePhotoInfo();
void ReadPrintFile(string Filename);
void WriteFileTest(string Filename);
void ParseRawCommand(string RawCommandLine);
int ParseCommand(const char* commandline, char splitcharacter, string* Result);
bool ProcessRawCommandElement(string Element);
void SetupOV7670ForVGAProcessedBayerRGB();
void SetupOV7670ForVGARawRGB();
void SetupOV7670ForQVGAYUV();
void SetupOV7670ForQQVGAYUV();

int main(){
  printf("Arduino SERIAL_MONITOR_CONTROLLED CAMERA Using ov7670 Camera \r\n");

  printf("---------- Camera Registers ----------\r\n");
  ResetCameraRegisters();
  printf("---------- ResetCameraRegisters Completed ----------\r\n");
  ReadRegisters();
  printf("-------------------------\r\n");
  InitializeOV7670Camera();
  printf("FINISHED INITIALIZING CAMERA \r\n \r\n");

  // Serial Input
  const int BUFFERLENGTH = 255;
  char IncomingByte[BUFFERLENGTH]; // for incoming serial data
  string RawCommandLine = "";
  while(1){
    // Wait for Command
    printf("Ready to Accept new Command => \r\n");
    while (1){
      serial_port.read(inputBuffer, 1);
      if(inputBuffer[0] == '\r'){
        break;
      }else{
        printf("Here \r\n");
        RawCommandLine += inputBuffer[0];
      }
    }

    // Print out the command from Android
    printf("Raw Command from Serial Monitor: ");
    printf("%s \r\n",RawCommandLine.c_str());
    if ((RawCommandLine == "h")||(RawCommandLine == "help")){
      DisplayHelpMenu();
    }else if (RawCommandLine == "help camera") {
      DisplayHelpCommandsParams();
    }else if (RawCommandLine == "d"){
      DisplayCurrentCommand();
    }else if (RawCommandLine == "t"){
      // Take Photo
      printf("\nGoing to take photo with current command: \r\n");
      DisplayCurrentCommand();
      // Take Photo
      ExecuteCommand(Command);
      printf("Photo Taken and Saved to Arduino SD CARD . \r\n");
      string Testfile = CreatePhotoFilename();
      printf("Image Output Filename :");
      printf("%s \r\n",Testfile.c_str());
      PhotoTakenCount++;
    }else if (RawCommandLine == "testread"){
      ReadPrintFile("TEST.TXT");
    }else if (RawCommandLine == "testwrite"){
      CheckRemoveFile("TEST.TXT");
      WriteFileTest("TEST.TXT");
    }else{
      printf("Changing command or parameters according to your input: \r\n");
      // Parse Command Line and Set Command Line Elements
      // Parse Raw Command into Command and Parameters
      ParseRawCommand(RawCommandLine);
      // Display new changed camera command with parameters
      DisplayCurrentCommand();
    }
    // Reset Command Line
    RawCommandLine = "";
    printf("\r\n");
    printf("\r\n");
  }
}






void ResetCameraRegisters(){
  // Reset Camera Registers
  // Reading needed to prevent error
  uint8_t data = ReadRegisterValue(COM7);
  // uint8_t result = OV7670WriteReg(COM7, COM7_VALUE_RESET );
  while(!sccb.write_3_phase(COM7, COM7_VALUE_RESET)){}
  string sresult = ParseI2CResult(0);
  printf("RESETTING ALL REGISTERS BY SETTING COM7 REGISTER to 0x80: %s \r\n", sresult.c_str());
  // Delay at least 500ms
  ThisThread::sleep_for(500);
}

uint8_t OV7670WriteReg(uint8_t reg, uint8_t data){
  uint8_t error;
  error = OV7670Write(reg, data, 1);
  return (error);
}

uint8_t OV7670Write(uint8_t start, uint8_t pData, uint8_t size){
  if(sccb.write_3_phase(start, pData)){
    return 0;
  }else{
    return I2C_ERROR_WRITING_DATA;
  }
  // int n, error;
  // Wire.beginTransmission(OV7670_I2C_ADDRESS);
  // n = Wire.write(start); // write the start address
  // if (n != 1){
  //   return (I2C_ERROR_WRITING_START_ADDRESS);
  // }
  // n = Wire.write(pData, size); // write data bytes
  // if (n != size){
  //   return (I2C_ERROR_WRITING_DATA);
  // }
  // error = Wire.endTransmission(true); // release the I2C-bus
  // if (error != 0) {
  //   return (error);
  // }
  return 0; // return : no error
}

string ParseI2CResult(uint8_t result){
  string sresult = "";
  switch(result){
    case 0:
      sresult = "I2C Operation OK .";
      break;
    case I2C_ERROR_WRITING_START_ADDRESS:
      sresult = "I2C_ERROR_WRITING_START_ADDRESS";
      break;
    case I2C_ERROR_WRITING_DATA:
      sresult = "I2C_ERROR_WRITING_DATA";
      break;
    case DATA_TOO_LONG:
      sresult = "DATA_TOO_LONG";
      break;
    case NACK_ON_TRANSMIT_OF_ADDRESS:
      sresult = "NACK_ON_TRANSMIT_OF_ADDRESS";
      break;
    case NACK_ON_TRANSMIT_OF_DATA:
      sresult = "NACK_ON_TRANSMIT_OF_DATA";
      break;
    case OTHER_ERROR:
      sresult = "OTHER_ERROR";
      break;
    default:
      sresult = "I2C ERROR TYPE NOT FOUND.";
      break;
  }
  return sresult;
}

uint8_t ReadRegisterValue(uint8_t RegisterAddress){
  uint8_t data = 0;
  data = sccb.read(RegisterAddress);
  // Wire.beginTransmission(OV7670_I2C_ADDRESS);
  // Wire.write(RegisterAddress);
  // Wire.endTransmission();
  // Wire.requestFrom(OV7670_I2C_ADDRESS, 1);
  // while(Wire.available() < 1);
  // data = Wire.read();
  return data;
}

void ReadRegisters(){
  uint8_t data = 0;

  printf("\r\n");
  data = ReadRegisterValue(CLKRC);
  printf("CLKRC = %X \r\n ", data);
  data = ReadRegisterValue(COM7);
  printf("COM7 = %X \r\n ", data);
  data = ReadRegisterValue(COM3);
  printf("COM3 = %X \r\n ", data);
  data = ReadRegisterValue(COM14);
  printf("COM14 = %X \r\n ", data);
  data = ReadRegisterValue(SCALING_XSC);
  printf("SCALING_XSC = %X \r\n ", data);
  data = ReadRegisterValue(SCALING_YSC);
  printf("SCALING_YSC = %X \r\n ", data);
  data = ReadRegisterValue(SCALING_DCWCTR);
  printf("SCALING_DCWCTR = %X \r\n ", data);
  data = ReadRegisterValue(SCALING_PCLK_DIV);
  printf("SCALING_PCLK_DIV = %X \r\n ", data);
  data = ReadRegisterValue(SCALING_PCLK_DELAY);
  printf("SCALING_PCLK_DELAY = %X \r\n ", data);
  // default value D
  data = ReadRegisterValue(TSLB);
  printf("TSLB (YUV Order- Higher Bit, Bit[3]) = %X \r\n ", data);
  // default value 88
  data = ReadRegisterValue(COM13);
  printf("COM13 (YUV Order - Lower Bit, Bit[1]) = %X \r\n ", data);
  data = ReadRegisterValue(COM17);
  printf("COM17 (DSP Color Bar Selection) = %X \r\n ", data);
  data = ReadRegisterValue(COM4);
  printf("COM4 (works with COM 17) = %X \r\n ", data);
  data = ReadRegisterValue(COM15);
  printf("COM15 (COLOR FORMAT SELECTION) = %X \r\n ", data);
  data = ReadRegisterValue(COM11);
  printf("COM11 (Night Mode) = %X \r\n ", data);
  data = ReadRegisterValue(COM8);
  printf("COM8 (Color Control, AWB) = %X \r\n ", data);
  data = ReadRegisterValue(HAECC7);
  printf("HAECC7 (AEC Algorithm Selection) = %X \r\n ", data);
  data = ReadRegisterValue(GFIX);
  printf("GFIX = %X \r\n ", data);
  // Window Output
  data = ReadRegisterValue(HSTART);
  printf("HSTART = %X \r\n ", data);
  data = ReadRegisterValue(HSTOP);
  printf("HSTOP = %X \r\n ", data);
  data = ReadRegisterValue(HREF);
  printf("HREF = %X \r\n ", data);
  data = ReadRegisterValue(VSTRT);
  printf("VSTRT = %X \r\n ", data);
  data = ReadRegisterValue(VSTOP);
  printf("VSTOP = %X \r\n ", data);
  data = ReadRegisterValue(VREF);
  printf("VREF = %X \r\n ", data);
}

void InitializeOV7670Camera(){
  printf("Initializing OV7670 Camera \r\n");
  //Set WRST to 0 and RRST to 0 , 0.1ms after power on.
  uint8_t DurationMicroSecs = 1;
  // Set mode for pins wither input or output
  // ALREADY DONE IN DECLARATION

  // Delay 1 ms
  ThisThread::sleep_for(1);
  PulseLowEnabledPin(wrst, DurationMicroSecs); // Need to clock the fifo manually to get it to reset
  rrst.write(0);
  PulsePin(rclk, DurationMicroSecs);
  PulsePin(rclk, DurationMicroSecs);
  rrst.write(1);
}

void PulseLowEnabledPin(DigitalOut pin, uint8_t DurationMicroSecs){
  // For Low Enabled Pins , 0 = on and 1 = off
  pin.write(0); // Sets the pin on
  wait_us(DurationMicroSecs); // Pauses for DurationMicroSecs microseconds
  pin.write(1); // Sets the pin off
  wait_us(DurationMicroSecs); // Pauses for DurationMicroSecs microseconds
}

void PulsePin(DigitalOut pin, uint8_t DurationMicroSecs){
  pin.write(1); // Sets the pin on
  wait_us(DurationMicroSecs); // Pauses for DurationMicroSecs microseconds
  pin.write(0); // Sets the pin off
  wait_us(DurationMicroSecs); // Pauses for DurationMicroSecs microseconds
}

void DisplayHelpMenu(){
  printf("....... Help Menu ...... \r\n");
  printf("d - Display Current Camera Command \r\n");
  printf("t - Take Photograph using current Command and Parameters \r\n");
  printf("testread - Tests reading files from the SDCard by reading and printig the contents of test.txt \r\n");
  printf("testwrite - Tests writing files to SDCard \r\n");
  printf("help camera - Displays Cameras Commands and Parameters \r\n");
  printf("\r\n");
  printf("\r\n");
}

void DisplayHelpCommandsParams(){
  printf("... Help Menu Camera Commands/Params .... \r\n");
  printf("Resolution Change Commands: VGA,VGAP,QVGA,QQVGA \r\n");
  printf("FPS Parameters: ThirtyFPS, NightMode \r\n");
  printf("AWB Parameters: SAWB, AAWB \r\n");
  printf("AEC Parameters: AveAEC, HistAEC \r\n");
  printf("YUV Matrix Parameters: YUVMatrixOn, YUVMatrixOff \r\n");
  printf("Denoise Parameters: DenoiseYes, DenoiseNo \r\n");
  printf("Edge Enchancement: EdgeYes, EdgeNo \r\n");
  printf("Automatic Black Level Calibration: AblcON, AblcOFF \r\n");
  printf("\r\n");
  printf("\r\n");
}

void DisplayCurrentCommand(){
  // Print out Command and Parameters
  printf("Current Command: \r\n");
  printf("Command: %s \r\n", Command.c_str());
  printf("FPSParam: %s \r\n", FPSParam.c_str());
  printf("AWBParam: %s \r\n", AWBParam.c_str());
  printf("AECParam: %s \r\n", AECParam.c_str());
  printf("YUVMatrixParam: %s \r\n", YUVMatrixParam.c_str());
  printf("DenoiseParam: %s \r\n", DenoiseParam.c_str());
  printf("EdgeParam: %s \r\n", EdgeParam.c_str());
  printf("ABLCParam: %s \r\n", ABLCParam.c_str());
  printf("\r\n");
}

void ExecuteCommand(string Command){
  // Set up Camera for VGA, QVGA, or QQVGA Modes
  if (Command == "VGA"){
    printf("Taking a VGA Photo. \r\n");
    if (Resolution != VGA){
      // If current resolution is not QQVGA then set camera for QQVGA
      ResetCameraRegisters();
      Resolution = VGA;
      SetupOV7670ForVGARawRGB();
      printf("---------- Camera Registers ----------\r\n");
      ReadRegisters();
      printf("-------------------------\r\n");
    }
  }else if (Command == "VGAP"){
    printf("Taking a VGAP Photo.\r\n");
    if (Resolution != VGAP){
      // If current resolution is not VGAP then set camera for VGAP
      ResetCameraRegisters();
      Resolution = VGAP; SetupOV7670ForVGAProcessedBayerRGB();
      printf("---------- Camera Registers ----------\r\n");
      ReadRegisters();
      printf("-------------------------\r\n");
    }
  }else if (Command == "QVGA"){
    printf("Taking a QVGA Photo.\r\n");
    if (Resolution != QVGA){
      // If current resolution is not QQVGA then set camera for QQVGA
      Resolution = QVGA;
      SetupOV7670ForQVGAYUV();
      printf("---------- Camera Registers ----------\r\n");
      ReadRegisters();
      printf("-------------------------\r\n");
    }
  }else if (Command == "QQVGA"){
    printf("Taking a QQVGA Photo.\r\n");
    if (Resolution != QQVGA){
      // If current resolution is not QQVGA then set camera for QQVGA
      Resolution = QQVGA;
      SetupOV7670ForQQVGAYUV();
      printf("---------- Camera Registers ----------\r\n");
      ReadRegisters();
      printf("-------------------------\r\n");
    }
  }else{
    printf("The command %s", Command.c_str());
    printf(" is not recognized .\r\n");
  }
  // Delay for registers to settle
  ThisThread::sleep_for(100);
  // Take Photo
  TakePhoto();
}

void SetCameraFPSMode(){
  // Set FPS for Camera
  if (FPSParam == "ThirtyFPS") {
    SetupCameraFor30FPS();
  }else if (FPSParam == "NightMode"){
    SetupCameraNightMode();
  }
}

void SetupCameraFor30FPS(){
  int result = 0;
  string sresult = "";
  printf("..... Setting Camera to 30 FPS ....\r\n");
  result = OV7670WriteReg(CLKRC, CLKRC_VALUE_30FPS);
  printf("CLKRC:");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(DBLV, DBLV_VALUE_30FPS);
  sresult = ParseI2CResult(result);
  printf("DBLV: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(EXHCH, EXHCH_VALUE_30FPS);
  sresult = ParseI2CResult(result);
  printf("EXHCH: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(EXHCL, EXHCL_VALUE_30FPS);
  sresult = ParseI2CResult(result);
  printf("EXHCL: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(DM_LNL, DM_LNL_VALUE_30FPS);
  sresult = ParseI2CResult(result);
  printf("DM_LNL: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(DM_LNH, DM_LNH_VALUE_30FPS);
  sresult = ParseI2CResult(result);
  printf("DM_LNH: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(COM11, COM11_VALUE_30FPS);
  sresult =ParseI2CResult(result);
  printf("COM11: ");
  printf("%s \r\n", sresult.c_str());
}

void SetupCameraNightMode(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","... Turning NIGHT MODE ON ....");
  result = OV7670WriteReg(CLKRC, CLKRC_VALUE_NIGHTMODE_AUTO);
  sresult = ParseI2CResult(result);
  printf("CLKRC: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(COM11, COM11_VALUE_NIGHTMODE_AUTO);
  sresult = ParseI2CResult(result);
  printf("COM11: ");
  printf("%s \r\n", sresult.c_str());
}

void SetCameraAEC(){
  // Process AEC
  if (AECParam == "AveAEC"){
    // Set Camera’s Average AEC/AGC Parameters
    SetupCameraAverageBasedAECAGC();
  }else if (AECParam == "HistAEC"){
    // Set Camera AEC algorithim to Histogram
    SetCameraHistogramBasedAECAGC();
  }
}

void SetupCameraAverageBasedAECAGC(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","----- Setting Camera Average Based AEC/AGC Registers -----");
  result = OV7670WriteReg(AEW, AEW_VALUE);
  sresult = ParseI2CResult(result);
  printf("AEW: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AEB, AEB_VALUE);
  sresult = ParseI2CResult(result);
  printf("AEB: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(VPT, VPT_VALUE);
  sresult = ParseI2CResult(result);
  printf("VPT: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HAECC7, HAECC7_VALUE_AVERAGE_AEC_ON);
  sresult = ParseI2CResult(result);
  printf("HAECC7: ");
  printf("%s \r\n", sresult.c_str());
}

void SetCameraHistogramBasedAECAGC(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","----- Setting Camera Histogram Based AEC/AGC Registers -----");
  result = OV7670WriteReg(AEW, AEW_VALUE);
  sresult = ParseI2CResult(result);
  printf("AEW: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AEB, AEB_VALUE);
  sresult = ParseI2CResult(result);
  printf("AEB: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HAECC1, HAECC1_VALUE);
  sresult = ParseI2CResult(result);
  printf("HAECC1: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HAECC2, HAECC2_VALUE);
  sresult = ParseI2CResult(result);
  printf("HAECC2: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HAECC3, HAECC3_VALUE);
  sresult = ParseI2CResult(result);
  printf("HAECC3: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HAECC4, HAECC4_VALUE);
  sresult = ParseI2CResult(result);
  printf("HAECC4: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HAECC5, HAECC5_VALUE);
  sresult = ParseI2CResult(result);
  printf("HAECC5: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HAECC6, HAECC6_VALUE);
  sresult = ParseI2CResult(result);
  printf("HAECC6: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HAECC7, HAECC7_VALUE_HISTOGRAM_AEC_ON);
  sresult = ParseI2CResult(result);
  printf("HAECC7: ");
  printf("%s \r\n", sresult.c_str());
}

void SetCameraSaturationControl(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","..... Setting Camera Saturation Level ....");
  result = OV7670WriteReg(SATCTR, SATCTR_VALUE);
  sresult = ParseI2CResult(result);
  printf("SATCTR: ");
  printf("%s \r\n", sresult.c_str());
}

void SetupCameraArrayControl(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","..... Setting Camera Array Control ....");
  result = OV7670WriteReg(CHLF, CHLF_VALUE);
  sresult = ParseI2CResult(result);
  printf("CHLF: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(ARBLM, ARBLM_VALUE);
  sresult = ParseI2CResult(result);
  printf("ARBLM: ");
  printf("%s \r\n", sresult.c_str());
}

void SetupCameraADCControl(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","..... Setting Camera ADC Control ....");
  result = OV7670WriteReg(ADCCTR1, ADCCTR1_VALUE);
  sresult = ParseI2CResult(result);
  printf("ADCCTR1: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(ADCCTR2, ADCCTR2_VALUE);
  sresult = ParseI2CResult(result);
  printf("ADCCTR2: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(ADC, ADC_VALUE);
  sresult = ParseI2CResult(result);
  printf("ADC: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(ACOM, ACOM_VALUE);
  sresult = ParseI2CResult(result);
  printf("ACOM: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(OFON, OFON_VALUE);
  sresult = ParseI2CResult(result);
  printf("OFON: ");
  printf("%s \r\n", sresult.c_str());
}

void SetupCameraABLC(){
  int result = 0;
  string sresult = "";
  // If ABLC is off then return otherwise
  // turn on ABLC.
  if (ABLCParam == "AblcOFF"){
    return;
  }
  printf("%s \r\n",".... Setting Camera ABLC ...");
  result = OV7670WriteReg(ABLC1, ABLC1_VALUE);
  sresult = ParseI2CResult(result);
  printf("ABLC1: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(THL_ST, THL_ST_VALUE);
  sresult = ParseI2CResult(result);
  printf("THL_ST: ");
  printf("%s \r\n", sresult.c_str());
}

void SetupCameraAWB(){
  // Set AWB Mode
  if (AWBParam == "SAWB"){
    // Set Simple Automatic White Balance
    SetupCameraSimpleAutomaticWhiteBalance(); // OK
    // Set Gain Config
    SetupCameraGain();
  }else if (AWBParam == "AAWB"){
    // Set Advanced Automatic White Balance
    SetupCameraAdvancedAutomaticWhiteBalance(); // ok
    // Set Camera Automatic White Balance Configuration
    SetupCameraAdvancedAutoWhiteBalanceConfig(); // ok
    // Set Gain Config
    SetupCameraGain();
  }
}

void SetupCameraSimpleAutomaticWhiteBalance(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","..... Setting Camera to Simple AWB ....");
  // COM8
  result = OV7670WriteReg(COM8, COM8_VALUE_AWB_ON);
  sresult = ParseI2CResult(result);
  printf("COM8(0x13): ");
  printf("%s \r\n", sresult.c_str());
  // AWBCTR0
  result = OV7670WriteReg(AWBCTR0, AWBCTR0_VALUE_NORMAL);
  sresult = ParseI2CResult(result);
  printf("AWBCTR0 Control Register 0(0x6F): ");
  printf("%s \r\n", sresult.c_str());
}

void SetupCameraGain(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","..... Setting Camera Gain ....");
  // Set Maximum Gain
  result = OV7670WriteReg(COM9, COM9_VALUE_4XGAIN);
  sresult = ParseI2CResult(result);
  printf("COM9: ");
  printf("%s \r\n", sresult.c_str());
  // Set Blue Gain
  result = OV7670WriteReg(BLUE, BLUE_VALUE);
  sresult = ParseI2CResult(result);
  printf("BLUE GAIN: ");
  printf("%s \r\n", sresult.c_str());
  // Set Red Gain result = OV7670WriteReg(RED, RED_VALUE);
  sresult = ParseI2CResult(result);
  printf("RED GAIN: ");
  printf("%s \r\n", sresult.c_str());
  // Set Green Gain
  result = OV7670WriteReg(GGAIN, GGAIN_VALUE);
  sresult = ParseI2CResult(result);
  printf("GREEN GAIN: ");
  printf("%s \r\n", sresult.c_str());
  // Enable AWB Gain
  result = OV7670WriteReg(COM16, COM16_VALUE);
  sresult = ParseI2CResult(result);
  printf("COM16(ENABLE GAIN): ");
  printf("%s \r\n", sresult.c_str());
}

void SetupCameraAdvancedAutomaticWhiteBalance(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","..... Setting Camera to Advanced AWB....");
  // AGC, AWB, and AEC Enable
  result = OV7670WriteReg(0x13, 0xE7);
  sresult = ParseI2CResult(result);
  printf("COM8(0x13): ");
  printf("%s \r\n", sresult.c_str());
  // AWBCTR0
  result = OV7670WriteReg(0x6f, 0x9E);
  sresult = ParseI2CResult(result);
  printf("AWB Control Register 0(0x6F): ");
  printf("%s \r\n", sresult.c_str());
}

void SetupCameraAdvancedAutoWhiteBalanceConfig(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","..... Setting Camera Advanced Auto White Balance Configs....");
  result = OV7670WriteReg(AWBC1, AWBC1_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBC1: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBC2, AWBC2_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBC2: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBC3, AWBC3_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBC3: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBC4, AWBC4_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBC4: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBC5, AWBC5_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBC5: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBC6, AWBC6_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBC6: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBC7, AWBC7_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBC7: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBC8, AWBC8_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBC8: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBC9, AWBC9_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBC9: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBC10, AWBC10_VALUE);
  sresult =ParseI2CResult(result);
  printf("AWBC10: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBC11, AWBC11_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBC11: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBC12, AWBC12_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBC12: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBCTR3, AWBCTR3_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBCTR3: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBCTR2, AWBCTR2_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBCTR2: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(AWBCTR1, AWBCTR1_VALUE);
  sresult = ParseI2CResult(result);
  printf("AWBCTR1: ");
  printf("%s \r\n", sresult.c_str());
}

void SetupCameraDenoiseEdgeEnhancement(){
  int result = 0;
  string sresult = "";
  if ((DenoiseParam == "DenoiseYes")&&(EdgeParam == "EdgeYes")){
    SetupCameraDenoise();
    SetupCameraEdgeEnhancement();
    result = OV7670WriteReg(COM16,COM16_VALUE_DENOISE_ON__EDGE_ENHANCEMENT_ON__AWBGAIN_ON);
    sresult = ParseI2CResult(result);
    printf("COM16: ");
    printf("%s \r\n", sresult.c_str());
  }else if ((DenoiseParam == "DenoiseYes")&&(EdgeParam == "EdgeNo")){
    SetupCameraDenoise();
    result = OV7670WriteReg(COM16,COM16_VALUE_DENOISE_ON__EDGE_ENHANCEMENT_OFF__AWBGAIN_ON);
    sresult = ParseI2CResult(result);
    printf("COM16: ");
    printf("%s \r\n", sresult.c_str());
  }else if ((DenoiseParam == "DenoiseNo")&&(EdgeParam == "EdgeYes")){
    SetupCameraEdgeEnhancement();
    result = OV7670WriteReg(COM16,COM16_VALUE_DENOISE_OFF__EDGE_ENHANCEMENT_ON__AWBGAIN_ON);
    sresult = ParseI2CResult(result);
    printf("COM16: ");
    printf("%s \r\n", sresult.c_str());
  }
}

void SetupCameraDenoise(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","..... Setting Camera Denoise ....");
  result = OV7670WriteReg(DNSTH, DNSTH_VALUE);
  sresult = ParseI2CResult(result);
  printf("DNSTH: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(REG77, REG77_VALUE);
  sresult = ParseI2CResult(result);
  printf("REG77: ");
  printf("%s \r\n", sresult.c_str());
}

void SetupCameraEdgeEnhancement(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","..... Setting Camera Edge Enhancement ....");
  result = OV7670WriteReg(EDGE, EDGE_VALUE);
  sresult = ParseI2CResult(result);
  printf("EDGE: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(REG75, REG75_VALUE);
  sresult = ParseI2CResult(result);
  printf("REG75: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(REG76, REG76_VALUE);
  sresult = ParseI2CResult(result);
  printf("REG76: ");
  printf("%s \r\n", sresult.c_str());
}

void SetupCameraUndocumentedRegisters(){
  int result = 0; string sresult = "";
  printf("%s \r\n","..... Setting Camera Undocumented Registers ....");
  result = OV7670WriteReg(0xB0, 0x84);
  sresult = ParseI2CResult(result);
  printf("Setting B0 UNDOCUMENTED register to 0x84:= ");
  printf("%s \r\n", sresult.c_str());
}

void SetCameraColorMatrixYUV(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","..... Setting Camera Color Matrix for YUV ....");
  result = OV7670WriteReg(MTX1, MTX1_VALUE);
  sresult = ParseI2CResult(result);
  printf("MTX1: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(MTX2, MTX2_VALUE);
  sresult = ParseI2CResult(result);
  printf("MTX2: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(MTX3, MTX3_VALUE);
  sresult = ParseI2CResult(result);
  printf("MTX3: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(MTX4, MTX4_VALUE);
  sresult = ParseI2CResult(result);
  printf("MTX4: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(MTX5, MTX5_VALUE);
  sresult = ParseI2CResult(result);
  printf("MTX5: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(MTX6, MTX6_VALUE);
  sresult = ParseI2CResult(result);
  printf("MTX6: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(CONTRAS, CONTRAS_VALUE);
  sresult = ParseI2CResult(result);
  printf("CONTRAS: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(MTXS, MTXS_VALUE);
  sresult = ParseI2CResult(result);
  printf("MTXS: ");
  printf("%s \r\n", sresult.c_str());
}

void TakePhoto(){
  unsigned long ElapsedTime = 0;
  Timer t;
  t.start();
  CaptureOV7670Frame();
  ReadTransmitCapturedFrame();
  t.stop();
  ElapsedTime = std::chrono::duration_cast<std::chrono::seconds>(t.elapsed_time()).count(); // Convert to seconds
  printf("Elapsed Time for Taking and Sending Photo(secs) = %lu \r\n", ElapsedTime);
}

unsigned long pulseIn(DigitalIn pin){
  // pulseIn
  Timer t;
  while (!pin.read()); // wait for high
  t.start();
  while (pin.read()); // wait for low
  t.stop();
  return t.elapsed_time().count();
}

void CaptureOV7670Frame(){
  unsigned long DurationStart = 0;
  unsigned long DurationStop = 0;
  unsigned long ElapsedTime = 0;
  //Capture one frame into FIFO memory // 0. Initialization.
  printf("\r\n");
  printf("%s \r\n","Starting Capture of Photo .");
  Timer t;
  t.start();
  // 1. Wait for VSync to pulse to indicate the start of the image
  DurationStart = pulseIn(vsync);
  // 2. Reset Write Pointer to 0. Which is the beginning of frame
  PulseLowEnabledPin(wrst, 6); // 3 microseconds + 3 microseconds for error factor on Arduino
  // 3. Set FIFO Write Enable to active (high) so that image can be written to ram
  wen.write(1);
  // 4. Wait for VSync to pulse again to indicate the end of the frame capture
  DurationStop = pulseIn(vsync);
  // 5. Set FIFO Write Enable to nonactive (low) so that no more images can be written to the ram
  wen.write(0);
  // 6. Print out Stats
  t.stop();
  ElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count(); // Convert to seconds
  printf("Time for Frame Capture (milliseconds) = ");
  printf("%lu \r\n",ElapsedTime);
  printf("VSync beginning duration (microseconds) = ");
  printf("%lu \r\n",DurationStart);
  printf("VSync end duration (microseconds) = ");
  printf("%lu \r\n",DurationStop);
  // 7. WAIT so that new data can appear on output pins Read new data.
  ThisThread::sleep_for(2);
}

void ReadTransmitCapturedFrame(){
  uint8_t PixelData = 0;
  uint8_t PinVal7 = 0;
  uint8_t PinVal6 = 0;
  uint8_t PinVal5 = 0;
  uint8_t PinVal4 = 0;
  uint8_t PinVal3 = 0;
  uint8_t PinVal2 = 0;
  uint8_t PinVal1 = 0;
  uint8_t PinVal0 = 0;
  printf("%s \r\n","Starting Transmission of Photo To SDCard .");
  // //////////////////// Code for SD Card /////////////////////////////////
  // // Image file to write to
  // File ImageOutputFile;
  // // Create name of Photo To save based on certain parameters
  // string Filename = CreatePhotoFilename();
  // // Check if file already exists and remove it if it does.
  // CheckRemoveFile(Filename);
  // ImageOutputFile = SD.open(Filename.c_str(), FILE_WRITE); // Test if file actually open
  // if (!ImageOutputFile){
  //   printf("%s \r\n","\nCritical ERROR . Can not open Image Ouput File for output . ");
  //   return;
  // }
  // ////////////////////////////////////////////////////////////////////////
  // Set Read Buffer Pointer to start of frame
  rrst.write(0);
  PulsePin(rclk, 1);
  PulsePin(rclk, 1);
  PulsePin(rclk, 1);
  rrst.write(1);
  printf("<<<<< START >>>>> \r\n");
  unsigned long ByteCounter = 0;
  for (int height = 0; height < PHOTO_HEIGHT; height++){
    for (int width = 0; width < PHOTO_WIDTH; width++){
      for (int bytenumber = 0; bytenumber < PHOTO_BYTES_PER_PIXEL; bytenumber++){
      // Pulse the read clock RCLK to bring in new byte of data.
      PulsePin(rclk, 1);
      // Convert Pin values to byte values for pins 0-7 of incoming pixel byte
      PinVal7 = ConvertPinValueToByteValue(d7.read(), 7);
      PinVal6 = ConvertPinValueToByteValue(d6.read(), 6);
      PinVal5 = ConvertPinValueToByteValue(d5.read(), 5);
      PinVal4 = ConvertPinValueToByteValue(d4.read(), 4);
      PinVal3 = ConvertPinValueToByteValue(d3.read(), 3);
      PinVal2 = ConvertPinValueToByteValue(d2.read(), 2);
      PinVal1 = ConvertPinValueToByteValue(d1.read(), 1);
      PinVal0 = ConvertPinValueToByteValue(d0.read(), 0);
      // Combine individual data from each pin into composite data in the form of a single byte
      PixelData = PinVal7 | PinVal6 | PinVal5 | PinVal4 | PinVal3 | PinVal2 | PinVal1 | PinVal0;
      // ///////////////////////////// SD Card ////////////////////////////////
      // ByteCounter = ByteCounter + ImageOutputFile.write(PixelData);
      // ///////////////////////////////////////////////////////////////////////
      printf("%d,",PixelData);
      ByteCounter += 1;
      }
    }
  }
  printf("\r\n<<<<< END >>>>> \r\n");
  // // Close SD Card File
  // ImageOutputFile.close();
  printf("Total Bytes Saved to SDCard = ");
  printf("%d \r\n",ByteCounter);
  // Write Photo’s Info File to SDCard.
  printf("%s \r\n","Writing Photo’s Info file (.txt file) to SD Card .");
  CreatePhotoInfoFile();
}

string CreatePhotoFilename(){
  // string Filename = "";
  // string Ext = "";
  // // Creates filename that the photo will be saved under
  // // Create file extension
  // // If Command = QQVGA or QVGA then extension is .yuv
  // if ((Command == "QQVGA") || (Command == "QVGA")){
  //   Ext = ".yuv";
  // }else if ((Command == "VGA") || (Command == "VGAP")){
  //   Ext = ".raw";
  // }
  // // Create Filename from
  // // Resolution + PhotoNumber + Extension
  // Filename = Command + PhotoTakenCount + Ext; return Filename;
}

void CheckRemoveFile(string Filename){
  // // Check if file already exists and remove it if it does.
  // char tempchar[50];
  // strcpy(tempchar, Filename.c_str());
  // if (SD.exists(tempchar)){
  //   printf("Filename: ");
  //   printf(tempchar);
  //   printf("%s \r\n"," Already Exists. Removing It.");
  //   SD.remove(tempchar);
  // }
  // // If file still exists then new image file cannot be saved to SD Card.
  // if (SD.exists(tempchar)){
  //   printf("%s \r\n","Error.. Image output file cannot be created.");
  //   return;
  // }
}

uint8_t ConvertPinValueToByteValue(int PinValue, int PinPosition){
  uint8_t ByteValue = 0;
  if (PinValue == 1){
    ByteValue = 1 << PinPosition;
  }
  return ByteValue;
}

void CreatePhotoInfoFile(){
  // // Creates the photo information file based on current settings
  // // .txt information File for Photo
  // File InfoFile;
  // // Create name of Photo To save based on certain parameters
  // string Filename = CreatePhotoInfoFilename();
  // // Check if file already exists and remove it if it does.
  // CheckRemoveFile(Filename);
  // // Open File
  // InfoFile = SD.open(Filename.c_str(), FILE_WRITE);
  // // Test if file actually open
  // if (!InfoFile){
  //   printf("%s \r\n","\nCritical ERROR . Can not open Photo Info File for output . ");
  //   return;
  // }
  // // Write Info to file
  // string Data = CreatePhotoInfo();
  // InfoFile.println(Data); // Close SD Card File
  // InfoFile.close();
}

string CreatePhotoInfoFilename(){
  string Filename = "";
  // string Ext = "";
  // // Creates filename that the information about the photo will
  // // be saved under.
  // // Create file extension
  // Ext = ".txt";
  // // Create Filename from
  // // Resolution + PhotoNumber + Extension
  // Filename = Command + PhotoTakenCount + Ext;
  return Filename;
}

string CreatePhotoInfo(){
  string Info = "";
  Info = Command + " " + FPSParam + " " + AWBParam + " " + AECParam + " " +
  YUVMatrixParam + " " +
  DenoiseParam + " " + EdgeParam + " " + ABLCParam;
  return Info;
}

void ReadPrintFile(string Filename){
  // File TempFile;
  // // Reads in file and prints it to screen via Serial
  // TempFile = SD.open(Filename.c_str());
  // if (TempFile){
  //   printf(Filename);
  //   printf("%s \r\n",":");
  //   // read from the file until there’s nothing else in it:
  //   while (TempFile.available()){
  //     Serial.write(TempFile.read());
  //   }
  //   // close the file:
  //   TempFile.close();
  // }
  // else{
  //   // Error opening file
  //   printf("Error opening ");
  //   printf("%s \r\n",Filename.c_str());
  // }
}

void WriteFileTest(string Filename){
  // File TempFile;
  // TempFile = SD.open(Filename.c_str(), FILE_WRITE);
  // if (TempFile){
  //   printf("Writing to testfile .");
  //   TempFile.print("TEST CAMERA SDCARD HOOKUP At Time. ");
  //   // TempFile.print(millis()/1000); // THERE IS NO MILLIS IN MBED
  //   TempFile.println(" Seconds");
  //   TempFile.print("Photo Info Filename: ");
  //   TempFile.println(CreatePhotoInfoFilename());
  //   TempFile.print("Photo Info:");
  //   TempFile.println(CreatePhotoInfo());
  //   // close the file:
  //   TempFile.close();
  //   printf("%s \r\n","Writing File Done.");
  // }else{
  //   // if the file didn’t open, print an error:
  //   printf("Error opening ");
  //   printf("%s \r\n",Filename.c_str());
  // }
}

void ParseRawCommand(string RawCommandLine){
  string Entries[10];
  bool success = false;
  // Parse into command and parameters
  int NumberElements = ParseCommand(RawCommandLine.c_str(), ' ', Entries);
  for (int i = 0 ; i < NumberElements; i++){
    bool success = ProcessRawCommandElement(Entries[i]);
    if (!success){
      printf("Invalid Command or Parameter: ");
      printf("%s \r\n",Entries[i].c_str());
    }else{
      printf("Command or parameter ");
      printf(Entries[i].c_str());
      printf("%s \r\n"," sucessfully set .");
    }
  } // Assume parameter change since user is setting parameters on command line manually
  // Tells the camera to re-initialize and set up camera according to new parameters
  Resolution = None; // Reset and reload registers
  ResetCameraRegisters();
}

int ParseCommand(const char* commandline, char splitcharacter, string* Result){
  int ResultIndex = 0;
  int length = strlen(commandline);
  string temp = "";
  for (int i = 0; i < length ; i++){
    char tempchar = commandline[i];
    if (tempchar == splitcharacter){
      Result[ResultIndex] += temp;
      ResultIndex++;
      temp = "";
    }else{
      temp += tempchar;
    }
  }
  // Put in end part of string
  Result[ResultIndex] = temp;
  return (ResultIndex + 1);
}

bool ProcessRawCommandElement(string Element){
  bool result = false;
  std::transform(Element.begin(), Element.end(), Element.begin(), [](unsigned char c){ return std::tolower(c); });
  // Element.toLowerCase();
  if ((Element == "vga") ||(Element == "vgap") ||(Element == "qvga")||(Element == "qqvga")){
    // Element.toUpperCase();
    std::transform(Element.begin(), Element.end(), Element.begin(), [](unsigned char c){ return std::toupper(c); });

    Command = Element;
    result = true;
  }else if (Element == "thirtyfps"){
  FPSParam = "ThirtyFPS";
  result = true;
  }else if (Element == "nightmode"){
  FPSParam = "NightMode";
  result = true;
  }else if (Element == "sawb"){
  AWBParam = "SAWB";
  result = true;
  }else if (Element == "aawb"){
  AWBParam = "AAWB";
  result = true;
  }else if (Element == "aveaec"){
  AECParam = "AveAEC";
  result = true;
  }else if (Element == "histaec"){
  AECParam = "HistAEC";
  result = true;
  }else if (Element == "yuvmatrixon"){
  YUVMatrixParam = "YUVMatrixOn";
  result = true;
  }else if (Element == "yuvmatrixoff"){
  YUVMatrixParam = "YUVMatrixOff";
  result = true;
  }else if (Element == "denoiseyes"){
  DenoiseParam = "DenoiseYes";
  result = true;
  }else if (Element == "denoiseno"){
  DenoiseParam = "DenoiseNo";
  result = true;
  }else if (Element == "edgeyes"){
  EdgeParam = "EdgeYes";
  result = true;
  }else if (Element == "edgeno"){
  EdgeParam = "EdgeNo";
  result = true;
  }else if (Element == "ablcon"){
  ABLCParam = "AblcON"; result = true;
  }else if (Element == "ablcoff"){
  ABLCParam = "AblcOFF";
  result = true;
  }
  return result;
}

void SetupOV7670ForVGAProcessedBayerRGB(){
  int result = 0;
  string sresult = "";
  // Call Base for VGA Raw Bayer RGB Mode
  SetupOV7670ForVGARawRGB();
  printf("%s \r\n","----- Setting Camera for VGA (Processed Bayer RGB) ------");
  // Set key register for selecting processed bayer rgb output result =
  OV7670WriteReg(COM7, COM7_VALUE_VGA_PROCESSED_BAYER );
  //result = OV7670WriteReg(COM7, COM7_VALUE_VGA_COLOR_BAR );
  sresult = ParseI2CResult(result);
  printf("COM7: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(TSLB, 0x04);
  sresult = ParseI2CResult(result);
  printf("Initializing TSLB register result = ");
  printf("%s \r\n", sresult.c_str());
  // Needed Color Correction, green to red
  result = OV7670WriteReg(0xB0, 0x8c);
  sresult = ParseI2CResult(result);
  printf("Setting B0 UNDOCUMENTED register to 0x84:= ");
  printf("%s \r\n", sresult.c_str());
  // Set Camera Automatic White Balance
  SetupCameraAWB();
  // Denoise and Edge Enhancement
  SetupCameraDenoiseEdgeEnhancement();
}

void SetupOV7670ForVGARawRGB(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","--------- Setting Camera for VGA (Raw RGB) ---------");
  PHOTO_WIDTH = 640;
  PHOTO_HEIGHT = 480;
  PHOTO_BYTES_PER_PIXEL = 1;
  printf("Photo Width = ");
  printf("%d \r\n",PHOTO_WIDTH);
  printf("Photo Height = ");
  printf("%d \r\n",PHOTO_HEIGHT);
  printf("Bytes Per Pixel = ");
  printf("%d \r\n",PHOTO_BYTES_PER_PIXEL);
  // Basic Registers
  result = OV7670WriteReg(CLKRC, CLKRC_VALUE_VGA);
  sresult = ParseI2CResult(result);
  printf("CLKRC: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(COM7, COM7_VALUE_VGA ); //result =
  OV7670WriteReg(COM7, COM7_VALUE_VGA_COLOR_BAR );
  sresult = ParseI2CResult(result);
  printf("COM7: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(COM3, COM3_VALUE_VGA);
  sresult = ParseI2CResult(result);
  printf("COM3: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(COM14, COM14_VALUE_VGA );
  sresult = ParseI2CResult(result);
  printf("COM14: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_XSC,SCALING_XSC_VALUE_VGA );
  sresult = ParseI2CResult(result);
  printf("SCALING_XSC: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_YSC,SCALING_YSC_VALUE_VGA );
  sresult = ParseI2CResult(result);
  printf("SCALING_YSC: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_DCWCTR, SCALING_DCWCTR_VALUE_VGA);
  sresult = ParseI2CResult(result);
  printf("SCALING_DCWCTR: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_PCLK_DIV,
  SCALING_PCLK_DIV_VALUE_VGA);
  sresult = ParseI2CResult(result);
  printf("SCALING_PCLK_DIV: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_PCLK_DELAY,SCALING_PCLK_DELAY_VALUE_VGA);
  sresult = ParseI2CResult(result);
  printf("SCALING_PCLK_DELAY: ");
  printf("%s \r\n", sresult.c_str());
  // COM17 - DSP Color Bar Enable/Disable
  // COM17_VALUE 0x08 // Activate Color Bar for DSP
  //result = OV7670WriteReg(COM17,COM17_VALUE_AEC_NORMAL_COLOR_BAR);
  result = OV7670WriteReg(COM17,COM17_VALUE_AEC_NORMAL_NO_COLOR_BAR);
  sresult = ParseI2CResult(result);
  printf("COM17: ");
  printf("%s \r\n", sresult.c_str());
  // Set Additional Parameters
  // Set Camera Frames per second
  SetCameraFPSMode();
  // Set Camera Automatic Exposure Control
  SetCameraAEC();
  // Needed Color Correction, green to red
  result = OV7670WriteReg(0xB0, 0x8c);
  printf("Setting B0 UNDOCUMENTED register to 0x84:= ");
  printf("%s \r\n", sresult.c_str());
  // Set Camera Saturation
  SetCameraSaturationControl();
  // Setup Camera Array Control
  SetupCameraArrayControl();
  // Set ADC Control
  SetupCameraADCControl();
  // Set Automatic Black Level Calibration
  SetupCameraABLC();
  printf("%s \r\n","..... Setting Camera Window Output Parameters ....");
  // Change Window Output parameters after custom scaling
  result = OV7670WriteReg(HSTART, HSTART_VALUE_VGA );
  sresult = ParseI2CResult(result);
  printf("HSTART: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HSTOP, HSTOP_VALUE_VGA );
  sresult = ParseI2CResult(result);
  printf("HSTOP: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HREF, HREF_VALUE_VGA );
  printf("HREF: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(VSTRT, VSTRT_VALUE_VGA );
  sresult = ParseI2CResult(result);
  printf("VSTRT: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(VSTOP, VSTOP_VALUE_VGA );
  sresult = ParseI2CResult(result);
  printf("VSTOP: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(VREF, VREF_VALUE_VGA );
  sresult = ParseI2CResult(result);
  printf("VREF: ");
  printf("%s \r\n", sresult.c_str());
}

void SetupOV7670ForQVGAYUV(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","--------- Setting Camera for QVGA (YUV) ---------");
  PHOTO_WIDTH = 320;
  PHOTO_HEIGHT = 240;
  PHOTO_BYTES_PER_PIXEL = 2;
  printf("Photo Width = ");
  printf("%d \r\n",PHOTO_WIDTH);
  printf("Photo Height = ");
  printf("%d \r\n",PHOTO_HEIGHT);
  printf("Bytes Per Pixel = ");
  printf("%d \r\n",PHOTO_BYTES_PER_PIXEL);
  // Basic Registers
  result = OV7670WriteReg(CLKRC, CLKRC_VALUE_QVGA);
  sresult = ParseI2CResult(result);
  printf("CLKRC: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(COM7, COM7_VALUE_QVGA );
  //result = OV7670WriteReg(COM7, COM7_VALUE_QVGA_COLOR_BAR );
  sresult = ParseI2CResult(result);
  printf("COM7: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(COM3, COM3_VALUE_QVGA);
  sresult = ParseI2CResult(result);
  printf("COM3: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(COM14, COM14_VALUE_QVGA );
  sresult = ParseI2CResult(result);
  printf("COM14: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_XSC,SCALING_XSC_VALUE_QVGA );
  sresult = ParseI2CResult(result);
  printf("SCALING_XSC: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_YSC,SCALING_YSC_VALUE_QVGA );
  sresult = ParseI2CResult(result);
  printf("SCALING_YSC: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_DCWCTR,
  SCALING_DCWCTR_VALUE_QVGA );
  sresult = ParseI2CResult(result);
  printf("SCALING_DCWCTR: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_PCLK_DIV,
  SCALING_PCLK_DIV_VALUE_QVGA);
  sresult = ParseI2CResult(result);
  printf("SCALING_PCLK_DIV: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_PCLK_DELAY,SCALING_PCLK_DELAY_VALUE_QVGA);
  sresult = ParseI2CResult(result);
  printf("SCALING_PCLK_DELAY: ");
  printf("%s \r\n", sresult.c_str());
  // YUV order control change from default use with COM13
  result = OV7670WriteReg(TSLB, 0x04);
  sresult = ParseI2CResult(result);
  printf("TSLB: ");
  printf("%s \r\n", sresult.c_str()); //COM13
  result = OV7670WriteReg(COM13, 0xC2); // from YCbCr reference specs
  sresult = ParseI2CResult(result);
  printf("COM13: ");
  printf("%s \r\n", sresult.c_str());
  // COM17 - DSP Color Bar Enable/Disable
  // COM17_VALUE 0x08 // Activate Color Bar for DSP
  //result = OV7670WriteReg(COM17,COM17_VALUE_AEC_NORMAL_COLOR_BAR);
  result = OV7670WriteReg(COM17,COM17_VALUE_AEC_NORMAL_NO_COLOR_BAR);
  sresult = ParseI2CResult(result);
  printf("COM17: ");
  printf("%s \r\n", sresult.c_str());
  // Set Additional Parameters
  // Set Camera Frames per second
  SetCameraFPSMode();
  // Set Camera Automatic Exposure Control
  SetCameraAEC();
  // Set Camera Automatic White Balance
  SetupCameraAWB();
  // Setup Undocumented Registers - Needed Minimum
  SetupCameraUndocumentedRegisters();
  // Set Color Matrix for YUV
  if (YUVMatrixParam == "YUVMatrixOn") {
    SetCameraColorMatrixYUV();
  }
  // Set Camera Saturation
  SetCameraSaturationControl();
  // Denoise and Edge Enhancement
  SetupCameraDenoiseEdgeEnhancement();
  // Set up Camera Array Control
  SetupCameraArrayControl();
  // Set ADC Control
  SetupCameraADCControl();
  // Set Automatic Black Level Calibration
  SetupCameraABLC();
  printf("%s \r\n","..... Setting Camera Window Output Parameters ....");
  // Change Window Output parameters after custom scaling
  result = OV7670WriteReg(HSTART, HSTART_VALUE_QVGA );
  sresult = ParseI2CResult(result);
  printf("HSTART: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HSTOP, HSTOP_VALUE_QVGA );
  sresult = ParseI2CResult(result);
  printf("HSTOP: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HREF, HREF_VALUE_QVGA );
  sresult = ParseI2CResult(result);
  printf("HREF: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(VSTRT, VSTRT_VALUE_QVGA );
  sresult = ParseI2CResult(result);
  printf("VSTRT: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(VSTOP, VSTOP_VALUE_QVGA );
  sresult = ParseI2CResult(result);
  printf("VSTOP: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(VREF, VREF_VALUE_QVGA );
  sresult = ParseI2CResult(result);
  printf("VREF: ");
  printf("%s \r\n", sresult.c_str());
}

void SetupOV7670ForQQVGAYUV(){
  int result = 0;
  string sresult = "";
  printf("%s \r\n","--------- Setting Camera for QQVGA YUV ---------");
  PHOTO_WIDTH = 160;
  PHOTO_HEIGHT = 120;
  PHOTO_BYTES_PER_PIXEL = 2;
  printf("Photo Width = ");
  printf("%d \r\n",PHOTO_WIDTH);
  printf("Photo Height = ");
  printf("%d \r\n",PHOTO_HEIGHT);
  printf("Bytes Per Pixel = ");
  printf("%d \r\n",PHOTO_BYTES_PER_PIXEL);
  printf("%s \r\n","..... Setting Basic QQVGA Parameters ....");
  result = OV7670WriteReg(CLKRC, CLKRC_VALUE_QQVGA);
  sresult = ParseI2CResult(result);
  printf("CLKRC: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(COM7, COM7_VALUE_QQVGA );
  //result = OV7670WriteReg(COM7, COM7_VALUE_QQVGA_COLOR_BAR );
  sresult = ParseI2CResult(result);
  printf("COM7: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(COM3, COM3_VALUE_QQVGA);
  sresult = ParseI2CResult(result);
  printf("COM3: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(COM14, COM14_VALUE_QQVGA );
  sresult = ParseI2CResult(result);
  printf("COM14: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_XSC,SCALING_XSC_VALUE_QQVGA );
  sresult = ParseI2CResult(result);
  printf("SCALING_XSC: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_YSC,SCALING_YSC_VALUE_QQVGA );
  sresult = ParseI2CResult(result);
  printf("SCALING_YSC: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_DCWCTR,
  SCALING_DCWCTR_VALUE_QQVGA );
  sresult = ParseI2CResult(result);
  printf("SCALING_DCWCTR: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_PCLK_DIV,
  SCALING_PCLK_DIV_VALUE_QQVGA);
  sresult = ParseI2CResult(result);
  printf("SCALING_PCLK_DIV: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(SCALING_PCLK_DELAY,SCALING_PCLK_DELAY_VALUE_QQVGA);
  sresult = ParseI2CResult(result);
  printf("SCALING_PCLK_DELAY: ");
  printf("%s \r\n", sresult.c_str());
  // YUV order control change from default use with COM13
  result = OV7670WriteReg(TSLB,TSLB_VALUE_YUYV_AUTO_OUTPUT_WINDOW_DISABLED); // Works
  sresult = ParseI2CResult(result);
  printf("TSLB: ");
  printf("%s \r\n", sresult.c_str());
  //COM13
  result = OV7670WriteReg(COM13, 0xC8); // Gamma Enabled, UV Auto Adj On
  sresult = ParseI2CResult(result);
  printf("COM13: ");
  printf("%s \r\n", sresult.c_str());
  // COM17 - DSP Color Bar Enable/Disable
  //result = OV7670WriteReg(COM17,COM17_VALUE_AEC_NORMAL_COLOR_BAR);
  result = OV7670WriteReg(COM17,COM17_VALUE_AEC_NORMAL_NO_COLOR_BAR);
  sresult = ParseI2CResult(result);
  printf("COM17: ");
  printf("%s \r\n", sresult.c_str());
  // Set Additional Parameters
  // Set Camera Frames per second
  SetCameraFPSMode();
  // Set Camera Automatic Exposure Control
  SetCameraAEC(); // Set Camera Automatic White Balance
  SetupCameraAWB();
  // Setup Undocumented Registers - Needed Minimum
  SetupCameraUndocumentedRegisters();
  // Set Color Matrix for YUV
  if (YUVMatrixParam == "YUVMatrixOn"){
    SetCameraColorMatrixYUV();
  }
  // Set Camera Saturation
  SetCameraSaturationControl();
  // Denoise and Edge Enhancement
  SetupCameraDenoiseEdgeEnhancement();
  // Set Array Control
  SetupCameraArrayControl();
  // Set ADC Control
  SetupCameraADCControl();
  // Set Automatic Black Level Calibration
  SetupCameraABLC();
  printf("%s \r\n","..... Setting Camera Window Output Parameters ...."); // Change Window Output parameters after custom scaling
  result = OV7670WriteReg(HSTART, HSTART_VALUE_QQVGA );
  sresult = ParseI2CResult(result);
  printf("HSTART: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HSTOP, HSTOP_VALUE_QQVGA );
  sresult = ParseI2CResult(result);
  printf("HSTOP: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(HREF, HREF_VALUE_QQVGA );
  sresult = ParseI2CResult(result);
  printf("HREF: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(VSTRT, VSTRT_VALUE_QQVGA );
  sresult = ParseI2CResult(result);
  printf("VSTRT: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(VSTOP, VSTOP_VALUE_QQVGA );
  sresult = ParseI2CResult(result);
  printf("VSTOP: ");
  printf("%s \r\n", sresult.c_str());
  result = OV7670WriteReg(VREF, VREF_VALUE_QQVGA );
  sresult = ParseI2CResult(result);
  printf("VREF: ");
  printf("%s \r\n", sresult.c_str());
}
