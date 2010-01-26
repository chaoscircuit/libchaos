/**
 * \file peaks.cpp
 * \brief Routines for finding and caching peak values
 */
 
#include "peaks.h"

int PEAKS_INITIALIZED = 0;
int PEAKS_PER_MDAC = 0;
int* PEAKS_CACHE[4096];
int* PEAKS_SAMPLES;
int PEAKS_NUM_SAMPLES;

int peaks_initCache(int peaks_per_mdac) {
    /** 
     * Initialize the peaks cache
     */

    const int samples_per_peak = 75;

    if(PEAKS_INITIALIZED) {
        for(int i = 0; i < 4096; i++) {
            free(PEAKS_CACHE[i]);
        }
        free(PEAKS_SAMPLES);
    }
    
    PEAKS_PER_MDAC = peaks_per_mdac;
    PEAKS_INITIALIZED = 1;
    
    for(int i = 0; i < 4096; i++) {
        // allocate the spcae
        PEAKS_CACHE[i] = (int*)malloc(peaks_per_mdac*4);
        // set the initial value to -1
        PEAKS_CACHE[i][0] = -1;
    }
    
    PEAKS_NUM_SAMPLES = PEAKS_PER_MDAC*samples_per_peak;
    PEAKS_SAMPLES = (int*)malloc(PEAKS_NUM_SAMPLES*sizeof(int));
    
    // mark as cache initialized
    return((int)PEAKS_CACHE);
}

int peaks_isCacheHit(int mdac_value) {
    /** 
     * Return true if the peaks are in the cache
     */
    if(mdac_value < 4096 && mdac_value > -1) {
        return(PEAKS_CACHE[mdac_value][0] != -1);
    } else {
        return 0;
    }
}

int* peaks_getPeaksAtMDAC(int mdac_value, int delta) {
    /** 
     * Get some peaks for a given MDAC value
     */

    if(!PEAKS_INITIALIZED) {
        peaks_initCache();
    }
    
    if(mdac_value > 4095 || mdac_value < 0) {
            // TODO: this should throw some sort of error
            return PEAKS_CACHE[0];
    }
    
    if(peaks_isCacheHit(mdac_value)) {
        // just return a pointer to the data if we already have it
        return PEAKS_CACHE[mdac_value];
    } else {
        // otherwise, find the peaks
        fprintf(DEBUG_FILE, "Taking peaks detection data %d\r\n", mdac_value);
        UC_sample(PEAKS_SAMPLES, PEAKS_NUM_SAMPLES, mdac_value);
        peaks_findPeaks(PEAKS_CACHE[mdac_value], //dst
                        PEAKS_PER_MDAC, //len
                        PEAKS_SAMPLES, //source data
                        PEAKS_NUM_SAMPLES, //num_samples in source
                        delta);
    }
	
	return PEAKS_CACHE[mdac_value];
}

int peaks_findPeaks(int* dst, int len, int* sample_data, int num_samples, int delta) {
    /** 
     * Find peaks and store them to memory buffer
     *
     * Returns the number of peaks detected
     */
    int min = 2000;
    int max = -1;
    int max_position = -1;
    int min_position = -1;
    int look_for_max = false;
    int max_count = 0;

    // Loop through data
    for(int i = 0; i < num_samples; i++)
    {
        int current = DP_getX1(sample_data[i]);
        // Is this a potential max?
        if(current > max) {
            max = current;
            max_position = i;
        }
        // Is this a potential min?
        if(current < min) {
            min = current;
            min_position = i;
        }
        if(look_for_max == true) {
            // If we are looking for a max and we go back down by delta,
            // then we must have found one. Let's store it!
            if(current < max - delta) {
            
                // Fit max to parabola
                const float timestep = 1/float(LIBCHAOS_SAMPLE_FREQUENCY);
                int xprev = DP_getX1(sample_data[max_position - 1]);
                int xnext = DP_getX1(sample_data[max_position + 1]);
                
                float aa = (xprev + xnext - 2*current)/(2*timestep*timestep);
                float bb = (4*current-xnext-3*xprev)/(2*timestep);
                float cc = xprev;
                
                dst[max_count] = int(cc-bb*bb/(4*aa));
                max_count++;
                if(max_count >= len) {
                    // Stop looking for maxes
                    break;
                }
                // Reset minimum
                min = current;
                min_position = i;
                // Find the next minimum
                look_for_max = false;
            }
        } else {
            if (current > min + delta) {
                // Reset maximum
                max = current;
                max_position = i;
                // Find the next maximum
                look_for_max = true;
            }
        }
    }
    return max_count;
}
