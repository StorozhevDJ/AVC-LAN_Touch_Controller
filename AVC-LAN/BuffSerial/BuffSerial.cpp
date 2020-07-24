/*
  BuffSerial.cpp - serial with transmit buffer library for Wiring
  Created by Kochetkov Aleksey, 28.11.2009
  Edit by Storozhev Denis 21/07/2020
  Version 0.1.2
*/
#include <stdio.h>
#include <avr/interrupt.h>

#include "BuffSerial.h"



/************************************************************************/
/* Serial init                                                          */
/************************************************************************/
void BuffSerial::begin(long speed){
	UCSR1B = (_BV(RXCIE1) | _BV(RXEN1) | _BV(TXCIE1) | _BV(TXEN1));  // enable rx, tx interrputs
	UBRR1H = ((F_CPU / 16 + speed / 2) / speed - 1) >> 8;            // usart speed
	UBRR1L = ((F_CPU / 16 + speed / 2) / speed - 1);

	rxBegin = rxEnd = 0;
	txBegin = txEnd = txOverflow = 0;
	txFull  = 0;
}



/************************************************************************/
/* USART Rx Complete interrupt handler                                  */
/************************************************************************/
SIGNAL(USART1_RX_vect)
{
	bSerial.rxBuffer[bSerial.rxEnd] = UDR1;
//PORTB&=~(1<<5);
	if (bSerial.rxEnd < RX_BUFF_SIZE) bSerial.rxEnd++;
}



/************************************************************************/
/* USART Tx Complete interrupt handler                                  */
/************************************************************************/
SIGNAL(USART1_TX_vect)
{
	if (bSerial.txEnd != bSerial.txBegin || bSerial.txFull != 0){
		UDR1 = bSerial.txBuffer[bSerial.txBegin];  // Send buffer
		bSerial.txFull = 0;
		bSerial.txBegin++;
		if (bSerial.txBegin == TX_BUFF_SIZE) bSerial.txBegin = 0;
	}
//PORTB&=~(1<<5);
}



/************************************************************************/
/* Send byte to serial (UART) or buffer if busy                         */
/************************************************************************/ 
void BuffSerial::sendByte(uint8_t data){
	if (txFull){
		txOverflow++;
	}else{
		//uint8_t oldSREG = SREG;
		cli();	//disable interrupt (global)
		if (txEnd != txBegin || (UCSR1A & _BV(UDRE1)) == 0){	//Check if buffer not empty or if USART Data Register Empty
			txBuffer[txEnd] = data;
			txEnd++;
			if (txEnd == TX_BUFF_SIZE) txEnd = 0;
			if (txEnd == txBegin) txFull = 1;          // buffer overflow
		}else{
			UDR1 = data;	//Start transmit first byte from buffer
		}
		sei();	//enable global interrupt
		//SREG = oldSREG;//тут происходил сброс
	}
}



/************************************************************************/
/* Print string                                                         */
/************************************************************************/ 
void BuffSerial::print(const char *pBuf){
	while (*pBuf)	{
		sendByte(*pBuf++);
	}
}

void BuffSerial::print(const char pBuf){
	sendByte(pBuf);
}

/************************************************************************/
/* Print string from flash                                              */
/************************************************************************/
void BuffSerial::print_p(const char *pBuf){
	char c;
	while ((c = pgm_read_byte_near( pBuf++ )))	{
		sendByte(c);
	}
}

/************************************************************************/
/* Print string line                                                    */
/************************************************************************/
void BuffSerial::println(const char *pBuf){
	print(pBuf);
	println();
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
void BuffSerial::println(const char pBuf){
	print(pBuf);
	println();
}

/************************************************************************/
/* Print new line                                                       */
/************************************************************************/
void BuffSerial::println(void){
	print("\r\n");
}

/************************************************************************/
/* Print string line from flash                                         */
/************************************************************************/
void BuffSerial::println_p(const char *pBuf){
	print_p(pBuf);
	println();
}

/************************************************************************/
/* Print Hex char (4 bit)                                               */
/************************************************************************/
void BuffSerial::printHex4(uint8_t data){
	uint8_t c = data & 0x0f;
	c += c < 10 ? '0' : 'A' - 10 ;
	sendByte(c);
}

/************************************************************************/
/* Print Hex byte (8 bit, 2 char)                                       */
/************************************************************************/
void BuffSerial::printHex8(uint8_t data){
    printHex4(data >> 4);
    printHex4(data);
} 

/************************************************************************/
/* Print decimal byte                                                   */
/************************************************************************/
void BuffSerial::printDec(uint8_t data){
	uint8_t buf[3]; 
	uint8_t i = 0;
	if (data == 0){
		sendByte('0');
		return;
	} 

	while (data > 0){
		buf[i++] = data % 10;
		data /= 10;
	}
	for (; i > 0; i--)
		sendByte((buf[i - 1] < 10 ? '0' + buf[i - 1] : 'A' + buf[i - 1] - 10));
}

/************************************************************************/
/* Check rx buffer not empty                                            */
/************************************************************************/
bool BuffSerial::rxEnabled(void){  
	return rxEnd;
}

/************************************************************************/
/* Read byte from serial buffer                                         */
/************************************************************************/
uint8_t BuffSerial::rxRead(void){
	cbi(UCSR1B, RXCIE1);                         // disable RX complete interrupt
	uint8_t readkey = rxBuffer[rxBegin];         // read begin of received Buffer
	rxBegin++;
	if (rxBegin == rxEnd) rxBegin = rxEnd = 0;   // if Buffer is empty reset Buffer
	sbi(UCSR1B, RXCIE1);                         // enable RX complete interrupt
	return readkey;
}

BuffSerial bSerial;
