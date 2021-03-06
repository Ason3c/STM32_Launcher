#include "stm32f10x.h"
#pragma once
#include "Util/fat_fs/inc/ff.h"
#include "Util/buffer.h"
#include "Util/delay.h"
#include "pwm.h"
#define BUFFER_SIZE 256

#define PRE_SIZE 1000000ul	/*Preallocate size*/

//Ignition system defines
#define IGNITION_END 1600


#define XY_RATE_LIMIT 10.0	/*10 degrees per second max in horizontal plane*/
#define Z_RATE_LIMIT 30.0	/*30 around the vertical axis*/
#define LAUNCH_STABLE_PERIOD 12000 /*need 12 seconds of stability before a launch can go ahead*/
#define PERMISSION_DURATION 90000 /*90 seconds duration for autolaunch*/
#define PERMISSION_HOLD 10000	/*Command must be less than 10 seconds from the end of the window*/

#define LAUNCH_ALTITUDE 37000UL

//Other important config
#define CALLSIGN "Foo"

#define UPLINK_CALLSIGN "$$ROK"	/* This should be a 5 character callsign, first 4 characters are used for the hardware address filter */

#define CUTDOWN_COMMAND 0	/* Bit 0 (i.e. command 0) is used to request a cutdown */

#define LAUNCH_COMMAND 6
#define LAUNCH_PERMISSION 7
#define LAUNCH_REFUSED 2
#define IGNITION_FLAG_BITS 3	/* Bits 3, 4, and 5 used for autolaunch feedback whilst permission is set*/
#define UPLINK_TEST_BIT 1	/* Bit 1  toggles when there is an uplink*/

#define MISSION_TIMEOUT 12600000UL /* 3.5 hours */

#define COUNTDOWN_DELAY 5500	/* 5s between GoPro turn on and start of launch sequence (measured from GoPro Hero3+, black edition at 240fps)*/
//GoPro stuff
#define GOPRO_TRIGGER_TIME 450	/*450 ms delay */

//#define SINGLE_LOGFILE
#ifndef SINGLE_LOGFILE
 #define LOGFILE_NAME print_string
#else
 #define LOGFILE_NAME "logfile.txt"
#endif

#define SHUTDOWNLOCK_MAGIC 0xFEED/*Used in the setting file to set flightmode - i.e. unit cannot be turned off unless USB is configured*/

#define SYSTEM_STATES 5		/*Number of different control states- skip GPS, test CUT, shutdown, test GoPro*/

#define WATCHDOG_TIMEOUT 9000	/*9 second timeout - enough for the uSD card to block its max time and a bit, and enough for RTTY*/

enum{BUTTON_TURNOFF=1,USB_INSERTED,ERR,LOW_BATTERY,MULTIPRESS_TURNOFF};
enum{BUTTON_SOURCE=0,USB_SOURCE=1};
enum{IGNITION_TEST=0,CUTDOWN_TEST,POLYGON_CUT,TIMEOUT_CUT,UPLINK_CUT,IGNITION_FIRED,IGNITION_FIRING,CUTDOWN_FIRING};//The bits of the cutdown/igintion flag byte
//Battery specific config goes here
#define BATTERY_STARTUP_LIMIT 3.7 /*Around 25% capacity remaining for lithium polymer at 25C slow discharge*/
#define MINIMUM_VOLTAGE 3.42	/* Lowest cell voltage through LDO regulator - no smps */

//function prototypes
void __fat_print_char(char c);
void __str_print_char(char c);
uint8_t detect_sensors(uint8_t init);
void file_preallocation_control(FIL* file);

//fatfs globals
extern volatile uint8_t file_opened;
extern FIL FATFS_logfile,FATFS_wavfile_gyro;
