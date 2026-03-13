#include "bitmap.h"


// Buffer for frame
uint16_t *frameBuffer = (uint16_t*)ps_malloc(WIDTH * HEIGHT * sizeof(uint16_t));

// BMP header template
// buffer toàn cục đủ lớn (ít nhất 66 bytes)
uint8_t bmpHeader[66];

void createBmpHeader16(int width, int height) {
  uint32_t dataSize = width * height * 2; // 2 bytes/pixel
  uint32_t fileSize = 66 + dataSize;

  memset(bmpHeader, 0, sizeof(bmpHeader));

  // BITMAPFILEHEADER
  bmpHeader[0] = 'B'; bmpHeader[1] = 'M';
  bmpHeader[2] = (uint8_t)(fileSize & 0xFF);
  bmpHeader[3] = (uint8_t)((fileSize >> 8) & 0xFF);
  bmpHeader[4] = (uint8_t)((fileSize >> 16) & 0xFF);
  bmpHeader[5] = (uint8_t)((fileSize >> 24) & 0xFF);
  bmpHeader[10] = 66; // offset: header (14+40) + masks (12)

  // BITMAPINFOHEADER
  bmpHeader[14] = 40; // biSize
  bmpHeader[18] = (uint8_t)(width & 0xFF);
  bmpHeader[19] = (uint8_t)((width >> 8) & 0xFF);
  bmpHeader[20] = (uint8_t)((width >> 16) & 0xFF);
  bmpHeader[21] = (uint8_t)((width >> 24) & 0xFF);
  bmpHeader[22] = (uint8_t)(height & 0xFF);
  bmpHeader[23] = (uint8_t)((height >> 8) & 0xFF);
  bmpHeader[24] = (uint8_t)((height >> 16) & 0xFF);
  bmpHeader[25] = (uint8_t)((height >> 24) & 0xFF);
  bmpHeader[26] = 1;   // planes
  bmpHeader[28] = 16;  // biBitCount = 16
  bmpHeader[30] = 3;   // biCompression = BI_BITFIELDS
  bmpHeader[34] = (uint8_t)(dataSize & 0xFF);
  bmpHeader[35] = (uint8_t)((dataSize >> 8) & 0xFF);
  bmpHeader[36] = (uint8_t)((dataSize >> 16) & 0xFF);
  bmpHeader[37] = (uint8_t)((dataSize >> 24) & 0xFF);

  // Color masks (12 bytes, little endian)
  // Red mask   0xF800
  bmpHeader[54] = 0x00;
  bmpHeader[55] = 0xF8;
  bmpHeader[56] = 0x00;
  bmpHeader[57] = 0x00;
  // Green mask 0x07E0
  bmpHeader[58] = 0xE0;
  bmpHeader[59] = 0x07;
  bmpHeader[60] = 0x00;
  bmpHeader[61] = 0x00;
  // Blue mask  0x001F
  bmpHeader[62] = 0x1F;
  bmpHeader[63] = 0x00;
  bmpHeader[64] = 0x00;
  bmpHeader[65] = 0x00;
}


void Write_FrameBuffer(uint32_t index,uint16_t val) {
  frameBuffer[index] = val;
}

uint8_t* Get_Image_Header() {
  return bmpHeader;
}
uint16_t* Get_Image_frameBuffer() {
  return frameBuffer;
}





