/*
 Автоматическое включение камеры по сигналу с фонаря ЗХ
 Управление AzFM
 */

#define AVCLAN_VERSION "24032016_1030"

        
#include <avr/eeprom.h>
#include <util/delay.h>

#include "AVCLanDrv/AVCLanDrv.h"
#include "AVCLanDVDch/AVCLanDVDch.h"
//#include "AVC-LAN/AVCLanNavi/AVCLanNavi.h"
//#include "AVC-LAN/AVCLanCDch/AVCLanCDch.h"
#include "BuffSerial/BuffSerial.h"
#include "AVCLanDrv/config.h"
#include "Navi_DVD_AVin.h"

byte readSeq = 0;
byte s_len   = 0;
byte s_dig   = 0;
byte s_c[2];
byte data_tmp[32];
byte i;

struct
{
	uint8_t e_master1;
	uint8_t e_master2;
	uint8_t readonly;
	uint8_t init;
} ee_data EEMEM;


const AvcOutMessage PROGMEM CmdPlayOk1_0	=  {AVC_MSG_BROADCAST,  0x06, {0x45, 0x12, 0x50, 0x13, 0x04, 0x01}}; // Play begin message 1
const AvcOutMessage PROGMEM CmdStopOk2_0	=  {AVC_MSG_BROADCAST,  0x0B, {0x45, 0x12, 0x51, 0x13, 0x04, 0x00}}; // Stop ok message 2
const AvcOutMessage PROGMEM CmdInit_01	=  {AVC_MSG_BROADCAST,  0x05, {0x01, 0x11, 0x13, 0x24, 0x45}};
const AvcOutMessage PROGMEM CmdAM_01	=  {AVC_MSG_BROADCAST,  0x05, {0x00, 0x25, 0x32, 0x80, 0x06}}; // Нужно подкорректировать под кнопку и возможно директ?



const char PROGMEM str_head[]			={"head="};
const char PROGMEM str_eeprom[]		={", eeprom="};
const char PROGMEM str_device[]		={"device="};
const char PROGMEM str_readonly[]		={"readonly="};
const char PROGMEM str_tx_overflow[]	={"TX Overflow: "};
//const char __flash str_AVCLan_Harrier[]	={"AVCLan Harrier 1998 v"};
const char PROGMEM str_AVCLan_Prius[]	={"AVCLan Prius 20 v"};
const char PROGMEM str_avclan_version[]	={AVCLAN_VERSION};
const char PROGMEM str_driver_type[]	={" Driver type: "};
const char PROGMEM str_avclan_driver_type[]={AVCLANDRIVER_TYPE};
const char PROGMEM str_buffserial[]	={" BuffServial v"};
const char PROGMEM str_buffserial_ver[]	={BUFFSERIAL_VERSION};
const char PROGMEM str_avclan_drv[]	={" AVCLanDrv v"};
const char PROGMEM str_avclan_drv_ver[]	={AVCLANDRV_VERSION};
const char PROGMEM str_avclan_dev_name[]={AVCLANDEVICE_NAME};
const char PROGMEM str_ver[]			={". v"};
const char PROGMEM str_avclan_dev_ver[]	={AVCLANDEVICE_VERSION};
const char PROGMEM str_help_ln1[]		={"P - print config\r\nV - version\r\nS - start command\r\nW - end direct command\r\nQ - end broadcast command\r\n? - help info\r\n"};
const char PROGMEM str_help_ln2[]		={"H - end of set HU Address, S0000H - auto\r\nT - send test message\r\n"};
const char PROGMEM str_help_ln3[]		={"h - Output mode. Set Hi level\r\nl - Output mode. Set Low level\r\ni - Out disable, only Input mode\r\nM - readonly mode on/off\r\n"};
const char PROGMEM str_help_ln4[]		={"D - Manual DVD on\r\nO - Manual DVD off\r\nI - Manual init of DVD\r\no - D11 pin On\r\nv - D11 pin off\r\nC - Set coordinales \"C=123,456,1\""};

const char PROGMEM str_x[]			={"\r\nx="};
const char PROGMEM str_y[]			={"\r\ny="};
const char PROGMEM str_touch[]		={"\r\nTouch="};



void AVCLan_Setup()
{
// setup led
sbi(LED_DDR,  LED_OUT);
LED_OFF;//sbi(LED_PORT, LED_OUT);
        
/*pinMode(12, INPUT); // конфигурим D12 порт на ввод для лампы заднего хода
digitalWrite (12, HIGH); //подтягиваем порт к земле 
pinMode(11, OUTPUT); // конфигурим D11 порт на вывод - управление реле
digitalWrite (11, LOW); // выключаем реле*///my comment
        
bSerial.begin(115200); // конфигурим последовательный порт на 115200
avclan.begin();
avclanDevice.begin();
EERPOM_read_config();
bSerial.println();
bSerial.println_p(str_AVCLan_Prius);
}



extern uint8_t adc_x, adc_y;
extern bool touched;

void AVCLan_Task()
{

if (INPUT_IS_SET)
	{
	LED_ON;
	byte res = avclan.readMessage();

	// проверка на задний ход
	/*if (digitalRead(12)&& !digitalRead(11))
		{
		bSerial.println("L:OFF Cam:OFF");
		};*///my comment
	
	/*if (!digitalRead(12) && !digitalRead(11))
		{
		bSerial.println("L:ON Cam:OFF");
		avclan.sendMessage(&CmdInit_01);	//включаем монитор для камеры заднего вида
		avclan.sendMessage(&CmdPlayOk1_0);
		digitalWrite(11, HIGH);
		};
	
	if (!digitalRead(12) && digitalRead(11))//для контроля
		{
		bSerial.println("L:ON Cam:ON");
		};
	
	if (digitalRead(12) && digitalRead(11))
		{
		bSerial.println("L:OFF Cam:ON");
		avclan.sendMessage(&CmdStopOk2_0);//выключаем монитор
		digitalWrite(11, LOW);
		};
	
	if (digitalRead(12) && !digitalRead(11))//для контроля
		{
		bSerial.println("L:ON Cam:ON");
		};*///my comment
	
	LED_OFF;
	if (!res)
		{
		if (!avclan.readonly) avclanDevice.getActionID();
		if (avclan.actionID != ACT_NONE)
			{
			avclanDevice.processAction((AvcActionID)avclan.actionID);
			}
		}
	}

if (avclan.event != EV_NONE)
	{
	avclanDevice.processEvent((AvcEventID)avclan.event);
	avclan.event = EV_NONE;
	}



if (bSerial.rxEnabled())
	{
	uint8_t readkey = bSerial.rxRead();

	switch (readkey)
		{
		case 'P':                                  // print config
			bSerial.println();
			bSerial.print_p(str_head);
			bSerial.printHex8(avclan.headAddress >> 8);
			bSerial.printHex8(avclan.headAddress);
			bSerial.print_p(str_eeprom);
			bSerial.printHex8(eeprom_read_byte(&ee_data.e_master1));
			bSerial.printHex8(eeprom_read_byte(&ee_data.e_master2));
			bSerial.println();
			bSerial.print_p(str_device);
			bSerial.printHex8(avclan.deviceAddress >> 8);
			bSerial.printHex8(avclan.deviceAddress);
			bSerial.println();
			bSerial.print_p(str_readonly);
			bSerial.printHex8(avclan.readonly);
			bSerial.println();
			bSerial.print_p(str_tx_overflow);
			bSerial.printHex8(bSerial.txOverflow >> 8);
			bSerial.printHex8(bSerial.txOverflow);
			bSerial.println();
		break;
		case 'T':  //send test message
			bSerial.println();
			bSerial.println("Test_message  ");
			sendMessBROADCAST();
			bSerial.println();
		break;
		case 'L':  //send test message
			bSerial.println();
			bSerial.println("Test_message  ");
			sendMessDIRECT();
			bSerial.println();
		break;
		case 'D':  //Manual DVD on
			bSerial.println();
			bSerial.println("DVD On");
			avclan.sendMessage(&CmdPlayOk1_0);
			bSerial.println();
		break;                   
		case 'O':  //Manual DVD off
			bSerial.println();
			bSerial.println("DVD Off");
			avclan.sendMessage(&CmdStopOk2_0);
			bSerial.println();
			break;                
		case 'I':  //Manual init of DVD changer
			bSerial.println();
			bSerial.println("Init");
			avclan.sendMessage(&CmdInit_01);
			bSerial.println();			
		break;                
		case 'A':  //Test AM on/off
			bSerial.println();
			bSerial.println("AM press");
			avclan.sendMessage(&CmdAM_01);
			bSerial.println();			
		break;           
		/*case 'o':  //D11 on
			bSerial.println();
			bSerial.println("Output_D11");
			digitalWrite(11,HIGH);
			bSerial.printHex8(digitalRead(11));
			bSerial.println();
		break;   
		case 'v':  //D11 off
			bSerial.println();
			bSerial.println("Output_D11");
			digitalWrite(11,LOW);
			bSerial.printHex8(digitalRead(11));
			bSerial.println();
		break;   *///my comment
		case 'M':  //readonly mode on/off
			avclan.readonly ^= (1 << 0);
			eeprom_write_byte(&ee_data.readonly, avclan.readonly);
		break;
		case 'h':  // set hi level 
			bSerial.println("H set");
			AVC_OUT_EN;
			OUTPUT_SET_1;
		break;
		case 'l':  // set low level
			bSerial.println("L set");
			AVC_OUT_EN;
			OUTPUT_SET_0;
		break;
		case 'i': // output disable. Input mode on
			bSerial.println("Out dis");
			AVC_OUT_DIS;
		break;
		case 'V':                      // version
			bSerial.print_p  (str_AVCLan_Prius);
			bSerial.println_p(str_avclan_version);
			bSerial.print_p  (str_driver_type);
			bSerial.println_p(str_avclan_driver_type);
			bSerial.print_p  (str_buffserial);
			bSerial.println_p(str_buffserial_ver);
			bSerial.print_p  (str_avclan_drv);
			bSerial.println_p(str_avclan_drv_ver);
			bSerial.print_p  (str_avclan_dev_name);
			bSerial.print_p  (str_ver);
			bSerial.println_p(str_avclan_dev_ver);
		break;		
		case 'S':	                   // start command
			readSeq = 1;       
			s_len=0;
			s_dig=0;
			s_c[0] = s_c[1] = 0;
		break;
		case 'W':                      // end of direct command
			readSeq=0;                     
			avclan.dataSize      = s_len;
			avclan.broadcast     = AVC_MSG_DIRECT;
			avclan.masterAddress = avclan.deviceAddress;
			avclan.slaveAddress  = avclan.headAddress;
			for (i=0; i<s_len; i++) avclan.message[i]=data_tmp[i];
			avclan.sendMessage();
		break;
		case 'Q' :                     // end of broadcast command
			readSeq=0;                     
			avclan.dataSize      = s_len;
			avclan.broadcast     = AVC_MSG_BROADCAST;
			avclan.masterAddress = avclan.deviceAddress;
			avclan.slaveAddress  = 0x01FF;
			for (i=0; i<s_len; i++) avclan.message[i]=data_tmp[i];
			avclan.sendMessage();
			break;
		case 'H' :                     // end of set Head Unid ID 
			readSeq=0;
			avclan.headAddress = (data_tmp[0] << 8) + data_tmp[1];
			eeprom_write_byte(&ee_data.e_master1, data_tmp[0]);
			eeprom_write_byte(&ee_data.e_master2, data_tmp[1]);
		break;
		case 'C' :                     // Manual set Coorninates
			{
			uint8_t X=0;
			for (char i=0; i<3; i++)
				{
				
				while(!bSerial.rxEnabled());
				char c=bSerial.rxRead();
				if ((X>='0')&&(X<='9')) X=(X*10)+(X&0x0F);
				if (c==',') {adc_x=X; break;}
				}
			uint8_t Y=0;
			for (char i=0; i<3; i++)
				{
				while(!bSerial.rxEnabled());
				char c=bSerial.rxRead();
				if ((Y>='0')&&(Y<='9')) Y=(Y*10)+(Y&0x0F);
				if (Y==',') {adc_y=Y; break;}
				}
			while(!bSerial.rxEnabled());
			if (bSerial.rxRead()=='1') touched=true; else touched=false;
			
			bSerial.print_p  (str_x);
			bSerial.printDec (X);
			bSerial.println_p(str_y);
			bSerial.printDec (Y);
			bSerial.print_p  (str_touch);
			bSerial.printDec (touched);
			}
		break;
		case '?':
			bSerial.println_p(str_help_ln1);
			_delay_ms(500);
			bSerial.println_p(str_help_ln2);
			_delay_ms(500);
			bSerial.println_p(str_help_ln3);
			_delay_ms(500);
			bSerial.println_p(str_help_ln4);
		break;
		default :
			if (readSeq==1)
				{
				if (readkey!=' ')
					{
					s_c[s_dig]=readkey;
	 				s_dig++;
	  				if (s_dig==2)
						{
						if (s_c[0]<':') s_c[0] -= 48;
						else s_c[0] -= 55;
						data_tmp[s_len] = 16 * s_c[0];
						if (s_c[1]<':') s_c[1] -= 48;
						else s_c[1] -= 55;
						data_tmp[s_len] += s_c[1];
						s_len++;
						s_dig=0;
						s_c[0]=s_c[1]=0;
						}
					}
				}
		}
	}
}



void sendMessBROADCAST()
{
//Заготовка для тестовой BROADCAST команды
avclan.broadcast = AVC_MSG_BROADCAST;
avclan.masterAddress = 0x0208;
avclan.slaveAddress  = 0x01FF;
avclan.dataSize      = 0x05;
avclan.message[0]    = 0x00;
avclan.message[1]    = 0x01;
avclan.message[2]    = 0x12;
avclan.message[3]    = 0x10;
avclan.message[4]    = 0x43;
byte res = avclan.sendMessage();
}



void sendMessDIRECT()
{	
//Заготовка для тестовой DIRECT команды
avclan.masterAddress = 0x0208;
avclan.slaveAddress  = 0x0190;
avclan.broadcast = AVC_MSG_DIRECT;
avclan.dataSize      = 0x05;
avclan.message[0]    = 0x00;
avclan.message[1]    = 0x01;
avclan.message[2]    = 0x12;
avclan.message[3]    = 0x10;
avclan.message[4]    = 0x43;
byte res = avclan.sendMessage();
}



// Чтение конфигурации из EEPROM
void EERPOM_read_config()
{
if (eeprom_read_byte(&ee_data.init) != 'T')
	{
	eeprom_write_byte(&ee_data.e_master1, 0x01);
	eeprom_write_byte(&ee_data.e_master2, 0x90);
	eeprom_write_byte(&ee_data.readonly, 0);
	eeprom_write_byte(&ee_data.init, 'T');
	}
else
	{
	avclan.headAddress = (eeprom_read_byte(&ee_data.e_master1) << 8) + eeprom_read_byte(&ee_data.e_master2);
	avclan.readonly    = eeprom_read_byte(&ee_data.readonly);
	}
}


