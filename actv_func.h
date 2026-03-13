#ifndef ACTIVATION_FUNCTION_H__
#define ACTIVATION_FUNCTION_H__
#include "Arduino.h"

using namespace std;

double sigmoid_(double z);
double d_sigmoid(double z);

double tanh_(double z);
double d_tanh(double z);

double relu_(double z);
double d_relu(double z);

double findMax(double *arr, int n);
int findMax_index(double *arr, int n);
void softmax(double *z, double *a, int n);


int GetRandom(int min, int max);

#endif