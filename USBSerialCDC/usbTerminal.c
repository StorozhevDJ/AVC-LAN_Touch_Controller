/*
 * usbTerminal.c
 *
 * Created: 27.07.2020 16:59:25
 *  Author: Storozhev Denis
 */ 


#include "Descriptors.h"
#include <LUFA/Drivers/USB/USB.h>



USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
{
	.Config =
	{
		.ControlInterfaceNumber         = 0,
		.DataINEndpoint                 =
		{
			.Address                = CDC_TX_EPADDR,
			.Size                   = CDC_TXRX_EPSIZE,
			.Banks                  = 1,
		},
		.DataOUTEndpoint                =
		{
			.Address                = CDC_RX_EPADDR,
			.Size                   = CDC_TXRX_EPSIZE,
			.Banks                  = 1,
		},
		.NotificationEndpoint           =
		{
			.Address                = CDC_NOTIFICATION_EPADDR,
			.Size                   = CDC_NOTIFICATION_EPSIZE,
			.Banks                  = 1,
		},
	},
};



/************************************************************************/
/* Read byte from USB and resend echo                                   */
/************************************************************************/
int16_t readUsbTerminalByteEcho() {
	//получение байтов по виртуалному последовательному порту и передача их диспетчеру комманд
	int16_t cdcdata=CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
	if (cdcdata>=0) {
		PORTB^=1;
		//cdcdata=cmdByte(cdcdata);
		//если диспетчер вернул положительное число, отправляем его в виртуальный последовательный порт
		if (cdcdata>=0) CDC_Device_SendByte(&VirtualSerial_CDC_Interface, cdcdata);
	}
	return cdcdata;
}



/************************************************************************/
/* Read one byte from USB                                               */
/* return: < 0 if bufer empty, else >= 0                                */
/************************************************************************/
int16_t readUsbTerminalByte(void) {
	CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
}



/************************************************************************/
/* Send one byte to USB device                                          */
/************************************************************************/
void sendUsbTermianlByte(int16_t data) {
	if (data>=0) CDC_Device_SendByte(&VirtualSerial_CDC_Interface, data);
}



/************************************************************************/
/* Print new line                                                       */
/************************************************************************/
void usbTerminalPrintln() {
	sendUsbTermianlByte('\r');
	sendUsbTermianlByte('\n');
}



/************************************************************************/
/* Print string from buffer                                             */
/************************************************************************/
void usbTerminalPrint(const char *pBuf) {
	char c;
	while ((c = pgm_read_byte_near( pBuf++ )))	{
		sendUsbTermianlByte(c);
	}
}



/************************************************************************/
/* Print String from buffer and add new line                            */
/************************************************************************/
void usbTerminalPrintlnStr(const char *pBuf) {
	usbTerminalPrint(pBuf);
	usbTerminalPrintln();
}



void usbTerminalTask() {
	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
}