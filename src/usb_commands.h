/**
 * \file usb_commands.h
 * \brief Defines the values to be used for USB commands
 *
 * This library is shared between the firmware and libchaos to guarantee
 * that we are using the same command set
 */

/* Commands */

#define CMD_reset 0x00
#define CMD_status 0x01
#define CMD_start_sample 0x02
#define CMD_get_data 0x03
#define CMD_end_sample 0x04
#define CMD_set_mdac 0x05
#define CMD_get_version 0x06

#define CMD_ping 0x80
#define CMD_LED_test 0x81

#define CMD_none 0xFF
