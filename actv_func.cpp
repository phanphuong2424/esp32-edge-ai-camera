#include "actv_func.h"
#include "math.h"
const double e = 2.7182818284590452353602874713527;

double sigmoid_(double z) {
  return 1 / (1 + pow(e, -z));
}

double d_sigmoid(double z) {
  double s = sigmoid_(z);
  return s * (1 - s);
}

double tanh_(double z) {
  return tanh(z);
}

double d_tanh(double z) {
  double term = tanh_(z);
  return 1 - term * term;
}

double relu_(double z) {
  double x = 0;
  if (z > 0)
    x = z;
  else
    x = 0;
  return x;
}

double d_relu(double z) {
  double x = 0;
  if (z > 0)
    x = 1;
  else
    x = 0;
  return x;
}

double relu_leak(double z) {
  double x = 0;
  if (z > 0)
    x = z;
  else
    x = 0.1 * z;
  return x;
}

double d_relu_leak(double z) {
  double x = 0;
  if (z > 0)
    x = 1;
  else
    x = 0.1;
  return x;
}

double findMax(double *arr, int n) {
  double max = 0;
  for (int i = 0; i < n; i++) {
    if (arr[i] > max) {
      max = arr[i];
    }
  }
  return max;
}
int findMax_index(double *arr, int n) {
  double max = 0;
  int index = 0;
  for (int i = 0; i < n; i++) {
    if (arr[i] > max) {
      max = arr[i];
      index = i;
    }
  }

  return index;
}

void softmax(double *z, double *a, int n) {
  double max = findMax(z, n);
  double sum = 0;
  for (int i = 0; i < n; i++) {
    sum += exp(z[i] - max);
  }
  for (int i = 0; i < n; i++) {
    a[i] = exp(z[i] - max) / sum;
  }
}


int GetRandom(int min, int max) {
  return min + (int)(rand() * (max - min + 1.0) / (1.0 + RAND_MAX));
}