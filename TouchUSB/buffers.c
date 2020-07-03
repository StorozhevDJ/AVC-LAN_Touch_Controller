/*
 * buffers.c
 *
 * Created: 13.03.2013 15:14:50
 *  Author: zyxel
 */ 

#include "buffers.h"

void bufInit(struct SBuffer* buf, void* data, uint8_t datalength){
	memset(buf,0,sizeof(struct SBuffer));
	buf->length=datalength;
	buf->data=data;
}

bool bufPutChar(struct SBuffer* buf, char data){
	char* bufdata = (char*) buf->data;
	if ((buf->last+1)==buf->first || ((buf->last+1)==buf->length && buf->first==0)) {
		return false;
	}
	bufdata[buf->last++]=data;
	if (buf->last >= buf->length) buf->last=0;
	return true;
}

bool bufGetChar(struct SBuffer* buf, char* data){
	char* bufdata = (char*) buf->data;
	if (buf->first!=buf->last) {
		*data=bufdata[buf->first++];
		if (buf->first >= buf->length) buf->first=0;
		return true;
	};
	return false;
}

bool bufPeekChar(struct SBuffer* buf, char* data){
	char* bufdata = (char*) buf->data;
	if (buf->first!=buf->last) {
		*data=bufdata[buf->first];
		return true;
	};
	return false;
}
