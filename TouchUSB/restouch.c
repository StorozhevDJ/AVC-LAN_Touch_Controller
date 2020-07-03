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

//индикатор касания
#define LEDTOUCH		PORTB
#define LEDTOUCHDDR		DDRB
#define LEDTOUCH_TOUCH	(1<<0)

//подключение сенсорного экрана
#define TOUCHDDR		DDRF
#define TOUCHPORT		PORTF
#define TOUCHPIN		PINF
#define TOUCH_X1		0b00010000
#define TOUCH_X2		0b01000000
#define TOUCH_Y1		0b00100000
#define TOUCH_Y2		0b10000000

#define ADMUX_X			1<<REFS0 | 1<<ADLAR | 4<<MUX0 //канал АЦП к которому подключена X панель
#define ADMUX_Y			1<<REFS0 | 1<<ADLAR | 5<<MUX0 //канал АЦП к которому подключена Y панель

#define TOUCH_ALLMASK	(TOUCH_X1|TOUCH_X2|TOUCH_Y1|TOUCH_Y2)
#define TOUCH_OFFMASK	(0xFF^(TOUCH_X1|TOUCH_X2|TOUCH_Y1|TOUCH_Y2))

//режимы экрана
#define MODE_OFF 1		//отключен
#define MODE_TOUCH 2	//определение касания
#define MODE_XMEAS 3	//измерение x координаты
#define MODE_YMEAS 4	//измерение y координаты

//последние полученные с АЦП данные
uint8_t adc_x; 
uint8_t adc_y;
//флаг наличия прикосновения
bool touched;

uint8_t cmode; //текущий режим экрана

SCalibrateData cdata; //калибровочные данные


//полное отключение панели (все порты на вход, подтяжки отключены)
static inline void touch_off() {
	TOUCHPORT&=TOUCH_OFFMASK;
	TOUCHDDR&=TOUCH_OFFMASK;
}

//запуск измерения в "бесшумном" режиме
static inline void adc_start(void) {
	set_sleep_mode (SLEEP_MODE_ADC);
	sleep_enable();
	// Enter Sleep Mode To Trigger ADC Measurement
	// CPU Will Wake Up From ADC Interrupt
	sleep_cpu();
	sleep_disable();
}

//переключение режима сенсорной панели
void rtSwitchMode(uint8_t newmode){
	if (cmode!=newmode) {
		switch(newmode){
			case MODE_OFF:
				touch_off();
				break;
			case MODE_TOUCH:
				//для определения наличия прикосновения
				TOUCHPORT=(TOUCHPORT&TOUCH_OFFMASK)|TOUCH_Y1;
				TOUCHDDR=(TOUCHDDR&TOUCH_OFFMASK)|TOUCH_X1|TOUCH_X2;
				break;
			case MODE_XMEAS:
			    //для измерения x координаты
				ADMUX = ADMUX_Y;
				TOUCHPORT=(TOUCHPORT&TOUCH_OFFMASK)|TOUCH_X2;
				TOUCHDDR=(TOUCHDDR&TOUCH_OFFMASK)|TOUCH_X1|TOUCH_X2;
				break;
			case MODE_YMEAS:
			    //для измерения y координаты
				ADMUX = ADMUX_X;
				TOUCHPORT=(TOUCHPORT&TOUCH_OFFMASK)|TOUCH_Y1;
				TOUCHDDR=(TOUCHDDR&TOUCH_OFFMASK)|TOUCH_Y1|TOUCH_Y2;
				break;
			default:
				newmode=cmode;
		}
		_delay_ms(1); //ожидание заверешения переходных процессов
		cmode=newmode;
	}
}

//чтение колибровачных данных с eeprom
void rtReadFromEeprom(){
	uint8_t s;
	s = eeprom_read_byte(0);
	if (s==0x55) {
		//в eeprom имеется конфигурация, будем читать
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

//запись колибровочных данных в eeprom
void rtWriteToEeprom(){
	eeprom_write_byte(0,0x55);
	eeprom_write_block(&cdata, 1, sizeof(cdata));
}

//инициализация
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

//измерение
uint8_t rtGetMeasurement() {
	int8_t i,j;
	uint8_t c;
	uint8_t meass[MEAS_COUNT]; //измерения
	uint8_t confs[MEAS_COUNT]; //сколько раз данное измерение повторяется
	uint8_t candidate; //значение - кандидат в результат
	uint8_t confidence = 0; //кол-во повторений кандидата
	
	for(i=0; i<MEAS_COUNT; i++) {
		//очередное измерение
		adc_start();
		meass[i]=ADCH;		
		//поиск повторений только среди ранее измеренных значений
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
//работа с сенсорным экраном
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
	
	//переключение в режим проверки наличия касание
	rtSwitchMode(MODE_TOUCH);
	if (!(TOUCHPIN&TOUCH_Y1)) {
		//есть касание к экрану
		#ifdef LEDTOUCH
		LEDON(LEDTOUCH,LEDTOUCH_TOUCH);
		#endif
		
		//включение АЦП
		ADCSRA = 1<<ADEN | 3<<ADPS0; 
		
		//замер x координаты
		rtSwitchMode(MODE_XMEAS);
		newx = rtGetMeasurement();
		
		//замер y координаты
		rtSwitchMode(MODE_YMEAS);
		newy = rtGetMeasurement();
				
		//выключение АЦП
		ADCSRA = 0;
		
		//снова проверяем касание
		rtSwitchMode(MODE_TOUCH);
		if (!(TOUCHPIN&TOUCH_Y1)) {
			//если касание по прежнему присутствует, результаты измерения признаем действительными
			adc_x=newx;
			adc_y=newy;
			touched = true;
		} else {		
			//результаты не действительны, не сохраняем
			touched = false;
		}		
	} else {
		//нет косания к экрану
		#ifdef LEDTOUCH
		LEDOFF(LEDTOUCH,LEDTOUCH_TOUCH);
		#endif
		
		touched = false;
	}
	return touched;
}


// ADC Interrupt Is Used To Wake Up CPU From Sleep Mode
EMPTY_INTERRUPT (ADC_vect);