#pragma once
#include "Arduino.h"
#include "driver/ledc.h"
#include "ov7670.h"
#include "bitmap.h"
#include "soc/lcd_cam_struct.h"
#include <Wire.h>


void scan() {
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission(true);  // gửi STOP
    if (err == 0) {
      Serial.printf("  - Found 7-bit 0x%02X  (8-bit: W=0x%02X, R=0x%02X)\n",
                    addr, (addr << 1), (addr << 1) | 1);
      found++;
    } else if (err == 4) {
      Serial.printf("  ! Unknown error @ 0x%02X\n", addr);
    }
    delay(2);
  }
  if (!found) Serial.println("  No I2C devices found.");
  else Serial.printf("  %u device(s) found.\n", found);
}

void sccb_write(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(OV7670_ADDR);
  Wire.write(reg);
  Wire.endTransmission(true);

  // Wire.beginTransmission(OV7670_ADDR);
  Wire.write(val);
  Wire.endTransmission(true);
}

uint8_t sccb_read(uint8_t reg) {
  // Gửi địa chỉ thanh ghi
  Wire.beginTransmission(OV7670_ADDR);
  Wire.write(reg);
  Wire.endTransmission(true);  // STOP (nếu repeated-start lỗi, dùng STOP thế này)

  // Đọc 1 byte, Wire sẽ tự NACK byte cuối
  Wire.requestFrom(OV7670_ADDR, (uint8_t)1, (uint8_t) true);
  if (Wire.available()) return Wire.read();
  return 0xFF;
}


void i2c_Init() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);  // 100 kHz an toàn
}

//----------- Clock Init-------------------
bool clockEnable(int pin, uint32_t freq) {
  ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_1_BIT,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = freq,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_channel_config_t ledc_channel = {
    .gpio_num = XCLK,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_0,
    .duty = 1,
    .hpoint = 0
  };

  ledc_timer_config(&ledc_timer);
  ledc_channel_config(&ledc_channel);
  Serial.printf("[LOG] XCLK chạy ở %d Hz trên GPIO %d\n", freq, XCLK);
  return true;
}

bool clockDisable() {
  ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0); // mức LOW
  Serial.printf("[LOG] XCLK đã tắt trên GPIO %d\n", XCLK);
  return true;
}
//----------- Clock Init-------------------


//----------- OV7670-------------------

void ov7670_test_mode() {
  sccb_write(0x71, 0x35 | 0x80);
}
void ov7670_writeList_basic() {
  sccb_write(0x12, 0x80);
  delay(150);

  // 2) Format nền RGB565 + nhịp nhẹ
  // sccb_write(0x12, 0x14); //COM7: RGB (no colorbar), format base 0x14 = 0b00010100
  sccb_write(0x12, 0b00010100); //COM7: RGB (no colorbar), format base 0x14 = 0b00010100
  sccb_write(0x8C, 0x00); // RGB444 off
  sccb_write(0x40, 0xD0); // COM15: RGB565 + full range
  sccb_write(0x3A, 0x04); // TSLB: byte order/phasing cho RGB565 phổ thông
  // sccb_write(0x11, 0x00); // CLKRC: XCLK/(1+1) -> giảm nhịp nội
  sccb_write(0x11, 0x01); // CLKRC: XCLK/(1+1) -> giảm nhịp nội
}

void ov7670_saturation(int s)  //-2 to 2
{
  //color matrix values
  sccb_write( 0x4f, 0x80 + 0x20 * s);
  sccb_write( 0x50, 0x80 + 0x20 * s);
  sccb_write( 0x51, 0x00);
  sccb_write( 0x52, 0x22 + (0x11 * s) / 2);
  sccb_write( 0x53, 0x5e + (0x2f * s) / 2);
  sccb_write( 0x54, 0x80 + 0x20 * s);
  sccb_write( 0x58, 0x9e);
}

void ov7670_frame_control(int hStart, int hStop, int vStart, int vStop) {
  sccb_write(REG_HSTART, hStart >> 3);
  sccb_write(REG_HSTOP,  hStop >> 3);
  sccb_write(REG_HREF, ((hStop & 0b111) << 3) | (hStart & 0b111));
  sccb_write(REG_VSTART, vStart >> 2);
  sccb_write(REG_VSTOP, vStop >> 2);
  sccb_write(REG_VREF, ((vStop & 0b11) << 2) | (vStart & 0b11));
}
void ov7670_writeList_qqvga() {
  sccb_write(REG_COM3, 0x04);
  sccb_write(REG_COM14, 0x1a);
  sccb_write(REG_SCALING_XSC, 0x3a);
  sccb_write(REG_SCALING_YSC, 0x35);
  sccb_write(REG_SCALING_DCWCTR, 0x22);
  sccb_write(REG_SCALING_PCLK_DIV, 0xf2);
  sccb_write(REG_SCALING_PCLK_DELAY, 0x02);
}

void ov7670_writeList_qqvga_RGB565() {
  ov7670_writeList_basic();

  sccb_write(REG_COM15, 0b11000000 | 0b010000);

  ov7670_writeList_qqvga();
  ov7670_frame_control(196, 52, 8, 488);
  sccb_write( 0xb0, 0x84);
  ov7670_saturation(0);
  // sccb_write( 0x13, 0xe7);
  // sccb_write( 0x6f, 0x9f);
  sccb_write( REG_MVFP, 0x2b);
}


void ov7670_writeList_qvga() {
  sccb_write(REG_COM3, 0x04);
  sccb_write(REG_COM14, 0x19);
  sccb_write(REG_SCALING_XSC, 0x3a);
  sccb_write(REG_SCALING_YSC, 0x35);
  sccb_write(REG_SCALING_DCWCTR, 0x11);
  sccb_write(REG_SCALING_PCLK_DIV, 0xf1);
  sccb_write(REG_SCALING_PCLK_DELAY, 0x02);
}
void ov7670_writeList_qvga_RGB565() {
  ov7670_writeList_basic();

  sccb_write(REG_COM15, 0b11000000 | 0b010000);

  ov7670_writeList_qvga();
  ov7670_frame_control(196, 52, 8, 488);
  sccb_write( 0xb0, 0x84);
  ov7670_saturation(0);
  // sccb_write( 0x13, 0xe7);
  // sccb_write( 0x6f, 0x9f);
  sccb_write( REG_MVFP, 0x2b);
}
void OV7670_Init() {
  pinMode(VSYNC, INPUT);
  pinMode(HREF, INPUT);
  pinMode(PCLK, INPUT);
  pinMode(D0, INPUT);
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(D4, INPUT);
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  pinMode(D7, INPUT);
  
  clockEnable(XCLK, XCLK_FREQ);
  delay(100);
  i2c_Init();
  scan();
  // ov7670_writeList_qqvga_RGB565();
  ov7670_writeList_qqvga_RGB565();

  clockDisable();
  clockEnable(XCLK, 1200000);
}

//----------- OV7670-------------------
void create_BmpImageTest16() {
  createBmpHeader16(WIDTH_QVGA, HEIGHT_QVGA);

  int idx = 0;
  for (int y = HEIGHT_QVGA - 1; y >= 0; y--) {
    for (int x = 0; x < WIDTH_QVGA; x++) {
      uint8_t r, g, b;
      if (x < WIDTH_QVGA / 3) {
        r = 255;
        g = 0;
        b = 0;
      } else if (x < 2 * WIDTH_QVGA / 3) {
        r = 0;
        g = 255;
        b = 0;
      } else {
        r = 0;
        g = 0;
        b = 255;
      }

      uint16_t pix = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

      // Ghi 1 pixel (16-bit) vào frameBuffer
      Write_FrameBuffer(idx++, pix);
    }
  }
  Serial.println("[LOG] Đã tạo BMP 16-bit test RGB565");
}

void capture_BmpImage_qqvga() {

  Serial.println("[LOG] Bắt đầu captureFrame QQVGA");
  createBmpHeader16(WIDTH_QQVGA, HEIGHT_QQVGA);

  while (digitalRead(VSYNC) == HIGH);  // Chờ frame bắt đầu
  while (digitalRead(VSYNC) == LOW);   // Chờ VSYNC chuyển sang HIGH
  int idx = 0;
  int line = HEIGHT_QQVGA-1;
  
  while(line >= 0) {
    while (digitalRead(HREF) == LOW);  // Chờ line bắt đầu
    idx = 0;
    while (digitalRead(HREF) == HIGH)   // Chờ Href chuyển sang HIGH
    { 
      
      while (digitalRead(PCLK) == HIGH)
        ;
      uint8_t high = 0;
      if (digitalRead(D7)) high |= 0x80;
      if (digitalRead(D6)) high |= 0x40;
      if (digitalRead(D5)) high |= 0x20;
      if (digitalRead(D4)) high |= 0x10;
      if (digitalRead(D3)) high |= 0x08;
      if (digitalRead(D2)) high |= 0x04;
      if (digitalRead(D1)) high |= 0x02;
      if (digitalRead(D0)) high |= 0x01;
      while (digitalRead(PCLK) == LOW);

      while (digitalRead(PCLK) == HIGH);

      
      uint8_t low = 0;
      if (digitalRead(D7)) low |= 0x80;
      if (digitalRead(D6)) low |= 0x40;
      if (digitalRead(D5)) low |= 0x20;
      if (digitalRead(D4)) low |= 0x10;
      if (digitalRead(D3)) low |= 0x08;
      if (digitalRead(D2)) low |= 0x04;
      if (digitalRead(D1)) low |= 0x02;
      if (digitalRead(D0)) low |= 0x01;

      while (digitalRead(PCLK) == LOW)
        ;

      uint16_t pixel = ((uint16_t)high << 8) | low;
      Write_FrameBuffer(idx + line*WIDTH_QQVGA, pixel);
      
      idx++;
    }
    line--;
  }
  Serial.printf("[LOG] Đã đọc xong %d frame, %d/%d pixel \n",WIDTH_QQVGA - line, idx, 120*160);
}

void capture_BmpImage_qvga() {

  Serial.println("[LOG] Bắt đầu captureFrame QVGA");
  createBmpHeader16(WIDTH_QVGA, HEIGHT_QVGA);

  while (digitalRead(VSYNC) == HIGH);  // Chờ frame bắt đầu
  while (digitalRead(VSYNC) == LOW);   // Chờ VSYNC chuyển sang HIGH
  int idx = 0;
  int line = 0;
  while(line < HEIGHT_QVGA) {
    while (digitalRead(HREF) == LOW);  // Chờ line bắt đầu
    idx = 0;
    while (digitalRead(HREF) == HIGH)   // Chờ Href chuyển sang HIGH
    { 
      
      while (digitalRead(PCLK) == HIGH)
        ;
      uint8_t high = 0;
      if (digitalRead(D7)) high |= 0x80;
      if (digitalRead(D6)) high |= 0x40;
      if (digitalRead(D5)) high |= 0x20;
      if (digitalRead(D4)) high |= 0x10;
      if (digitalRead(D3)) high |= 0x08;
      if (digitalRead(D2)) high |= 0x04;
      if (digitalRead(D1)) high |= 0x02;
      if (digitalRead(D0)) high |= 0x01;
      while (digitalRead(PCLK) == LOW);

      while (digitalRead(PCLK) == HIGH);

      
      uint8_t low = 0;
      if (digitalRead(D7)) low |= 0x80;
      if (digitalRead(D6)) low |= 0x40;
      if (digitalRead(D5)) low |= 0x20;
      if (digitalRead(D4)) low |= 0x10;
      if (digitalRead(D3)) low |= 0x08;
      if (digitalRead(D2)) low |= 0x04;
      if (digitalRead(D1)) low |= 0x02;
      if (digitalRead(D0)) low |= 0x01;

      while (digitalRead(PCLK) == LOW)
        ;

      uint16_t pixel = ((uint16_t)high << 8) | low;
      Write_FrameBuffer(idx + line*WIDTH_QVGA, pixel);
      
      idx++;
    }
    line++;
  }
  Serial.printf("[LOG] Đã đọc xong %d line, %d pixel \n",line, idx, 320*240);
}

