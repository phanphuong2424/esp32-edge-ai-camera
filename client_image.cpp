#include <WiFi.h>
#include <HTTPClient.h>
#include "bitmap.h"       // Giả sử có WIDTH, HEIGHT, Write_FrameBuffer(), v.v.

// Nếu bạn có PSRAM → dùng để cấp phát buffer lớn
// extern "C" { #include "esp_system.h" }  // nếu cần kiểm tra PSRAM

const char* ssid     = "Tine";
const char* password = "kaiyuanxi137238";

// URL server trả BMP 24-bit (RGB888)
const char* serverUrl = "http://192.168.1.3:3000/image?file=fish21.bmp";

// #define WIDTH       160
// #define HEIGHT      240
#define BYTES_PER_PIXEL_888  3
#define BMP_HEADER_SIZE 54    // Chuẩn cho BMP 24-bit uncompressed


void Client_Init() {

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected, IP: " + WiFi.localIP().toString());

}

void fetchAndSaveImage888(uint8_t *rgb888Buffer) {
  HTTPClient http;
  http.begin(serverUrl);
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP GET failed, code: %d → %s\n", httpCode, http.errorToString(httpCode).c_str());
    http.end();
    return;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t contentLength = http.getSize();

  if (contentLength < BMP_HEADER_SIZE + (WIDTH * HEIGHT * 3)) {
    Serial.println("Content size too small or invalid");
    http.end();
    return;
  }

  Serial.printf("Image received, total size: %u bytes\n", contentLength);

  // Đọc header BMP (54 bytes)
  uint8_t header[54];
  if (stream->readBytes(header, 54) != 54) {
    Serial.println("Failed to read BMP header");
    http.end();
    return;
  }

  // Kiểm tra cơ bản
  if (header[0] != 'B' || header[1] != 'M' ||
      *(uint16_t*)(header + 28) != 24) {  // bit depth phải là 24
    Serial.println("Not a valid 24-bit BMP file");
    http.end();
    return;
  }

  uint32_t width  = *(int32_t*)(header + 18);
  uint32_t height = abs(*(int32_t*)(header + 22));

  if (width != WIDTH || height != HEIGHT) {
    Serial.printf("Size mismatch: got %dx%d, expected %dx%d\n", width, height, WIDTH, HEIGHT);
    http.end();
    return;
  }

  // Tính kích thước dòng (có padding đến bội 4 bytes)
  uint32_t rowBytes = (WIDTH * 3 + 3) & ~3;   // padding

  uint8_t rowBuffer[rowBytes];   // buffer tạm cho 1 dòng

  Serial.println("Reading RGB888 data...");

  // BMP lưu bottom-up → đọc từ dưới lên
  for (int row = 0; row < HEIGHT; row++) {
    size_t readLen = stream->readBytes(rowBuffer, rowBytes);
    if (readLen != rowBytes) {
      Serial.printf("Failed to read row %d (read %u / %u)\n", row, readLen, rowBytes);
      break;
    }

    // Copy dữ liệu pixel (bỏ padding)
    for (int col = 0; col < WIDTH; col++) {
      // BMP: B G R
      uint8_t b = rowBuffer[col * 3 + 0];
      uint8_t g = rowBuffer[col * 3 + 1];
      uint8_t r = rowBuffer[col * 3 + 2];

      // Lưu vào buffer toàn cục (top-down: row 0 là trên cùng)
      int idx = (HEIGHT - 1 - row) * WIDTH * 3 + col * 3;

      rgb888Buffer[idx + 0] = r;   // RGB order (nếu driver cần RGB)
      rgb888Buffer[idx + 1] = g;
      rgb888Buffer[idx + 2] = b;
      // Nếu driver mong BGR → đổi thứ tự: b,g,r
    }
  }

  Serial.println("RGB888 image saved to rgb888Buffer");
  http.end();
}

// Hàm ví dụ: convert RGB888 buffer → RGB565 và vẽ vào frame buffer
void convert888to565AndDraw(uint8_t *rgb888Buffer) {
  for (int i = 0; i < WIDTH * HEIGHT; i++) {
    int offset = i * 3;
    uint8_t r = rgb888Buffer[offset + 0];
    uint8_t g = rgb888Buffer[offset + 1];
    uint8_t b = rgb888Buffer[offset + 2];

    uint16_t pixel565 = ((r >> 3) << 11) |
                        ((g >> 2) << 5)  |
                        (b >> 3);

    Write_FrameBuffer(i, pixel565);
  }
  Serial.println("Converted RGB888 → RGB565 and written to frame buffer");
}
