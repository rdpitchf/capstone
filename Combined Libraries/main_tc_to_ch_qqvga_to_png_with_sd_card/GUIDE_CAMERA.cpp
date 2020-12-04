
#include "mbed.h"

#include "OV7670_SCCB.cpp"
#include "GUIDE_DEFINE_REGISTERS.cpp"

static OV7670_SCCB i2c;

// static string FPSParam = "ThirtyFPS" or "NightMode"
// static string AECParam = "AveAEC" or "HistAEC"
// static string AWBParam = "SAWB" or "AAWB"
// static string DenoiseParam = "DenoiseYes" or "DenoiseNo"
// static string EdgeParam = "EdgeYes" or "EdgeNo"
// static string ABLCParam = "AblcON" or "AblcOFF"

static string FPSParam = "ThirtyFPS";
static string AWBParam = "SAWB";
static string AECParam = "HistAEC";
static string YUVMatrixParam = "YUVMatrixOn";
static string DenoiseParam = "DenoiseYes";
static string EdgeParam = "EdgeYes";
static string ABLCParam = "AblcON";

static uint8_t frame_pic[640*480*1]{0};

class Camera{

  public:
    // Camera input/output pin connection to Arduino
    DigitalOut wrst; // Output Write Pointer Reset
    DigitalOut rrst; // Output Read Pointer Reset
    DigitalOut wen; // Output Write Enable
    DigitalIn vsync; // Input Vertical Sync marking frame capture
    DigitalOut rclk; // Output FIFO buffer output clock
    // set OE to low gnd
    // FIFO Ram input pins
    DigitalIn d0;
    DigitalIn d1;
    DigitalIn d2;
    DigitalIn d3;
    DigitalIn d4;
    DigitalIn d5;
    DigitalIn d6;
    DigitalIn d7;

    uint16_t PHOTO_WIDTH = 160;
    uint16_t PHOTO_HEIGHT = 120;
    uint16_t PHOTO_BYTES_PER_PIXEL = 2;

    uint8_t frame[1];

    int sd_card_counter = 0;


    Camera():wrst(PD_15), rrst(PF_12), wen(PE_4), vsync(PE_5), rclk(PE_2),
    d0(PD_9),d1(PD_8),d2(PF_15),d3(PE_13),d4(PF_14),d5(PE_11),d6(PE_9),d7(PF_13){

      printf("---------- Camera Registers ----------\r\n");
      this->ResetCameraRegisters();
      printf("---------- ResetCameraRegisters Completed ----------\r\n");
      this->ReadRegisters();
      printf("-------------------------\r\n");
      this->InitializeOV7670Camera();
      printf("FINISHED INITIALIZING CAMERA \r\n \r\n");
    }

    // Initalization on Startup
    void ResetCameraRegisters(){
      // Reset Camera Registers
      // Reading needed to prevent error
      uint8_t data = i2c.read(COM7);
      // uint8_t i2c.write_until_true(COM7, COM7_VALUE_RESET );
      while(!i2c.write_3_phase(COM7, COM7_VALUE_RESET)){}
      printf("RESETTING ALL REGISTERS BY SETTING COM7 REGISTER to 0x80: %X \r\n", data);
      // Delay at least 500ms
      ThisThread::sleep_for(500ms);
    }
    void ReadRegisters(){
      uint8_t data = 0;

      printf("\r\n");
      data = i2c.read(CLKRC);
      printf("CLKRC = %X \r\n", data);
      data = i2c.read(COM7);
      printf("COM7 = %X \r\n", data);
      data = i2c.read(COM3);
      printf("COM3 = %X \r\n", data);
      data = i2c.read(COM14);
      printf("COM14 = %X \r\n", data);
      data = i2c.read(SCALING_XSC);
      printf("SCALING_XSC = %X \r\n", data);
      data = i2c.read(SCALING_YSC);
      printf("SCALING_YSC = %X \r\n", data);
      data = i2c.read(SCALING_DCWCTR);
      printf("SCALING_DCWCTR = %X \r\n", data);
      data = i2c.read(SCALING_PCLK_DIV);
      printf("SCALING_PCLK_DIV = %X \r\n", data);
      data = i2c.read(SCALING_PCLK_DELAY);
      printf("SCALING_PCLK_DELAY = %X \r\n", data);
      // default value D
      data = i2c.read(TSLB);
      printf("TSLB (YUV Order- Higher Bit, Bit[3]) = %X \r\n", data);
      // default value 88
      data = i2c.read(COM13);
      printf("COM13 (YUV Order - Lower Bit, Bit[1]) = %X \r\n", data);
      data = i2c.read(COM17);
      printf("COM17 (DSP Color Bar Selection) = %X \r\n", data);
      data = i2c.read(COM4);
      printf("COM4 (works with COM 17) = %X \r\n", data);
      data = i2c.read(COM15);
      printf("COM15 (COLOR FORMAT SELECTION) = %X \r\n", data);
      data = i2c.read(COM11);
      printf("COM11 (Night Mode) = %X \r\n", data);
      data = i2c.read(COM8);
      printf("COM8 (Color Control, AWB) = %X \r\n", data);
      data = i2c.read(HAECC7);
      printf("HAECC7 (AEC Algorithm Selection) = %X \r\n", data);
      data = i2c.read(GFIX);
      printf("GFIX = %X \r\n", data);
      // Window Output
      data = i2c.read(HSTART);
      printf("HSTART = %X \r\n", data);
      data = i2c.read(HSTOP);
      printf("HSTOP = %X \r\n", data);
      data = i2c.read(HREF);
      printf("HREF = %X \r\n", data);
      data = i2c.read(VSTRT);
      printf("VSTRT = %X \r\n", data);
      data = i2c.read(VSTOP);
      printf("VSTOP = %X \r\n", data);
      data = i2c.read(VREF);
      printf("VREF = %X \r\n", data);
    }
    void InitializeOV7670Camera(){
      printf("Initializing OV7670 Camera \r\n");
      //Set WRST to 0 and RRST to 0 , 0.1ms after power on.
      uint8_t DurationMicroSecs1 = 1;
      uint32_t DurationMicroSecs2 = 1000;
      // Set mode for pins wither input or output
      // ALREADY DONE IN DECLARATION

      // Delay 1 ms
      ThisThread::sleep_for(1ms);
      PulseLowEnabledPin(&wrst, DurationMicroSecs1); // Need to clock the fifo manually to get it to reset
      rrst.write(0);
      PulsePin(&rclk, DurationMicroSecs2);
      PulsePin(&rclk, DurationMicroSecs2);
      rrst.write(1);
    }
    void PulseLowEnabledPin(DigitalOut *pin, uint8_t DurationMicroSecs){
      // For Low Enabled Pins , 0 = on and 1 = off
      pin->write(0); // Sets the pin on
      wait_us(DurationMicroSecs); // Pauses for DurationMicroSecs microseconds
      pin->write(1); // Sets the pin off
      wait_us(DurationMicroSecs); // Pauses for DurationMicroSecs microseconds
    }
    void PulsePin(DigitalOut *pin, uint32_t DurationNanoSecs){
      pin->write(1); // Sets the pin on
      wait_ns(DurationNanoSecs); // Pauses for DurationMicroSecs microseconds
      pin->write(0); // Sets the pin off
      wait_ns(DurationNanoSecs); // Pauses for DurationMicroSecs microseconds
    }

    // FPS Functions
    void SetCameraFPSMode(){
      // Set FPS for Camera
      if (FPSParam == "ThirtyFPS") {
        SetupCameraFor30FPS();
      }else if (FPSParam == "NightMode"){
        SetupCameraNightMode();
      }
    }
    void SetupCameraFor30FPS(){
      printf("..... Setting Camera to 30 FPS ....\r\n");

      i2c.write_until_true(CLKRC, CLKRC_VALUE_30FPS);
      // i2c.write_until_true(CLKRC, 0x01);
      printf("CLKRC Completed. \r\n");
      i2c.write_until_true(DBLV, DBLV_VALUE_30FPS);
      // i2c.write_until_true(DBLV, 0x0A);
      printf("DBLV Completed. \r\n");
      i2c.write_until_true(EXHCH, EXHCH_VALUE_30FPS);
      printf("EXHCH Completed. \r\n");
      i2c.write_until_true(EXHCL, EXHCL_VALUE_30FPS);
      printf("EXHCL Completed. \r\n");
      i2c.write_until_true(DM_LNL, DM_LNL_VALUE_30FPS);
      // i2c.write_until_true(DM_LNL, 0x2B);
      printf("DM_LNL Completed. \r\n");
      i2c.write_until_true(DM_LNH, DM_LNH_VALUE_30FPS);
      printf("DM_LNH Completed. \r\n");
      i2c.write_until_true(COM11, COM11_VALUE_30FPS);
      printf("COM11 Completed. \r\n");
    }
    void SetupCameraNightMode(){
      printf("%s \r\n","... Turning NIGHT MODE ON ....");

      i2c.write_until_true(CLKRC, CLKRC_VALUE_NIGHTMODE_AUTO);
      printf("CLKRC Completed. \r\n");
      i2c.write_until_true(COM11, COM11_VALUE_NIGHTMODE_AUTO);
      printf("COM11 Completed. \r\n");
    }

    // Automatic Exposure Functions
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
      printf("%s \r\n","----- Setting Camera Average Based AEC/AGC Registers -----");

      i2c.write_until_true(AEW, AEW_VALUE);
      printf("AEW Completed. \r\n");
      i2c.write_until_true(AEB, AEB_VALUE);
      printf("AEB Completed. \r\n");
      i2c.write_until_true(VPT, VPT_VALUE);
      printf("VPT Completed. \r\n");
      i2c.write_until_true(HAECC7, HAECC7_VALUE_AVERAGE_AEC_ON);
      printf("HAECC7 Completed. \r\n");
    }
    void SetCameraHistogramBasedAECAGC(){
      printf("%s \r\n","----- Setting Camera Histogram Based AEC/AGC Registers -----");

      i2c.write_until_true(AEW, AEW_VALUE);
      printf("AEW Completed. \r\n");
      i2c.write_until_true(AEB, AEB_VALUE);
      printf("AEB Completed. \r\n");
      i2c.write_until_true(HAECC1, HAECC1_VALUE);
      printf("HAECC1 Completed. \r\n");
      i2c.write_until_true(HAECC2, HAECC2_VALUE);
      printf("HAECC2 Completed. \r\n");
      i2c.write_until_true(HAECC3, HAECC3_VALUE);
      printf("HAECC3 Completed. \r\n");
      i2c.write_until_true(HAECC4, HAECC4_VALUE);
      printf("HAECC4 Completed. \r\n");
      i2c.write_until_true(HAECC5, HAECC5_VALUE);
      printf("HAECC5 Completed. \r\n");
      i2c.write_until_true(HAECC6, HAECC6_VALUE);
      printf("HAECC6 Completed. \r\n");
      i2c.write_until_true(HAECC7, HAECC7_VALUE_HISTOGRAM_AEC_ON);
      printf("HAECC7 Completed. \r\n");
    }

    // Automatic Balancing Functions
    void SetupCameraAWB(){
      // Set AWB Mode
      if (AWBParam == "SAWB"){
        // Set Simple Automatic White Balance
        SetupCameraSimpleAutomaticWhiteBalance(); // OK
        // Set Gain Config
        //
        // Ignoring functions:
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
      printf("%s \r\n","..... Setting Camera to Simple AWB ....");

      // COM8
      i2c.write_until_true(COM8, COM8_VALUE_AWB_ON);
      printf("COM8(0x13) Completed. \r\n");
      // AWBCTR0
      i2c.write_until_true(AWBCTR0, AWBCTR0_VALUE_NORMAL);
      printf("AWBCTR0 Control Register 0(0x6F) Completed. \r\n");
    }
    void SetupCameraGain(){
      printf("%s \r\n","..... Setting Camera Gain ....");

      // Set Maximum Gain
      i2c.write_until_true(COM9, COM9_VALUE_4XGAIN);
      printf("COM9 Completed. \r\n");

      // Set Blue Gain
      i2c.write_until_true(BLUE, BLUE_VALUE);
      printf("BLUE GAIN Completed. \r\n");

      // Set Red Gain
      i2c.write_until_true(RED, RED_VALUE);
      printf("RED GAIN Completed. \r\n");

      // Set Green Gain
      i2c.write_until_true(GGAIN, GGAIN_VALUE);
      printf("GREEN GAIN Completed. \r\n");

      // Enable AWB Gain
      i2c.write_until_true(COM16, COM16_VALUE);
      printf("COM16(ENABLE GAIN) Completed. \r\n");
    }
    void SetupCameraAdvancedAutomaticWhiteBalance(){
      printf("%s \r\n","..... Setting Camera to Advanced AWB....");

      // AGC, AWB, and AEC Enable
      i2c.write_until_true(0x13, 0xE7);
      printf("COM8(0x13) Completed. \r\n");
      // AWBCTR0
      i2c.write_until_true(0x6f, 0x9E);
      printf("AWB Control Register 0(0x6F) Completed. \r\n");
    }
    void SetupCameraAdvancedAutoWhiteBalanceConfig(){
      printf("%s \r\n","..... Setting Camera Advanced Auto White Balance Configs....");

      i2c.write_until_true(AWBC1, AWBC1_VALUE);
      printf("AWBC1 Completed. \r\n");
      i2c.write_until_true(AWBC2, AWBC2_VALUE);
      printf("AWBC2 Completed. \r\n");
      i2c.write_until_true(AWBC3, AWBC3_VALUE);
      printf("AWBC3 Completed. \r\n");
      i2c.write_until_true(AWBC4, AWBC4_VALUE);
      printf("AWBC4 Completed. \r\n");
      i2c.write_until_true(AWBC5, AWBC5_VALUE);
      printf("AWBC5 Completed. \r\n");
      i2c.write_until_true(AWBC6, AWBC6_VALUE);
      printf("AWBC6 Completed. \r\n");
      i2c.write_until_true(AWBC7, AWBC7_VALUE);
      printf("AWBC7 Completed. \r\n");
      i2c.write_until_true(AWBC8, AWBC8_VALUE);
      printf("AWBC8 Completed. \r\n");
      i2c.write_until_true(AWBC9, AWBC9_VALUE);
      printf("AWBC9 Completed. \r\n");
      i2c.write_until_true(AWBC10, AWBC10_VALUE);
      printf("AWBC10 Completed. \r\n");
      i2c.write_until_true(AWBC11, AWBC11_VALUE);
      printf("AWBC11 Completed. \r\n");
      i2c.write_until_true(AWBC12, AWBC12_VALUE);
      printf("AWBC12 Completed. \r\n");
      i2c.write_until_true(AWBCTR3, AWBCTR3_VALUE);
      printf("AWBCTR3 Completed. \r\n");
      i2c.write_until_true(AWBCTR2, AWBCTR2_VALUE);
      printf("AWBCTR2 Completed. \r\n");
      i2c.write_until_true(AWBCTR1, AWBCTR1_VALUE);
      printf("AWBCTR1 Completed. \r\n");
    }

    // Setup Undocumented Registers - Needed Minimum
    void SetupCameraUndocumentedRegisters(){
      printf("%s \r\n","..... Setting Camera Undocumented Registers ....");

      i2c.write_until_true(0xB0, 0x84);
      printf("Setting B0 UNDOCUMENTED register to 0x84. \r\n");
    }
    // Set Color Matrix for YUV
    void SetCameraColorMatrixYUV(){
      printf("%s \r\n","..... Setting Camera Color Matrix for YUV ....");

      i2c.write_until_true(MTX1, MTX1_VALUE);
      printf("MTX1 Completed. \r\n");
      i2c.write_until_true(MTX2, MTX2_VALUE);
      printf("MTX2 Completed. \r\n");
      i2c.write_until_true(MTX3, MTX3_VALUE);
      printf("MTX3 Completed. \r\n");
      i2c.write_until_true(MTX4, MTX4_VALUE);
      printf("MTX4 Completed. \r\n");
      i2c.write_until_true(MTX5, MTX5_VALUE);
      printf("MTX5 Completed. \r\n");
      i2c.write_until_true(MTX6, MTX6_VALUE);
      printf("MTX6 Completed. \r\n");
      i2c.write_until_true(CONTRAS, CONTRAS_VALUE);
      printf("CONTRAS Completed. \r\n");
      i2c.write_until_true(MTXS, MTXS_VALUE);
      printf("MTXS Completed. \r\n");
    }
    // Set Camera Saturation
    void SetCameraSaturationControl(){
      printf("%s \r\n","..... Setting Camera Saturation Level ....");

      i2c.write_until_true_saturation(SATCTR, SATCTR_VALUE);
      printf("SATCTR Completed. \r\n");
    }
    // Denoise and Edge Enhancement
    void SetupCameraDenoiseEdgeEnhancement(){

      if ((DenoiseParam == "DenoiseYes")&&(EdgeParam == "EdgeYes")){
        SetupCameraDenoise();
        SetupCameraEdgeEnhancement();
        i2c.write_until_true(COM16,COM16_VALUE_DENOISE_ON__EDGE_ENHANCEMENT_ON__AWBGAIN_ON);
        printf("COM16 Completed. \r\n");
      }else if ((DenoiseParam == "DenoiseYes")&&(EdgeParam == "EdgeNo")){
        SetupCameraDenoise();
        i2c.write_until_true(COM16,COM16_VALUE_DENOISE_ON__EDGE_ENHANCEMENT_OFF__AWBGAIN_ON);
        printf("COM16 Completed. \r\n");
      }else if ((DenoiseParam == "DenoiseNo")&&(EdgeParam == "EdgeYes")){
        SetupCameraEdgeEnhancement();
        i2c.write_until_true(COM16,COM16_VALUE_DENOISE_OFF__EDGE_ENHANCEMENT_ON__AWBGAIN_ON);
        printf("COM16 Completed. \r\n");
      }
    }
    void SetupCameraDenoise(){
      printf("%s \r\n","..... Setting Camera Denoise ....");

      i2c.write_until_true(DNSTH, DNSTH_VALUE);
      printf("DNSTH Completed. \r\n");
      i2c.write_until_true(REG77, REG77_VALUE);
      printf("REG77 Completed. \r\n");
    }
    void SetupCameraEdgeEnhancement(){
      printf("%s \r\n","..... Setting Camera Edge Enhancement ....");

      i2c.write_until_true(EDGE, EDGE_VALUE);
      printf("EDGE Completed. \r\n");
      i2c.write_until_true(REG75, REG75_VALUE);
      // i2c.write_until_true(REG75, 0x00);
      printf("REG75 Completed. \r\n");
      i2c.write_until_true(REG76, REG76_VALUE);
      printf("REG76 Completed. \r\n");
    }
    // Set Array Control
    void SetupCameraArrayControl(){
      printf("%s \r\n","..... Setting Camera Array Control ....");

      i2c.write_until_true(CHLF, CHLF_VALUE);
      printf("CHLF Completed. \r\n");
      i2c.write_until_true(ARBLM, ARBLM_VALUE);
      printf("ARBLM Completed. \r\n");
    }
    // Set ADC Control
    void SetupCameraADCControl(){
      printf("%s \r\n","..... Setting Camera ADC Control ....");

      i2c.write_until_true(ADCCTR1, ADCCTR1_VALUE);
      printf("ADCCTR1 Completed. \r\n");
      i2c.write_until_true(ADCCTR2, ADCCTR2_VALUE);
      printf("ADCCTR2 Completed. \r\n");
      i2c.write_until_true(ADC, ADC_VALUE);
      printf("ADC Completed. \r\n");
      i2c.write_until_true(ACOM, ACOM_VALUE);
      printf("ACOM Completed. \r\n");
      i2c.write_until_true(OFON, OFON_VALUE);
      printf("OFON Completed. \r\n");
    }
    // Set Automatic Black Level Calibration
    void SetupCameraABLC(){
      // If ABLC is off then return otherwise
      // turn on ABLC.
      if (ABLCParam == "AblcOFF"){
        return;
      }
      printf("%s \r\n",".... Setting Camera ABLC ...");

      i2c.write_until_true(ABLC1, ABLC1_VALUE);
      printf("ABLC1 Completed. \r\n");
      i2c.write_until_true(THL_ST, THL_ST_VALUE);
      printf("THL_ST Completed. \r\n");
    }

    void SetupOV7670ForQQVGAYUV(){
      printf("--------- Setting Camera for QQVGA YUV --------- \r\n");

      PHOTO_WIDTH = 160;
      PHOTO_HEIGHT = 120;
      PHOTO_BYTES_PER_PIXEL = 2;
      printf("Photo Width = ");
      printf("%d \r\n",PHOTO_WIDTH);
      printf("Photo Height = ");
      printf("%d \r\n",PHOTO_HEIGHT);
      printf("Bytes Per Pixel = ");
      printf("%d \r\n",PHOTO_BYTES_PER_PIXEL);

      printf("..... Setting Basic QQVGA Parameters .... \r\n");

      i2c.write_until_true(CLKRC, CLKRC_VALUE_QQVGA);
      printf("CLKRC Completed. \r\n");
      i2c.write_until_true(COM7, COM7_VALUE_QQVGA );
      // i2c.write_until_true(COM7, COM7_VALUE_QQVGA_COLOR_BAR );
      printf("COM7 Completed. \r\n");
      i2c.write_until_true(COM3, COM3_VALUE_QQVGA);
      // i2c.write_until_true(COM3, COM3_VALUE_QQVGA_SCALE_ENABLED);
      printf("COM3 Completed. \r\n");
      i2c.write_until_true(COM14, COM14_VALUE_QQVGA );
      // i2c.write_until_true(COM14, 0x18 );
      printf("COM14 Completed. \r\n");
      i2c.write_until_true(SCALING_XSC,SCALING_XSC_VALUE_QQVGA );
      // i2c.write_until_true(SCALING_XSC,0xba );
      printf("SCALING_XSC Completed. \r\n");
      i2c.write_until_true(SCALING_YSC,SCALING_YSC_VALUE_QQVGA );
      // i2c.write_until_true(SCALING_YSC,0xb5 );
      printf("SCALING_YSC Completed. \r\n");
      i2c.write_until_true(SCALING_DCWCTR,SCALING_DCWCTR_VALUE_QQVGA );
      printf("SCALING_DCWCTR Completed. \r\n");
      i2c.write_until_true(SCALING_PCLK_DIV,SCALING_PCLK_DIV_VALUE_QQVGA);
      printf("SCALING_PCLK_DIV Completed. \r\n");
      i2c.write_until_true(SCALING_PCLK_DELAY,SCALING_PCLK_DELAY_VALUE_QQVGA);
      printf("SCALING_PCLK_DELAY Completed. \r\n");
      // YUV order control change from default use with COM13
      i2c.write_until_true(TSLB,TSLB_VALUE_YUYV_AUTO_OUTPUT_WINDOW_DISABLED); // Works
      printf("TSLB Completed. \r\n");
      //COM13
      i2c.write_until_true(COM13, 0xC8); // Gamma Enabled, UV Auto Adj On
      printf("COM13 Completed. \r\n");
      // COM17 - DSP Color Bar Enable/Disable
      // i2c.write_until_true(COM17,COM17_VALUE_AEC_NORMAL_COLOR_BAR);
      i2c.write_until_true(COM17,COM17_VALUE_AEC_NORMAL_NO_COLOR_BAR);
      printf("COM17 Completed. \r\n");

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

      i2c.write_until_true(HSTART, HSTART_VALUE_QQVGA );
      printf("HSTART Completed. \r\n");
      i2c.write_until_true(HSTOP, HSTOP_VALUE_QQVGA );
      printf("HSTOP Completed. \r\n");
      i2c.write_until_true(HREF, HREF_VALUE_QQVGA );
      printf("HREF Completed. \r\n");
      i2c.write_until_true(VSTRT, VSTRT_VALUE_QQVGA );
      printf("VSTRT Completed. \r\n");
      i2c.write_until_true(VSTOP, VSTOP_VALUE_QQVGA );
      printf("VSTOP Completed. \r\n");
      i2c.write_until_true(VREF, VREF_VALUE_QQVGA );
      printf("VREF Completed. \r\n");
    }
    void SetupOV7670ForVGARawRGB(){
      printf("--------- Setting Camera for VGA (Raw RGB) --------- \r\n");
      PHOTO_WIDTH = 640;
      PHOTO_HEIGHT = 480;
      PHOTO_BYTES_PER_PIXEL = 1;
      printf("Photo Width = %d \r\n",PHOTO_WIDTH);
      printf("Photo Height = %d \r\n",PHOTO_HEIGHT);
      printf("Bytes Per Pixel = %d \r\n",PHOTO_BYTES_PER_PIXEL);
      // Basic Registers
      i2c.write_until_true(CLKRC, CLKRC_VALUE_VGA);
      printf("CLKRC Completed. \r\n");
      // i2c.write_until_true(COM7, COM7_VALUE_VGA );
      i2c.write_until_true(COM7, COM7_VALUE_VGA_COLOR_BAR );
      printf("COM7 Completed. \r\n");
      i2c.write_until_true(COM3, COM3_VALUE_VGA);
      printf("COM3 Completed. \r\n");
      i2c.write_until_true(COM14, COM14_VALUE_VGA );
      printf("COM14 Completed. \r\n");
      i2c.write_until_true(SCALING_XSC,SCALING_XSC_VALUE_VGA );
      printf("SCALING_XSC Completed. \r\n");
      i2c.write_until_true(SCALING_YSC,SCALING_YSC_VALUE_VGA );
      printf("SCALING_YSC Completed. \r\n");
      i2c.write_until_true(SCALING_DCWCTR, SCALING_DCWCTR_VALUE_VGA);
      printf("SCALING_DCWCTR Completed. \r\n");
      i2c.write_until_true(SCALING_PCLK_DIV,SCALING_PCLK_DIV_VALUE_VGA);
      printf("SCALING_PCLK_DIV Completed. \r\n");
      i2c.write_until_true(SCALING_PCLK_DELAY,SCALING_PCLK_DELAY_VALUE_VGA);
      printf("SCALING_PCLK_DELAY Completed. \r\n");

      // COM17 - DSP Color Bar Enable/Disable
      // COM17_VALUE 0x08 // Activate Color Bar for DSP
      i2c.write_until_true(COM17,COM17_VALUE_AEC_NORMAL_COLOR_BAR);
      // i2c.write_until_true(COM17,COM17_VALUE_AEC_NORMAL_NO_COLOR_BAR);
      printf("COM17 Completed. \r\n");

      // Set Additional Parameters
      // Set Camera Frames per second
      SetCameraFPSMode();
      // Set Camera Automatic Exposure Control
      SetCameraAEC();
      // Needed Color Correction, green to red
      i2c.write_until_true(0xB0, 0x8c);
      printf("Setting B0 UNDOCUMENTED register to 0x84:= \r\n");

      // Set Camera Saturation
      SetCameraSaturationControl();
      // Setup Camera Array Control
      SetupCameraArrayControl();
      // Set ADC Control
      SetupCameraADCControl();
      // Set Automatic Black Level Calibration
      SetupCameraABLC();
      printf("..... Setting Camera Window Output Parameters .... \r\n");
      // Change Window Output parameters after custom scaling
      i2c.write_until_true(HSTART, HSTART_VALUE_VGA );
      printf("HSTART Completed. \r\n");
      i2c.write_until_true(HSTOP, HSTOP_VALUE_VGA );
      printf("HSTOP Completed. \r\n");
      i2c.write_until_true(HREF, HREF_VALUE_VGA );
      printf("HREF Completed. \r\n");
      i2c.write_until_true(VSTRT, VSTRT_VALUE_VGA );
      printf("VSTRT Completed. \r\n");
      i2c.write_until_true(VSTOP, VSTOP_VALUE_VGA );
      printf("VSTOP Completed. \r\n");
      i2c.write_until_true(VREF, VREF_VALUE_VGA );
      printf("VREF Completed. \r\n");

    }
    void SetupOV7670ForVGAProcessedBayerRGB(){
      // Call Base for VGA Raw Bayer RGB Mode
      SetupOV7670ForVGARawRGB();
      printf("----- Setting Camera for VGA (Processed Bayer RGB) ------ \r\n");
      // Set key register for selecting processed bayer rgb output
      // i2c.write_until_true(COM7, COM7_VALUE_VGA_PROCESSED_BAYER );
      i2c.write_until_true(COM7, COM7_VALUE_VGA_COLOR_BAR );
      printf("COM7 Completed. \r\n");
      i2c.write_until_true(TSLB, 0x04);
      printf("Initializing TSLB register result = \r\n");
      // Needed Color Correction, green to red
      i2c.write_until_true(0xB0, 0x8c);
      printf("Setting B0 UNDOCUMENTED register to 0x84:= \r\n");
      // Set Camera Automatic White Balance
      SetupCameraAWB();
      // Denoise and Edge Enhancement
      SetupCameraDenoiseEdgeEnhancement();
    }
    void SetupOV7670ForQVGAYUV(){
      printf("--------- Setting Camera for QVGA (YUV) --------- \r\n");
      PHOTO_WIDTH = 320;
      PHOTO_HEIGHT = 240;
      PHOTO_BYTES_PER_PIXEL = 2;
      printf("Photo Width = %d \r\n",PHOTO_WIDTH);
      printf("Photo Height = %d \r\n",PHOTO_HEIGHT);
      printf("Bytes Per Pixel = %d \r\n",PHOTO_BYTES_PER_PIXEL);
      // Basic Registers
      i2c.write_until_true(CLKRC, CLKRC_VALUE_QVGA);
      printf("CLKRC Completed. \r\n");
      // i2c.write_until_true(COM7, COM7_VALUE_QVGA );
      i2c.write_until_true(COM7, COM7_VALUE_QVGA_COLOR_BAR );
      printf("COM7 Completed. \r\n");
      i2c.write_until_true(COM3, COM3_VALUE_QVGA);
      printf("COM3 Completed. \r\n");
      i2c.write_until_true(COM14, COM14_VALUE_QVGA );
      printf("COM14 Completed. \r\n");
      i2c.write_until_true(SCALING_XSC,SCALING_XSC_VALUE_QVGA );
      printf("SCALING_XSC Completed. \r\n");
      i2c.write_until_true(SCALING_YSC,SCALING_YSC_VALUE_QVGA );
      printf("SCALING_YSC Completed. \r\n");
      i2c.write_until_true(SCALING_DCWCTR,SCALING_DCWCTR_VALUE_QVGA );
      printf("SCALING_DCWCTR Completed. \r\n");
      i2c.write_until_true(SCALING_PCLK_DIV,SCALING_PCLK_DIV_VALUE_QVGA);
      printf("SCALING_PCLK_DIV Completed. \r\n");
      i2c.write_until_true(SCALING_PCLK_DELAY,SCALING_PCLK_DELAY_VALUE_QVGA);
      printf("SCALING_PCLK_DELAY Completed. \r\n");
      // YUV order control change from default use with COM13
      i2c.write_until_true(TSLB, 0x04);
      printf("TSLB Completed. \r\n");
       //COM13
      i2c.write_until_true(COM13, 0xC2); // from YCbCr reference specs
      printf("COM13 Completed. \r\n");
      // COM17 - DSP Color Bar Enable/Disable
      // COM17_VALUE 0x08 // Activate Color Bar for DSP
      i2c.write_until_true(COM17,COM17_VALUE_AEC_NORMAL_COLOR_BAR);
      // i2c.write_until_true(COM17,COM17_VALUE_AEC_NORMAL_NO_COLOR_BAR);
      printf("COM17 Completed. \r\n");
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
      printf("..... Setting Camera Window Output Parameters .... \r\n");
      // Change Window Output parameters after custom scaling
      i2c.write_until_true(HSTART, HSTART_VALUE_QVGA );
      printf("HSTART Completed. \r\n");
      i2c.write_until_true(HSTOP, HSTOP_VALUE_QVGA );
      printf("HSTOP Completed. \r\n");
      i2c.write_until_true(HREF, HREF_VALUE_QVGA );
      printf("HREF Completed. \r\n");
      i2c.write_until_true(VSTRT, VSTRT_VALUE_QVGA );
      printf("VSTRT Completed. \r\n");
      i2c.write_until_true(VSTOP, VSTOP_VALUE_QVGA );
      printf("VSTOP Completed. \r\n");
      i2c.write_until_true(VREF, VREF_VALUE_QVGA );
      printf("VREF Completed. \r\n");
    }
    void new_parameters(){
      PHOTO_WIDTH = 640;
      PHOTO_HEIGHT = 480;
      PHOTO_BYTES_PER_PIXEL = 1;
      i2c.write_until_true(0x8c, 0x00);              // Disable RGB444
      i2c.write_until_true(0x15, 0x02);               // 0x02   VSYNC negative (http://nasulica.homelinux.org/?p=959)
      i2c.write_until_true(0x1e, 0x27);                // mirror image

      i2c.write_until_true(0x11, 0x80);               // prescaler x1
      i2c.write_until_true(0x6b, 0x0a);                    // bypass PLL

      i2c.write_until_true(0x3b, 0x0A) ;
      i2c.write_until_true(0x3a, 0x04);                // 0D = UYVY  04 = YUYV
      i2c.write_until_true(0x3d, 0x88);               // connect to 0x3a

      i2c.write_until_true(0x12, 0x00);           // YUV
      i2c.write_until_true(0x42, 0x00);          // color bar disable
      i2c.write_until_true(0x0c, 0x04);
      i2c.write_until_true(0x40, 0xC0);          // Set normal rgb with Full range

      i2c.write_until_true(0x11, 0x01);
      i2c.write_until_true(0x3a, 0x04);
      i2c.write_until_true(0x12, 0x01);
      i2c.write_until_true(0x6b, 0x4a);
      i2c.write_until_true(0x0c, 0x00);
      i2c.write_until_true(0x3e, 0x00);

      i2c.write_until_true(0x17, 0x13);
      i2c.write_until_true(0x18, 0x01);
      i2c.write_until_true(0x32, 0xb6);
      i2c.write_until_true(0x19, 0x02);
      i2c.write_until_true(0x1a, 0x7a);
      i2c.write_until_true(0x03, 0x0a);
      i2c.write_until_true(0x72, 0x11);
      i2c.write_until_true(0x73, 0xf0);

      /* Gamma curve values */
      i2c.write_until_true(0x7a, 0x20);
      i2c.write_until_true(0x7b, 0x10);
      i2c.write_until_true(0x7c, 0x1e);
      i2c.write_until_true(0x7d, 0x35);
      i2c.write_until_true(0x7e, 0x5a);
      i2c.write_until_true(0x7f, 0x69);
      i2c.write_until_true(0x80, 0x76);
      i2c.write_until_true(0x81, 0x80);
      i2c.write_until_true(0x82, 0x88);
      i2c.write_until_true(0x83, 0x8f);
      i2c.write_until_true(0x84, 0x96);
      i2c.write_until_true(0x85, 0xa3);
      i2c.write_until_true(0x86, 0xaf);
      i2c.write_until_true(0x87, 0xc4);
      i2c.write_until_true(0x88, 0xd7);
      i2c.write_until_true(0x89, 0xe8);

      /* AGC and AEC parameters.  Note we start by disabling those features,
      then turn them only after tweaking the values. */
      i2c.write_until_true(0x13, 0x80 | 0x40 | 0x20);
      i2c.write_until_true(0x00, 0x00);
      i2c.write_until_true(0x10, 0x00);
      i2c.write_until_true(0x0d, 0x40);
      i2c.write_until_true(0x14, 0x18);
      i2c.write_until_true(0xa5, 0x05);
      i2c.write_until_true(0xab, 0x07);
      i2c.write_until_true(0x24, 0x95);
      i2c.write_until_true(0x25, 0x33);
      i2c.write_until_true(0x26, 0xe3);
      i2c.write_until_true(0x9f, 0x78);
      i2c.write_until_true(0xa0, 0x68);
      i2c.write_until_true(0xa1, 0x03);
      i2c.write_until_true(0xa6, 0xd8);
      i2c.write_until_true(0xa7, 0xd8);
      i2c.write_until_true(0xa8, 0xf0);
      i2c.write_until_true(0xa9, 0x90);
      i2c.write_until_true(0xaa, 0x94);
      i2c.write_until_true(0x13, 0x80|0x40|0x20|0x04|0x01);

      /* Almost all of these are magic "reserved" values.  */
      i2c.write_until_true(0x0e, 0x61);
      i2c.write_until_true(0x0f, 0x4b);
      i2c.write_until_true(0x16, 0x02);
      i2c.write_until_true(0x1e, 0x27);
      i2c.write_until_true(0x21, 0x02);
      i2c.write_until_true(0x22, 0x91);
      i2c.write_until_true(0x29, 0x07);
      i2c.write_until_true(0x33, 0x0b);
      i2c.write_until_true(0x35, 0x0b);
      i2c.write_until_true(0x37, 0x1d);
      i2c.write_until_true(0x38, 0x71);
      i2c.write_until_true(0x39, 0x2a);
      i2c.write_until_true(0x3c, 0x78);
      i2c.write_until_true(0x4d, 0x40);
      i2c.write_until_true(0x4e, 0x20);
      i2c.write_until_true(0x69, 0x00);
      i2c.write_until_true(0x6b, 0x0a);
      i2c.write_until_true(0x74, 0x10);
      i2c.write_until_true(0x8d, 0x4f);
      i2c.write_until_true(0x8e, 0x00);
      i2c.write_until_true(0x8f, 0x00);
      i2c.write_until_true(0x90, 0x00);
      i2c.write_until_true(0x91, 0x00);
      i2c.write_until_true(0x96, 0x00);
      i2c.write_until_true(0x9a, 0x00);
      i2c.write_until_true(0xb0, 0x84);
      i2c.write_until_true(0xb1, 0x0c);
      i2c.write_until_true(0xb2, 0x0e);
      i2c.write_until_true(0xb3, 0x82);
      i2c.write_until_true(0xb8, 0x0a);

      /* More reserved magic, some of which tweaks white balance */
      i2c.write_until_true(0x43, 0x0a);
      i2c.write_until_true(0x44, 0xf0);
      i2c.write_until_true(0x45, 0x34);
      i2c.write_until_true(0x46, 0x58);
      i2c.write_until_true(0x47, 0x28);
      i2c.write_until_true(0x48, 0x3a);
      i2c.write_until_true(0x59, 0x88);
      i2c.write_until_true(0x5a, 0x88);
      i2c.write_until_true(0x5b, 0x44);
      i2c.write_until_true(0x5c, 0x67);
      i2c.write_until_true(0x5d, 0x49);
      i2c.write_until_true(0x5e, 0x0e);
      i2c.write_until_true(0x6c, 0x0a);
      i2c.write_until_true(0x6d, 0x55);
      i2c.write_until_true(0x6e, 0x11);
      i2c.write_until_true(0x6f, 0x9f);
      i2c.write_until_true(0x6a, 0x40);
      i2c.write_until_true(0x01, 0x40);
      i2c.write_until_true(0x02, 0x60);
      i2c.write_until_true(0x13, 0x80|0x40|0x20|0x04|0x01|0x02);

      /* Matrix coefficients */
      i2c.write_until_true(0x4f, 0x80);
      i2c.write_until_true(0x50, 0x80);
      i2c.write_until_true(0x51, 0x00);
      i2c.write_until_true(0x52, 0x22);
      i2c.write_until_true(0x53, 0x5e);
      i2c.write_until_true(0x54, 0x80);
      i2c.write_until_true(0x58, 0x9e);

      i2c.write_until_true(0x41, 0x08);
      i2c.write_until_true(0x3f, 0x00);
      i2c.write_until_true(0x75, 0x05);
      i2c.write_until_true(0x76, 0xe1);
      i2c.write_until_true(0x4c, 0x00);
      i2c.write_until_true(0x77, 0x01);
      i2c.write_until_true(0x3d, 0xc3);
      i2c.write_until_true(0x4b, 0x09);
      i2c.write_until_true_saturation(0xc9, 0x60);
      i2c.write_until_true(0x41, 0x38);
      i2c.write_until_true(0x56, 0x40);

      i2c.write_until_true(0x34, 0x11);
      i2c.write_until_true(0x3b, 0x02|0x10);
      i2c.write_until_true(0xa4, 0x88);
      i2c.write_until_true(0x96, 0x00);
      i2c.write_until_true(0x97, 0x30);
      i2c.write_until_true(0x98, 0x20);
      i2c.write_until_true(0x99, 0x30);
      i2c.write_until_true(0x9a, 0x84);
      i2c.write_until_true(0x9b, 0x29);
      i2c.write_until_true(0x9c, 0x03);
      i2c.write_until_true(0x9d, 0x4c);
      i2c.write_until_true(0x9e, 0x3f);
      i2c.write_until_true(0x78, 0x04);

      /* Extra-weird stuff.  Some sort of multiplexor register */
      i2c.write_until_true(0x79, 0x01);
      i2c.write_until_true(0xc8, 0xf0);
      i2c.write_until_true(0x79, 0x0f);
      i2c.write_until_true(0xc8, 0x00);
      i2c.write_until_true(0x79, 0x10);
      i2c.write_until_true(0xc8, 0x7e);
      i2c.write_until_true(0x79, 0x0a);
      i2c.write_until_true(0xc8, 0x80);
      i2c.write_until_true(0x79, 0x0b);
      i2c.write_until_true(0xc8, 0x01);
      i2c.write_until_true(0x79, 0x0c);
      i2c.write_until_true(0xc8, 0x0f);
      i2c.write_until_true(0x79, 0x0d);
      i2c.write_until_true(0xc8, 0x20);
      i2c.write_until_true(0x79, 0x09);
      i2c.write_until_true(0xc8, 0x80);
      i2c.write_until_true(0x79, 0x02);
      i2c.write_until_true(0xc8, 0xc0);
      i2c.write_until_true(0x79, 0x03);
      i2c.write_until_true(0xc8, 0x40);
      i2c.write_until_true(0x79, 0x05);
      i2c.write_until_true(0xc8, 0x30);
      i2c.write_until_true(0x79, 0x26);

      i2c.write_until_true(0xff, 0xff);
      printf("completed new setup \r\n");
    }
    void TakePhoto(){
      Timer t;
      t.start();
      CaptureOV7670Frame();
      // ReadTransmitCapturedFrame();
      t.stop();
      unsigned long ElapsedTime = std::chrono::duration_cast<std::chrono::seconds>(t.elapsed_time()).count(); // Convert to seconds
      printf("Elapsed Time for Taking Photo(secs) = %lu \r\n", ElapsedTime);
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
      DurationStart = pulseIn(&vsync);
      // 2. Reset Write Pointer to 0. Which is the beginning of frame
      PulseLowEnabledPin(&wrst, 6); // 3 microseconds + 3 microseconds for error factor on Arduino
      // 3. Set FIFO Write Enable to active (high) so that image can be written to ram
      wen.write(1);
      // 4. Wait for VSync to pulse again to indicate the end of the frame capture
      DurationStop = pulseIn(&vsync);
      // 5. Set FIFO Write Enable to nonactive (low) so that no more images can be written to the ram
      wen.write(0);
      // 6. Print out Stats
      t.stop();
      ElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count(); // Convert to seconds
      printf("Time for Frame Capture (milliseconds) = %lu \r\n",ElapsedTime);
      printf("VSync beginning duration (microseconds) = %lu \r\n",DurationStart);
      printf("VSync end duration (microseconds) = %lu \r\n",DurationStop);
      // 7. WAIT so that new data can appear on output pins Read new data.
      ThisThread::sleep_for(2*10ms);
    }
    unsigned long pulseIn(DigitalIn *pin){
      // pulseIn
      Timer t;
      while (!pin->read()); // wait for high
      t.start();
      while (pin->read()); // wait for low
      t.stop();
      return t.elapsed_time().count();
    }

    void prepare_fifo_data(){
      rrst.write(0);
      PulsePin(&rclk, 1000);
      PulsePin(&rclk, 1000);
      PulsePin(&rclk, 1000);
      rrst.write(1);
      this->sd_card_counter = 0;
    }
    uint32_t get_data(uint8_t *buffer, uint32_t max_buffer_size){
      uint32_t counter = 0;
      bool exit = false;

      while(this->sd_card_counter++ < PHOTO_HEIGHT*PHOTO_WIDTH*PHOTO_BYTES_PER_PIXEL){
        this->rclk.write(1);
        // Convert Pin values to byte values for pins 0-7 of incoming pixel byte
        uint8_t PixelData = this->d0.read() | (this->d1.read() << 1) | (this->d2.read() << 2) | (this->d3.read() << 3) | (this->d4.read() << 4) | (this->d5.read() << 5) | (this->d6.read() << 6) |  (this->d7.read() << 7);
        wait_us(1);
        this->rclk.write(0);
        wait_us(1);
        buffer[counter] = PixelData;
        counter++;
        if(counter == max_buffer_size){
          break;
        }
      }
      return counter;
    }
};
