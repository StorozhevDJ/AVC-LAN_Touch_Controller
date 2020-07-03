/*
 * restouch.c
 *
 * Created: 25.03.2013 21:25:42
 *  Author: zyxel
 */ 

#include "restouch.h"
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/sleep.h> 

#define MEAS_COUNT 10

#define LEDON(port,mask) port|=mask
#define LEDOFF(port,mask) port&=(mask^0xFF)

//��������� �������
#define LEDTOUCH		PORTB
#define LEDTOUCHDDR		DDRB
#define LEDTOUCH_TOUCH	(1<<0)

//����������� ���������� ������
#define TOUCHDDR		DDRF
#define TOUCHPORT		PORTF
#define TOUCHPIN		PINF
#define TOUCH_X1		0b00010000
#define TOUCH_X2		0b01000000
#define TOUCH_Y1		0b00100000
#define TOUCH_Y2		0b10000000

#define ADMUX_X			1<<REFS0 | 1<<ADLAR | 4<<MUX0 //����� ��� � �������� ���������� X ������
#define ADMUX_Y			1<<REFS0 | 1<<ADLAR | 5<<MUX0 //����� ��� � �������� ���������� Y ������

#define TOUCH_ALLMASK	(TOUCH_X1|TOUCH_X2|TOUCH_Y1|TOUCH_Y2)
#define TOUCH_OFFMASK	(0xFF^(TOUCH_X1|TOUCH_X2|TOUCH_Y1|TOUCH_Y2))

//������ ������
#define MODE_OFF 1		//��������
#define MODE_TOUCH 2	//����������� �������
#define MODE_XMEAS 3	//��������� x ����������
#define MODE_YMEAS 4	//��������� y ����������

//��������� ���������� � ��� ������
uint8_t adc_x; 
uint8_t adc_y;
//���� ������� �������������
bool touched;

uint8_t cmode; //������� ����� ������

SCalibrateData cdata; //������������� ������


//������ ���������� ������ (��� ����� �� ����, �������� ���������)
static inline void touch_off() {
	TOUCHPORT&=TOUCH_OFFMASK;
	TOUCHDDR&=TOUCH_OFFMASK;
}

//������ ��������� � "���������" ������
static inline void adc_start(void) {
	set_sleep_mode (SLEEP_MODE_ADC);
	sleep_enable();
	// Enter Sleep Mode To Trigger ADC Measurement
	// CPU Will Wake Up From ADC Interrupt
	sleep_cpu();
	sleep_disable();
}

//������������ ������ ��������� ������
void rtSwitchMode(uint8_t newmode){
	if (cmode!=newmode) {
		switch(newmode){
			case MODE_OFF:
				touch_off();
				break;
			case MODE_TOUCH:
				//��� ����������� ������� �������������
				TOUCHPORT=(TOUCHPORT&TOUCH_OFFMASK)|TOUCH_Y1;
				TOUCHDDR=(TOUCHDDR&TOUCH_OFFMASK)|TOUCH_X1|TOUCH_X2;
				break;
			case MODE_XMEAS:
			    //��� ��������� x ����������
				ADMUX = ADMUX_Y;
				TOUCHPORT=(TOUCHPORT&TOUCH_OFFMASK)|TOUCH_X2;
				TOUCHDDR=(TOUCHDDR&TOUCH_OFFMASK)|TOUCH_X1|TOUCH_X2;
				break;
			case MODE_YMEAS:
			    //��� ��������� y ����������
				ADMUX = ADMUX_X;
				TOUCHPORT=(TOUCHPORT&TOUCH_OFFMASK)|TOUCH_Y1;
				TOUCHDDR=(TOUCHDDR&TOUCH_OFFMASK)|TOUCH_Y1|TOUCH_Y2;
				break;
			default:
				newmode=cmode;
		}
		_delay_ms(1); //�������� ����������� ���������� ���������
		cmode=newmode;
	}
}

//������ ������������� ������ � eeprom
void rtReadFromEeprom(){
	uint8_t s;
	s = eeprom_read_byte(0);
	if (s==0x55) {
		//� eeprom ������� ������������, ����� ������
		eeprom_read_block(&cdata, 1, sizeof(cdata));
	} else {
		cdata.kx=0x10000;
		cdata.ky=0x10000;
		cdata.dx=0;
		cdata.dy=0;
		cdata.xsize=0xFF;
		cdata.ysize=0xFF;
	}
}

//������ ������������� ������ � eeprom
void rtWriteToEeprom(){
	eeprom_write_byte(0,0x55);
	eeprom_write_block(&cdata, 1, sizeof(cdata));
}

//�������������
void rtInit(){
	#ifdef LEDTOUCH
	LEDTOUCHDDR|=LEDTOUCH_TOUCH;
	LEDOFF(LEDTOUCH,LEDTOUCH_TOUCH);
	#endif	
	
	touch_off();
	touched = false;
	cmode=MODE_OFF;
	rtReadFromEeprom();
}

//���������
uint8_t rtGetMeasurement() {
	int8_t i,j;
	uint8_t c;
	uint8_t meass[MEAS_COUNT]; //���������
	uint8_t confs[MEAS_COUNT]; //������� ��� ������ ��������� �����������
	uint8_t candidate; //�������� - �������� � ���������
	uint8_t confidence = 0; //���-�� ���������� ���������
	
	for(i=0; i<MEAS_COUNT; i++) {
		//��������� ���������
		adc_start();
		meass[i]=ADCH;		
		//����� ���������� ������ ����� ����� ���������� ��������
		c=1;
		for(j=i-1; j>=0; j--) 
			if (meass[j]==meass[i]) {
				c = confs[j]+1;
				break;
			}
		confs[i] = c;
		
		if (c>confidence) {
			candidate=meass[i];
			confidence=c;
		}
	}
	
	return candidate;
}

uint32_t testc=0, testt=0;
//������ � ��������� �������
bool rtTouchTask()
{
if (++testc>1000)
	{
	testc=0;
	if ((++testt%100)==0)
		{
		adc_x++;
		adc_y++;
		touched= true;
		LEDOFF(LEDTOUCH,LEDTOUCH_TOUCH);
		}
	else
		{
		touched = false;
		LEDON(LEDTOUCH,LEDTOUCH_TOUCH);
		}
	return touched;
	}
else return touched;

uint8_t newx;
	uint8_t newy;
	
	//������������ � ����� �������� ������� �������
	rtSwitchMode(MODE_TOUCH);
	if (!(TOUCHPIN&TOUCH_Y1)) {
		//���� ������� � ������
		#ifdef LEDTOUCH
		LEDON(LEDTOUCH,LEDTOUCH_TOUCH);
		#endif
		
		//��������� ���
		ADCSRA = 1<<ADEN | 3<<ADPS0; 
		
		//����� x ����������
		rtSwitchMode(MODE_XMEAS);
		newx = rtGetMeasurement();
		
		//����� y ����������
		rtSwitchMode(MODE_YMEAS);
		newy = rtGetMeasurement();
				
		//���������� ���
		ADCSRA = 0;
		
		//����� ��������� �������
		rtSwitchMode(MODE_TOUCH);
		if (!(TOUCHPIN&TOUCH_Y1)) {
			//���� ������� �� �������� ������������, ���������� ��������� �������� ���������������
			adc_x=newx;
			adc_y=newy;
			touched = true;
		} else {		
			//���������� �� �������������, �� ���������
			touched = false;
		}		
	} else {
		//��� ������� � ������
		#ifdef LEDTOUCH
		LEDOFF(LEDTOUCH,LEDTOUCH_TOUCH);
		#endif
		
		touched = false;
	}
	return touched;
}


// ADC Interrupt Is Used To Wake Up CPU From Sleep Mode
EMPTY_INTERRUPT (ADC_vect);