/**
 * \file usb_comm.cpp
 * \brief Routines to handle the USB communication
 */

#include "usb_comm.h"
#include "string.h"
usb_dev_handle * UC_device_handle = NULL;
/* initialization routines */

int LAST_PACKET_ID;
bool USB_DEV_CONNECTED = false;

int UC_TRANSIENT_DATA = 4;

char UC_OUT_BUF[8];
char UC_IN_BUF[1024];

int UC_init() {
    /** 
     * Initialize the USB communication
     *
     * 1. Initialize libUSB
     *
     * 2. Open the device
     *
     * 3. Set the USB configuration
     *
     * 4. Claim the interface
     */
    usb_init(); /* initialize the library */
    usb_find_busses(); /* find all busses */
    usb_find_devices(); /* find all connected devices */
    usb_set_debug(0);

    return UC_connect();
}

int UC_connect() {
	fprintf(DEBUG_FILE,"Opening the device...");
	USB_DEV_CONNECTED = false;
    if(UC_open()) {
        fprintf(DEBUG_FILE,"error: device not found!\n");
        return -1;
    }
    fprintf(DEBUG_FILE,"Success\n");

    fprintf(DEBUG_FILE,"Setting USB configuration...");
    if(usb_set_configuration(UC_device_handle, 1)) {
        fprintf(DEBUG_FILE,"error: setting config 1 failed\n");
        UC_close();
        return -2;
    }
    fprintf(DEBUG_FILE,"Success\n");

    fprintf(DEBUG_FILE,"Claiming the USB interface...");
    if(usb_claim_interface(UC_device_handle, 0) < 0) {
        fprintf(DEBUG_FILE,"error: claiming interface 0 failed\n");
        UC_close();
        return -3;
    }
	
    fprintf(DEBUG_FILE,"Success\n");
	USB_DEV_CONNECTED = true;
	return 0;
}

int UC_open() {
    /** 
     * Open the USB device
     */
    struct usb_bus *bus;
    struct usb_device *dev;

    for(bus = usb_get_busses(); bus; bus = bus->next)  {
        for(dev = bus->devices; dev; dev = dev->next)  {
                if(dev->descriptor.idVendor == MY_VID
                     && dev->descriptor.idProduct == MY_PID) {
                        UC_device_handle = usb_open(dev);    
                        return 0;
                    }
            }
    }
    return -1;
}

int UC_close() {
    /** 
     * Close the USB device
     */
    usb_release_interface(UC_device_handle, 0);
    usb_close(UC_device_handle);
	USB_DEV_CONNECTED = false;
    return 0;
}

/* Low level read and write */

int UC_write(char* buf, int size) {
    /** 
     * Write data to USB
     */
	int result = -1;
	if(USB_DEV_CONNECTED == false) {
		if(UC_connect() == 0) {
			result = usb_bulk_write(UC_device_handle, EP_OUT, buf, size, UC_TIMEOUT);
		}
	} else {
		result = usb_bulk_write(UC_device_handle, EP_OUT, buf, size, UC_TIMEOUT);
	}
    return result;
}

int UC_read(char* buf, int size) {
    /** 
     * Read data from USB
     */
	int result = -1;
	if(USB_DEV_CONNECTED == false) {
		if(UC_connect() == 0) {
			result = usb_bulk_read(UC_device_handle, EP_IN, buf, size, UC_TIMEOUT);
		}
	} else {
		result = usb_bulk_read(UC_device_handle, EP_IN, buf, size, UC_TIMEOUT);
	}
    return result;
}

/* Command implemenations */

int UC_reset() {
    /** 
     * Send the reset command
     */
    int bytes_read;
    
    for(int i = 0; i < 8; i++) {
        UC_OUT_BUF[i] = 0x00;
    }
    
    UC_OUT_BUF[0] = CMD_reset;
    
    if(UC_write(UC_OUT_BUF,8) != 8) {
        fprintf(DEBUG_FILE,"Write failed, checking for pending read.\n");
        if(bytes_read = UC_read(UC_IN_BUF,1024) < 0) {
            fprintf(DEBUG_FILE,"Read failed.\n");
            return -1;
        } else {
            fprintf(DEBUG_FILE,"Read %d bytes.\n",bytes_read);
        }
        fprintf(DEBUG_FILE,"Read succeeded (%d bytes).\n",bytes_read);
        UC_OUT_BUF[0] = CMD_reset;
        if(UC_write(UC_OUT_BUF,8) != 8) {
            fprintf(DEBUG_FILE,"Write still failed even after read.\n");
            return -1;
        }
    }
    
    if(bytes_read = UC_read(UC_IN_BUF,1) != 1) {
        fprintf(DEBUG_FILE,"Read failed after reset sent.\n");
        return -1;
    }
    
    return 0;
}

int UC_setMDAC(short int tap) {
    /** 
     * Send the command to set the MDAC
     *
     */
    char* buf = UC_OUT_BUF;

    // start the sample
    buf[0] = CMD_set_mdac;
    *(short int*)&buf[4] = tap;
    if(UC_write(buf, 8) != 8) {
        fprintf(DEBUG_FILE,"error: set MDAC write failed\n");
        return -1;
    }
  
    if(UC_read(buf,1) != 1) {
      fprintf(DEBUG_FILE,"error: set MDAC read failed\n");
      return -1;
    }
    return 0;
}

int UC_startSample(short int tap) {
    /** 
     * Send the command to start a sample
     */
    char* buf = UC_OUT_BUF;
    int* in = (int*)UC_IN_BUF;
    
    #ifdef EXTRA_TRANSIENT_REMOVAL
        const int num_above = 300;
    
        if(tap - num_above >= 0) {
            buf[0] = CMD_start_sample;
            *(short int*)&buf[4] = tap - num_above;
            if(UC_write(buf, 8) != 8) {
                fprintf(DEBUG_FILE,"error: start sampling write failed\n");
                return -1;
            }
          
            if(UC_read(buf,1) != 1) {
              fprintf(DEBUG_FILE,"error: start sampling read failed\n");
              return -1;
            }
            
            // take a few packets of data and drop them to clear transient behavior
            for( int i = 0; i <= UC_TRANSIENT_DATA; i++ ) {
                if((UC_getData(in)) < 0) {
                    fprintf(DEBUG_FILE,"error getting data\n");
                    return -1;
                }
            }
    
            UC_endSample();
        }
    #endif

    /* start the real sample */
    buf[0] = CMD_start_sample;
    *(short int*)&buf[4] = tap;
    if(UC_write(buf, 8) != 8) {
        fprintf(DEBUG_FILE,"error: start sampling write failed\n");
        return -1;
    }
  
    if(UC_read(buf,1) != 1) {
      fprintf(DEBUG_FILE,"error: start sampling read failed\n");
      return -1;
    }

    // take a few packets of data and drop them to clear transient behavior
    for( int i = 0; i <= UC_TRANSIENT_DATA; i++ ) {
        if((UC_getData(in)) < 0) {
            fprintf(DEBUG_FILE,"error getting data\n");
            return -1;
        }
    }
    LAST_PACKET_ID = UC_TRANSIENT_DATA;
    return 0;
}

int UC_getData(int* dst) {
    /** 
     * Send a data request to the device
     */
    char* out = UC_OUT_BUF;
    
    out[0] = CMD_get_data;
    
    if(UC_write(out, 8) != 8) {
        fprintf(DEBUG_FILE,"error: bulk write failed\n");
        return -1;
    }

    if(UC_read((char*)dst, 1024) < 0) {
      fprintf(DEBUG_FILE,"error: bulk read failed\n");
      return -1;
    }
    
    return dst[0];
}

int UC_endSample() {
    /** 
     * Send a request to end the sample
     */
    char* buf = UC_OUT_BUF;

    buf[0] = CMD_end_sample;
    if(UC_write(buf, 8) != 8) {
        fprintf(DEBUG_FILE,"error: end sampling write failed\n");
        return -1;
    }
  
    if(UC_read(buf,1) != 1) {
      fprintf(DEBUG_FILE,"error: end sampling read failed\n");
      return -1;
    }
    return 0;
}

int UC_sample(int* dst, int num_samples, int value) {
    /** 
     * Read a sample from the device into a destination buffer
     *
     * dst is a pointer to an area of memory large enough to hold the data
     * num_samples is the number of data points to get
     * value is the value to send to the MDAC
     */
    UC_startSample(value);
    UC_sampleCurrent(dst, num_samples);
    UC_endSample();
    return 0;
}

int UC_sampleCurrent(int* dst, int num_samples) {
    /** 
     * Read a sample to a buffer at the current MDAC value
     *
     * dst is a pointer to an area of memory large enough to hold the data
     * num_samples is the number of data points to get
     * the mdac value is unchanged
     * start sample must be called before this
     * end sample must be called after this
     */
    int packet_id = 0;
    int current_sample = 0;
    
    int* in = (int*)UC_IN_BUF;
    
    int len = 0;
    
    for(current_sample = 0; current_sample < num_samples; current_sample+=255) {
        if((packet_id = UC_getData(in)) < 0) {
            fprintf(DEBUG_FILE,"error getting data\n");
            return -1;
        } else {
            if ( num_samples - current_sample > 255 ) {
                len = 1020;
            } else {
                len = (num_samples - current_sample)*4;
            }
            memcpy((char*)&dst[current_sample],in + 1,len);
            if(packet_id != LAST_PACKET_ID + 1) {
                fprintf(DEBUG_FILE,"MISSING %d PACKETS (%d)\n",(packet_id - LAST_PACKET_ID) - 1,packet_id);
            }
        LAST_PACKET_ID = packet_id;
        }
    }
    return 0;
}

int UC_getStatus(int* mdac_value) {
    /** 
     * Get the status from the device
     */
	
    char* buf = UC_OUT_BUF;
    buf[0] = CMD_status;
    if(UC_write(buf, 8) != 8) {
        fprintf(DEBUG_FILE,"error: status write failed\n");
        return -1;
    }
  
    if(UC_read((char*)mdac_value,4) != 4) {
      fprintf(DEBUG_FILE,"error: status read failed\n");
      return -1;
    }
    return 0;
}

int UC_getVersion() {
    /** 
     * Get the firmware version
     */
	
    int version;
     
    char* buf = UC_OUT_BUF;
    buf[0] = CMD_get_version;
    if(UC_write(buf, 8) != 8) {
        fprintf(DEBUG_FILE,"error: status write failed\n");
        return -1;
    }
  
    if(UC_read((char*)&version,4) != 4) {
      fprintf(DEBUG_FILE,"error: status read failed\n");
      return -1;
    }
    
    return version;
}

bool UC_isConnected() {
	/**
	* Return true if the device is found on the system.
	*/
    usb_find_busses(); /* find all busses */
    usb_find_devices(); /* find all connected devices */
	
    struct usb_bus *bus;
    struct usb_device *dev;
    for(bus = usb_get_busses(); bus; bus = bus->next)  {
        for(dev = bus->devices; dev; dev = dev->next)  {
                if(dev->descriptor.idVendor == MY_VID
                     && dev->descriptor.idProduct == MY_PID) {
                        return true;
                    } 
            }
    }
	if(USB_DEV_CONNECTED == true) {
		UC_close();
	}
	return false;
}
