/*
 * usbTerminal.h
 *
 * Created: 27.07.2020 17:02:29
 *  Author: Storozhev Denis
 */ 


#ifndef USBTERMINAL_H_
#define USBTERMINAL_H_

#if defined(__cplusplus)
extern "C" {
#endif



int16_t readUsbTerminalByteEcho();
int16_t readUsbTerminalByte(void);
void sendUsbTermianlByte(int16_t data);
void usbTerminalTask();

void usbTerminalPrintln();
void usbTerminalPrint(const char *pBuf);
void usbTerminalPrintlnStr(const char *pBuf);


#if defined(__cplusplus)
}
#endif


#endif /* USBTERMINAL_H_ */