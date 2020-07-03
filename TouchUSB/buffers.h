/*
 * buffers.h
 *
 * Created: 13.03.2013 15:14:33
 *  Author: zyxel
 */ 


#ifndef BUFFERS_H_
#define BUFFERS_H_

#include <avr/io.h>
#include <stdbool.h>

//���������, ����������� �����
struct SBuffer {
	uint8_t length; //������ ������
	uint8_t first;  //������ ������� � ������
	uint8_t last;   //������ ��������� ������� � ������
	void* data;     //��������� �� ������ ������
};


void bufInit(struct SBuffer* buf, void* data, uint8_t datalength);
bool bufPutChar(struct SBuffer* buf, char data);
bool bufGetChar(struct SBuffer* buf, char* data);
bool bufPeekChar(struct SBuffer* buf, char* data);



#endif /* BUFFERS_H_ */