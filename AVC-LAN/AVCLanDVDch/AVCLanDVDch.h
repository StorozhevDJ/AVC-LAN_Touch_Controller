/*
  AVCLanDrv.h - AVCLan DVD changer library for 'duino / Wiring
  Created by Kochetkov Aleksey, 04.08.2010
  Modified by Podmoskovny Dmitry, September 2013
  Version 0.2.3
*/

#ifndef AVCLanDVDch_h
#define AVCLanDVDch_h

#include <avr/pgmspace.h>
#include "AVCLanDrv/AVCLanDrv.h"
//#include "Arduino.h"
//#include "WProgram.h"

// timer1 overflow every 1 sec 
#define TI1_H	(((word)-(F_CPU / 1024)) >> 8)
#define TI1_L	(((word)-(F_CPU / 1024)) & 0xff )

#if defined(__AVR_ATmega8__)
#define ENABLE_TIMER1_INT  (sbi(TIMSK, TOIE1));
#define DISABLE_TIMER1_INT (cbi(TIMSK, TOIE1));
#else
#define ENABLE_TIMER1_INT  (sbi(TIMSK1, TOIE1));
#define DISABLE_TIMER1_INT (cbi(TIMSK1, TOIE1));
#endif

#define AVCLANDEVICE_NAME    " DVD changer"
#define AVCLANDEVICE_VERSION "0.2.4"

extern const AvcOutMessage CmdReset; // reset AVCLan. This causes HU to send ACT_REGISTER
extern const AvcOutMessage CmdRegister; // register CD changer
// extern AvcOutMessage *CmdTest; // test message

typedef enum{
	ACT_REGISTER = 1,
	ACT_INIT,
	ACT_DEVSTATUS_E0,
	ACT_DEVSTATUS_E2,
	ACT_DEVSTATUS_E4,
	ACT_DEVSTATUS_EU,
	ACT_STATUS_E0,
	ACT_PLAY_REQ1,
	ACT_PLAY_REQ2,
	ACT_PLAY_IT,
	ACT_STOP_REQ1,
	ACT_STOP_REQ2,
	ACT_LAN_STATUS1,
	ACT_LAN_STATUS2,
	ACT_LAN_STATUS3,
	ACT_LAN_STATUS4,
	ACT_LAN_STATUS5,
	ACT_LAN_CHECK,
	ACT_SCAN_ON,
	ACT_SCAN_OFF,
	ACT_SCAN_D_ON,
	ACT_SCAN_D_OFF,
	ACT_REPEAT_ON,
	ACT_REPEAT_OFF,
	ACT_REPEAT_D_ON,
	ACT_REPEAT_D_OFF,
	ACT_RANDOM_ON,
	ACT_RANDOM_OFF,
	ACT_RANDOM_D_ON,
	ACT_RANDOM_D_OFF,
	ACT_AM_PRESS,
	ACT_SCREEN,
} AvcActionID;

typedef enum{
	EV_STATUS = 1,
} AvcEventID;

typedef enum{
	stWait = 0x03,
	stPlay = 0x10,
	stStop = 0x30,
} cdStatus;

typedef enum{
	pmScan    = 0x40,
	pmScanD   = 0x20,
	pmRepeat  = 0x10,
	pmRepeatD = 0x08,
	pmRandom  = 0x04,
	pmRandomD = 0x02,
	pmNormal  = 0x00,
} cdPlayMode;

class AVCLanDVDch{
	public:
		byte		cd_min;                      // minutes play
		byte		cd_sec;                      // seconds play
		cdStatus	cd_status;                   // cd changer status
		byte		cd_playmode;                 // play mode (scan, random etc..)
		void		begin ();                    // initialisation, obligatory method
		void		getActionID();               // get action id by recieved message, obligatory method
		void		processAction(AvcActionID);  // process action, obligatory method
		void		processEvent(AvcEventID);    // process event, obligatory method
		byte		sendStatus();                // send CD-changer status to head
		byte		hexInc(byte data);
		byte		hexDec(byte data);
		bool 		am;
		byte		x_coord;			// X координата 00.FF
		byte		y_coord;			// Y координата 00.FF

};
#endif

extern AVCLanDVDch avclanDevice;