#include "cnn.h"
#include "actv_func.h"
#include "load_weight_cnn.h"
#include "bitmap.h"

#define poolsize  2

/**
 * class kernel
 */
KERNEL_t::KERNEL_t(int n_f, int s, int n_k) {
  n_feature_in = n_f;
  size = s;
  n_kernel = n_k;
  data = (float *)ps_malloc(n_feature_in * size * size * n_kernel * sizeof(float));
  bias = (float *)ps_malloc(n_kernel * sizeof(float));
}
KERNEL_t::~KERNEL_t() {
  free(data);
  free(bias);
}

float KERNEL_t::Get(int index_f, int index_y, int index_x, int index_c) const {
  return data[(index_f) * (size * size * n_feature_in) + (index_y) * (size * n_feature_in) + (index_x) * (n_feature_in) + (index_c)];
}

void KERNEL_t::UpdateData(float *data_update) {
  for (long i = 0; i < n_feature_in * size * size * n_kernel; i++) {
    data[i] = data_update[i];
  }
}
void KERNEL_t::RandomData() {
  for (long i = 0; i < n_feature_in * size * size * n_kernel; i++) {
    data[i] = (float)GetRandom(-50, 50) / 100.0;
  }
  for (int i = 0; i < n_kernel; i++) {
    bias[i] = (float)GetRandom(-50, 50) / 100.0;
  }
}
/**
 * class kernel
 */

#define FEATURE(feature_maps, y, x, f, num_feature_maps) \
  (feature_maps[(y) * (img_w * num_feature_maps) + (x) * (num_feature_maps) + (f)])


float lr = 0.1;

constexpr int C0 = 3;         // RGB
constexpr int C1 = 8;         // conv1 output kernels
constexpr int C2 = 16;        // conv2 output kernels
constexpr int C3 = 32;        // conv2 output kernels
constexpr int C4 = (5 + 10);  // 1 conf, 4 point of pos, 10 class

KERNEL_t kernels_1(C0, 3, C1);
KERNEL_t kernels_2(C1, 3, C2);
KERNEL_t kernels_3(C2, 3, C3);
KERNEL_t kernels_4(C3, 1, C4);

void PrintKernel(KERNEL_t &kernel, int kernel_index){
    int K = kernel.size;
    int Cin = kernel.n_feature_in;

    Serial.printf("\n===== KERNEL %d =====\n", kernel_index);

    if(kernel.bias)
        Serial.printf("bias: %f\n", kernel.bias[kernel_index]);

    for(int c = 0; c < Cin; c++)
    {
        Serial.printf("[C%d]\n", c);

        for(int ky = 0; ky < K; ky++)
        {
            for(int kx = 0; kx < K; kx++)
            {
                long idx =
                kernel_index * (K*K*Cin) +
                ky * (K*Cin) +
                kx * Cin +
                c;

                Serial.printf("% .6f ", kernel.data[idx]);
            }

            Serial.println();
        }

        Serial.println();
    }
}

void CNN_Init() {
  
  Init_SPIFFS();

  bool ok1 = Load_CNN_AllKernels_FromTxtFiles("/kernels_1", kernels_1);
  bool ok2 = Load_CNN_AllKernels_FromTxtFiles("/kernels_2", kernels_2);
  bool ok3 = Load_CNN_AllKernels_FromTxtFiles("/kernels_3", kernels_3);
  bool ok4 = Load_CNN_AllKernels_FromTxtFiles("/kernels_4", kernels_4);

  if (!ok1) {
    kernels_1.RandomData();
    Serial.println("Load fail");
  } else {
    Serial.println("Load success");
  }

  if (!ok2) {
    kernels_2.RandomData();
    Serial.println("Load fail");
  } else {
    Serial.println("Load success");
  }

  if (!ok3) {
    kernels_3.RandomData();
    Serial.println("Load fail");
  } else {
    Serial.println("Load success");
  }

  if (!ok4) {
    kernels_4.RandomData();
    Serial.println("Load fail");
  } else {
    Serial.println("Load success");
  }
}

void applyConvolutionCNN(float *input_image, int img_h, int img_w, KERNEL_t &kernel, float *feature_maps_out) {

  if (kernel.size % 2 == 0) {
    return;  // kernel phải lẻ
  }

  int offset = kernel.size / 2;

  // Duyệt từng vị trí (y, x) trên ảnh
  for (int y = 0; y < img_h; ++y) {
    for (int x = 0; x < img_w; ++x) {
      // Duyệt từng output feature map
      for (int f = 0; f < kernel.n_kernel; ++f) {
        float sum = 0;

        // Áp dụng kernel 3D cho feature map f
        for (int ky = 0; ky < kernel.size; ++ky) {
          for (int kx = 0; kx < kernel.size; ++kx) {
            for (int c = 0; c < kernel.n_feature_in; ++c) {
              int iy = y + ky - offset;
              int ix = x + kx - offset;

              float pixel_val = 0;
              if (iy >= 0 && iy < img_h && ix >= 0 && ix < img_w) {
                // Truy cập pixel: row-major, channel cuối cùng
                pixel_val = input_image[iy * img_w * kernel.n_feature_in + ix * kernel.n_feature_in + c];
              }  // zero-padding

              float kernel_val = kernel.Get(f, ky, kx, c);
              sum += pixel_val * kernel_val;
            }
          }
        }
        sum += kernel.bias[f];
        sum = relu_(sum);

        FEATURE(feature_maps_out, y, x, f, kernel.n_kernel) = static_cast<float>(sum);
      }
    }
  }
}

void applyMaxPooling(const float *input_feature_maps, int img_h, int img_w, int num_channels, int pool_size, int stride, float *output_pooled) {
  // Tính kích thước output
  int out_h = (img_h - pool_size) / stride + 1;
  int out_w = (img_w - pool_size) / stride + 1;

  // Duyệt từng feature map (channel)
  for (int c = 0; c < num_channels; ++c) {
    for (int y = 0; y < out_h; ++y) {
      for (int x = 0; x < out_w; ++x) {
        float max_val = 0;
        uint32_t max_idx = 0;

        // Duyệt vùng pool_size x pool_size
        for (int py = 0; py < pool_size; ++py) {
          for (int px = 0; px < pool_size; ++px) {
            int iy = y * stride + py;
            int ix = x * stride + px;

            // Kiểm tra giới hạn (nếu có padding thì bỏ kiểm tra này)
            if (iy < img_h && ix < img_w) {
              // Truy cập đúng layout channels-last
              int index = iy * img_w * num_channels + ix * num_channels + c;
              float val = input_feature_maps[index];
              if (val > max_val) {
                max_val = val;
                max_idx = index;
              }
            }
          }
        }

        // Ghi vào output
        int out_index = y * out_w * num_channels + x * num_channels + c;
        output_pooled[out_index] = max_val;
      }
    }
  }
}


const int HEIGHT_convo_1 = HEIGHT;
const int WIDTH_convo_1 = WIDTH;
const uint16_t HEIGHT_pooling_1 = HEIGHT / poolsize;
const uint16_t WIDTH_pooling_1 = WIDTH / poolsize;

const uint16_t HEIGHT_convo_2 = HEIGHT / poolsize;
const uint16_t WIDTH_convo_2 = WIDTH / poolsize;
const uint16_t HEIGHT_pooling_2 = HEIGHT / poolsize / poolsize;
const uint16_t WIDTH_pooling_2 = WIDTH / poolsize / poolsize;

const uint16_t HEIGHT_convo_3 = HEIGHT / poolsize / poolsize;
const uint16_t WIDTH_convo_3 = WIDTH / poolsize / poolsize;
const uint16_t HEIGHT_pooling_3 = HEIGHT / poolsize / poolsize / poolsize;
const uint16_t WIDTH_pooling_3 = WIDTH / poolsize / poolsize / poolsize;






void CNN_FeedForward(uint8_t *inputImage_uint8, float *flatmap) {

  float *inputImage = (float *)ps_malloc(WIDTH * HEIGHT * 3 * sizeof(float));

  for (long i = 0; i < WIDTH * HEIGHT * 3; i++) {
    inputImage[i] = (float)inputImage_uint8[i] / 255.0f;
  }
  
  // convolutional lần 1
  Serial.println("conv1 start");
  float *image_convolution = (float *)ps_malloc(HEIGHT_convo_1 * WIDTH_convo_1 * C1 * sizeof(float));
  float *image_pooling = (float *)ps_malloc(HEIGHT_pooling_1 * WIDTH_pooling_1 * C1 * sizeof(float));
  applyConvolutionCNN(inputImage, HEIGHT_convo_1, WIDTH_convo_1, kernels_1, image_convolution);
  Serial.println("conv1 done");

  Serial.println("pool1 start");
  applyMaxPooling(image_convolution, HEIGHT_convo_1, WIDTH_convo_1, kernels_1.n_kernel, poolsize, poolsize, image_pooling);
  free(image_convolution);
  image_convolution = NULL;
  Serial.println("pool1 done");
  

  // convolutional lần 2
  Serial.println("conv2 start");
  float *image_convolution_2 = (float *)ps_malloc(HEIGHT_convo_2 * WIDTH_convo_2 * C2 * sizeof(float));
  float *image_pooling2 = (float *)ps_malloc(HEIGHT_pooling_2 * WIDTH_pooling_2 * C2 * sizeof(float));
  applyConvolutionCNN(image_pooling, HEIGHT_convo_2, WIDTH_convo_2, kernels_2, image_convolution_2);
  Serial.println("conv2 done");

  Serial.println("pool2 start");
  applyMaxPooling(image_convolution_2, HEIGHT_convo_2, WIDTH_convo_2, kernels_2.n_kernel, poolsize, poolsize, image_pooling2);
  free(image_convolution_2);
  image_convolution_2 = NULL;
  free(image_pooling);
  image_pooling = NULL;
  Serial.println("pool2 done");

  // convolutional lần 3
  Serial.println("conv3 start");
  float *image_convolution_3 = (float *)ps_malloc(HEIGHT_convo_3 * WIDTH_convo_3 * C3 * sizeof(float));
  float *image_pooling3 = (float *)ps_malloc(HEIGHT_pooling_3 * WIDTH_pooling_3 * C3 * sizeof(float));
  applyConvolutionCNN(image_pooling2, HEIGHT_convo_3, WIDTH_convo_3, kernels_3, image_convolution_3);
  Serial.println("conv3 done");

  Serial.println("pool3 start");
  applyMaxPooling(image_convolution_3, HEIGHT_convo_3, WIDTH_convo_3, kernels_3.n_kernel, poolsize, poolsize, image_pooling3);
  free(image_convolution_3);
  image_convolution_3 = NULL;
  free(image_pooling2);
  image_pooling2 = NULL;
  Serial.println("pool3 done");

  // convolutional head
  Serial.println("conv4 start");
  applyConvolutionCNN(image_pooling3, HEIGHT_pooling_3, WIDTH_pooling_3, kernels_4, flatmap);
  free(image_pooling3);
  image_pooling3 = NULL;
  Serial.println("conv4 done");

  free(inputImage);
  inputImage = NULL;
}
