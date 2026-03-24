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

/**
 * class kernel
 */

#define KERNEL_AT(kernel, f, y, x, c) \
  ((kernel).data[(f) * ((kernel).size * (kernel).size * (kernel).n_feature_in) + (y) * ((kernel).size * (kernel).n_feature_in) + (x) * ((kernel).n_feature_in) + (c)])

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

void CNN_Init() {
  
  Init_SPIFFS();

  bool ok1 = Load_CNN_AllKernels_FromTxtFiles("/kernels_1", kernels_1);
  bool ok2 = Load_CNN_AllKernels_FromTxtFiles("/kernels_2", kernels_2);
  bool ok3 = Load_CNN_AllKernels_FromTxtFiles("/kernels_3", kernels_3);
  bool ok4 = Load_CNN_AllKernels_FromTxtFiles("/kernels_4", kernels_4);

  if (!ok1) {
    Serial.println("Load fail");
  } else {
    Serial.println("Load success");
  }

  if (!ok2) {
    Serial.println("Load fail");
  } else {
    Serial.println("Load success");
  }

  if (!ok3) {
    Serial.println("Load fail");
  } else {
    Serial.println("Load success");
  }

  if (!ok4) {
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
        

        // nếu pixel đang ở các biên thì giảm kích thước kernel sao cho không lấy pixel thừa ngoài biên
        int xt = 0;
        int yt = 0;
        int xe = 0;
        int ye = 0;
        
        if(y == 0) yt = 1;
        if(x == 0) xt = 1;
        if(y == img_h-1) ye = 1;
        if(x == img_w-1) xe = 1;

        for (int ky = yt; ky < kernel.size-ye; ++ky) {
          for (int kx = xt; kx < kernel.size-xe; ++kx) {
            for (int c = 0; c < kernel.n_feature_in; ++c) {
              int iy = y + ky - offset;
              int ix = x + kx - offset;
              sum += KERNEL_AT(kernel, f, ky, kx, c) * input_image[iy * img_w * kernel.n_feature_in + ix * kernel.n_feature_in + c];// time CNN: 4569 - 1890 - 1846 - 1395 - 1375
            }
          }
        }
        sum += kernel.bias[f];
        sum = relu_(sum);

        FEATURE(feature_maps_out, y, x, f, kernel.n_kernel) =sum;
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
  float *image_convolution = (float *)ps_malloc(HEIGHT_convo_1 * WIDTH_convo_1 * C1 * sizeof(float));
  float *image_pooling = (float *)ps_malloc(HEIGHT_pooling_1 * WIDTH_pooling_1 * C1 * sizeof(float));

  applyConvolutionCNN(inputImage, HEIGHT_convo_1, WIDTH_convo_1, kernels_1, image_convolution);
  applyMaxPooling(image_convolution, HEIGHT_convo_1, WIDTH_convo_1, kernels_1.n_kernel, poolsize, poolsize, image_pooling);

  free(image_convolution);
  image_convolution = NULL;
  

  // convolutional lần 2
  float *image_convolution_2 = (float *)ps_malloc(HEIGHT_convo_2 * WIDTH_convo_2 * C2 * sizeof(float));
  float *image_pooling2 = (float *)ps_malloc(HEIGHT_pooling_2 * WIDTH_pooling_2 * C2 * sizeof(float));

  applyConvolutionCNN(image_pooling, HEIGHT_convo_2, WIDTH_convo_2, kernels_2, image_convolution_2);
  applyMaxPooling(image_convolution_2, HEIGHT_convo_2, WIDTH_convo_2, kernels_2.n_kernel, poolsize, poolsize, image_pooling2);

  free(image_convolution_2);
  image_convolution_2 = NULL;
  free(image_pooling);
  image_pooling = NULL;

  // convolutional lần 3
  float *image_convolution_3 = (float *)ps_malloc(HEIGHT_convo_3 * WIDTH_convo_3 * C3 * sizeof(float));
  float *image_pooling3 = (float *)ps_malloc(HEIGHT_pooling_3 * WIDTH_pooling_3 * C3 * sizeof(float));

  applyConvolutionCNN(image_pooling2, HEIGHT_convo_3, WIDTH_convo_3, kernels_3, image_convolution_3);
  applyMaxPooling(image_convolution_3, HEIGHT_convo_3, WIDTH_convo_3, kernels_3.n_kernel, poolsize, poolsize, image_pooling3);

  free(image_convolution_3);
  image_convolution_3 = NULL;
  free(image_pooling2);
  image_pooling2 = NULL;

  // convolutional head
  applyConvolutionCNN(image_pooling3, HEIGHT_pooling_3, WIDTH_pooling_3, kernels_4, flatmap);
  free(image_pooling3);
  image_pooling3 = NULL;

  free(inputImage);
  inputImage = NULL;
}
