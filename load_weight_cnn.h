#ifndef LOAD_WEIGHT_H__
#define LOAD_WEIGHT_H__ =
  #include "cnn.h"
  #include "Arduino.h"
  void Init_SPIFFS();
  bool Load_CNN_OneKernel_FromTxt(const char* filename, KERNEL_t& kernel, int kernel_index);
  bool Load_CNN_AllKernels_FromTxtFiles(const char* prefix, KERNEL_t& kernel);
#endif