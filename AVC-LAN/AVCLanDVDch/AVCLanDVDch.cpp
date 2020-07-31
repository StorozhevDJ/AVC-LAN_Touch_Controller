/*
  AVCLanDrv.cpp - AVCLan DVD changer library for 'duino / Wiring
  Created by Kochetkov Aleksey, 04.08.2010
  Modified by Podmoskovny Dmitry, September 2013
  Edit by Storozhev Denis 23/07/2020
  Version 0.2.4
*/

#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "AVCLanDrv/AVCLanDrv.h"
#include "AVCLanDVDch.h"
#include "BuffSerial/BuffSerial.h"


const AvcInMessageTable  mtMain[] PROGMEM = {
	{ACT_REGISTER,       0x03, {0x12, 0x01, 0x00}}, //*
	{ACT_INIT,           0x03, {0x12, 0x01, 0x01}}, //*
	{ACT_PLAY_REQ1,      0x04, {0x00, 0x25, 0x43, 0x80}},//*
	{ACT_PLAY_IT,        0x04, {0x12, 0x01, 0x45, 0x43 }},
	{ACT_LAN_STATUS1,    0x03, {0x00, 0x01, 0x0A}},
	{ACT_LAN_STATUS2,    0x03, {0x00, 0x01, 0x08}},
	{ACT_LAN_STATUS3,    0x03, {0x00, 0x01, 0x0D}},
	{ACT_LAN_STATUS4,    0x03, {0x00, 0x01, 0x0C}},
	{ACT_LAN_STATUS5,    0x04, {0x00, 0x00, 0x01, 0x08}},
	{ACT_SCAN_ON,        0x04, {0x00, 0x25, 0x43, 0xA6}},
	{ACT_SCAN_OFF,       0x04, {0x00, 0x25, 0x43, 0xA7}},
	{ACT_SCAN_D_ON,      0x04, {0x00, 0x25, 0x43, 0xA9}},
	{ACT_SCAN_D_OFF,     0x04, {0x00, 0x25, 0x43, 0xAA}},
	{ACT_REPEAT_ON,      0x04, {0x00, 0x25, 0x43, 0xA0}},
	{ACT_REPEAT_OFF,     0x04, {0x00, 0x25, 0x43, 0xA1}},
	{ACT_REPEAT_D_ON,    0x04, {0x00, 0x25, 0x43, 0xA3}},
	{ACT_REPEAT_D_OFF,   0x04, {0x00, 0x25, 0x43, 0xA4}},
	{ACT_RANDOM_ON,      0x04, {0x00, 0x25, 0x43, 0xB0}},
	{ACT_RANDOM_OFF,     0x04, {0x00, 0x25, 0x43, 0xB1}},
	{ACT_RANDOM_D_ON,    0x04, {0x00, 0x25, 0x43, 0xB3}},
	{ACT_RANDOM_D_OFF,   0x04, {0x00, 0x25, 0x43, 0xB4}},
	{ACT_AM_PRESS,       0x05, {0x00, 0x25, 0x32, 0x80, 0x06}}, // AM button from JBL radio (кнопка AM на магнитоле JBL)

	// power off 0401015F01
};
const byte mtMainSize = sizeof(mtMain) / sizeof(AvcInMessageTable);

const AvcInMaskedMessageTable  mtMaskedMain[] PROGMEM = {
	{ACT_DEVSTATUS_E0, 0x04, {0x00, 0, 0x45, 0xE0}, _BV(1)},//*
	{ACT_DEVSTATUS_E2, 0x04, {0x00, 0, 0x45, 0xE2}, _BV(1)},//*
	{ACT_DEVSTATUS_E4, 0x04, {0x00, 0, 0x45, 0xE4}, _BV(1)},//*
	{ACT_DEVSTATUS_EU, 0x04, {0x00, 0x34, 0x45, 0xE4}, _BV(1)},//*
	{ACT_LAN_CHECK,    0x04, {0x12, 0x01, 0x20, 0}, _BV(3)}, //*
	{ACT_PLAY_REQ2,    0x06, {0x00, 0x12, 0x45, 0x42, 0, 0x01}, _BV(4)},
	{ACT_COORDS,       0x08, {0x00, 0x21, 0x24, 0x78, 0, 0, 0, 0}, 0xF0}, // координаты нажатия, маскированы 4 байта
	{ACT_STOP_REQ1,    0x05, {0x00, 0x12, 0x43, 0x43, 0}, _BV(4)},
	{ACT_STOP_REQ2,    0x06, {0x00, 0x12, 0x43, 0x43, 0, 0x00}, _BV(4)},
};
const byte mtMaskedMainSize = sizeof(mtMaskedMain) / sizeof(AvcInMaskedMessageTable);

const AvcInMessageTable  mtSearchHead[] PROGMEM = {
	{ACT_REGISTER,  0x03, {0x12, 0x01, 0x00}},           // AVC LAN register
	{ACT_REGISTER,  0x03, {0x12, 0x01, 0x01}},           // AVC LAN init
	{ACT_REGISTER,  0x03, {0x01, 0x01, 0x58}},
	{ACT_REGISTER,  0x03, {0x01, 0x01, 0x5B}},
	{ACT_REGISTER,  0x04, {0x01, 0x01, 0x5F, 0x01}},
	{ACT_REGISTER,  0x04, {0x01, 0x01, 0x5F, 0x00}},
};
const byte mtSearchHeadSize = sizeof(mtSearchHead) / sizeof(AvcInMessageTable);

const AvcOutMessage CmdInit_0        PROGMEM =  {AVC_MSG_BROADCAST,  0x05, {0x01, 0x11, 0x13, 0x24, 0x45}};
const AvcOutMessage CmdReset         PROGMEM =  {AVC_MSG_BROADCAST,  0x05, {0x00, 0x00, 0x00, 0x00, 0x00}}; // reset AVCLan. This causes HU to send ACT_REGISTER
const AvcOutMessage CmdRegister      PROGMEM =  {AVC_MSG_DIRECT,     0x06, {0x00, 0x01, 0x12, 0x10, 0x24, 0x45}}; // register CD-changer
const AvcOutMessage CmdInit3         PROGMEM =  {AVC_MSG_BROADCAST,  0x14, {0x45, 0x31, 0xF3, 0x00, 0x02, 0x04, 0x06, 0x08, 0x09, 0x0A, 0x0C, 0x15, 0x17, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}; //* init command 3
const AvcOutMessage CmdDevStatusE0   PROGMEM =  {AVC_MSG_DIRECT,     0x10, {0x00, 0x45, 0, 0xF0, 0x03, 0x10, 0xA0, 0x01, 0x02, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}}; //* Device status E0
const AvcOutMessage CmdDevStatusE2   PROGMEM =  {AVC_MSG_DIRECT,     0x10, {0x00, 0x45, 0, 0xF2, 0x03, 0x10, 0xA0, 0x01, 0x02, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}}; //* Device status E0
const AvcOutMessage CmdDevStatusE4   PROGMEM =  {AVC_MSG_DIRECT,     0x10, {0x00, 0x45, 0, 0xF4, 0x03, 0x10, 0xA0, 0x01, 0x02, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}}; //* Device status E0
const AvcOutMessage CmdDevStatusEU   PROGMEM =  {AVC_MSG_DIRECT,     0x0A, {0x00, 0x45, 0x34, 0xF0, 0xF0, 0x06, 0x80, 0x90, 0x83}}; //* Device status EU
const AvcOutMessage CmdLanCheckOk    PROGMEM =  {AVC_MSG_DIRECT,     0x06, {0x00, 0x01, 0x12, 0x30, 0, 0x00}}; // *Check, device ok

const AvcOutMessage CmdPlayOk1       PROGMEM =  {AVC_MSG_BROADCAST,  0x06, {0x45, 0x12, 0x50, 0x13, 0x04, 0x01}}; // Play begin message 1
const AvcOutMessage CmdPlayOk2       PROGMEM =  {AVC_MSG_DIRECT,     0x06, {0x00, 0x45, 0x12, 0x52, 0x12, 0x01}}; // Play begin message 2
const AvcOutMessage CmdPlayOk3       PROGMEM =  {AVC_MSG_BROADCAST,  0x0F, {0x45, 0x31, 0xF1, 0x01, 0x10, 0xA0, 0x01, 0x02, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}}; // Play begin message 3, ch changer open



const AvcOutMessage CmdInit1         PROGMEM =  {AVC_MSG_BROADCAST,  0x05, {0x45, 0x31, 0xF7, 0x00, 0x43}}; // init command 1
const AvcOutMessage CmdInit2         PROGMEM =  {AVC_MSG_BROADCAST,  0x0B, {0x45, 0x31, 0xF1, 0x00, 0x30, 0x01, 0x01, 0x00, 0x01, 0x00, 0x80}}; // init command 2
const AvcOutMessage CmdPlayOk4       PROGMEM =  {AVC_MSG_BROADCAST,  0x0B, {0x43, 0x31, 0xF1, 0x05, 0x28, 0x01, 0x02, 0x00, 0x25, 0x00, 0x80}}; // Play begin message 4
const AvcOutMessage CmdPlayOk5       PROGMEM =  {AVC_MSG_BROADCAST,  0x05, {0x43, 0x31, 0xF7, 0x05, 0x43}}; // Play begin message 5
const AvcOutMessage CmdStopOk1       PROGMEM =  {AVC_MSG_DIRECT,     0x05, {0x00, 0x43, 0x12, 0x53, 0x01}}; // Stop ok message 1
const AvcOutMessage CmdStopOk2       PROGMEM =  {AVC_MSG_BROADCAST,  0x0B, {0x43, 0x31, 0xF1, 0x00, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00, 0x80}}; // Stop ok message 2
const AvcOutMessage CmdPlayStatus    PROGMEM =  {AVC_MSG_BROADCAST,  0x0B, {0x43, 0x31, 0xF1, 0x05, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00, 0x80}}; // CD-changer play status
const AvcOutMessage CmdLanStatus1    PROGMEM =  {AVC_MSG_DIRECT,     0x04, {0x00, 0x01, 0x00, 0x1A}}; // Lan status 1
const AvcOutMessage CmdLanStatus2    PROGMEM =  {AVC_MSG_DIRECT,     0x04, {0x00, 0x01, 0x00, 0x18}}; // Lan status 2
const AvcOutMessage CmdLanStatus3    PROGMEM =  {AVC_MSG_DIRECT,     0x04, {0x00, 0x01, 0x00, 0x1D}}; // Lan status 3
const AvcOutMessage CmdLanStatus4    PROGMEM =  {AVC_MSG_DIRECT,     0x05, {0x00, 0x01, 0x00, 0x1C, 0x00}}; // Lan status 4
const AvcOutMessage CmdLanStatus5    PROGMEM =  {AVC_MSG_DIRECT,     0x04, {0x00, 0x01, 0x00, 0x18}}; // Lan status 5



/************************************************************************/
/* AVCLan DVDchanger  & timer1 init,                                    */
/************************************************************************/
void AVCLanDVDch::begin(){
	avclan.deviceAddress = 0x0208;
	avclan.BoardDeviceAddress = 0x0190;
	// timer1 setup, prescaler factor - 1024
	TCCR1A = 0;     // normal mode
	TCCR1B = 5;     // Prescaler /1024
	TCNT1H = TI1_H; // Load counter value hi
	TCNT1L = TI1_L;	// Load counter value lo
	DISABLE_TIMER1_INT;
	AZFM_INIT;
	cd_min = cd_sec = cd_playmode = 0;
	cd_status = stWait;
	avclan.sendMessage(&CmdInit_0);
}



/************************************************************************/
/* Use the last received message to determine the corresponding         */
/* action ID, store it in avclan object                                 */
/************************************************************************/
void AVCLanDVDch::getActionID()
{
if (avclan.headAddress == 0) {
	avclan.actionID = avclan.getActionID((AvcInMessageTable *)mtSearchHead, mtSearchHeadSize);
	} else {
	avclan.actionID = avclan.getActionID((AvcInMessageTable *)mtMain, mtMainSize);
	if (avclan.actionID == ACT_NONE) avclan.actionID = avclan.getActionID((AvcInMaskedMessageTable *)mtMaskedMain, mtMaskedMainSize);
}
}; 



/************************************************************************/
/* process action                                                       */
/************************************************************************/
void AVCLanDVDch::processAction(AvcActionID ActionID){
	byte r;
	switch (ActionID) {
		// register device
		case ACT_REGISTER:
			if (avclan.headAddress == 0) avclan.headAddress = avclan.masterAddress;
			avclan.sendMessage(&CmdRegister);
		break;
		// init device
		case ACT_INIT:
			r = avclan.sendMessage(&CmdInit1);
			if (!r) r = avclan.sendMessage(&CmdInit2);
			if (!r) r = avclan.sendMessage(&CmdInit3);
			avclan.sendMessage(&CmdInit_0);
		break;
		// Device status E0
		case ACT_DEVSTATUS_E0:
			r = avclan.message[1];
			avclan.loadMessage(&CmdDevStatusE0);
			avclan.message[2] = r;
			avclan.sendMessage();
		break;
		// Device status E2
		case ACT_DEVSTATUS_E2:
			r = avclan.message[1];
			avclan.loadMessage(&CmdDevStatusE2);
			avclan.message[2] = r;
			avclan.sendMessage();
		break;
		// Device status E4
		case ACT_DEVSTATUS_E4:
			r = avclan.message[1];
			avclan.loadMessage(&CmdDevStatusE4);
			avclan.message[2] = r;
			avclan.sendMessage();
		break;
		// Play request 1
		case ACT_PLAY_REQ1:
			avclan.sendMessage(&CmdPlayOk1);
		break;
		// Play request 2
		case ACT_PLAY_REQ2:
			r = avclan.message[4];
			avclan.loadMessage(&CmdPlayOk2);
			avclan.message[4] = r;
			avclan.sendMessage();
            avclan.sendMessage(&CmdPlayOk3);
		break;
			//r = avclan.sendMessage(&CmdPlayOk2);
			//if (!r) avclan.sendMessage(&CmdPlayOk3);
			//break;
		// Press AM button
		case ACT_AM_PRESS:
			am=!am;
			if (am) {
				bSerial.print("On!");
				avclan.sendMessage(&CmdInit_0);
				avclan.sendMessage(&CmdPlayOk1);
			}; 
			if (!am) {
				bSerial.print("Off!");
				DISABLE_TIMER1_INT;
				AZFM_OFF;
				cd_status = stStop;
				cd_min = cd_sec = 0;
				avclan.sendMessage(&CmdStopOk1);
				r = avclan.message[4];
				avclan.loadMessage(&CmdStopOk1);
				avclan.message[4] = r;
				avclan.sendMessage();
				avclan.sendMessage(&CmdPlayOk2);
			};
		break;
		// Coordinates calculation
		case ACT_COORDS:
			x_coord = avclan.message[4];
			y_coord = avclan.message[5];
			bSerial.print("Coords X=");
			bSerial.printHex8(x_coord);
			bSerial.print("; Y=");
			bSerial.printHex8(y_coord);
		break;
		// device play
		case ACT_PLAY_IT:
			if (cd_status != stPlay || (cd_min == 0 && cd_sec == 0)) {
				avclan.loadMessage(&CmdPlayOk4);
				avclan.message[5] = 1;       // cd disk
				avclan.message[6] = 1;       // cd track
				avclan.message[7] = cd_min;  // play time min
				avclan.message[8] = cd_sec;  // play time sec
				r = avclan.sendMessage();
			}
			if (!r && cd_status != stPlay) avclan.sendMessage(&CmdPlayOk5);
			ENABLE_TIMER1_INT;
			AZFM_ON;
			cd_status = stPlay;
		break;
		// Stop request
		case ACT_STOP_REQ1:
		case ACT_STOP_REQ2:
			DISABLE_TIMER1_INT;
			AZFM_OFF;
			cd_status = stStop;
			cd_min = cd_sec = 0;
			avclan.sendMessage(&CmdStopOk1);
			r = avclan.message[4];
			avclan.loadMessage(&CmdStopOk1);
			avclan.message[4] = r;
			avclan.sendMessage();
			avclan.sendMessage(&CmdPlayOk2);
		break;
//			avclan.sendMessage(&CmdStopOk2);
//			avclan.sendMessage(&CmdInit1);
//			avclan.loadMessage(&CmdStopOk2);
//			avclan.message[4] = 0x30;
//			avclan.sendMessage();
			//break;
		// Lan status 1
		case ACT_LAN_STATUS1:
			avclan.sendMessage(&CmdLanStatus1);
			DISABLE_TIMER1_INT;
		break;
		// Lan status 2
		case ACT_LAN_STATUS2:
			avclan.sendMessage(&CmdLanStatus2);
			DISABLE_TIMER1_INT;
		break;
		// Lan status 3
		case ACT_LAN_STATUS3:
			avclan.sendMessage(&CmdLanStatus3);
		break;
		// Lan status 4
		case ACT_LAN_STATUS4:
			avclan.sendMessage(&CmdLanStatus4);
		break;
		// Lan status 5
		case ACT_LAN_STATUS5:
			avclan.sendMessage(&CmdLanStatus5);
		break;
		// Lan status 5
		case ACT_LAN_CHECK:
			r = avclan.message[3];
			avclan.loadMessage(&CmdLanCheckOk);
			avclan.message[4] = r;
			avclan.sendMessage();
		break;
		// Scan mode on
		case ACT_SCAN_ON:
			cd_playmode |= pmScan;
			sendStatus();
		break;
		// Scan mode off
		case ACT_SCAN_OFF:
			cd_playmode &= ~pmScan;
			sendStatus();
		break;
		// Scan directory mode on
		case ACT_SCAN_D_ON:
			cd_playmode |= pmScanD;
			sendStatus();
		break;
		// Scan directory mode off
		case ACT_SCAN_D_OFF:
			cd_playmode &= ~pmScanD;
			sendStatus();
		break;
		// Repeat mode on
		case ACT_REPEAT_ON:
			cd_playmode |= pmRepeat;
			sendStatus();
		break;
		// Repeat mode off
		case ACT_REPEAT_OFF:
			cd_playmode &= ~pmRepeat;
			sendStatus();
		break;
		// Repeat directory mode on
		case ACT_REPEAT_D_ON:
			cd_playmode |= pmRepeatD;
			sendStatus();
		break;
		// Repeat directory mode off
		case ACT_REPEAT_D_OFF:
			cd_playmode &= ~pmRepeatD;
			sendStatus();
		break;
		// Random mode on
		case ACT_RANDOM_ON:
			cd_playmode |= pmRandom;
			sendStatus();
		break;
		// Random mode off
		case ACT_RANDOM_OFF:
			cd_playmode &= ~pmRandom;
			sendStatus();
		break;
		// Random directory mode on
		case ACT_RANDOM_D_ON:
			cd_playmode |= pmRandomD;
			sendStatus();
		break;
		// Random directory mode off
		case ACT_RANDOM_D_OFF:                             
			cd_playmode &= ~pmRandomD;
			sendStatus();
		break;
	}
};



/************************************************************************/
/* process event                                                        */
/************************************************************************/
void AVCLanDVDch::processEvent(AvcEventID EventID){
	switch (EventID){
		case EV_STATUS:
			sendStatus();
			avclan.event &= ~EV_STATUS;
			break;
	}
};



/************************************************************************/
/* send DVD-changer status to head                                      */
/************************************************************************/
byte AVCLanDVDch::sendStatus(){
	avclan.loadMessage(&CmdPlayStatus);
	avclan.message[4] = cd_status;   // cd changer status: 10-play, 80-load, 01-open, 02=err1, 03-wait
	avclan.message[5] = 1;           // cd disk
	avclan.message[6] = 1;           // cd track
	avclan.message[7] = cd_min;      // play tme min
	avclan.message[8] = cd_sec;      // play time sec
	avclan.message[9] = cd_playmode; // play mode: 0-normal, 2-disc rand, 4-rand, 8-disc repeat, 10-repeat, 20-disc scan, 40-scan
	
	return avclan.sendMessage();
}



/************************************************************************/
/*                                                                      */
/************************************************************************/
byte AVCLanDVDch::hexInc(byte data){
	if ((data & 0x9) == 0x9) return (data + 7);
	return (data + 1);
}



/************************************************************************/
/*                                                                      */
/************************************************************************/
byte AVCLanDVDch::hexDec(byte data)
{
if ((data & 0xF) == 0) return (data - 7);
return (data - 1);
}



/************************************************************************/
/* timer1 overflow                                                      */
/************************************************************************/ 
ISR (TIMER1_OVF_vect)
{
TCNT1H = TI1_H; // Load counter value hi
TCNT1L = TI1_L;	// Load counter value lo
/*avclanDevice.cd_sec = avclanDevice.hexInc(avclanDevice.cd_sec);
if (avclanDevice.cd_sec == 0x60)
	{
	avclanDevice.cd_sec = 0;
	avclanDevice.cd_min = avclanDevice.hexInc(avclanDevice.cd_min);
	if (avclanDevice.cd_min == 0xA0) {avclanDevice.cd_min=0x0;}
	}
avclan.event = EV_STATUS;*/
}



AVCLanDVDch avclanDevice;