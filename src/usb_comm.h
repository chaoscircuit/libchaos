/**
 * \file usb_comm.h
 * \brief Header file for usb_comm.cpp
 */

#ifndef USB_COMM_H
#define USB_COMM_H

#include <usb.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "usb_commands.h"
#include "libchaos.h"

/* the device's vendor and product id */
#define MY_VID 0x0945
#define MY_PID 0x7777

/* the device's endpoints */
#define EP_IN 0x81
#define EP_OUT 0x01

/* additional settings */
#define UC_TIMEOUT 1000

extern usb_dev_handle * UC_device_handle;
extern int UC_TRANSIENT_DATA;

int UC_init();
int UC_open();
int UC_close();
int UC_reset();
int UC_write(char* buf, int size);
int UC_read(char* buf, int size);
int UC_setMDAC(short int tap);
int UC_startSample(short int tap);
int UC_getData(int *dst);
int UC_endSample();
int UC_sample(int* dst, int num_samples, int value);
int UC_sampleCurrent(int* dst, int num_samples);
int UC_getStatus(int* mdac_value);
int UC_getVersion();
bool UC_isConnected();
int UC_connect();

#endif
