/*
 * TouchController.c
 *
 * Created: 25.03.2013 20:45:54
 *  Author: zyxel
 */ 


#include "TouchController.h"
#include "restouch.h"
#include "buffers.h"

#include "../USBSerialCDC/usbTerminal.h"

#include "../AVC-LAN/AVCLanDrv/config.h"
#include "../AVC-LAN/Navi_DVD_AVin.h"


#define CDCBUFLENGTH 16
#define TCNT3SET 256-128  //10 мсек


extern USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface;


bool usemouse;

static uint8_t PrevDigitizerHIDReportBuffer[sizeof(USB_DigitizerReport_Data_t)];

USB_ClassInfo_HID_Device_t Digitizer_HID_Interface =
{
	.Config =
	{
		.InterfaceNumber                = 2,
		.ReportINEndpoint               =
		{
			.Address                = MOUSE_EPADDR,
			.Size                   = MOUSE_EPSIZE,
			.Banks                  = 1,
		},
		.PrevReportINBuffer             = PrevDigitizerHIDReportBuffer,
		.PrevReportINBufferSize         = sizeof(PrevDigitizerHIDReportBuffer),
	},
};







int main(void)
{
	SetupHardware();
	AVCLan_Setup();
	
	usemouse = true;
	
    while(1)
    {
		AVCLan_Task();
		terminalTask();
		
		//задачи сенсорного экрана	
		//rtTouchTask();

		//sendUsbTermianlByte(readUsbTerminalByte());
		

		//задачи USB
		usbTerminalTask();
		HID_Device_USBTask(&Digitizer_HID_Interface);
		USB_USBTask();		
    }

}



void SetupHardware()
{
	//LED port Init
	DDRB |= (1<<0);	
	DDRD |= (1<<5);
	
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
	
	rtInit(); //подготовка работы с панелью
	
	cmdInit(); //подготовка диспетчера комманд
	
	
	/* Timer 0 */
	TCCR3A = 0;
	TCCR3B = 5; // CK/1024
	TCNT3 = TCNT3SET;
	TIMSK3 = (1<<TOIE3); //overflow interrupt enable
	
	USB_Init(); //инициализация USB
		
	sei();
}



/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;
	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Digitizer_HID_Interface);
	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
	USB_Device_EnableSOFEvents();
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
	HID_Device_ProcessControlRequest(&Digitizer_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Digitizer_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean true to force the sending of the report, false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
	
	USB_DigitizerReport_Data_t* DigitizerReport = (USB_DigitizerReport_Data_t*)ReportData;

	if (usemouse) {
		//вычисляем координаты в диапазоне 0..32767
		//DigitizerReport->X=(cdata.kx*(adc_x-cdata.dx))>>16;
		//DigitizerReport->Y=(cdata.ky*(adc_y-cdata.dy))>>16;
		DigitizerReport->X=(uint16_t)(adc_x)<<7;
		DigitizerReport->Y=(uint16_t)(adc_y)<<7;
		//косание
		if (touched) {
			//if (WF_PIN&WF_MASK)
			{
				DigitizerReport->Button=0b010001; //так работает в Андроид
			}
			//else
			{
				//DigitizerReport->Button=0b110001; //так работает в Винде
			}
		}		
	}
			
	*ReportID=1;
	*ReportSize=sizeof(USB_DigitizerReport_Data_t);
	
	return false;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{

}

uint16_t test2=0;
ISR(TIMER3_OVF_vect) {
	TCNT3=TCNT3SET;
	cmdTic();
	if (++test2==100){test2=0; PORTB^=0x01;}
}
