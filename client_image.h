#ifndef _CLIENT_IMAGE_H__
#define _CLIENT_IMAGE_H__
#include "Arduino.h"
void Client_Init();
void fetchAndSaveImage888(uint8_t *rgb888Buffer);
void convert888to565AndDraw(uint8_t *rgb888Buffer);
#endif