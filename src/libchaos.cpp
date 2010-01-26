/**
 * \file libchaos.cpp
 * \brief Public routines for the chaos library
 */

#include "libchaos.h"

#include "usb_comm.h"
#include "device_test.h"
#include "data_processing.h"
#include "peaks.h"

// debug mode
// 0 = to file DEBUG_FILENAME
// 1 = to stdout
#define DEBUG_MODE 0

FILE* DEBUG_FILE = 0x00;

int * DATA;
int START;
int MDAC_VALUE;
int END;
int STEP;
int NUM_SAMPLES;

#define NUM_FFT_PLOT_POINTS 8192
#define RETURN_MAP_MAX_POINTS 600
#define POINTS_AFTER_TRIGGER 300
#define MAX_PLOT_POINTS 8192
int NUM_PLOT_POINTS = 2040;
int PLOT_DATA[MAX_PLOT_POINTS];
float FFT_DATA[NUM_FFT_PLOT_POINTS*2];
int PLOT_RETURN_MAP_POINTS[RETURN_MAP_MAX_POINTS*3];
int PLOT_RETURN_MAP_NUM_POINTS = 0;
int TRIGGER_INDEX;
int FFT_ENABLED = 1;

/* main routines */

int libchaos_init() {
    /** 
     * Initialize the chaos library
     */
    // setup the debugging/logging file
    if(DEBUG_MODE == 0) {
        if(!DEBUG_FILE) {
            DEBUG_FILE = fopen("libchaos.log","w");
        }
    } else if (DEBUG_MODE == 1) {
        DEBUG_FILE = stdout;
    }
    
    // connect to the chaos circuit
    int result = UC_init();
    return result;
}

int libchaos_connect() {
    /** 
     * Connect to the chaos unit
     */
    int result = UC_connect();
    return result;
}

bool libchaos_isConnected() {
    /** 
     * Returns true if the device is connected.
     * If there is no connection to the device, this function attempts to create one. 
     */
     if(UC_isConnected() == false) {
        UC_connect();
     }
     return UC_isConnected();
}

int libchaos_reconnect() {
    /** 
     * Reconnect to the device
     */
    // close the USB connection
    UC_close();
    
    int result = UC_init();
    return result;
}

int libchaos_close() {
    /** 
     * Close libchaos
     */
    return UC_close();
}

int libchaos_testDevice() {
    /** 
     * Run the device test
     */
    return DT_testDevice();
}

/* Sample To CSV */

int libchaos_startSampleToCSV(char* filename, int start, int end, int step, int periods) {
    /** 
     * Start a sample sweep to a CSV file
     */
    START = start;
    END = end;
    STEP = step;
    NUM_SAMPLES = periods * 60;
    MDAC_VALUE = start;
    
    fprintf(DEBUG_FILE, "Starting partitioned sample sweep:\n");
    fprintf(DEBUG_FILE, "start:%d end:%d step:%d samples:%d\n",START,END,STEP,NUM_SAMPLES);

    DATA = (int*)malloc(NUM_SAMPLES*4);
    
    if(!DP_newCSV(filename)) {
        fprintf(DEBUG_FILE, "File %s failed to open\n", filename);
        return -1;
    }
    
    return 0;
}

int libchaos_samplePartToCSV() {
    /** 
     * Take the next chunk of data
     *
     * This function should be called over and over until it returns 0
     */
    static int calls_this_tap = 0;
    int * start_ptr;
    int length;
    const int samples_per_call = 1020*64;
    
    // if this is the first call for this tap value
    // inform the device that we are beginning sampling
    if( calls_this_tap == 0) {
        UC_startSample(MDAC_VALUE);
    }

    // set up the start pointer for this call to point to the correct location
    // in the data
    start_ptr = DATA + calls_this_tap*samples_per_call;
    
    // set the length to it maximum or what we have left
    if ((calls_this_tap+1)*samples_per_call > NUM_SAMPLES) {
        length = NUM_SAMPLES - calls_this_tap*samples_per_call;
    } else {
        length = samples_per_call;
    }

    // get the sample portion
    fprintf(DEBUG_FILE,"Collecting %d samples for tap number %d...",length,MDAC_VALUE);
    UC_sampleCurrent(start_ptr, length);
    fprintf(DEBUG_FILE,"Done\n");
    
    // calculate the percentage complete
    int total = END-START;
    if ( total < STEP ) {
        total = 1;
    } else {
        total = (total+1) / STEP;
    }
    int position = (MDAC_VALUE-START)/STEP;
    int percent_complete = (position*100)/(total+1);
    float weight = 100.0/float(total+1);
    percent_complete += (int)(float(weight) * float((float)calls_this_tap*(float)samples_per_call/(float)NUM_SAMPLES));
    if( percent_complete >= 100 ) percent_complete = 99;
    if( percent_complete <= 0 ) percent_complete = 1;
    fprintf(DEBUG_FILE, "%d percent complete\n",percent_complete);

    // if this isn't the last call
    bool more_left = calls_this_tap*samples_per_call + length < NUM_SAMPLES; 
    if ( more_left ) {
        calls_this_tap++;
        return percent_complete;
    } else {
        // were done with this tap
        UC_endSample();
        fprintf(DEBUG_FILE,"Storing data...");
        DP_appendToCSV(DATA,NUM_SAMPLES,MDAC_VALUE);
        fprintf(DEBUG_FILE,"Done\n");

        MDAC_VALUE += STEP;
        if(MDAC_VALUE <= END) {
            // more taps left
            calls_this_tap = 0;
            return percent_complete;
        } else {
            // all done
            return 0;
        }
    }
}

int libchaos_endSampleToCSV() {
    /** 
     * End a sample sweep to CSV
     *
     * Write the file, and free the memory used
     */
    DP_writeCSV();
    free(DATA);
    fprintf(DEBUG_FILE,"Sample sweep finished\n");
    return 0;
}

int libchaos_sampleToCSV(char* filename, int mdac_start, int mdac_end, 
                         int mdac_step, int periods) {
    /** 
     * Perform a sample sweep to a CSV file
     */
    int mdac_value;
    int num_samples = periods * 60;
    int* data;
    
    data = (int*)malloc(num_samples*4);
    
    if(!DP_newCSV(filename)) {
        fprintf(DEBUG_FILE, "File %s failed to open\n", filename);
        return -1;
    }
    for( mdac_value = mdac_start; mdac_value<=mdac_end; mdac_value += mdac_step) {
        fprintf(DEBUG_FILE,"Collecting %d samples for tap number %d...",num_samples,mdac_value);
        UC_sample(data,num_samples,mdac_value);
        fprintf(DEBUG_FILE,"Done\n");
        fprintf(DEBUG_FILE,"Storing data to data.csv...");
        DP_appendToCSV(data,num_samples,mdac_value);
        fprintf(DEBUG_FILE,"Done\n");
    }
    DP_writeCSV();
    free(data);
    printf("----- Data collection finished. -----\n\n");
    return 0;
}

/* Version Information */

int libchaos_getFirmwareVersion() {
    /** 
     * Get the current MDAC value from the device
     */
    return  UC_getVersion();
}

int libchaos_getVersion() {
    /** 
     * Get the current MDAC value from the device
     */
    return LIBCHAOS_VERSION;
}

/* MDAC */

int libchaos_getMDACValue() {
    /** 
     * Get the current MDAC value from the device
     */
    int mdac_value;
    UC_getStatus(&mdac_value);
    return mdac_value;
}

int libchaos_setMDACValue(int mdac_value) {
    /** 
     * Set the current MDAC value on the device
     */
    return(UC_setMDAC(mdac_value));
}

int libchaos_mdacToResistance(int mdac) {
    /** 
     * Convert given MDAC value to a resistance
     *
     * \param mdac Value of the MDAC
     * \return resistance in Ohms
     */
    return int((21100.0*36000.0)/(21100.0+float((mdac)/4095.0)*36000.0));
}

int libchaos_resistanceToMdac(int resistance) {
    /** 
     * Convert a given resistance to an MDAC value.
     *
     * \param resistance Resistance in Ohms
     * \return MDAC value
     */
    return int(4095.0*(21100.0)/float(resistance)-((4095.0*21100.0)/36000.0));
}

/* Basic Plot */

int libchaos_readPlot(int mdac_value) {
    /** 
     * Read plot data from the device at the current MDAC value
     *
     * This data gets stored to a buffer inside of libchaos. It can be 
     * accessed via calls to getNextPlotPoint.
     * 
     * An mdac_value of -1 (or any invalid value) will not change the mdac and
     * use the current value.
     */
    static int last_mdac_value = 0;
    static int last_fft = 0;
    
    int ret_val;
    int current_mdac = libchaos_getMDACValue();
    
    last_fft++;

    // check to see if the FFT should run this time
    if(last_fft > 20 || (current_mdac != last_mdac_value ) && FFT_ENABLED) {
        last_fft = 0;
        // get the data from the device
        ret_val = UC_sample(PLOT_DATA, NUM_FFT_PLOT_POINTS, mdac_value);
        DP_FFT(PLOT_DATA, FFT_DATA, NUM_FFT_PLOT_POINTS);
    } else {
        ret_val = UC_sample(PLOT_DATA, NUM_PLOT_POINTS, mdac_value);
    }
    
    // get the trigger location
    TRIGGER_INDEX = DP_findTriggerIndex(POINTS_AFTER_TRIGGER);

    // check to see if the MDAC value has changed since last call
    if(current_mdac != last_mdac_value) {
        PLOT_RETURN_MAP_NUM_POINTS = 0;
    } else {
    }
    last_mdac_value = current_mdac;
    
    // parse more return map data if needed
    if( PLOT_RETURN_MAP_NUM_POINTS < RETURN_MAP_MAX_POINTS - 50 ) {
        PLOT_RETURN_MAP_NUM_POINTS += 
            DP_getReturnMapPoints(PLOT_RETURN_MAP_POINTS+PLOT_RETURN_MAP_NUM_POINTS*3,
                                  RETURN_MAP_MAX_POINTS-PLOT_RETURN_MAP_NUM_POINTS,
                                  PLOT_DATA,
                                  NUM_PLOT_POINTS);
    }
    
    return(ret_val);
}

int libchaos_getTriggerIndex() {
    /** 
     * Get the trigger index for the plot
     *
     * The trigger index should be at approximately the same location in 
     * the waveform as the previous trigger index if the trigger 
     * succeeded.
     */
    return TRIGGER_INDEX;
}

int libchaos_getPlotPoint(int* x1, int*x2, int* x3, int index) {
    /** 
     * Get the data at a specified plot point.
     */
    
     if(index > NUM_PLOT_POINTS) {
        return -1;
     } else if (index < 0) {
        return -1;
     }
     
     int tmp1 = DP_getX1(PLOT_DATA[index]);
     int tmp2 = DP_getX2(PLOT_DATA[index]);
     int tmp3 = DP_getX3(PLOT_DATA[index]);
     
     *x1 = tmp1;
     *x2 = tmp2;
     *x3 = tmp3;
     
     return 0;
}

int libchaos_getNumPlotPoints() {
    /** 
     * Returns the number of plots available for plotting
     */
     
     return NUM_PLOT_POINTS;
}

int libchaos_setNumPlotPoints(int num) {
    /** 
     * Set the number of points on a plot
     */
    if( num >= 1020 && num <= MAX_PLOT_POINTS) {
        NUM_PLOT_POINTS = num;
        return 0;
    }
    return -1;
}

int libchaos_setTransientData(int amount) {
    /** 
     * Set the amount of transient data to drop
     *
     * \param amount Number of packets to drop before storing data.
     */
    if( amount >= 0 && amount <= 24 ) {
        UC_TRANSIENT_DATA = amount;
        return 0;
    }
    return -1;
}

/* FFT */
void libchaos_getFFTPlotPoint(float* val, int index) {
    /** 
     * Get an FFT plot point
     */
    *val = (FFT_DATA[(index*2)]);
}

void libchaos_enableFFT() {
    /** 
     * Enable the FFT
     */
    FFT_ENABLED = 1;
}

void libchaos_disableFFT() {
    /** 
     * Disable the FFT
     */
    FFT_ENABLED = 0;
}

/* Return Map */
int libchaos_getReturnMap1Point(int* x1, int* x2, int index) {
    /** 
     * Get the data at a specified return map point.
     */
    
     if(index > NUM_PLOT_POINTS) {
        return -1;
     } else if (index < 0) {
        return -1;
     }
     
     *x1 = PLOT_RETURN_MAP_POINTS[index*3];
     *x2 = PLOT_RETURN_MAP_POINTS[(index*3)+1];
     
     return 0;
}

int libchaos_getReturnMap2Point(int* x1, int* x2, int index) {
    /** 
     * Get the data at a specified return map point.
     */
    
     if(index > NUM_PLOT_POINTS) {
        return -1;
     } else if (index < 0) {
        return -1;
     }
     
     *x1 = PLOT_RETURN_MAP_POINTS[index*3];
     *x2 = PLOT_RETURN_MAP_POINTS[(index*3)+2];
     
     return 0;
}

int libchaos_getNumReturnMapPoints() {
    /** 
     * Returns the number of plots available for plotting
     */
     
     return PLOT_RETURN_MAP_NUM_POINTS;
}

/* Peaks */

int* libchaos_getPeaks(int mdac_value) {
    /** 
     * Get some peaks for a given MDAC value
     */
     
     return peaks_getPeaksAtMDAC(mdac_value);
}

int libchaos_setPeaksPerMDAC(int peaks_per_mdac) {
    /** 
     * Set the number of peaks to take store at each MDAC value
     */
    peaks_initCache(peaks_per_mdac);
    return 0;
}

bool libchaos_peaksCacheHit(int mdac_value) {
    /** 
     * Check to see if the give MDAC value will score a cache hit
     */
    return peaks_isCacheHit(mdac_value);
}
