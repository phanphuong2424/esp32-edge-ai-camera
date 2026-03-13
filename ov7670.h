
#ifndef _OV7670_H__
#define _OV7670_H__

#include "Arduino.h"
#define OV7670_ADDR 0x21
#define XCLK_FREQ 12000000

#define SDA_PIN 42
#define SCL_PIN 41 

#define XCLK 8
#define PCLK 3
#define VSYNC 9
#define HREF 46 

#define D0 4
#define D1 5  
#define D2 6  
#define D3 7
#define D4 15
#define D5 16
#define D6 17
#define D7 18

#define REG_GAIN 0x00
#define REG_BLUE 0x01
#define REG_RED 0x02
#define REG_COM1 0x04
#define REG_VREF 0x03
#define REG_COM4 0x0d
#define REG_COM5 0x0e
#define REG_COM6 0x0f
#define REG_AECH 0x10
#define REG_CLKRC 0x11
#define REG_COM7 0x12
#define COM7_RGB 0x04
#define REG_COM8 0x13
#define COM8_FASTAEC 0x80  // Enable fast AGC/AEC
#define COM8_AECSTEP 0x40  // Unlimited AEC step size
#define COM8_BFILT 0x20    // Band filter enable
#define COM8_AGC 0x04      // Auto gain enable
#define COM8_AWB 0x02      // White balance enable
#define COM8_AEC 0x0
#define REG_COM9 0x14
#define REG_COM10 0x15
#define REG_COM14 0x3E
#define REG_COM11 0x3B
#define COM11_NIGHT 0x80
#define COM11_NMFR 0x60
#define COM11_HZAUTO 0x10
#define COM11_50HZ 0x08
#define COM11_EXP 0x0
#define REG_TSLB 0x3A
#define REG_RGB444 0x8C
#define REG_COM15 0x40
#define COM15_RGB565 0x10
#define COM15_R00FF 0xc0
#define REG_HSTART 0x17
#define REG_HSTOP 0x18
#define REG_HREF 0x32
#define REG_VSTART 0x19
#define REG_VSTOP 0x1A
#define REG_COM3 0x0C
#define REG_MVFP 0x1E
#define REG_COM13 0x3d
#define COM13_UVSAT 0x40
#define REG_SCALING_XSC 0x70
#define REG_SCALING_YSC 0x71
#define REG_SCALING_DCWCTR 0x72
#define REG_SCALING_PCLK_DIV 0x73
#define REG_SCALING_PCLK_DELAY 0xa2
#define REG_BD50MAX 0xa5
#define REG_BD60MAX 0xab
#define REG_AEW 0x24
#define REG_AEB 0x25
#define REG_VPT 0x26
#define REG_HAECC1 0x9f
#define REG_HAECC2 0xa0
#define REG_HAECC3 0xa6
#define REG_HAECC4 0xa7
#define REG_HAECC5 0xa8
#define REG_HAECC6 0xa9
#define REG_HAECC7 0xaa
#define REG_COM12 0x3c
#define REG_GFIX 0x69
#define REG_COM16 0x41
#define COM16_AWBGAIN 0x08
#define REG_EDGE 0x3f
#define REG_REG76 0x76
#define ADCCTR0 0x20

// ================== Cấu hình sensor QQVGA RGB565 ==================
struct regval {
  uint8_t reg;
  uint8_t val;
};

const regval ov7670_basic[] = {
  { 0x12, 0x14 },  // COM7: RGB (no colorbar), format base
  { 0x8C, 0x00 },  // RGB444 off
  { 0x40, 0xD0 },  // COM15: RGB565 + full range
  { 0x3A, 0x04 },  // TSLB: byte order/phasing cho RGB565 phổ thông
  // Nhịp nội/tiền chia clock để hạ băng thông cho debug:
  // { 0x11, 0x01 },  // CLKRC: XCLK/(1+1) -> giảm nhịp nội
  { 0x11, 0x07 },  // CLKRC: XCLK/(7+1) -> giảm nhịp nội // thử giảm 8 lần
  // Một số mặc định an toàn (giữ nguyên window full, chưa crop):
  { 0x0C, 0x00 },  // COM3: tạm tắt scale/DCW, set sau khi chọn khung
  // { 0x3E, 0x00 },  // COM14: tạm tắt manual scaling
  { 0x3E, 0x10 },  // COM14: tạm tắt manual scaling
  // (Tùy chọn) tự động AE/AGC/AWB bật mặc định của sensor
  { 0xFF, 0xFF }
};

const regval ov7670_qqvga[] = {
  // Bật scale/DCW
  { 0x0C, 0x04 },  // COM3: enable DCW/scaling
  // Manual scaling + chia PCLK (giảm nhịp lấy mẫu)
  { 0x3E, 0x1A },  // COM14: manual scaling + PCLK divide (phổ biến)

  // Downsample cả ngang & dọc. Với QQVGA thường /4 mỗi chiều
  { 0x72, 0x22 },  // SCALING_DCWCTR: V_div=2, H_div=2 (tức /4 tổng thể)
  // { 0x73, 0xF2 },  // SCALING_PCLK_DIV: chia bổ sung PCLK (nhẹ bus)
  { 0x73, 0xF8 },  // SCALING_PCLK_DIV: chia bổ sung PCLK (nhẹ bus) thử giảm 4 lần
  // Tinh chỉnh phase/biên độ scale (giá trị "an toàn" thường dùng)

  { 0x70, 0x3A },  // SCALING_XSC
  { 0x71, 0x35 },  // SCALING_YSC
  { 0xA2, 0x02 },  // SCALING_PCLK_DELAY
  { 0xFF, 0xFF }
};


void scan();
void i2c_Init();
void sccb_write(uint8_t reg, uint8_t val);
uint8_t sccb_read(uint8_t reg);


bool clockEnable(int pin, uint32_t freq);
bool clockDisable();
void OV7670_Init();
void capture_BmpImage_qqvga();
void capture_BmpImage_qvga();
void create_BmpImageTest16();
void testSignalCounters();
void testOneFrame();
void captureFrame();

#endif