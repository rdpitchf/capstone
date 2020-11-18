
#include "mbed.h"

#include "AL422_FIFO.cpp"

#include "OV7670_Registers.h"
#include "OV7670_SCCB.cpp"


static AL422_FIFO fifo;
static OV7670_SCCB i2c;

#define VGA_RESOLUTION 640*480

class CAMERA{

  public:
    CAMERA(){
      initalize_vga_camera();
      // Initialize settings
      frame_rate_15fps();
      light_banding_filter_auto_detect();
      simple_white_balance();
      light_mode_auto();
      colour_staturation_zero();
      brightness_zero();
      contrast_zero();
      special_effect_normal();
    }

    void prepareCapture(){
      fifo.writeReset();
    }

    void startCapture(){
      fifo.writeEnable();
    }

    void stopCapture(){
      fifo.writeDisable();
    }

    void take_vga_picture(uint8_t *frame){
      fifo.readReset();
      for(int i = 0; i < 640*480*2; i+=2){
        frame[i] = fifo.readByte();
        frame[i+1] = fifo.readByte();
      }
    }


  // ***************** Frame Rate Adjustment for 24MHz input clock *****************
  // 15 FPS, PCLK = 12 MHz.
  void frame_rate_15fps(){
    i2c.write_until_true(CLKRC, 0x00);
    i2c.write_until_true(0x6b, 0x0A);
    i2c.write_until_true(EXHCH, 0x00);
    i2c.write_until_true(EXHCL, 0x00);
    i2c.write_until_true(DM_LNL, 0x00);
    i2c.write_until_true(DM_LNH, 0x00);
    i2c.write_until_true(COM11, 0x0A);
  }

  // ***************** Light banding *****************
  // 15 Fps for 60Hz light frequency (24MHz Input Clock)
  void light_banding_filter_60Hz(){
    i2c.write_until_true(COM8, 0xE7); // Banding filter enable
    i2c.write_until_true(BD50ST, 0x4C); // 50Hz banding filter
    i2c.write_until_true(BD60ST, 0x3F); // 60Hz banding filter
    i2c.write_until_true(BD50MAX, 0x05); // 6 step for 50Hz
    i2c.write_until_true(BD60MAX, 0x07); // 8 step for 60Hz
    // Select Banding Filter by Region Information:
    i2c.write_until_true(COM11, 0x02); // Select 60Hz banding filter *******
  }
  void light_banding_filter_auto_detect(){
    i2c.write_until_true(COM8, 0xE7); // Banding filter enable
    i2c.write_until_true(BD50ST, 0x4C); // 50Hz banding filter
    i2c.write_until_true(BD60ST, 0x3F); // 60Hz banding filter
    i2c.write_until_true(BD50MAX, 0x05); // 6 step for 50Hz
    i2c.write_until_true(BD60MAX, 0x07); // 8 step for 60Hz
    // Select Banding Filter by Automatic Light Frequency Detection:
    i2c.write_until_true(COM11, 0x12); // Automatic Detect banding filter *******
  }

  // ***************** Simple White Balance *****************
  void simple_white_balance(){
    i2c.write_until_true(COM8, 0xE7); // AWB on
    i2c.write_until_true(AWBCTR0, 0x9F); // Simple AWB
  }

  // ***************** Light Mode *****************
  void light_mode_auto(){
    // Auto:
    i2c.write_until_true(COM8, 0xE7); // AWB on
  }
  void light_mode_sunny(){
    // Sunny:
    i2c.write_until_true(COM8, 0xE5); // AWB off
    i2c.write_until_true(BLUE, 0x5A);
    i2c.write_until_true(RED, 0x5C);
  }
  void light_mode_cloudy(){
    // Cloudy:
    i2c.write_until_true(COM8, 0xE5); // AWB off
    i2c.write_until_true(BLUE, 0x58);
    i2c.write_until_true(RED, 0x60);
  }
  void light_mode_office(){
    // Office:
    i2c.write_until_true(COM8, 0xE5); // AWB off
    i2c.write_until_true(BLUE, 0x84);
    i2c.write_until_true(RED, 0x4C);
  }
  void light_mode_home(){
    // Home:
    i2c.write_until_true(COM8, 0xE5); // AWB off
    i2c.write_until_true(BLUE, 0x96);
    i2c.write_until_true(RED, 0x40);
  }

  // ***************** Colour Saturation *****************
  void colour_staturation_plus_2(){
    // Saturation +2:
    i2c.write_until_true(MTX1, 0xC0);
    i2c.write_until_true(MTX2, 0xC0);
    i2c.write_until_true(MTX3, 0x00);
    i2c.write_until_true(MTX4, 0x33);
    i2c.write_until_true(MTX5, 0x8D);
    i2c.write_until_true(MTX6, 0xC0);
    i2c.write_until_true(MTXS, 0x9E);
  }
  void colour_staturation_plus_1(){
    // Saturation +1:
    i2c.write_until_true(MTX1, 0x99);
    i2c.write_until_true(MTX2, 0x99);
    i2c.write_until_true(MTX3, 0x00);
    i2c.write_until_true(MTX4, 0x28);
    i2c.write_until_true(MTX5, 0x71);
    i2c.write_until_true(MTX6, 0x99);
    i2c.write_until_true(MTXS, 0x9E);
  }
  void colour_staturation_zero(){
    // Saturation 0:
    i2c.write_until_true(MTX1, 0x80);
    i2c.write_until_true(MTX2, 0x80);
    i2c.write_until_true(MTX3, 0x00);
    i2c.write_until_true(MTX4, 0x22);
    i2c.write_until_true(MTX5, 0x5E);
    i2c.write_until_true(MTX6, 0x80);
    i2c.write_until_true(MTXS, 0x9E);
  }
  void colour_staturation_minus_1(){
    // Saturation -1:
    i2c.write_until_true(MTX1, 0x66);
    i2c.write_until_true(MTX2, 0x66);
    i2c.write_until_true(MTX3, 0x00);
    i2c.write_until_true(MTX4, 0x1B);
    i2c.write_until_true(MTX5, 0x4B);
    i2c.write_until_true(MTX6, 0x66);
    i2c.write_until_true(MTXS, 0x96);
  }
  void colour_staturation_minus_2(){
    // Saturation -2:
    i2c.write_until_true(MTX1, 0x40);
    i2c.write_until_true(MTX2, 0x40);
    i2c.write_until_true(MTX3, 0x00);
    i2c.write_until_true(MTX4, 0x11);
    i2c.write_until_true(MTX5, 0x2F);
    i2c.write_until_true(MTX6, 0x40);
    i2c.write_until_true(MTXS, 0x9E);
  }

  // ***************** Brightness *****************
  void brightness_plus_2(){
    // Brightness +2:
    i2c.write_until_true(BRIGHT, 0x30);
  }
  void brightness_plus_1(){
    // Brightness +1:
    i2c.write_until_true(BRIGHT, 0x18);
  }
  void brightness_zero(){
    // Brightness 0:
    i2c.write_until_true(BRIGHT, 0x00);
  }
  void brightness_minus_1(){
    // Brightness -1:
    i2c.write_until_true(BRIGHT, 0x98);
  }
  void brightness_minus_2(){
    // Brightness -2:
    i2c.write_until_true(BRIGHT, 0xB0);
  }

  // ***************** Contrast *****************
  void contrast_plus_2(){
    // Contrast +2:
    i2c.write_until_true(CONTRAS, 0x60);
  }
  void contrast_plus_1(){
    // Contrast +1:
    i2c.write_until_true(CONTRAS, 0x50);
  }
  void contrast_zero(){
    // Contrast 0:
    i2c.write_until_true(CONTRAS, 0x40);
  }
  void contrast_minus_1(){
    // Contrast -1:
    i2c.write_until_true(CONTRAS, 0x38);
  }
  void contrast_minus_2(){
    // Contrast -2:
    i2c.write_until_true(CONTRAS, 0x30);
  }

  // ***************** Special Effects *****************
  void special_effect_antique(){
    // Antique:
    i2c.write_until_true(TSLB, 0x14);
    i2c.write_until_true(MANU, 0xA0);
    i2c.write_until_true(MANV, 0x40);
  }
  void special_effect_bluish(){
    // Bluish:
    i2c.write_until_true(TSLB, 0x14);
    i2c.write_until_true(MANU, 0x80);
    i2c.write_until_true(MANV, 0xC0);
  }
  void special_effect_greenish(){
    // Greenish:
    i2c.write_until_true(TSLB, 0x14);
    i2c.write_until_true(MANU, 0x40);
    i2c.write_until_true(MANV, 0x40);
  }
  void special_effect_redish(){
    // Redish:
    i2c.write_until_true(TSLB, 0x14);
    i2c.write_until_true(MANU, 0xC0);
    i2c.write_until_true(MANV, 0x80);
  }
  void special_effect_bw(){
    // B & W:
    i2c.write_until_true(TSLB, 0x14);
    i2c.write_until_true(MANU, 0xC80);
    i2c.write_until_true(MANV, 0x80);
  }
  void special_effect_negative(){
    // Negative:
    i2c.write_until_true(TSLB, 0x24);
    i2c.write_until_true(MANU, 0x80);
    i2c.write_until_true(MANV, 0x80);
  }
  void special_effect_bw_negative(){
    // B & W Negative:
    i2c.write_until_true(TSLB, 0x34);
    i2c.write_until_true(MANU, 0x80);
    i2c.write_until_true(MANV, 0x80);
  }
  void special_effect_normal(){
    // Normal:
    i2c.write_until_true(TSLB, 0x04);
    i2c.write_until_true(MANU, 0x80);
    i2c.write_until_true(MANV, 0x80);
  }

  // ***************** Initalize Camera to VGA *****************
  void initalize_vga_camera(){
    i2c.write_until_true(0x12, 0x80);                  // RESET CAMERA

    i2c.write_until_true(0x8c, 0x00);              // Disable RGB444
    i2c.write_until_true(0x15, 0x02);               // 0x02   VSYNC negative (http://nasulica.homelinux.org/?p=959)
    i2c.write_until_true(0x1e, 0x27);                // mirror image

    i2c.write_until_true(0x11, 0x80);               // prescaler x1
    i2c.write_until_true(0x6b, 0x0a);                    // bypass PLL

    i2c.write_until_true(0x3b, 0x0A) ;
    i2c.write_until_true(0x3a, 0x04);                // 0D = UYVY  04 = YUYV
    i2c.write_until_true(0x3d, 0x88);               // connect to 0x3a

    i2c.write_until_true(0x12, 0x04);           // RGB + color bar disable
    i2c.write_until_true(0x8c, 0x00);         // Disable RGB444
    i2c.write_until_true(0x40, 0x10);          // Set rgb565 with Full range    0xD0
    i2c.write_until_true(0x0c, 0x04);
    i2c.write_until_true(0x11, 0x80);          // prescaler x1
    i2c.write_until_true(0x70, 0x3A);                   // Scaling Xsc
    i2c.write_until_true(0x71, 0x35);                   // Scaling Ysc
    i2c.write_until_true(0xA2, 0x02);                   // pixel clock delay



    // 640*480
    i2c.write_until_true(0x11, 0x01);
    i2c.write_until_true(0x3a,  0x04);
    i2c.write_until_true(0x12, 0x01);
    i2c.write_until_true(0x6b, 0x4a);
    i2c.write_until_true(0x0c, 0);
    i2c.write_until_true(0x3e, 0);

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
    i2c.write_until_true(0x00, 0);
    i2c.write_until_true(0x10, 0);
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
    i2c.write_until_true(0x69, 0);
    i2c.write_until_true(0x6b, 0x0a);
    i2c.write_until_true(0x74, 0x10);
    i2c.write_until_true(0x8d, 0x4f);
    i2c.write_until_true(0x8e, 0);
    i2c.write_until_true(0x8f, 0);
    i2c.write_until_true(0x90, 0);
    i2c.write_until_true(0x91, 0);
    i2c.write_until_true(0x96, 0);
    i2c.write_until_true(0x9a, 0);
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
    i2c.write_until_true(0x51, 0);
    i2c.write_until_true(0x52, 0x22);
    i2c.write_until_true(0x53, 0x5e);
    i2c.write_until_true(0x54, 0x80);
    i2c.write_until_true(0x58, 0x9e);

    i2c.write_until_true(0x41, 0x08);
    i2c.write_until_true(0x3f, 0);
    i2c.write_until_true(0x75, 0x05);
    i2c.write_until_true(0x76, 0xe1);
    i2c.write_until_true(0x4c, 0);
    i2c.write_until_true(0x77, 0x01);
    i2c.write_until_true(0x3d, 0xc3);
    i2c.write_until_true(0x4b, 0x09);
    i2c.write_until_true(0xc9, 0x60);
    i2c.write_until_true(0x41, 0x38);
    i2c.write_until_true(0x56, 0x40);

    i2c.write_until_true(0x34, 0x11);
    i2c.write_until_true(0x3b, 0x02|0x10);
    i2c.write_until_true(0xa4, 0x88);
    i2c.write_until_true(0x96, 0);
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

    i2c.write_until_true(0xff, 0xff); /* END MARKER */
  }
};
