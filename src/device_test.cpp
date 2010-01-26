/**
 * \file device_test.cpp
 * \brief Routines for running basic tests on the chaos unit
 */

#include "device_test.h"

int DT_testDevice() {
    /** 
     * Run the device self tests
     */
    char buf[64];
    int bytes_read;
    static int tries = 0;
    const int max_tries = 5;
    
    for(int i = 0; i < 8; i++) {
        buf[i] = 0x00;
    }
    
    /* Ping Test */
    buf[0] = CMD_ping;
    buf[1] = 64;
        
    fprintf(DEBUG_FILE,"Write test...");
    if(UC_write(buf,8) != 8) {
        fprintf(DEBUG_FILE,"\nTest write failed\n");
        if(UC_reset()) {
            return -1;
        } else {
            if( tries < max_tries ) {
                tries++;
                return(DT_testDevice());
            }
        }
    }
    fprintf(DEBUG_FILE,"Success\n");

    fprintf(DEBUG_FILE,"Read test...");
    if(bytes_read = UC_read(buf,64) != 64) {
        fprintf(DEBUG_FILE,"\nTest read failed. Read %d bytes.\n",bytes_read);
        if(UC_reset()) {
            return -1;
        } else {
            if( tries < max_tries ) {
                tries++;
                return(DT_testDevice());
            }
        }
    }
    if(buf[0] != 0x55) {
        fprintf(DEBUG_FILE,"\nIncorrect value in test response: %x\n",buf[0]);
        if(UC_reset()) {
            return -1;
        } else {
            if( tries < max_tries ) {
                tries++;
                return(DT_testDevice());
            }
        }
    }
    fprintf(DEBUG_FILE,"Success\n");
    
    /* LED test */
    buf[0] = CMD_LED_test;
        
    fprintf(DEBUG_FILE,"Flashing LEDs...");
    if(UC_write(buf,8) != 8) {
        fprintf(DEBUG_FILE,"\nWrite failed\n");
        if(UC_reset()) {
            return -1;
        } else {
            if( tries < max_tries ) {
                tries++;
                return(DT_testDevice());
            }
        }
    }

    bytes_read = UC_read(buf,1);
    if(bytes_read != 1) {
        fprintf(DEBUG_FILE,"\nRead failed. Read %d bytes.\n",bytes_read);
        if(UC_reset()) {
            return -1;
        } else {
            if( tries < max_tries ) {
                tries++;
                return(DT_testDevice());
            }
        }
    }
    fprintf(DEBUG_FILE,"Complete\n");
    
    return 0;
}
