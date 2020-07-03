/*
 * commands.h
 *
 * Created: 26.03.2013 18:04:56
 *  Author: zyxel
 */ 


#ifndef COMMANDS_H_
#define COMMANDS_H_

	#include <avr/io.h>
	#include <stdbool.h>
	
	void cmdInit();
	int16_t cmdByte(uint8_t data);
	void cmdTic();

#endif /* COMMANDS_H_ */