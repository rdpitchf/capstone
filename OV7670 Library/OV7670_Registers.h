
// ***************** Frame Rate Adjustment for 24MHz input clock *****************
// 15 FPS, PCLK = 12 MHz.
#define CLKRC 0x11
#define DBLV 0x6B
#define EXHCH 0x2A
#define EXHCL 0x2B
#define DM_LNL 0x92
#define DM_LNH 0x93
// COM11 DUPLICATE DEFINE:
#define COM11 0x3B

// ***************** Light banding *****************
// 15 Fps for 60Hz light frequency (24MHz Input Clock)
// COM8 DUPLICATE DEFINE:
#define COM8 0x13
#define BD50ST 0x9D
#define BD60ST 0x9E
#define BD50MAX 0xA5
#define BD60MAX 0xAB
// COM11 DUPLICATE DEFINE:
// #define COM11 0x3B

// ***************** Simple White Balance *****************
// COM8 DUPLICATE DEFINE:
// #define COM8 0x13
#define AWBCTR0 0x6F

// ***************** Light Mode *****************
// COM8 DUPLICATE DEFINE:
// #define COM8 0x13
#define BLUE 0x01
#define RED 0x02

// ***************** Colour Saturation *****************
#define MTX1 0x4F
#define MTX2 0x50
#define MTX3 0x51
#define MTX4 0x52
#define MTX5 0x53
#define MTX6 0x54
#define MTXS 0x58

// ***************** Brightness *****************
#define BRIGHT 0x55

// ***************** Contrast *****************
#define CONTRAS 0x56

// ***************** Special Effects *****************
#define TSLB 0x3A
#define MANU 0x67
#define MANV 0x68
