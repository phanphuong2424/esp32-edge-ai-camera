#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#include "cnn.h"
#include "actv_func.h"
#include "bitmap.h"
#include "client_image.h"
#include "get_image_camera.h"


#define TFT_DC 45
#define TFT_RST 21  // nếu không nối RST, có thể dùng -1
#define TFT_CS 10
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

//---------------------------------
#define w_head 20
#define h_head 15

float CONF_THRESHOLD = 0.50;
float IOU_THRESHOLD = 0.5;

const char *label[10] = { "person", "cat", "dog", "bird", "fish", "class5", "class6", "class7", "class8", "class9" };

float head_conv[h_head][w_head][15];
uint8_t *inputImage = (uint8_t *)ps_malloc(HEIGHT * WIDTH * 3);


int draw_offset_x = (320 - WIDTH)/2;
int draw_offset_y = (240 - HEIGHT)/2;

void convertRGB565toRGB888(uint16_t * rgb565Data) {

  int pixelCount = WIDTH * HEIGHT;

  for (int i = 0; i < pixelCount; i++) {
    uint16_t pixel = rgb565Data[i];

    uint8_t r = (pixel >> 11) & 0x1F;
    uint8_t g = (pixel >> 5) & 0x3F;
    uint8_t b = pixel & 0x1F;

    // scale lên 8bit
    r = (r * 255) / 31;
    g = (g * 255) / 63;
    b = (b * 255) / 31;

    inputImage[i * 3 + 0] = r;
    inputImage[i * 3 + 1] = g;
    inputImage[i * 3 + 2] = b;
  }

  Serial.println("convert done");
}

#define CAMERA_INPUT
// #define FETCH_IMAGE_INPUT

void setup() {
  Serial.begin(115200);
  
  Serial.println(psramFound());
  Serial.println(ESP.getPsramSize());
  Serial.println(ESP.getFreePsram());

  tft.begin();
  tft.setRotation(1);  // 0-3: xoay màn (1 = ngang WIDTHxHEIGHT)
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);

  CNN_Init();

  #ifdef FETCH_IMAGE_INPUT  
    Client_Init();
  #endif

  #ifdef CAMERA_INPUT  
    Init_Camera();
  #endif


}
void loop() {
  uint32_t tick_total = millis();
  #ifdef FETCH_IMAGE_INPUT  
    fetchAndSaveImage888(inputImage);
  #endif

  #ifdef CAMERA_INPUT  
    Camera_fb_return();

    uint16_t *rgb565Data = get_Image_Buff();
    convertRGB565toRGB888(rgb565Data);
  #endif


  uint32_t tick_CNN = millis();
  CNN_FeedForward(inputImage, &head_conv[0][0][0]);
  Serial.print("time CNN: ");Serial.println(millis() - tick_CNN); // time CNN: 4569 - 1890 - 1846

  convert888to565AndDraw(inputImage);
  tft.fillScreen(ILI9341_BLACK);
  tft.drawRGBBitmap(draw_offset_x, draw_offset_y, Get_Image_frameBuffer(), WIDTH, HEIGHT);

  for (int i = 0; i < h_head; i++) {
    for (int j = 0; j < w_head; j++) {

      float tx = sigmoid_(head_conv[i][j][0]);
      float ty = sigmoid_(head_conv[i][j][1]);

      float tw = head_conv[i][j][2];
      float th = head_conv[i][j][3];

      float cx = (tx + j) / w_head;
      float cy = (ty + i) / h_head;

      float w = tw * tw;
      float h = th * th;

      float x1 = cx - w / 2;
      float y1 = cy - h / 2;

      float x2 = cx + w / 2;
      float y2 = cy + h / 2;

      int index = 0;
      float max_class = 0;
      for (int c = 0; c < 10; c++) {
        float actv = sigmoid_(head_conv[i][j][c + 5]);

        if (actv > max_class) {
          max_class = actv;
          index = c;
        }
      }


      float conf = sigmoid_(head_conv[i][j][4]) * max_class;

      if (conf > CONF_THRESHOLD) {

        char conf_to_string[80];
        char indx_to_string[80];

        snprintf(conf_to_string, sizeof(conf_to_string), "%0.0f%%", conf * 100);
        snprintf(indx_to_string, sizeof(indx_to_string), "%d", index);

        tft.drawRect (draw_offset_x + x1*WIDTH, draw_offset_y + y1*HEIGHT, w * WIDTH, h * HEIGHT, ILI9341_RED);
        tft.setTextSize(1);
        tft.setCursor(draw_offset_x + x1*WIDTH, draw_offset_y + y1*HEIGHT);
        tft.print(label[index]);
        tft.print(conf_to_string);
      }
    }
  }
  Serial.print("time total: ");Serial.println(millis() - tick_total);Serial.println();Serial.println();
}
