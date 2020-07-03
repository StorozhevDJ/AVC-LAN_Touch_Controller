/*
 * commands.c
 *
 * Created: 26.03.2013 18:04:23
 *  Author: zyxel
 */ 

#include "commands.h"
#include "restouch.h"

#define VERSIONBYTE 1
#define BETWEENBYTEMAXPAUSE 20

#define CMDGETVER 1
#define CMDGETSTATE 10
#define CMDMOUSEOFF 11
#define CMDMOUSEON 12
#define CMDSETCDATA 13

uint8_t ChkSum; //для подсчета контрольной суммы получаемого пакета
uint8_t BtCnt; //сколько байт пакета уже получено
uint8_t RsTim; //когда значение данной переменной достигнет 0, пакет отбрасывается
uint8_t cmd;  //текущая команда

extern bool usemouse;

void pktReset(){
	RsTim=0;
	BtCnt=0;
	cmd=0;
}

void cmdInit(){
	pktReset();
}

int16_t cmdByte(uint8_t data) {
	RsTim = BETWEENBYTEMAXPAUSE;
	ChkSum^=data; //ведем подсчет контрольной суммы
	BtCnt++;	

	if (BtCnt<5) {
		switch(BtCnt) {
			case 1:
			ChkSum = data;
			case 2:
			if (data!=0xFF) pktReset();
			break;
			case 3:
			if (data!=VERSIONBYTE) pktReset();
			break;
			case 4:
			cmd = data;
		}
	} else {
		//заголовок принят, принимаем команду
		switch(cmd){
			case CMDGETVER:		//--------------------------------------------------GETVER
				//запрос версии
				pktReset();
				if (ChkSum==0) return VERSIONBYTE; else return 0xFF;
				break;
				
			case CMDGETSTATE:	//--------------------------------------------------GETSTATE
				switch(BtCnt) {
					case 5:
						return adc_x;
					case 6:
						return adc_y;
					case 7:
						if (touched) return 1; else return 0;
					default:
						pktReset();
						return 0;
				}
				break;

			case CMDMOUSEOFF:	//--------------------------------------------------MOUSEOFF
				//отключить мышку
				pktReset();
				if (ChkSum==0) {
					usemouse = false;
					return 0;
				} else return 0xFF;
				break;

			case CMDMOUSEON:	//--------------------------------------------------MOUSEON
				//включить мышку
				pktReset();
				if (ChkSum==0) {
					usemouse = true;
					return 0;
				} else return 0xFF;
				break;
			
			case CMDSETCDATA:  //--------------------------------------------------SETCDATA
				if (BtCnt==(5+sizeof(cdata))) {
					//конфигурация принята
					pktReset();
					if (ChkSum==0) {
						//запишем в eeprom
						rtWriteToEeprom(); 
						return 0;
					} else {
						//ошибка контрольной суммы, восстановим старую настройку
						rtReadFromEeprom();
						return 0xFF;
					}					
				}
				uint8_t *cdp = (uint8_t*)&cdata;
				cdp[BtCnt-5]=data;
				break;
		}
	}			
	return -1;
}



void cmdTic()
{
if (RsTim)
	{
	RsTim--;
	if (!RsTim) pktReset();
	}
}