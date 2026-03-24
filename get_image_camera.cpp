#include "esp_camera.h"
#include "get_image_camera.h"

camera_fb_t *fb;

void Init_Camera() {
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 12000000;

  config.pixel_format = PIXFORMAT_RGB565;

  config.frame_size = FRAMESIZE_QQVGA;

  config.jpeg_quality = 12;

  config.fb_count = 1;

  esp_camera_init(&config);
}


void swapRGB565Bytes(uint16_t *buffer, int len) {
  for (int i = 0; i < len; i++) {
    uint16_t v = buffer[i];
    buffer[i] = ((v & 0x00FF) << 8) | ((v & 0xFF00) >> 8);
  }
}

uint16_t * get_Image_Buff() {
  fb = esp_camera_fb_get();

  uint16_t *image = (uint16_t *)fb->buf;
  swapRGB565Bytes(image, fb->len / 2);
  return image;
}

void Camera_fb_return() {
  esp_camera_fb_return(fb);
}






