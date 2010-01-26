/**
 * \file data_processing.cpp
 * \brief Routines for processing data
 */

#include "data_processing.h"

FILE* DP_CSV;

int DP_getX1(int data_point) {
    /** 
     * Returns 10 bit x1 as an int from a sample point
     */
    return (int)((data_point & 0x000FFC) >> 2);
}

int DP_getX2(int data_point) {
    /** 
     * Returns 10 bit x2 as an int from a sample point
     */
    return (int)((data_point & 0x3FF000) >> 12);
}

int DP_getX3(int data_point) {
    /** 
     * Returns 10 bit x3 as an int from a sample point
     */
    return (int)((data_point & 0xFFC00000) >> 22);
}

int DP_newCSV(char* filename) {
    /** 
     * Create a new CSV file
     *
     * Deletes existing file if it exists    
     */
    return (int)(DP_CSV = fopen(filename,"w"));
}

void DP_appendToCSV(int* src_data, int length, int mdac_value) {
    /** 
     * Append data to the CSV file
     *
     * newCSV must be called before this so that the data has a place 
     * to go
     */
    int x1, x2, x3;
    int x1_prev = 0, x2_prev = 0, x3_prev = 0;
    unsigned int data;
    
    for(int i = 0; i<length; i++) {
        data = *(unsigned int*)&src_data[i];
        x1 = DP_getX1(data);
        x2 = DP_getX2(data);
        x3 = DP_getX3(data);
        if((abs(x1 - x1_prev) > 100 || 
            abs(x2 - x2_prev) > 100 || 
            abs(x3 - x3_prev) > 100) &&
            (i > 0) && i < length - 4) {
           fprintf(DEBUG_FILE,"ERROR: Discontinuity at point %d\n",i);
        }
        fprintf(DP_CSV,"%d,%d,%d,%d\n",mdac_value,x1,x2,x3);
        x1_prev = x1;
        x2_prev = x2;
        x3_prev = x3;
    }
}

void DP_writeCSV() {
    /** 
     * Write the data to the CSV file
     */
    fclose(DP_CSV);
}

int DP_getReturnMapPoints(int* dst, int len, int* src, int num_samples) {
    /** 
     * Get the points for a return map.
     */
    int* peaks = (int*)malloc(sizeof(int) * (num_samples / 20));
    int num_peaks;
    int i;
    int max_points;
    
    num_peaks = peaks_findPeaks(peaks, num_samples / 20, src, num_samples, 5);
    
    if ( len < num_peaks ) {
        max_points = len;
    } else {
        max_points = num_peaks;
    }
    
    for( i = 0 ; i < (max_points-2); i+=1 ) {
        dst[i*3] = peaks[i];
        dst[(i*3)+1] = peaks[i+1];
        dst[(i*3)+2] = peaks[i+2];
    }

    return max_points - 3;
}

void DP_swap(float* a, float* b) {
    /** 
     * Swap the contents of a with the contents of b
     */
    double tmp;
    
    tmp = *a;
    
    *a = *b;
    *b = tmp;
}

void DP_FFT(int* in, float* out, unsigned int length) {
    /** 
     * Perform the FFT.
     *
     * \param data The data to perform the FFT operation on
     * \param nn The number of points in the data set
     *
     * This wraps the main FFT routine and provides a power curve (by 
     * squaring the value) it also places the data on a log scale so 
     * that it is more useful.
     */
    unsigned int i;

    // convert plot data to floats for FFT
    for( i = 0; i < length; i++ ) {
        out[i*2] = (float)(DP_getX1(in[i]));
        out[(i*2)+1] = 0;
    }
    
    // perform the FFT
    DP_runFFT(out, length);
    
    // post processing
    for(i = 0; i < length; i++) {
        // square the output
        out[i] = pow(out[i],2);
        // put it on a log scale
        out[i] = log10(out[i]);
    }
}

void DP_runFFT(float* data, unsigned long nn) {
    /** 
     * Run the FFT algoritm
     *
     * \param data The data to perform the FFT operation on
     * \param nn The number of points in the data set
     *
     * This is an in place algorithm (Cooley-Tukey), so the data is simply
     * replaced by its discrete Fourier transform.
     *
     * This isn't written to be the fastest FFT possible, but it gets
     * the job done and is really quite simple.
     *
     * Some of this code was derived from work published here:
     *
     * http://www.ddj.com/cpp/199500857
     */
    unsigned long n, mmax, m, j, istep, i;
    float wtemp, wr, wpr, wpi, wi, theta;
    float tempr, tempi;

    // reverse-binary reindexing
    n = nn<<1;
    j=1;
    for (i=1; i<n; i+=2) {
        if (j>i) {
            DP_swap(&data[j-1], &data[i-1]);
            DP_swap(&data[j], &data[i]);
        }
        m = nn;
        while (m>=2 && j>m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    };


    // here begins the Danielson-Lanczos section
    mmax=2;
    while (n>mmax) {
        istep = mmax<<1;
        theta = -(2*M_PI/mmax);
        wtemp = sin(0.5*theta);
        wpr = -2.0*wtemp*wtemp;
        wpi = sin(theta);
        wr = 1.0;
        wi = 0.0;
        for (m=1; m < mmax; m += 2) {
            for (i=m; i <= n; i += istep) {
                j=i+mmax;
                tempr = wr*data[j-1] - wi*data[j];
                tempi = wr * data[j] + wi*data[j-1];


                data[j-1] = data[i-1] - tempr;
                data[j] = data[i] - tempi;
                data[i-1] += tempr;
                data[i] += tempi;
            }
            wtemp=wr;
            wr += wr*wpr - wi*wpi;
            wi += wi*wpr + wtemp*wpi;
        }
        mmax=istep;
    }
}

int DP_findTriggerIndex(int points_after_trigger) {
    /** 
     * Set the trigger index
     *
     * This looks for the same location in the waveform as the previous
     * trigger and then sets this value in the library.
     */
    // find TRIGGER_INDEX
    static int x1_trig = 0;
    static int x2_trig = 0;
    const int sensativity = 2;
    int x1,x2,x3;
    
    int trigger_index = 0;

    for(int i = 0; i < libchaos_getNumPlotPoints()-points_after_trigger; i++) {
        libchaos_getPlotPoint(&x1,&x2,&x3, i);
        if(x1 < x1_trig + sensativity &&
           x1 > x1_trig - sensativity &&
           x2 < x2_trig + sensativity &&
           x2 > x2_trig - sensativity) {
            trigger_index = i;
            break;
        }
    }
    if(trigger_index==0) {
        x1_trig = x1;
        x2_trig = x2;
    }
    return trigger_index;
}
