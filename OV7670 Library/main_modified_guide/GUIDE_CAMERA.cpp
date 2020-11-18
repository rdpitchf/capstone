
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
static string YUVMatrixParam = "YUVMatrixOff";
static string DenoiseParam = "DenoiseYes";
static string EdgeParam = "EdgeYes";
static string ABLCParam = "AblcON";


class CAMERA{

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

    BufferedSerial *serial_port;

    CAMERA(BufferedSerial *serial_port_main):wrst(PD_15), rrst(PF_12), wen(PA_5), vsync(PA_6), rclk(PD_14),
    d0(PD_9),d1(PD_8),d2(PF_15),d3(PE_13),d4(PF_14),d5(PE_11),d6(PE_9),d7(PF_13){
      serial_port = serial_port_main;
    }

    // Initalization on Startup
    void ResetCameraRegisters(){
      // Reset Camera Registers
      // Reading needed to prevent error
      uint8_t data = i2c.read(COM7);
      // uint8_t result = OV7670WriteReg(COM7, COM7_VALUE_RESET );
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
      uint8_t DurationMicroSecs = 1;
      // Set mode for pins wither input or output
      // ALREADY DONE IN DECLARATION

      // Delay 1 ms
      ThisThread::sleep_for(1ms);
      PulseLowEnabledPin(&wrst, DurationMicroSecs); // Need to clock the fifo manually to get it to reset
      rrst.write(0);
      PulsePin(&rclk, DurationMicroSecs);
      PulsePin(&rclk, DurationMicroSecs);
      rrst.write(1);
    }
    void PulseLowEnabledPin(DigitalOut *pin, uint8_t DurationMicroSecs){
      // For Low Enabled Pins , 0 = on and 1 = off
      pin->write(0); // Sets the pin on
      wait_us(DurationMicroSecs); // Pauses for DurationMicroSecs microseconds
      pin->write(1); // Sets the pin off
      wait_us(DurationMicroSecs); // Pauses for DurationMicroSecs microseconds
    }
    void PulsePin(DigitalOut *pin, uint8_t DurationMicroSecs){
      pin->write(1); // Sets the pin on
      wait_us(DurationMicroSecs); // Pauses for DurationMicroSecs microseconds
      pin->write(0); // Sets the pin off
      wait_us(DurationMicroSecs); // Pauses for DurationMicroSecs microseconds
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
      printf("COM7 Completed. \r\n");
      i2c.write_until_true(COM3, COM3_VALUE_QQVGA);
      printf("COM3 Completed. \r\n");
      i2c.write_until_true(COM14, COM14_VALUE_QQVGA );
      printf("COM14 Completed. \r\n");
      i2c.write_until_true(SCALING_XSC,SCALING_XSC_VALUE_QQVGA );
      printf("SCALING_XSC Completed. \r\n");
      i2c.write_until_true(SCALING_YSC,SCALING_YSC_VALUE_QQVGA );
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
      //i2c.write_until_true(COM17,COM17_VALUE_AEC_NORMAL_COLOR_BAR);
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

    void TakePhoto(){
      Timer t;
      t.start();
      CaptureOV7670Frame();
      ReadTransmitCapturedFrame();
      t.stop();
      unsigned long ElapsedTime = std::chrono::duration_cast<std::chrono::seconds>(t.elapsed_time()).count(); // Convert to seconds
      printf("Elapsed Time for Taking and Sending Photo(secs) = %lu \r\n", ElapsedTime);
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
      ThisThread::sleep_for(2ms);
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
    void ReadTransmitCapturedFrame(){
      printf("ReadTransmitCapturedFrame. \r\n");

      // Set Read Buffer Pointer to start of frame
      rrst.write(0);
      PulsePin(&rclk, 1);
      PulsePin(&rclk, 1);
      PulsePin(&rclk, 1);
      rrst.write(1);

      uint8_t PixelData = 0;
      unsigned long ByteCounter = 0;

      printf("<<<<< START >>>>> \r\n");
      ThisThread::sleep_for(2s);

      for (int height = 0; height < PHOTO_HEIGHT; height++){
        for (int width = 0; width < PHOTO_WIDTH; width++){
          for (int bytenumber = 0; bytenumber < PHOTO_BYTES_PER_PIXEL; bytenumber++){
          // Pulse the read clock RCLK to bring in new byte of data.
          PulsePin(&rclk, 1);
          // Get data
          // // Convert Pin values to byte values for pins 0-7 of incoming pixel byte
          PixelData = d0.read() | (d1.read() << 1) | (d2.read() << 2) | (d3.read() << 3) | (d4.read() << 4) | (d5.read() << 5) | (d6.read() << 6) |  (d7.read() << 7);

          frame[0] = PixelData;
          // printf("%d,", PixelData);
          serial_port->write(frame, 1);
          ByteCounter += 1;
          }
        }
      }
      printf("\r\n<<<<< END >>>>> \r\n");
      printf("Total Bytes Saved to SDCard = %lu \r\n", ByteCounter);
    }
};
