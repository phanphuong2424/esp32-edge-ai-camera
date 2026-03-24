
#ifndef _GET_IMAGE_CAMERA_H__
#define _GET_IMAGE_CAMERA_H__

#include "Arduino.h"

#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  -1

#define SIOD_GPIO_NUM  42
#define SIOC_GPIO_NUM  41

#define Y9_GPIO_NUM    18
#define Y8_GPIO_NUM    17
#define Y7_GPIO_NUM    16
#define Y6_GPIO_NUM    15
#define Y5_GPIO_NUM    7
#define Y4_GPIO_NUM    6
#define Y3_GPIO_NUM    5
#define Y2_GPIO_NUM    4

#define VSYNC_GPIO_NUM 9
#define HREF_GPIO_NUM  46
#define PCLK_GPIO_NUM  3

void Init_Camera();
uint16_t * get_Image_Buff();
void swapRGB565Bytes(uint16_t* buffer, int len);
void Camera_fb_return();
#endif