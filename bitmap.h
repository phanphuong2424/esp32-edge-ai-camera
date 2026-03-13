#ifndef _BITMAP_H__
#define _BITMAP_H__
#include "Arduino.h"
// Resolution
const int WIDTH = 160;
const int HEIGHT = 120;

const int WIDTH_QQVGA  = 160;
const int HEIGHT_QQVGA = 120;

const int WIDTH_QVGA  = 320;
const int HEIGHT_QVGA = 240;

void createBmpHeader16(int width, int height);
void Write_FrameBuffer(uint32_t index,uint16_t val);

uint8_t*  Get_Image_Header();
uint16_t* Get_Image_frameBuffer();

#endif