/**
 * \file data_processing.h
 * \brief Header file for data_processing.cpp
 */

#ifndef DATA_PROCESSING_H
#define DATA_PROCESSING_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "libchaos.h"
#include "peaks.h"

void DP_appendToCSV(int* src_data, int length, int mdac_value);
int DP_newCSV(char* filename);
void DP_writeCSV();
int DP_getX1(int data_point);
int DP_getX2(int data_point);
int DP_getX3(int data_point);
int DP_getReturnMapPoints(int* dst, int len, int* src, int num_samples);
void DP_swap(float* a, float* b);
void DP_FFT(int* in, float* out, unsigned int length);
void DP_runFFT(float* data, unsigned long nn);
int DP_findTriggerIndex(int points_after_trigger);

#endif
