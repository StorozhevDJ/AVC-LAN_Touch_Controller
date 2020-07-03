/*
 * MultiKeyboard.h
 *
 * Created: 18.03.2013 11:42:15
 *  Author: zyxel
 */ 


#ifndef TOUCHCONTROLLER_H_
#define TOUCHCONTROLLER_H_


	#include <avr/io.h>
	#include <avr/wdt.h>
	#include <avr/power.h>
	#include <avr/interrupt.h>
	
	
	#include <stdbool.h>
	#include <string.h>

	#include "Descriptors.h"
	#include <LUFA/Drivers/USB/USB.h>


	typedef struct
	{
		uint16_t Button; 
		int16_t  X; 
		int16_t  Y; 
	} ATTR_PACKED USB_DigitizerReport_Data_t;


int main(void);

void SetupHardware(void);
	
#endif /* TOUCHCONTROLLER_H_ */