#ifndef CONVOLUTIONAL_NEURALNETWORKS_H__
#define CONVOLUTIONAL_NEURALNETWORKS_H__
#include "Arduino.h"

/**
     * class kernel
     */
class KERNEL_t {
private:

public:
  int n_feature_in;
  int size;
  int n_kernel;
  float *data;
  float *bias;
  KERNEL_t(int n_f, int s, int n_k);
  ~KERNEL_t();
  float Get(int index_f, int index_y, int index_x, int index_c) const;
  void UpdateData(float *data_update);
  void RandomData();
};
/**
     * class kernel
     */


void CNN_Init();
void applyConvolutionCNN(float *input_image, int img_h, int img_w, KERNEL_t &kernel, float *feature_maps);
void applyMaxPooling(const float *input_feature_maps, int img_h, int img_w, int num_channels, int pool_size, int stride, float *output_pooled);
void CNN_FeedForward(uint8_t *inputImage, float *flatmap);

#endif