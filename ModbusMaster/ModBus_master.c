//#include "main.h"
#include "mbcrc.h"
#include "ModBus_master.h"
//#include "usart.h"
#include "string.h"
//#include "gpio.h"
//#include "tim.h"
#include <stdio.h>
#include "LogSD.h"

#include "lwip\tcpip.h"
#include "lwip\mem.h"
#include "lwip\pbuf.h"
#include "lwip\ip.h"
#include "lwip\tcp.h"

#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_cdc.h"

#include "stm32_ub_fatfs.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "ff.h"

extern xSemaphoreHandle SemMbus;
extern xSemaphoreHandle SemMem;
extern xSemaphoreHandle SemGSM;
extern xSemaphoreHandle SemPlan;
extern xSemaphoreHandle SemGateway;
extern xSemaphoreHandle xMbusG;


extern USBH_HandleTypeDef hUsbHostHS;

unsigned int    n_master_com, line_master,n_com3=0,n_com33=0, line3;
unsigned char   data_master_IN[com_max];
unsigned char   data_master_OUT[20];
unsigned short expect_add4, expect_add3, expect_dadd;
unsigned char   data_com3_IN[1200];
unsigned char   data_com3_OUT[2000];

extern unsigned char SMSbuf[com_max];

extern char MSREGADR;
extern char GSMTCP_On;

extern struct devices
{
	unsigned char addr, interface,QH,QI,NumQH,NumQI,sizeH,sizeI;			// адресс слэйв для опроса и интерфейс
	unsigned char ipadd[4];
	unsigned short int port;
	unsigned char type3, type4;							// 3 или 4
	char log;														// логировать
	unsigned short time;								// период опроса и логирования
	char regsH[51];
	char regsI[51];
	char floatsH[300];
	char floatsI[300];
	unsigned short int Holding[100];
	unsigned short int Input[100];
	unsigned short int PatternH[20][2];
	unsigned short int PatternI[20][2];
	unsigned int Packets,BadPackets;
	TaskHandle_t THandle;
	char* filename;
	
	FIL* file1;
	
	unsigned char filestatus; // 0 - закрыт, 1 - открыт
	char path[50];
	unsigned int startH, startM, devN, N;
	char Adapter[50];
	int timecounter;
	unsigned short int PL[100][2];
	unsigned short int fl[100];	// Номера float регистров
	char NumF,NumP;									// Число float регистров
	
	unsigned short int PL2[100][2];
	unsigned short int fl2[100];	// Номера float регистров
	char NumF2,NumP2;		
};

extern struct devices *StructQwAdr[20];
extern char NumQw;
extern u8_t GatewayDev,GatewayON;

char* smsnum 		= "+7XXXXXXXXXX";

unsigned char smsMSG[17][51];
unsigned char smsMSGutf8[17][100];


unsigned char SendGW = 0;

unsigned int BadOutPackets = 0,AllOutPakets = 0;

extern char FirstTurnOn;

unsigned char NumPhones = 0;
unsigned char Phones[10][13]={0};

uint16_t comcom = 0;

/*
Ну тут понятно, что ЦЭ ЭР ЦЭ 16
*/
void CRC16_M (char* data, unsigned int line)         
	{ 
uint8_t n;  
uint16_t j, r;

r=0xFFFF;   
for(j=0;j<line;j++)
  { 
  r^=data[j];
  for(n=0; n<8;n++)
      {
       if(r&0x0001)     r=(r>>1)^0xA001;  //0xA001; }
              else      r>>=1; 
      }
  }
data[line++]=r;        // L
data[line]=r>>8;       // H
}

/*
Обработка ответа ModbusRTU
*/
int RTU_M(void)  
{
unsigned int ad, n, j; 
unsigned int dat;
unsigned int i=0;
	uint16_t comcom = 0;
	
	struct devices Dev;

		if(SendGW == 0)
		{
	
			for(i=0;i<NumQw;i++)
				{	
					if(StructQwAdr[i]->addr == data_master_IN[0])
					{
						
						goto ok;
					}
				}
	
	
	
	goto m_eror;

    // if((data_com[0]!=expect_dadd)) goto m_eror;    // ??? ID
 
	
ok:  ad=data_master_IN[2]+5;
				comcom = n_master_com;
     n_master_com=0;
				
     if(ad<6 || comcom < 6) goto m_eror;
     n=data_master_IN[ad-1];
     j=data_master_IN[ad-2];
     CRC16_M(data_master_IN, ad-2);
     if((n!=data_master_IN[ad-1])||(j!=data_master_IN[ad-2]))
		 {
			 StructQwAdr[i]->BadPackets++;
			goto m_eror;   //CRC
		 }

		if(data_master_IN[1]==0x03)
          {
          j=3;  
						//ad=expect_add3;
						ad = StructQwAdr[i]->QH;
          for(n=0; n<data_master_IN[2];n+=2)
                  {
                  dat=data_master_IN[j++]; dat<<=8;       // H
                  dat|=data_master_IN[j++];               // L
                  StructQwAdr[i]->Holding[ad++]=dat;
                  }
									//StructQwAdr[i]->QH = ad;
          }

     if(data_master_IN[1]==0x04) 
          {
          j=3;
						//ad=expect_add4;
						ad = StructQwAdr[i]->QI;
              for(n=0; n<data_master_IN[2];n+=2)
                  {
                  dat=data_master_IN[j++]; dat<<=8;       // H
                  dat|=data_master_IN[j++];               // L
                  StructQwAdr[i]->Input[ad++]=dat;
									}
										//StructQwAdr[i]->QI = ad;
									
          }
					
					memset(data_master_IN,0,com_max);
					//тут семафор, разрещающий след запрос...
					osSemaphoreRelease(SemPlan);
					//xSemaphoreGive(SemPlan);
					n_master_com=0;
					return 1;
				}
			else
			{
				
				
				if(expect_dadd!=data_master_IN[0])
				{
					SendGW = 2;
					osSemaphoreRelease(SemGateway);
					n_master_com=0;
					return 0;
				}
				else
				{
					osSemaphoreRelease(SemGateway);
					n_master_com=0;
					return 1;
				}
				
			}
			
			m_eror:
			return 0;		
			n_master_com=0;
		//	BadOutPackets++;
}

/*
Норм функция запроса по Modbus RTU
*/
void zapros_M(unsigned char dadd, unsigned char rez_com, unsigned short ladd, unsigned short hadd)
{
osSemaphoreWait(SemMbus, portMAX_DELAY);
	
unsigned short num = (hadd-ladd)+1;
expect_dadd = dadd;
  switch (rez_com)
  {
    case 4:    
			data_master_OUT[0]=dadd;
      data_master_OUT[1]=0x04;   
			data_master_OUT[2]=((ladd-1) & 0xFF00)>>8;  
			data_master_OUT[3]=((ladd-1) & 0x00FF);  
			data_master_OUT[4]=(num & 0xFF00)>>8;		
      data_master_OUT[5]=(num & 0x00FF); 
		
		expect_add4 = ladd;
      break;
    case 3:    
			data_master_OUT[0]=dadd;
      data_master_OUT[1]=0x03;   
			data_master_OUT[2]=((ladd-1) & 0xFF00)>>8;  
			data_master_OUT[3]=((ladd-1) & 0x00FF);  
			data_master_OUT[4]=(num & 0xFF00)>>8;		
      data_master_OUT[5]=(num & 0x00FF); 
		
		expect_add3 = ladd;
      break;
    case 6:    
			data_master_OUT[0]=dadd;
      data_master_OUT[1]=0x06;   
			data_master_OUT[2]=((ladd-1) & 0xFF00)>>8;  
			data_master_OUT[3]=((ladd-1) & 0x00FF);  
			data_master_OUT[4]=(hadd & 0xFF00)>>8;		
      data_master_OUT[5]=(hadd & 0x00FF); 
      break;
  }

  CRC16_M(data_master_OUT, 6); 
  line_master=8; n_master_com=0;
  USART2->CR1  &=  ~USART_CR1_RE;              
  USART2->CR1  |=   USART_CR1_TE;               	  
  RTS_SET;
	MB_LED_ON;	
  USART2->DR=data_master_OUT[n_master_com++];  
	
	//AllOutPakets++;

	osSemaphoreRelease(SemMbus);
}


/*
0х10я функция
*/
void zapros_M10(unsigned char buf[500])
{  
char len = buf[12];
	osSemaphoreWait(SemMbus, portMAX_DELAY);
	
	for(int i=0;i<(len+7);i++)
	{
		data_master_OUT[i] = buf[i+6]; 
	}

  CRC16_M(data_master_OUT, len+7); 
  line_master=len+9; n_master_com=0;
  USART2->CR1  &=  ~USART_CR1_RE;              
  USART2->CR1  |=   USART_CR1_TE;               
  RTS_SET; 
  USART2->DR=data_master_OUT[n_master_com++];                
	
//	AllOutPakets++;
	osSemaphoreRelease(SemMbus);
}

/*
Чтение смс, ну и удаление после чтения.
Опасно, удаление надо сделать отдельно.
*/
void ReadSMS(void)
{
	char* cmgf = "at+cmgf=1";
	char* cmgl = "at+cmgl=\"REC UNREAD\"";
	char cmd = 1;
	
	while(cmd<3)
	{
		switch (cmd) 
		{	
			case 1:
			SendGSM(cmgf, strlen(cmgf));
			break;
				
			case 2:
			SendGSM(cmgl, strlen(cmgl));
			break;

		}
		cmd++;
		osDelay(400);
	}
	osDelay(4000);
}

/*
Отчистка СМС
*/
void DeleteAllSMS(void)
{
		char* cmgd = "at+cmgd=30,3";
			SendGSM(cmgd, strlen(cmgd));
			osDelay(500);
}

/*
ОПТИМИЗАЦИЯ? НЕ, НЕ СЛЫШАЛ
Но попытки оптимизации уже были...
*/
char cyrbuf[600]={0};
//char text[100]={0};
unsigned char pdu1[150]={0}, pdu3[400]={0}, tlen;
/*
Норм функция отправки СМС.
Но не идеальная, еще есть cmd, надо избавиться от этого...
*/
void SendSMS5(char* text)
{
	
	
		char *cmgf="AT+CMGF=0";
	
	
	for(unsigned int v=0; v < NumPhones; v++)
	{
		smsnum = &Phones[v][0];
		char cmgs1[15]="AT+CMGS=";
		char phone[15]={0}, done=0;
	unsigned short int pdu2[60]={0}, a = 1105, b = 848,len=0;
	
		for(unsigned int cmd=0; cmd < 3; cmd++)
			{
				osDelay(300);
				//memset(data_com3_OUT,0,2000);
				switch(cmd)
				{
					
					case 0:
					len = strlen(cmgf);
					line3=len+2; n_com3=0;
					
					strcpy(data_com3_OUT,cmgf);
					data_com3_OUT[len++] = '\r';	
					data_com3_OUT[len++] = '\n';
					break;
					
			
					case 1:
					//GSM_LED_ON;
			////////////////////////////////////////////////Кодирование где-то тут/////////////////////////////////////////////////////////////////////////////////		
					
			memset(cyrbuf, 0,600); // Очистка буфферов
			memset(pdu3, 0,400);
			memset(pdu1, 0,150);
			//memset(text, 0,100);
			tlen = 0;
									
			//0011000B91 Префикс
			cyrbuf[0] = '0';
			cyrbuf[1] = '0';
			cyrbuf[2] = '1';
			cyrbuf[3] = '1';
			cyrbuf[4] = '0';
			cyrbuf[5] = '0';
			cyrbuf[6] = '0';
			cyrbuf[7] = 'B';
			cyrbuf[8] = '9';
			cyrbuf[9] = '1';

			
			strcat(cyrbuf,smsnum);

			cyrbuf[22] = 'F';
			
			for(int i=0;i<7;i++)	// Кодирование номера телефона
				{
					a = 10+i*2;
					b = 10+(i+1)*2;
					cyrbuf[a]=cyrbuf[b];
				}
			
			
				//0008FF Еще префикс
			cyrbuf[22] = '0';
			cyrbuf[23] = '0';
			cyrbuf[24] = '0';
			cyrbuf[25] = '8';
			cyrbuf[26] = 'F';
			cyrbuf[27] = 'F';
			
			tlen = strlen(text)*2; // Запись размера сообщения
				sprintf(pdu1,"%02X",tlen);
					strcat(cyrbuf,pdu1);
				
				strcpy(pdu1,text);
				
				for(int i=0;i<strlen(text);i++) // Кодирование текста
				{
						if(pdu1[i]>=192&&pdu1[i]<=255)
						{
							pdu2[i] = pdu1[i]+848;	
						}
						else if(pdu1[i]==168)
						{
							pdu2[i] = 1025;
						}
						else if(pdu1[i]==184)
						{
							pdu2[i] = 1105;
						}
						else
						{
							pdu2[i] = pdu1[i];
						}


				}
				
				memset(pdu1, 0,150);
				for(int i=0;i<strlen(text);i++) // Перевод кода в ASCII
					{
						sprintf(pdu1,"%04X",pdu2[i]);	
						//strcat(pdu3,pdu1);
						strcat(cyrbuf,pdu1);
					}
				
			
			//strcat(cyrbuf,pdu3);
			a = strlen(cyrbuf);
			cyrbuf[a] = 0x1A; // Окончание ввода сообщения
					
					//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					b = (strlen(cyrbuf)-2)/2; 
					sprintf(pdu1,"%d",b);	
					strcat(cmgs1,pdu1);
					len = strlen(cmgs1);
					line3=len+2; n_com3=0;
					
					strcpy(data_com3_OUT,cmgs1);
					data_com3_OUT[len++] = '\r';	
					data_com3_OUT[len++] = '\n';
					break;
					
					case 2:
					
					
					
					b = (strlen(cyrbuf)-2)/2;
					memset(data_com3_OUT, 0,300);
					memcpy(data_com3_OUT,cyrbuf,strlen(cyrbuf));
					len = strlen(data_com3_OUT);
					line3=len+2; n_com3=0;
					data_com3_OUT[len++] = '\r';	
					data_com3_OUT[len++] = '\n';
					//GSM_LED_OFF;
					break;
					
					default:
					
					break;
					
					
				}
				
			USART3->CR1  &=  ~USART_CR1_RE;              //Off ?????????
			USART3->CR1  |=   USART_CR1_TE;              //????????? ??????????? 	  
			//RTS_SET3; 
			USART3->DR=data_com3_OUT[n_com3++];  
			}
			
			osDelay(4000);
	}
		
		
		
		
		
}

/*
void SendSMS5(char cmd, char* text)
{
	char phone[15]={0}, done=0;
	unsigned short int pdu2[60]={0}, a = 1105, b = 848,len;
	
		char *cmgf="AT+CMGF=0";
	char cmgs1[15]="AT+CMGS=";
	
	
		switch(cmd)
		{
			
			case 0:
			len = strlen(cmgf);
			line3=len+2; n_com3=0;
			
			strcpy(data_com3_OUT,cmgf);
			data_com3_OUT[len++] = '\r';	
			data_com3_OUT[len++] = '\n';
			break;
			
	
			case 1:
			//GSM_LED_ON;
	////////////////////////////////////////////////Кодирование где-то тут/////////////////////////////////////////////////////////////////////////////////		
			
	memset(cyrbuf, 0,600); // Очистка буфферов
	memset(pdu3, 0,400);
	memset(pdu1, 0,150);
							
	//0011000B91 Префикс
	cyrbuf[0] = '0';
	cyrbuf[1] = '0';
	cyrbuf[2] = '1';
	cyrbuf[3] = '1';
	cyrbuf[4] = '0';
	cyrbuf[5] = '0';
	cyrbuf[6] = '0';
	cyrbuf[7] = 'B';
	cyrbuf[8] = '9';
	cyrbuf[9] = '1';

	
	strcat(cyrbuf,smsnum);

	cyrbuf[22] = 'F';
	
	for(int i=0;i<7;i++)	// Кодирование номера телефона
		{
			a = 10+i*2;
			b = 10+(i+1)*2;
			cyrbuf[a]=cyrbuf[b];
		}
	
	
		//0008FF Еще префикс
	cyrbuf[22] = '0';
	cyrbuf[23] = '0';
	cyrbuf[24] = '0';
	cyrbuf[25] = '8';
	cyrbuf[26] = 'F';
	cyrbuf[27] = 'F';
	
	tlen = strlen(text)*2; // Запись размера сообщения
		sprintf(pdu1,"%02X",tlen);
			strcat(cyrbuf,pdu1);
		
		strcpy(pdu1,text);
		
		for(int i=0;i<strlen(text);i++) // Кодирование текста
		{
				if(pdu1[i]>=192&&pdu1[i]<=255)
				{
					pdu2[i] = pdu1[i]+848;	
				}
				else if(pdu1[i]==168)
				{
					pdu2[i] = 1025;
				}
				else if(pdu1[i]==184)
				{
					pdu2[i] = 1105;
				}
				else
				{
					pdu2[i] = pdu1[i];
				}


		}
		
		memset(pdu1, 0,150);
		for(int i=0;i<strlen(text);i++) // Перевод кода в ASCII
			{
				sprintf(pdu1,"%04X",pdu2[i]);	
				//strcat(pdu3,pdu1);
				strcat(cyrbuf,pdu1);
			}
		
	
	//strcat(cyrbuf,pdu3);
	a = strlen(cyrbuf);
	cyrbuf[a] = 0x1A; // Окончание ввода сообщения
			
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			b = (strlen(cyrbuf)-2)/2; 
			sprintf(pdu1,"%d",b);	
			strcat(cmgs1,pdu1);
			len = strlen(cmgs1);
			line3=len+2; n_com3=0;
			
			strcpy(data_com3_OUT,cmgs1);
			data_com3_OUT[len++] = '\r';	
			data_com3_OUT[len++] = '\n';
			break;
			
			case 2:
			
			
			
			b = (strlen(cyrbuf)-2)/2;
			memset(data_com3_OUT, 0,300);
			memcpy(data_com3_OUT,cyrbuf,strlen(cyrbuf));
			len = strlen(data_com3_OUT);
			line3=len+2; n_com3=0;
			data_com3_OUT[len++] = '\r';	
			data_com3_OUT[len++] = '\n';
			//GSM_LED_OFF;
			break;
			
			default:
			
			break;
			
			
		}
		
	USART3->CR1  &=  ~USART_CR1_RE;              //Off ?????????
  USART3->CR1  |=   USART_CR1_TE;              //????????? ??????????? 	  
  //RTS_SET3; 
  USART3->DR=data_com3_OUT[n_com3++];  
		
		
		
		
}
*/
/*
Ля, тут надо кароч начальный инит придумать, потому что с завода модем не будет норм раработать
Ну и тут можно заинитить параметры соединений всяких
*/
void InitGSM(void) // 
{
	//at+cnmi=2,1,0,0,1
	char *cmgf="at+cmgf=1";
	char *cmee = "at+cmee=1";
	char *cfun = "at+cfun=1";
	char *sxrat = "at^sxrat=2";
	char *cops = "at+cops=0";
	char *cops1 = "at+cops?";
	char *ate = "ATE0";
	char *csq = "at+csq";
	char *atf = "at&f";
	char *sics = "at^sics?";
	char *sici = "at^sici=5";
	char *atd = "atd0876";
	char *at = "AT";
	//char *SCFG = "AT^SCFG=\"Serial/USB/DDD\",0";
	char *SCFG = "AT^SCFG=\"Serial/Interface/Allocation\", \"2\"";
	if(FirstTurnOn)
	{
		char *ipr = "at+ipr=921600";
		MX_USART3_UART_Init(115200);
		SendGSM(ipr, strlen(ipr));
		osDelay(2000);
		MX_USART3_UART_Init(921600);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13,0);
	osDelay(20);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13,1);
		
		osDelay(5000);
	}
	//char *scfg = "at^scfg=\"Serial/Interface/Allocation\",\"2\"";
	//at^scfg="Serial/Interface/Allocation","2"
	
	char cmd[10][100]={0};
	
	char find = 1;
	
//	SendGSM(ipr, strlen(ipr));
//	osDelay(2000);
//	MX_USART3_UART_Init(921600);
//	SendGSM(csq, strlen(csq));
//	osDelay(2000);
//	
//	SendGSM(at, strlen(at));
//	osDelay(2000);


// Определяем оператора для настройки интернет соединения

do{
	
	SendGSM(cops, strlen(cops));
	osDelay(7000);
	
	SendGSM(cops1, strlen(cops1)); // Запрос имени оператора
	osDelay(7000);

	if((strstr(data_com3_IN, "MegaFon")!=0) || (strstr(data_com3_IN, "MEGAFON")!=0))
	{
		find = 0;
		// Если мегафон:
		strcpy(&cmd[0][0],"at^sics=5,conType,GPRS0");
		strcpy(&cmd[1][0],"at^sics=5,apn,internet");
		strcpy(&cmd[2][0],"at^sics=5,dns1,8.8.8.8");
		strcpy(&cmd[3][0],"at^sics=5,user,gdata");
		strcpy(&cmd[4][0],"at^sics=5,passwd,gdata");
		strcpy(&cmd[5][0],"at^sics=5,alphabet,0");
		strcpy(&cmd[6][0],"at^sics=5,inactTO,20");
	}
	
		if((strstr(data_com3_IN, "Beeline")!=0) || (strstr(data_com3_IN, "BEELINE")!=0))
	{
		find = 0;
		// Если билайн:
		strcpy(&cmd[0][0],"at^sics=5,conType,GPRS0");
		strcpy(&cmd[1][0],"at^sics=5,apn,home.beeline.ru");
		strcpy(&cmd[2][0],"at^sics=5,dns1,8.8.8.8");
		strcpy(&cmd[3][0],"at^sics=5,user,beeline");
		strcpy(&cmd[4][0],"at^sics=5,passwd,beeline");
		strcpy(&cmd[5][0],"at^sics=5,alphabet,0");
		strcpy(&cmd[6][0],"at^sics=5,inactTO,20");
	}
	
		if((strstr(data_com3_IN, "Tele2")!=0) || (strstr(data_com3_IN, "TELE2")!=0))
	{
		find = 0;
		// Если теле2:
		strcpy(&cmd[0][0],"at^sics=5,conType,GPRS0");
		strcpy(&cmd[1][0],"at^sics=5,apn,internet.tele2.ru");
		strcpy(&cmd[2][0],"at^sics=5,dns1,0.0.0.0");
		strcpy(&cmd[3][0],"at^sics=5,user,\"\"");
		strcpy(&cmd[4][0],"at^sics=5,passwd,\"\"");
		strcpy(&cmd[6][0],"at^sics=5,alphabet,0");
		strcpy(&cmd[5][0],"at^sics=5,inactTO,20");
	}
	
			if((strstr(data_com3_IN, "MTS")!=0) || (strstr(data_com3_IN, "mts")!=0))
	{
		find = 0;
		// Если мтс:
		strcpy(&cmd[0][0],"at^sics=5,conType,GPRS0");
		strcpy(&cmd[1][0],"at^sics=5,apn,internet.mts.ru");
		//strcpy(&cmd[2][0],"at^sics=5,dns1,0.0.0.0");
		strcpy(&cmd[3][0],"at^sics=5,user,mts");
		strcpy(&cmd[4][0],"at^sics=5,passwd,mts");
		strcpy(&cmd[6][0],"at^sics=5,alphabet,0");
		strcpy(&cmd[5][0],"at^sics=5,inactTO,20");
	}
	osDelay(500);
}
while(find);



	SendGSM(ate, strlen(ate)); // отключение эхо
	osDelay(1000);
	SendGSM(cmgf, strlen(cmgf));
	osDelay(1000);
	SendGSM(cfun, strlen(cfun));
	osDelay(1000);
	SendGSM(sxrat, strlen(sxrat));
	osDelay(1000);
	SendGSM(cmee, strlen(cmee));
	osDelay(1000);


	
Holding_reg[1] = GSMQ();

for(int i=0; i<7; i++) // устанавливаем параметры интернет соединения
	{
	osDelay(300);
	SendGSM(&cmd[i][0], strlen(&cmd[i][0]));
	}
	
	
//	char *comPPP_0 = "AT+CGDCONT=1,\"IP\",";
//	char *comPPP_2 = "AT+CGQMIN=1,0,0,0,0,0";
//	char *comPPP_3 = "AT+CGQREQ=1,2,4,3,6,31";
//	char *comPPP_4 = "ATD*99***1#";
//	
//	SendGSM(comPPP_0, strlen(comPPP_0));
//	osDelay(1000);	
//	SendGSM(comPPP_2, strlen(comPPP_2));
//	osDelay(1000);	
//	SendGSM(comPPP_3, strlen(comPPP_3));
//	osDelay(1000);	
//	SendGSM(comPPP_4, strlen(comPPP_4));
//	osDelay(10000);
	
	
}

/*
Обработка входящего ModbusTCP пакета.
Дада, слэйв функции в файле для мастер функций, потому что я могу!
Тут местами такая сатона, надо бы структурировать...
Делать я это кончено же не буду...
*/
static int Modbus_TCP_Slave_serve(struct netconn *conn)
{
	struct pbuf *p, *q;
  uint32_t  len,indx;
	
	char bufer[500], smsbuf[20];
	unsigned int ad, n, j; 
	unsigned int dat,k;
	
	struct netbuf *inbuf;
  err_t recv_err;
  char* buf;
	u16_t buflen;
	
	recv_err = netconn_recv(conn, &inbuf);
     
	
  if (recv_err == ERR_OK)
  {
    if (netconn_err(conn) == ERR_OK) 
    {
      netbuf_data(inbuf, (void**)&buf, &buflen);
			memset(bufer, 0,500);
			memcpy(bufer,buf,210); // Ну тут кароч строку в масссив переводим, чтоб дальше проще было работать
															// Имеет смысл подумать над strcpy();
			netbuf_delete(inbuf);
	
	
     if(((bufer[6]== MSREGADR) && (GatewayON == 0))||(GatewayON == 1)) 
		 {
			 
			 switch (bufer[7])
			 {
				 case 0x3:
					 if(!GatewayON)
						{					
							ad=bufer[8]; ad<<=8; 
							ad|=bufer[9]; ad++;   

							n=bufer[11]<<1;      
							bufer[8]=n;  
							j=9;         
							while(n!=0)
								{
								dat=Holding_reg[ad++];
								bufer[j++]=dat>>8;
								bufer[j++]=dat;
								n-=2;
								}


								k=j;
								bufer[5] = k-6;						
							//goto m_ok1;
						}
						else
						{
							unsigned short l=0,h=0;
							l = bufer[8];
							l<<=8;
							l|=bufer[9];
							h = l + bufer[11];
							
							osSemaphoreWait(SemPlan, 1000);
							SendGW = 1;
							zapros_M(bufer[6],bufer[7],l+1,h);
							osSemaphoreWait(SemGateway, 1000);
							
							if(SendGW!=2) // типа норм пакет
							{
								SendGW = 0;
								
								for(int i = 0;i<data_master_IN[2]+3;i++)
								{
									bufer[6+i] = data_master_IN[i];
								}
								
								bufer[8] = bufer[8];
								bufer[5] = bufer[8]+3;	
								k=bufer[5]+6;
								
								osSemaphoreRelease(SemPlan);
								
								//goto m_ok1;
							}
							else
							{
								bufer[7] = bufer[7]|| 0x80;
								bufer[5] = 3;
								k = 9;
								//goto m_ok1;
							}
						}
				 break;
				 
				 case 0x4: 
					 if(!GatewayON)
						{
							
						ad=bufer[8]; ad<<=8;  
						ad|=bufer[9];  ad++; 
					
						n=bufer[11]<<1;                  
						bufer[8]=n;        
						j=9;        

													while(n!=0)
																{
																dat=Input_reg[ad++];          
																bufer[j++]=dat>>8;             // H
																bufer[j++]=dat;                // L
																n-=2;
																}

						
							k=j;
						bufer[5] = k-6;		
						//goto m_ok1;
						}
					else
					{
						unsigned short l=0,h=0;
						l = bufer[8];
						l<<=8;
						l|=bufer[9];
						h = l + bufer[11];
						
						osSemaphoreWait(SemPlan, 1000);
						SendGW = 1;
						zapros_M(bufer[6],bufer[7],l+1,h);
						osSemaphoreWait(SemGateway, 1000);
						
						if(SendGW!=2) // типа норм пакет
							{
								SendGW = 0;
								
								for(int i = 0;i<data_master_IN[2]+3;i++)
								{
									bufer[6+i] = data_master_IN[i];
								}
								
								bufer[8] = bufer[8];
								bufer[5] = bufer[8]+3;	
								k=bufer[5]+6;
								
								osSemaphoreRelease(SemPlan);
								
								//goto m_ok1;
							}
							else
							{
								bufer[7] = bufer[7]|| 0x80;
								bufer[5] = 3;
								k = 9;
								//goto m_ok1;
							}
					}	
				 break;
				 
				 case 0x6:
					 if(!GatewayON)
						{
							n=bufer[8];    n<<=8;  
							n|=bufer[9];   n++;

							dat=bufer[10]; dat<<=8;
							dat|=bufer[11];
							Holding_reg[n]=dat;

						 k=12; 
							
							//goto m_ok1;	
						}
						else
						{
							unsigned short l=0,h=0;
						l = bufer[8];
						l<<=8;
						l|=bufer[9];
							
						h = bufer[10];
						h<<=8;
						h|=bufer[11];
						
						osSemaphoreWait(SemPlan, 1000);
						SendGW = 1;
						zapros_M(bufer[6],bufer[7],l+1,h);
						osSemaphoreWait(SemGateway, 1000);
							if(SendGW!=2) // типа норм пакет
							{
						SendGW = 0;

						k=12;
						
						osSemaphoreRelease(SemPlan);
						
						//goto m_ok1;
							}
							else
							{
								bufer[7] = bufer[7]|| 0x80;
								bufer[5] = 3;
								k = 9;
								//goto m_ok1;
							}
						}
				 break;
				 
				 case 0x10:
					 if(!GatewayON)
							{
							ad=bufer[8];    ad<<=8;  
							ad|=bufer[9];   ad++;
		
							n=bufer[10];    n<<=8;  
							n|=bufer[11];
								n=n*2;
									j = 12;
									while(n!=0)
									{
													dat = 0;
										j++;
													dat=bufer[j]; dat<<=8;
										j++;
													dat|=bufer[j];
													Holding_reg[ad]=dat;
													n-=2;
													ad++;
									}
		
							k=12; 
							bufer[5] = 6;
							bufer[10] = 0;
							//goto m_ok1;	
							}
							else
							{
								osSemaphoreWait(SemPlan, 1000);
						SendGW = 1;
						zapros_M10(bufer);
						osSemaphoreWait(SemGateway, 1000);
							if(SendGW!=2) // типа норм пакет
							{
							SendGW = 0;
							for(int i = 0;i<7;i++)
							{
								bufer[6+i] = data_master_IN[i];
							}
							k=12;
							
							osSemaphoreRelease(SemPlan);
							bufer[5] = 6;
							//goto m_ok1;
							}
							else
							{
								bufer[7] = bufer[7]|| 0x80;
								bufer[5] = 3;
								k = 9;
								//goto m_ok1;
							}
								

							}	
				 break;
							
				 case 0x55: // Чтение номера телефона
					
						strcpy(smsbuf, smsnum);
						bufer[8] = strlen(smsnum);
						n = bufer[8]+1;
						for(char i=0;i<n;i++)
						{
							bufer[i+9] = smsbuf[i];
						}
						
					k=bufer[8]+9; 
					bufer[5]=k-6;
				break;		

				 case 0x56: // Запись номера телефона
					len = bufer[8];

							for(int i=0;i<len;i++)
							{
								SMSbuf[i] = bufer[9+i];
								smsMSG[0][i] = bufer[9+i];
							}
							
					k=bufer[8]+9; 
					bufer[5]=k-6;
					
				osSemaphoreRelease(SemMem);
							
				break;								
							
							
				 default:
						bufer[7] = bufer[7]|| 0x80;
						bufer[5] = 3;
								k = 9;
								//goto m_ok1;
				 break;
			 }
			 
			 
			 						
				m_ok1:    
				// 

    netconn_write(conn, (char*) bufer, k, NETCONN_NOCOPY);       

    return 1;
						
		 }
		 else
		 {
			 //goto m_eror1;    //Тут проверяем адрес, но надо переменную придумать, туда сюда
			 return 1;
		 }
		}
			else
			{
				return 0; // Если соединение не ок, то разрываем его
				//netbuf_delete(inbuf);
			}
	
	}
		else
		{
			return 0; // Если принятый пакет не ок, то разрываем соединение (мб не стоит разрывать его, но пока так)
			//netbuf_delete(inbuf);
		}
				
}


/*
Данная задача создается при подключени по ModbusTCP.
Таким образом, для каждого клиента свой таск, что позволяет обслуживать большое количество клиентов.
Количество клиентов будет ограничено настройками LwIP
*/
void Modbus_TCP_Task(struct netconn *conn)
{
	
	int errM;
	
	netconn_set_recvtimeout(conn,1000*10); // Кароч, если за 10 сек нет данных, то отъезжаем
  conn->pcb.tcp->keep_idle  =  5000;
  conn->pcb.tcp->keep_intvl =  1000;
  conn->pcb.tcp->keep_cnt   =  5;
  conn->pcb.tcp->so_options |= SOF_KEEPALIVE;


			for(;;)
			{					
				errM = Modbus_TCP_Slave_serve(conn); // Ну тут тип смотрим приходящие пакеты и парсим их
				if(!errM) // Если что-то не ок в обработке пакетов было, то выходим из цикла, далее разрываем соединение
						{
							break;
						}
			}
	
		netconn_close(conn);	// Закрываем соединение
		netconn_delete(conn); // Удаляем явки, номера
			
	osThreadTerminate(NULL); // Жжем мосты
}


/*
Ф-я отправки TCP запроса
*/
void zapros_T(struct netconn *conn, unsigned char dadd, unsigned char func, unsigned short ladd, unsigned short hadd)
{
//xSemaphoreTake(SemMbus, portMAX_DELAY);
char Mbuf[20]={0};
unsigned short num = (hadd-ladd)+1;
unsigned short ErrCount = 0;
//expect_dadd = dadd;
  switch (func)
  {
    case 4:    
			Mbuf[6]	=	dadd;
      Mbuf[7]	=	0x04;   
			Mbuf[8]	=	((ladd-1) & 0xFF00)>>8;  
			Mbuf[9]	=	((ladd-1) & 0x00FF);  
			Mbuf[10]=	(num & 0xFF00)>>8;		
      Mbuf[11]=	(num & 0x00FF); 
		
	//	expect_add4 = ladd;
      break;
    case 3:    
			Mbuf[6]	=	dadd;
      Mbuf[7]	=	0x03;   
			Mbuf[8]	=	((ladd-1) & 0xFF00)>>8;  
			Mbuf[9]	=	((ladd-1) & 0x00FF);  
			Mbuf[10]=	(num & 0xFF00)>>8;		
      Mbuf[11]=	(num & 0x00FF); 
		
		//expect_add3 = ladd;
      break;
    case 6:    
			Mbuf[6]	=	dadd;
      Mbuf[7]	=	0x06;   
			Mbuf[8]	=	((ladd-1) & 0xFF00)>>8;  
			Mbuf[9]	=	((ladd-1) & 0x00FF);  
			Mbuf[10]=	(hadd & 0xFF00)>>8;		
      Mbuf[11]=	(hadd & 0x00FF); 
      break;
  }

	
	for(;;)
	{
	if(netconn_write(conn, (char*) Mbuf, 12, NETCONN_NOCOPY)!=ERR_OK)
	{
		ErrCount++;
		osDelay(200);
		if(ErrCount<5)
		{
			continue;
		}
	}
	break;
	}
	//netconn_write(conn, (char*) Mbuf, 12, NETCONN_NOCOPY);    
//	xSemaphoreGive(SemMbus);
}


/*
ф-я отпраки комманды GSM модему С переносом строки "\r\n" (Обычные команды)
*/
void SendGSM(unsigned char *cmd, unsigned int len)
{
	osDelay(50);
	osSemaphoreWait(SemGSM, portMAX_DELAY); 
//GSM_LED_ON;
	memcpy(data_com3_OUT, cmd, len);
	memset(data_com3_IN,0,1200);
	
	line3=len+2;
	data_com3_OUT[len] = '\r';
	data_com3_OUT[len+1] = '\n';
			
			n_com3=0;

		
	USART3->CR1  &=  ~USART_CR1_RE;
  USART3->CR1  |=   USART_CR1_TE;
	//RTS_SET3; 
  USART3->DR=data_com3_OUT[n_com3++];
//GSM_LED_OFF;
	osSemaphoreRelease(SemGSM);
}

/*
ф-я отпраки комманды GSM модему БЕЗ переноса строки "\r\n" (Для прозрачного режима (Transparent))
*/
void SendGSMTrans(unsigned char *cmd, unsigned int len)
{
	osDelay(50);
	osSemaphoreWait(SemGSM, portMAX_DELAY); 
//GSM_LED_ON;
	memcpy(data_com3_OUT, cmd, len);
	
	line3=len;
		
			n_com3=0;

	USART3->CR1  &=  ~USART_CR1_RE;
  USART3->CR1  |=   USART_CR1_TE;
	//RTS_SET3; 
  USART3->DR=data_com3_OUT[n_com3++];
//GSM_LED_OFF;
	osSemaphoreRelease(SemGSM);
}

/*
ф-я обработки Modbus TCP пакета по каналу 3G
*/
int GSM_S(void)
{
	unsigned short int lenTCP = 0,k;
	unsigned char bufTCP[200];
	lenTCP = n_com33;
	n_com33=0; 
	unsigned short l=0,h=0;
	for(int i=0;i<lenTCP;i++)
	{
		bufTCP[i] = data_com3_IN[i];
	}
	
	//strncpy(&bufTCP[0],&data_com3_IN[0],lenTCP);
	
		switch (data_com3_IN[7])
			 {
				 
				case 0x3:
							l = bufTCP[8];
							l<<=8;
							l|=bufTCP[9];
							h = l + bufTCP[11];
							
							osSemaphoreWait(SemPlan, 5000);
							SendGW = 1;
							zapros_M(bufTCP[6],bufTCP[7],l+1,h);
							osSemaphoreWait(SemGateway, 5000);
							
							if(SendGW!=2) // типа норм пакет
							{
								SendGW = 0;
								
								for(int i = 0;i<data_master_IN[2]+3;i++)
								{
									bufTCP[6+i] = data_master_IN[i];
								}
								
								bufTCP[8] = bufTCP[8];
								bufTCP[5] = bufTCP[8]+3;	
								k=data_com3_IN[5]+6;
								
								osSemaphoreRelease(SemPlan);
								k=bufTCP[5]+6;
								//goto m_ok1;
							}
							else
							{
								bufTCP[7] = bufTCP[7]|| 0x80;
								bufTCP[5] = 3;
								k = 9;
								//goto m_ok1;
							}
				break;
				
				case 0x4:
					
							l = bufTCP[8];
							l<<=8;
							l|=bufTCP[9];
							h = l + bufTCP[11];
							
							osSemaphoreWait(SemPlan, 5000);
							SendGW = 1;
							zapros_M(bufTCP[6],bufTCP[7],l+1,h);
							osSemaphoreWait(SemGateway, 5000);
							
							if(SendGW!=2) // типа норм пакет
							{
								SendGW = 0;
								
								for(int i = 0;i<data_master_IN[2]+3;i++)
								{
									bufTCP[6+i] = data_master_IN[i];
								}
								
								bufTCP[8] = bufTCP[8];
								bufTCP[5] = bufTCP[8]+3;	
								k=data_com3_IN[5]+6;
								
								osSemaphoreRelease(SemPlan);
								k=bufTCP[5]+6;
								//goto m_ok1;
							}
							else
							{
								bufTCP[7] = bufTCP[7]|| 0x80;
								bufTCP[5] = 3;
								k = 9;
								//goto m_ok1;
							}
				break;
				
				case 0x6:
									
						l = bufTCP[8];
						l<<=8;
						l|=bufTCP[9];
							
						h = bufTCP[10];
						h<<=8;
						h|=bufTCP[11];
						
						osSemaphoreWait(SemPlan, 5000);
						SendGW = 1;
						zapros_M(bufTCP[6],bufTCP[7],l+1,h);
						osSemaphoreWait(SemGateway, 5000);
							if(SendGW!=2) // типа норм пакет
							{
						SendGW = 0;

						k=12;
						
						osSemaphoreRelease(SemPlan);
						
						//goto m_ok1;
							}
							else
							{
								bufTCP[7] = bufTCP[7]|| 0x80;
								bufTCP[5] = 3;
								k = 9;
								//goto m_ok1;
							}
				break;
				
				case 0x10:
					osSemaphoreWait(SemPlan, 5000);
						SendGW = 1;
						zapros_M10(bufTCP);
						osSemaphoreWait(SemGateway, 5000);
							if(SendGW!=2) // типа норм пакет
							{
							SendGW = 0;
							for(int i = 0;i<7;i++)
							{
								bufTCP[6+i] = data_master_IN[i];
							}
							k=12;
							
							osSemaphoreRelease(SemPlan);
							bufTCP[5] = 6;
							//goto m_ok1;
							}
							else
							{
								bufTCP[7] = bufTCP[7]|| 0x80;
								bufTCP[5] = 3;
								k = 9;
								//goto m_ok1;
							}
				break;
				
				default:
					n_com33 = 0;
					return 1;
				break;
				 
				 
			 }
			 
			 SendGSMTrans(&bufTCP[0],k);
	return 0;
}


/*
Задача ожидание пакета Modbus TCP по каналу 3G
*/
void ModbusGSM(void const * argument)
{
	for(;;)
	{
		if(GSMTCP_On)
		{
			osSemaphoreWait(xMbusG, portMAX_DELAY); 
			CLEARBIT(bit_G);
			GSM_S();
		}
		else
		{
			osDelay(1000);
		}
	}
}


/*
Запрос уровня сигнала GSM
*/
unsigned short int GSMQ(void)
{
	unsigned short int q = 0;
	char *csq = "at+csq";
	osDelay(1000);
	SendGSM(csq, strlen(csq));
	osDelay(7000);

	char* token_buff;
	strtok_r(data_com3_IN," ",&token_buff);
	q = atoi((char*)strtok_r(NULL,",",&token_buff));
	return q;
}

/*
ф-я отпраки комманды GSM модему С переносом строки "\r\n" (Обычные команды) через USB
*/
void SendGSMUSB(unsigned char *cmd)
{
	osSemaphoreWait(SemGSM, 100); 
	unsigned int len = strlen(cmd);
	memcpy(data_com3_OUT, cmd, len);
	data_com3_OUT[len] = '\r';
	data_com3_OUT[len+1] = '\n';
	len = len + 2;
	USBH_CDC_Transmit(&hUsbHostHS,data_com3_OUT,len);
}

/*
ф-я отпраки комманды GSM модему БЕЗ переноса строки "\r\n" (Для прозрачного режима (Transparent)) через USB
*/
void SendGSMTransUSB(unsigned char *cmd)
{
	osSemaphoreWait(SemGSM, 100); 
	unsigned int len = strlen(cmd);
	memcpy(data_com3_OUT, cmd, len);
	USBH_CDC_Transmit(&hUsbHostHS,data_com3_OUT,len);
}

/*
ф-я отпраки комманды GSM модему БЕЗ переноса строки "\r\n" (Для прозрачного режима (Transparent)) через USB
*/
void SendGSMTransUSB2(unsigned char *cmd, unsigned int len)
{
	//osMessageGet(EndTranc, 50);
	osSemaphoreWait(SemGSM, 1000); 
	memcpy(data_com3_OUT, cmd, len);
	USBH_CDC_Transmit(&hUsbHostHS,data_com3_OUT,len);
}