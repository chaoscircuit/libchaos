/**
 * \file peaks.h
 * \brief Header file for peaks.cpp
 */
 
#ifndef PEAKS_H
#define PEAKS_H

#include "libchaos.h"
#include "usb_comm.h"
#include "data_processing.h"

int peaks_initCache(int peaks_per_mdac = 10);
int* peaks_getPeaksAtMDAC(int mdac_value, int delta = 2);
int peaks_isCacheHit(int mdac_value);
int peaks_findPeaks(int* dst, int len, int* sample_data, int num_samples, int delta);

#endif
