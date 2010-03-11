/**
 * \file libchaos.h
 * \brief Main header file for libchaos
 */

#ifndef LIBCHAOS_H
#define LIBCHAOS_H

#define LIBCHAOS_VERSION 1006

// sampling frequency in Hz
#define LIBCHAOS_SAMPLE_FREQUENCY 60*1200

#include <stdio.h>
#include <usb.h>

extern FILE* DEBUG_FILE;

/* Main */
int libchaos_init();
int libchaos_connect() ;
bool libchaos_isConnected();
int libchaos_reconnect();
int libchaos_close();
int libchaos_testDevice();

/* Sample To CSV */
int libchaos_startSampleToCSV(char* filename, int start, int end, int step, int periods);
int libchaos_samplePartToCSV();
int libchaos_endSampleToCSV();
int libchaos_sampleToCSV(char* filename, int start, int end, int step, int periods);

/* MDAC */
int libchaos_getMDACValue();
int libchaos_setMDACValue(int tap);
int libchaos_mdacToResistance(int mdac);
int libchaos_resistanceToMdac(int resistance);

/* Basic plot */
int libchaos_readPlot(int mdac_value);
int libchaos_getPlotPoint(int* x1, int*x2, int* x3, int index);
int libchaos_getNumPlotPoints();
int libchaos_setNumPlotPoints(int num);
int libchaos_getTriggerIndex();
int libchaos_setTransientData(int amount);

/* Peaks */
int* libchaos_getPeaks(int mdac_value);
bool libchaos_peaksCacheHit(int mdac_value);
int libchaos_setPeaksPerMDAC(int peaks_per_mdac);

/* Return map */
int libchaos_getReturnMap1Point(int* x1, int* x2, int index);
int libchaos_getReturnMap2Point(int* x1, int* x2, int index);
int libchaos_getNumReturnMapPoints();
void libchaos_refreshReturnMapPoints();

/* FFT */
void libchaos_getFFTPlotPoint(float* val, int index);
void libchaos_enableFFT();
void libchaos_disableFFT();

/* Version Information */
int libchaos_getFirmwareVersion();
int libchaos_getVersion();

#endif
