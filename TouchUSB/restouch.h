/*
 * restouch.h
 *
 * Created: 25.03.2013 21:25:59
 *  Author: zyxel
 */ 


#ifndef RESTOUCH_H_
#define RESTOUCH_H_


	#include <avr/io.h>
	#include <stdbool.h>

	typedef struct {
		int32_t kx;
		int32_t ky;
		uint8_t dx;
		uint8_t dy;
		uint16_t xsize;
		uint16_t ysize;
	} SCalibrateData;
	
	extern SCalibrateData cdata;
	
	extern uint8_t adc_x;
	extern uint8_t adc_y;
	extern bool touched;
	
	
	void rtInit();
	bool rtTouchTask();
	void rtReadFromEeprom();
	void rtWriteToEeprom();
	
#endif /* RESTOUCH_H_ */