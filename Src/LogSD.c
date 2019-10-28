#include "LogSD.h"
#include "ModBus_master.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32_ub_fatfs.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "ff.h"
#include "string.h"
#include <locale.h>  

#include "lwip/api.h"
#include "lwip/memp.h"
#include "lwip/api.h"
#include "lwip/ip.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/priv/api_msg.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/priv/tcpip_priv.h"
#include "lwip/opt.h"
#include "lwip\tcpip.h"
#include "lwip\mem.h"
#include "lwip\pbuf.h"
#include "lwip\api.h"
#include "lwip\ip.h"
#include "lwip\tcp.h"

#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_cdc.h"



extern RTC_HandleTypeDef hrtc;
extern xSemaphoreHandle SemMem;
extern xSemaphoreHandle SemFiles;
extern xSemaphoreHandle SemPlan;
extern xSemaphoreHandle SemSaveDev;
extern xSemaphoreHandle SemSMS;
extern xSemaphoreHandle SemEventWork;

extern USBH_HandleTypeDef hUsbHostHS;

extern unsigned int    n_com3,n_com33, line3;
extern unsigned char   data_com3_OUT[2000],data_com3_IN[1200];

volatile struct devices
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
	short int Holding[100];
	short int Input[100];
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

struct devices *StructQwAdr[20];
char NumQw=0;

void ParsingAdd(struct devices *ThisDev);

int CreateLOG(struct devices *logdev);
static void WriteLOG(void const * argument);

typedef struct ConvLetter {
        char    win1251;
        int     unicode;
} Letter;
 

//FIL file1;
extern FATFS SDFatFs;
extern char SDPath[4]; 
FRESULT f_err2;
char flagLOG = 0;
char KillLOG = 0;
char PlanerStatus = 0;
uint32_t FileErr = 0;

extern unsigned int LOADING;
extern unsigned int Timeout;

struct devices* WhichDevNowInPlaner;

extern unsigned int TimeoutCounter;

unsigned int SemErrorCounter=0;

char GSMTCP_On = 0;

extern unsigned char smsMSG[17][51];

unsigned char CutTime = 60;

unsigned char TransMode = 0;

unsigned char LOGTask = 0;

/*
Ф-я определяет действия для создания опроса и логирования
*/
void CreateMSlaveTask(struct device *bis)
{
	size_t heap;
	/*
	ПРОВЕРKA, ЕСТЬ ЛИ УЖЕ УСТРОЙСТВО, И НЕ СОЗДАВАТЬ НОВОЕ, А ДОБАВЛЯТЬ РЕГИСТРЫ
	*/
	char exflag = 0;
	if(NumQw>0)
	{
		for(int i=0;i<NumQw;i++)
		{
			if((StructQwAdr[i]->interface == bis->interface) 
			&& (StructQwAdr[i]->addr == bis->addr) 
			&& (StructQwAdr[i]->ipadd[0] == bis->ipadd[0]) 
			&& (StructQwAdr[i]->ipadd[1] == bis->ipadd[1])
			&& (StructQwAdr[i]->ipadd[2] == bis->ipadd[2])
			&& (StructQwAdr[i]->ipadd[3] == bis->ipadd[3])) // Собсна, такое устройство уже есть, надо упихать новые параметры в имеющуюся структуру
			{
				char IncQ = 0;
				
				if(bis->type3 == 3)
				{
					StructQwAdr[i]->type3 = 3;
					IncQ = ParsingAddr(bis->regs, &StructQwAdr[i]->PatternH[StructQwAdr[i]->NumQH]);
					StructQwAdr[i]->NumQH =StructQwAdr[i]->NumQH+IncQ;
					StructQwAdr[i]->sizeH = 0;
					
					for(int j=0;j<StructQwAdr[i]->NumQH;j++)
					{
						StructQwAdr[i]->sizeH = StructQwAdr[i]->sizeH + (StructQwAdr[i]->PatternH[j][1] - StructQwAdr[i]->PatternH[j][0]) + 1;
					}
					
						if(strlen(StructQwAdr[i]->regsH)!=0)
						{
						strcat(StructQwAdr[i]->regsH,",");
						}
						strcat(StructQwAdr[i]->regsH,bis->regs);
						
						if(strlen(StructQwAdr[i]->floatsH)!=0)
						{
							strcat(StructQwAdr[i]->floatsH,",");
						}
						strcat(StructQwAdr[i]->floatsH,bis->floats);
					
					if((StructQwAdr[i]->log == 0) && (bis->log == 1)) // Если лог был не создан, то создаем
					{
						StructQwAdr[i]->log = 1;
						CreateLOG(StructQwAdr[i]);
					}
					if(StructQwAdr[i]->log == 1) // Если лог уже создан, то надо указать, что что-то было изменено, и надо сменить параметры логирования
					{
						
						osSemaphoreWait(SemFiles, 10000);
						StructQwAdr[i]->log = 3;
						CreateLOG(StructQwAdr[i]);
						osSemaphoreRelease(SemFiles);
					}
				}
					else if(bis->type4 == 4)
					{
							StructQwAdr[i]->type4 = 4;
							IncQ = ParsingAddr(bis->regs, &StructQwAdr[i]->PatternI[StructQwAdr[i]->NumQI]);
							StructQwAdr[i]->NumQI =StructQwAdr[i]->NumQI+IncQ;
							StructQwAdr[i]->sizeI = 0;
							
							for(int j=0;j<StructQwAdr[i]->NumQI;j++)
							{
								StructQwAdr[i]->sizeI = StructQwAdr[i]->sizeI + (StructQwAdr[i]->PatternI[j][1] - StructQwAdr[i]->PatternI[j][0]) + 1;
							}
							
								if(strlen(StructQwAdr[i]->regsI)!=0)
								{
									strcat(StructQwAdr[i]->regsI,",");
								}
								strcat(StructQwAdr[i]->regsI,bis->regs);
								
								if(strlen(StructQwAdr[i]->floatsI)!=0)
								{
									strcat(StructQwAdr[i]->floatsI,",");
								}
								strcat(StructQwAdr[i]->floatsI,bis->floats);
							
							if((StructQwAdr[i]->log == 0) && (bis->log == 1)) // Если лог был не создан, то создаем
							{
								StructQwAdr[i]->log = 1;
								CreateLOG(StructQwAdr[i]);
							}
							if(StructQwAdr[i]->log == 1) // Если лог уже создан, то надо указать, что что-то было изменено, и надо сменить параметры логирования
							{
								osSemaphoreWait(SemFiles, 10000);
								StructQwAdr[i]->log = 3;
								CreateLOG(StructQwAdr[i]);
								osSemaphoreRelease(SemFiles);
							}
					}				
				
					if(LOADING)
					{
					osSemaphoreRelease(SemSaveDev);
					}
					
				exflag = 1;
				break;
			}
			
		}
	}
	
		if(!exflag)
		{
			if(bis->interface==0) 
			{
				MSlaveTask(bis); // Добавляем устройство в планировщик
			}
			else if(bis->interface==1)
			{
							xTaskCreate((pdTASK_CODE)MBusTCPMasterTask,
											 "MbusMTCP",
											 768,
											 bis,
											 osPriorityNormal,
											 NULL
											); // Создаем задачу для устройства Modbus TCP
			}
		}
	
		if(PlanerStatus == 0) // Запускаем планировщик запросов, если еще не запущен
		{
			PlanerStatus = 1;
			xTaskCreate((pdTASK_CODE)MBusMasterPlaner,
										 "Planer",
										 768,
										 bis,
										 osPriorityAboveNormal,
										 NULL
										);
		}
	
}

/*
Задача опроса TCP устройства.
Для каждого устройства своя задача.
*/
void MBusTCPMasterTask(struct device *dev)
{
	struct netconn *conn;
	struct devices ThisDev;
	struct netbuf *inbuf;
	char bufer[200];
	char* buf;
	u16_t buflen;
	ip4_addr_t ipad;
	err_t err;
	
	unsigned int ad, n, j, i; 
	unsigned int dat,ErrCountH,ErrCountI;
	
	TaskHandle_t xHandle1;
	TaskStatus_t xTaskDetails1;
	//Узнаем номер задачи, чтоб потом ее убивать!
  memset(&ThisDev,0,sizeof(ThisDev));
	
    xHandle1 = xTaskGetHandle(NULL);

    configASSERT( xHandle1 );

    vTaskGetInfo( 
                  xHandle1,
                  &xTaskDetails1,
                  pdTRUE,
                  eInvalid );
	
	
	ThisDev.THandle = xTaskDetails1.xHandle;



	ThisDev.addr = dev->addr;
	ThisDev.interface = dev->interface;
	ThisDev.log = dev->log;
	ThisDev.type3 = dev->type3;
	ThisDev.type4 = dev->type4;
	ThisDev.time = dev->time;
	ThisDev.QH = 0;
	ThisDev.QI = 0;
	
	StructQwAdr[NumQw] = &ThisDev;
	NumQw++;
	
	ThisDev.ipadd[0] = dev->ipadd[0];
	ThisDev.ipadd[1] = dev->ipadd[1];
	ThisDev.ipadd[2] = dev->ipadd[2];
	ThisDev.ipadd[3] = dev->ipadd[3];
	ThisDev.port = dev->port;
	
	IP4_ADDR(&ipad,ThisDev.ipadd[0], ThisDev.ipadd[1], ThisDev.ipadd[2], ThisDev.ipadd[3]);
	
	
	//Парс адресов
	//ParsingAdd(&ThisDev);
	
	
	if(ThisDev.type3 == 3)
	{
		strcpy(ThisDev.regsH, dev->regs);
		strcpy(ThisDev.floatsH, dev->floats);
	ThisDev.NumQH = ParsingAddr(ThisDev.regsH, ThisDev.PatternH);
	ThisDev.sizeH = 0;
		// тут надо посчитать, сколько регистров вообще запрашиваем
		for(int i=0;i<ThisDev.NumQH;i++)
		{
			ThisDev.sizeH = ThisDev.sizeH + (ThisDev.PatternH[i][1]-ThisDev.PatternH[i][0]) + 1;
		}
	}
	
		if(ThisDev.type4 == 4)
			{
			strcpy(ThisDev.regsI, dev->regs);
			strcpy(ThisDev.floatsI, dev->floats);	
			ThisDev.NumQI = ParsingAddr(ThisDev.regsI, ThisDev.PatternI);
			ThisDev.sizeI = 0;
				// тут надо посчитать, сколько регистров вообще запрашиваем
				for(int i=0;i<ThisDev.NumQI;i++)
				{
					ThisDev.sizeI = ThisDev.sizeI + (ThisDev.PatternI[i][1]-ThisDev.PatternI[i][0]) + 1;
				}
			}
	
	//Если надо логировать, запускаем лог
			if(ThisDev.log)
			{
				CreateLOG(&ThisDev);
			}
	
			
				if(LOADING)
					{
						osSemaphoreRelease(SemSaveDev);
					}
	
	
	CONNECTING:
	conn = netconn_new(NETCONN_TCP);
      netconn_set_recvtimeout(conn,1000*60*2); //2 min timeout
     // timeout == keep_idle + (keep_intvl * keep_cnt)
		
			conn->pcb.tcp->keep_idle  =  5000;
      conn->pcb.tcp->keep_intvl =  1000;
      conn->pcb.tcp->keep_cnt   =  5;
      conn->pcb.tcp->so_options |= SOF_KEEPALIVE;
		
		for(;;)
		{
			
      err = netconn_connect(conn, &ipad,ThisDev.port);
      if ( err == ERR_OK )
      {
				ErrCountH = 0;
				ErrCountI = 0;
				for(;;)
				{
					if(ThisDev.type3==3)
					{
						if(ThisDev.QH>=ThisDev.sizeH)
						{
							ThisDev.QH=0;
						}
						
						
						for(int i = 0;i<ThisDev.NumQH;i++)
						{
						
						//Отправка запроса
						zapros_T(conn, 1, ThisDev.type3, ThisDev.PatternH[i][0],ThisDev.PatternH[i][1]);
							ThisDev.Packets++;
							// Ожидание ответного Modbus TCP фрейма в течении TIMEOUT_RESPONSE_FRAME ms
							if(netconn_recv(conn,&inbuf)!=ERR_OK)
								{
								// Нет ответного фрейма в течении TIMEOUT_RESPONSE_FRAME ms
								osDelay(200);
									ErrCountH++;
									ThisDev.BadPackets++;
										if(ErrCountH<10)
											{
												continue;
											}
											else
											{
												netconn_close(conn);
												netconn_delete(conn);
												goto CONNECTING;
											}
											
								}
			 
								netbuf_data(inbuf, (void**)&buf, &buflen);
								memset(bufer, 0,200);
								memcpy(bufer,buf,buflen); // Ну тут кароч строку в масссив переводим, чтоб дальше проще было работать
								netbuf_delete(inbuf);
								
								if(bufer[7]==0x03)
									{
									j=9;  
										//ad=expect_add3;
										ad = ThisDev.QH;
									for(n=0; n<bufer[8];n+=2)
													{
													dat=bufer[j++]; dat<<=8;       // H
													dat|=bufer[j++];               // L
													ThisDev.Holding[ad++]=dat;
													}
													ThisDev.QH = ad;
									}
								
									osDelay(10);//
								}
						}
					
						
					
				if(ThisDev.type4==4)
				{	
					if(ThisDev.QI>=ThisDev.sizeI)
						{
							ThisDev.QI=0;
						}
					
					for(int i = 0;i<ThisDev.NumQI;i++)
					{
					
					//Отправка запроса
					zapros_T(conn, 1, ThisDev.type4, ThisDev.PatternI[i][0],ThisDev.PatternI[i][1]);
						ThisDev.Packets++;
						// Ожидание ответного Modbus TCP фрейма в течении TIMEOUT_RESPONSE_FRAME ms
						if(netconn_recv(conn,&inbuf)!=ERR_OK)
							{
							// Нет ответного фрейма в течении TIMEOUT_RESPONSE_FRAME ms
							osDelay(200);
								ErrCountI++;
								ThisDev.BadPackets++;
									if(ErrCountI<10)
											{
												continue;
											}
											else
											{
												netconn_close(conn);
												netconn_delete(conn);
												goto CONNECTING;
											}
											
							}
		 
							netbuf_data(inbuf, (void**)&buf, &buflen);
							memset(bufer, 0,200);
							memcpy(bufer,buf,buflen); // Ну тут кароч строку в масссив переводим, чтоб дальше проще было работать
							netbuf_delete(inbuf);

							 if(bufer[7]==0x04)  
										{
										j=9;
											//ad=expect_add4;
											ad = ThisDev.QI;
												for(n=0; n<bufer[8];n+=2)
														{
														dat=bufer[j++]; dat<<=8;       // H
														dat|=bufer[j++];               // L
														ThisDev.Input[ad++]=dat;
														}
														ThisDev.QI = ad;
										}
							
								osDelay(10);//
							}
						}
					osDelay(ThisDev.time);
				}
				
			}
			osDelay(500);
		}
	
	
	
}


/*
Функция-задача, которая формирует структуры для устройств RS485.
*/
void MSlaveTask(struct device *dev)
{
	struct devices ThisDev;
	char* token_buff;	
	unsigned short int a,b,c, time;
	u32_t ad1,ad2;
	
	StructQwAdr[NumQw] = pvPortMalloc(sizeof(ThisDev));
	
	memset(StructQwAdr[NumQw],0,sizeof(ThisDev));

	StructQwAdr[NumQw]->addr = dev->addr;
	StructQwAdr[NumQw]->interface = dev->interface;
	StructQwAdr[NumQw]->log = dev->log;
	StructQwAdr[NumQw]->type3 = dev->type3;
	StructQwAdr[NumQw]->type4 = dev->type4;
	StructQwAdr[NumQw]->time = dev->time;
	StructQwAdr[NumQw]->QH = 0;
	StructQwAdr[NumQw]->QI = 0;

	time = StructQwAdr[NumQw]->time/2;

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	
	if(StructQwAdr[NumQw]->type3 == 3)
	{
	strcpy(StructQwAdr[NumQw]->regsH, dev->regs);
	strcpy(StructQwAdr[NumQw]->floatsH, dev->floats);
		
	StructQwAdr[NumQw]->NumQH = ParsingAddr(StructQwAdr[NumQw]->regsH, StructQwAdr[NumQw]->PatternH);
	StructQwAdr[NumQw]->sizeH = 0;
		// тут надо посчитать, сколько регистров вообще запрашиваем
		for(int i=0;i<StructQwAdr[NumQw]->NumQH;i++)
		{
			StructQwAdr[NumQw]->sizeH = StructQwAdr[NumQw]->sizeH + (StructQwAdr[NumQw]->PatternH[i][1]-StructQwAdr[NumQw]->PatternH[i][0]) + 1;
		}
	}
	
		if(StructQwAdr[NumQw]->type4 == 4)
			{
			strcpy(StructQwAdr[NumQw]->regsI, dev->regs);
			strcpy(StructQwAdr[NumQw]->floatsI, dev->floats);
			StructQwAdr[NumQw]->NumQI = ParsingAddr(StructQwAdr[NumQw]->regsI, StructQwAdr[NumQw]->PatternI);
			StructQwAdr[NumQw]->sizeI = 0;
				// тут надо посчитать, сколько регистров вообще запрашиваем
				for(int i=0;i<StructQwAdr[NumQw]->NumQI;i++)
				{
					StructQwAdr[NumQw]->sizeI = StructQwAdr[NumQw]->sizeI + (StructQwAdr[NumQw]->PatternI[i][1]-StructQwAdr[NumQw]->PatternI[i][0]) + 1;
				}
			}
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	NumQw++;
		//Если надо логировать, запускаем лог
			if(StructQwAdr[NumQw-1]->log)
				{
					CreateLOG(StructQwAdr[NumQw-1]);
				}
			
			
			
			if(LOADING)
				{
					osSemaphoreRelease(SemSaveDev);
				}
}

/*
Планировщик запросов.
Отправляет запросы на основе сгенерених ранее структур.
*/
void MBusMasterPlaner()
{
		osStatus result = osOK;
	for(;;)
	{
	if(NumQw>0)
	{
		for(int j=0;j<NumQw;j++)
		{
			if((StructQwAdr[j]->type3==3)&&(StructQwAdr[j]->interface==0))
			{
					for(int i = 0;i<StructQwAdr[j]->NumQH;i++)
						{
							result = osSemaphoreWait(SemPlan, Timeout);
							
							TimeoutCounter = 0;
							if(i!=0)
							{
								StructQwAdr[j]->QH = StructQwAdr[j]->QH + StructQwAdr[j]->PatternH[i-1][1] - StructQwAdr[j]->PatternH[i-1][0] + 1;
							}
							else
							{
								StructQwAdr[j]->QH=0;
							}
							
							if(NumQw==0) 
							{
								break;
							}

								zapros_M(StructQwAdr[j]->addr,StructQwAdr[j]->type3,StructQwAdr[j]->PatternH[i][0],StructQwAdr[j]->PatternH[i][1]);
								osDelay(50);
								WhichDevNowInPlaner = StructQwAdr[j];
								if(StructQwAdr[j]->Packets>0x7fffffff)
								{
									StructQwAdr[j]->Packets=0;
								}
								StructQwAdr[j]->Packets++;
						}
			}
					
							
			if((StructQwAdr[j]->type4==4)&&(StructQwAdr[j]->interface==0))
			{
				for(int i = 0;i<StructQwAdr[j]->NumQI;i++)
					{
						result = osSemaphoreWait(SemPlan, Timeout);
						
						TimeoutCounter = 0;
							if(i!=0)
							{
								StructQwAdr[j]->QI = StructQwAdr[j]->QI + StructQwAdr[j]->PatternI[i-1][1] - StructQwAdr[j]->PatternI[i-1][0] + 1;
							}
							else
							{
								StructQwAdr[j]->QI=0;
							}
						
							if(NumQw==0)  
							{
								break;
							}
							
								zapros_M(StructQwAdr[j]->addr,StructQwAdr[j]->type4,StructQwAdr[j]->PatternI[i][0],StructQwAdr[j]->PatternI[i][1]);
								osDelay(50);
								WhichDevNowInPlaner = StructQwAdr[j];
								if(StructQwAdr[j]->Packets>0x7fffffff)
								{
									StructQwAdr[j]->Packets=0;
								}
								StructQwAdr[j]->Packets++;
					}
			}
		}

	}
	else
		{
			PlanerStatus = 0;
			osThreadTerminate(NULL);
			osDelay(1000);
		}
	}
	
}

/*
Эта штука создает таск для логирования
*/
int CreateLOG(struct devices *logdev)
{
//836
//////////////////////////////////////////////////////////////////////////////////////////////////////////
	RTC_DateTypeDef sdatestructureget;
  RTC_TimeTypeDef stimestructureget;
	
	
char p=0;
	
	if(logdev->interface==0)
	{
	sprintf(logdev->Adapter,"Adapter;\"1;;COM1;115200;0;1;8;50;50;50\"");
	}
	else if(logdev->interface==1)
	{
		sprintf(logdev->Adapter,"Adapter;\"0;;%d.%d.%d.%d;%d;4500;4500\"",logdev->ipadd[0],logdev->ipadd[1],logdev->ipadd[2],logdev->ipadd[3],logdev->port);
	}
	
	logdev->devN = logdev->addr;

	logdev->filename = &logdev->path[0];
	
	
refresh:

	logdev->time = logdev->time;

	memset(logdev->path,0,50);
	memset(logdev->fl,0,sizeof(logdev->fl));	// Номера float регистров
	logdev->NumF = 0;
	logdev->NumP = 0;									// Число float регистров
	
	memset(logdev->fl2,0,sizeof(logdev->fl2));	// Номера float регистров
	logdev->NumF2 = 0;
	logdev->NumP2 = 0;									// Число float регистров
	
	p=0;

if((logdev->type3 == 3) && (strlen(logdev->floatsH)!=0))
{
	logdev->NumP = ParsingAddr(logdev->floatsH, logdev->PL);
	for(int i=0;i<logdev->NumP;i++)
	{
		for(int j=logdev->PL[i][0];j<logdev->PL[i][1]+1;j++)
		{
			logdev->fl[logdev->NumF] = j;
			logdev->NumF++;
		}
	}
}

// тут надо проверить, если опрос создали, а надо флот записывать, входит ли второй регистр в опрос
for(int i=0; i<logdev->NumF; i++)
{
	for(int j=0; j<logdev->NumQH; j++)
	{
		if((logdev->fl[i]>=logdev->PatternH[j][0]) && (logdev->fl[i]<=logdev->PatternH[j][1]))
		{
			int k = logdev->PatternH[j][1] - logdev->PatternH[j][0]+1;
			if(k>1)
			{
				for(int o=logdev->PatternH[j][0];o<=logdev->PatternH[j][1];o++)
				{
					if((logdev->fl[i]==o) && (o == logdev->PatternH[j][1]))
					{
						logdev->PatternH[j][1]++;
						logdev->sizeH++;
					}
				}
			}
			else if(k==1)
			{
				if((logdev->fl[i]==logdev->PatternH[j][1])&&(logdev->fl[i]==logdev->PatternH[j][0]))
				{
					logdev->PatternH[j][1]++;
					logdev->sizeH++;
				}
			}
			break;
		}
	}
}



if(logdev->type4 == 4 && (strlen(logdev->floatsI)!=0))
{
	logdev->NumP2 = ParsingAddr(logdev->floatsI, logdev->PL2);
	for(int i=0;i<logdev->NumP2;i++)
	{
		for(int j=logdev->PL2[i][0];j<logdev->PL2[i][1]+1;j++)
		{
			logdev->fl2[logdev->NumF2] = j;
			logdev->NumF2++;
		}
	}
}


// тут надо проверить, если опрос создали, а надо флот записывать, входит ли второй регистр в опрос
for(int i=0; i<logdev->NumF2; i++)
{
	for(int j=0; j<logdev->NumQI; j++)
	{
		if((logdev->fl2[i]>=logdev->PatternI[j][0]) && (logdev->fl2[i]<=logdev->PatternI[j][1]))
		{
			int k = logdev->PatternI[j][1] - logdev->PatternI[j][0]+1;
			if(k>1)
			{
				for(int o=logdev->PatternI[j][0];o<=logdev->PatternI[j][1];o++)
				{
					if((logdev->fl2[i]==o) && (o == logdev->PatternI[j][1]))
					{
						logdev->PatternI[j][1]++;
						logdev->sizeI++;
					}
				}
			}
			else if(k==1)
			{
				if((logdev->fl2[i]==logdev->PatternI[j][1])&&(logdev->fl2[i]==logdev->PatternI[j][0]))
				{
					logdev->PatternI[j][1]++;
					logdev->sizeI++;
				}
			}
			break;
		}
	}
}


logdev->N = logdev->sizeH + logdev->sizeI;

logdev->timecounter = logdev->time;
logdev->log = 1;


logdev->filestatus = 0;

if(logdev->log == 1)
{
	//logdev->file1 = pvPortMalloc(sizeof(FIL));
	logdev->file1 = 0;
	logdev->file1 = pvPortMalloc(sizeof(FIL));
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////	
	if(!LOGTask)
	{
		if(xTaskCreate((pdTASK_CODE)WriteLOG,
										 "LOG",
										 1024,
										 logdev,
										 osPriorityHigh,
										 NULL
										)==pdTRUE)
				{
					osDelay(50);
					LOGTask = 1;
				}
				
	}
}



#define SIZELOGBUF 1200
	char strbuf[SIZELOGBUF] = {0};

	char* Adresses = "Adresses";
	char* Adapters = "Adapters";
	char* DevAdresses = "DevAdresses";
	char* Formats = "Formats";
	char* Types = "Types";
	char* Names = "Names";
	char* R = ";";
	char* end = "\r\n";
	unsigned int Open1TimeCount = 0, LseekTimeCount = 0, ReadTimeCount = 0, SendTimeCount = 0;

	//FIL file1;
	//FIL* file1;
	
/*
Вот эта штука и пишет лог
*/
static void WriteLOG(void const * argument)
{
	RTC_DateTypeDef sdatestructureget;
  RTC_TimeTypeDef stimestructureget;
	FRESULT f_err;
	unsigned int p = 0, b, c;
	unsigned char NeedtoLogSomething = 0;
	
	char buf[100];
while(!LOADING)
{
	osDelay(1000);
}
	
	while(1)
	{
		if(NumQw>0)
		{
			for(int d = 0;d < NumQw;d++)
			{
				if(StructQwAdr[d]->log > 0)
				{
					NeedtoLogSomething = 1;
					break;
				}
				else
				{
					NeedtoLogSomething = 0;
				}
			}
		}
		else
		{
			NeedtoLogSomething = 0;
		}
		
		if(NeedtoLogSomething)
		{
			for(int d = 0;d < NumQw;d++) // Перебираем устройства
			{
				if((StructQwAdr[d]->log > 0) && (StructQwAdr[d]->timecounter == 0)) // Если надо логировать и пора
				{
					osSemaphoreWait(SemFiles, portMAX_DELAY);
					StructQwAdr[d]->timecounter = StructQwAdr[d]->time;
					if(StructQwAdr[d]->log == 3)
					{
							taskENTER_CRITICAL();
						f_err = f_close(StructQwAdr[d]->file1);
							taskEXIT_CRITICAL();
						
						StructQwAdr[d]->filestatus = 0;
					}
					
					if(StructQwAdr[d]->filestatus == 0) // Если файл не открыт, то открываем и пишем шапку
					{
						HAL_RTC_GetTime(&hrtc, &stimestructureget, RTC_FORMAT_BIN);
						HAL_RTC_GetDate(&hrtc, &sdatestructureget, RTC_FORMAT_BIN);
						
						StructQwAdr[d]->startH = stimestructureget.Hours + (CutTime/60);
						StructQwAdr[d]->startM = stimestructureget.Minutes + CutTime%60;
						
						if(StructQwAdr[d]->startM > 59)
						{
							StructQwAdr[d]->startM = StructQwAdr[d]->startM - 60;
							StructQwAdr[d]->startH++;
						}
						
						if(StructQwAdr[d]->startH>23)
						{
							StructQwAdr[d]->startH = StructQwAdr[d]->startH - 24;
						}
						
						//Генерим имя файла в формате LogD<номер девайса>_<ддммгг>_<ччмм>.csv
							memset(StructQwAdr[d]->path,0,50);
							if(StructQwAdr[d]->interface == 0)
							{
								sprintf(StructQwAdr[d]->path,"LogD%d_%02d%02d%02d_%02d%02d.csv",StructQwAdr[d]->devN, sdatestructureget.Date, sdatestructureget.Month, sdatestructureget.Year, stimestructureget.Hours, stimestructureget.Minutes);
							}
							else if(StructQwAdr[d]->interface == 1)
							{
								sprintf(StructQwAdr[d]->path,"LogD%d_%d_%d_%d-%d-%02d%02d%02d_%02d%02d.csv",StructQwAdr[d]->ipadd[0],StructQwAdr[d]->ipadd[1],StructQwAdr[d]->ipadd[2],StructQwAdr[d]->ipadd[3], StructQwAdr[d]->port, sdatestructureget.Date, sdatestructureget.Month, sdatestructureget.Year, stimestructureget.Hours, stimestructureget.Minutes);
							}
							
							
							if(SDFatFs.fs_type != 0) // Файловая система доступна
							{
								
								if(StructQwAdr[d]->file1 != 0)
								{
									taskENTER_CRITICAL();
								f_err = f_open(StructQwAdr[d]->file1, StructQwAdr[d]->path, FA_OPEN_ALWAYS | FA_WRITE); // открываем файл
									taskEXIT_CRITICAL();	
								}
								if(f_err != FR_OK)
								{
									FileErr++;
								}
								if(f_err == FR_OK) // Удачно открылось
								{
									StructQwAdr[d]->filestatus = 1;

			////////////////////////////////////////////////////Начало шапки//////////////////////////////////////////////////////////
			//
			//osDelay(50);
					
										memset(strbuf,0,SIZELOGBUF);
										strcat(strbuf,StructQwAdr[d]->Adapter);
										for(int i=0;i<StructQwAdr[d]->N-1-StructQwAdr[d]->NumF-StructQwAdr[d]->NumF2;i++)
											{
												strcat(strbuf,R);
											}
										strcat(strbuf,end);
															
										p = 0;
										strcat(strbuf,Adresses);
											for(int i=0;i<StructQwAdr[d]->NumQH;i++)
											{
												for(int j=StructQwAdr[d]->PatternH[i][0];j<=StructQwAdr[d]->PatternH[i][1];j++)
												{
													if(StructQwAdr[d]->fl[p]==j)
													{
														sprintf(buf,";%d",j-1);
														j++;
														p++;
													}
													else
													{
													sprintf(buf,";%d",j-1);
													}
													strcat(strbuf,buf); 
																					
												}
											}
											
											p = 0;
											for(int i=0;i<StructQwAdr[d]->NumQI;i++)
											{
												for(int j=StructQwAdr[d]->PatternI[i][0];j<=StructQwAdr[d]->PatternI[i][1];j++)
												{
													if(StructQwAdr[d]->fl2[p]==j)
													{
														sprintf(buf,";%d",j-1);
														j++;
														p++;
													}
													else
													{
													sprintf(buf,";%d",j-1);
													}
													 
													strcat(strbuf,buf); 
												}
											}
										strcat(strbuf,end); 

										strcat(strbuf,Adapters); 		 
										for(int i=0;i<StructQwAdr[d]->N-StructQwAdr[d]->NumF-StructQwAdr[d]->NumF2;i++)
											{
												sprintf(buf,";0");
												 strcat(strbuf,buf); 
											}
										strcat(strbuf,end); 
							 
										strcat(strbuf,DevAdresses);	  
										for(int i=0;i<StructQwAdr[d]->N-StructQwAdr[d]->NumF-StructQwAdr[d]->NumF2;i++)
											{
												sprintf(buf,";%d",StructQwAdr[d]->devN);
												 strcat(strbuf,buf);
											}
										strcat(strbuf,end);
											
															
									p=0;
									strcat(strbuf,Formats);					
											for(int i=0;i<StructQwAdr[d]->NumQH;i++)
											{
												for(int j=StructQwAdr[d]->PatternH[i][0];j<=StructQwAdr[d]->PatternH[i][1];j++)
												{
													if(StructQwAdr[d]->fl[p]==j)
														{
															sprintf(buf,";1");
															p++;
															j++;
														}
														else
														{
															sprintf(buf,";0");
														}
												
														 strcat(strbuf,buf);	
								
												}
											}
											
											p = 0;
											for(int i=0;i<StructQwAdr[d]->NumQI;i++)
											{
												for(int j=StructQwAdr[d]->PatternI[i][0];j<=StructQwAdr[d]->PatternI[i][1];j++)
												{
													if(StructQwAdr[d]->fl2[p]==j)
														{
															sprintf(buf,";1");
															p++;
															j++;
														}
														else
														{
															sprintf(buf,";0");
														}

														strcat(strbuf,buf);		  
												}
											}
									strcat(strbuf,end);
									
									strcat(strbuf,Types);	  
										for(int i=0;i<StructQwAdr[d]->sizeH-StructQwAdr[d]->NumF;i++)
											{
												sprintf(buf,";2");
											 strcat(strbuf,buf);	
											}
										for(int i=0;i<StructQwAdr[d]->sizeI-StructQwAdr[d]->NumF2;i++)
											{
												sprintf(buf,";3");
												 strcat(strbuf,buf);  
											}	 
									strcat(strbuf,end);
											
									strcat(strbuf,Names);		 
										for(int i=0;i<StructQwAdr[d]->N-StructQwAdr[d]->NumF-StructQwAdr[d]->NumF2;i++)
											{
												 strcat(strbuf,R); 
											}
									strcat(strbuf,end);
									strcat(strbuf,end);
											
											
											taskENTER_CRITICAL();
										f_err = f_write(StructQwAdr[d]->file1, strbuf, strlen(strbuf), &c);
										f_err = f_sync(StructQwAdr[d]->file1);
											taskEXIT_CRITICAL();	
										
											osDelay(500);
								}
							}
					}
					else
					{
					
					// Тут пишем новые данные в открытом файле
						
						HAL_RTC_GetTime(&hrtc, &stimestructureget, RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc, &sdatestructureget, RTC_FORMAT_BIN);

			u32_t Ms;
			Ms = 1000*(stimestructureget.SecondFraction-stimestructureget.SubSeconds)/(stimestructureget.SecondFraction+1); // Вычисляем милисекунды
					
					memset(strbuf,0,SIZELOGBUF);
					
				sprintf(buf,"%02d.%02d.%02d",sdatestructureget.Date, sdatestructureget.Month, 2000 + sdatestructureget.Year);
					strcat(strbuf,buf);
				sprintf(buf," %02d:%02d:%02d.%03d",stimestructureget.Hours, stimestructureget.Minutes, stimestructureget.Seconds, Ms);
					strcat(strbuf,buf);

										
					
					float holdf;
					int hi,k;
						p = 0;
						k = 0;
					
								for(int i=0;i<StructQwAdr[d]->NumQH;i++)
								{
									for(int j=StructQwAdr[d]->PatternH[i][0];j<=StructQwAdr[d]->PatternH[i][1];j++)
									{
										if(StructQwAdr[d]->fl[p]==j)
											{
												if(swFloat)
												{
													hi=0;
													hi = StructQwAdr[d]->Holding[k];
													j++;
													k++;
													hi += StructQwAdr[d]->Holding[k]<<16;
													holdf = *((float*)&hi);
													//memcpy(&holdf, &hi, sizeof holdf);
													sprintf(buf,";%f",holdf);
													*strchr(buf,'.') = ',';
													strcat(strbuf,buf);
													p++;
													k++;
												}
												else
												{
													hi=0;
													hi = StructQwAdr[d]->Holding[k]<<16;
													j++;
													k++;
													hi += StructQwAdr[d]->Holding[k];
													holdf = *((float*)&hi);
													//memcpy(&holdf, &hi, sizeof holdf);
													sprintf(buf,";%f",holdf);
													*strchr(buf,'.') = ',';
													strcat(strbuf,buf);
													p++;
													k++;
												}
											}
											else
											{
												sprintf(buf,";%d",StructQwAdr[d]->Holding[k]);
												strcat(strbuf,buf);
											
												k++;
											}
									}
								}
								
																				
								p = 0;
								k = 0;
																
								for(int i=0;i<StructQwAdr[d]->NumQI;i++)
								{
									for(int j=StructQwAdr[d]->PatternI[i][0];j<=StructQwAdr[d]->PatternI[i][1];j++)
									{
										if(StructQwAdr[d]->fl2[p]==j)
											{
												if(swFloat)
												{
													hi=0;
													holdf = 0.0;
													hi = StructQwAdr[d]->Input[k];
													j++;
													k++;
													hi += StructQwAdr[d]->Input[k]<<16;
													holdf = *((float*)&hi);																														
													//memcpy(&holdf, &hi, sizeof holdf);													
													sprintf(buf,";%f",holdf);																		
													*strchr(buf,'.') = ',';
													strcat(strbuf,buf);																							
													p++;
													k++;
												}
												else
												{
													hi=0;
													holdf = 0.0;
													hi = StructQwAdr[d]->Input[k]<<16;
													j++;
													k++;
													hi += StructQwAdr[d]->Input[k];
													holdf = *((float*)&hi);																																																				
													//memcpy(&holdf, &hi, sizeof holdf);													
													sprintf(buf,";%f",holdf);													
													*strchr(buf,'.') = ',';
													strcat(strbuf,buf);																							
													p++;
													k++;
												}
												
											}
											else
											{
												sprintf(buf,";%d",StructQwAdr[d]->Input[k]);
													strcat(strbuf,buf);
												k++;
											}
									}
								}

					strcat(strbuf,end);
								
								
							taskENTER_CRITICAL();
						f_err = f_lseek(StructQwAdr[d]->file1, f_size(StructQwAdr[d]->file1));
						f_err = f_write(StructQwAdr[d]->file1, strbuf, strlen(strbuf), &c);

						f_err = f_sync(StructQwAdr[d]->file1);
							taskEXIT_CRITICAL();
								
								
								
								
						if((stimestructureget.Hours==StructQwAdr[d]->startH)&&(stimestructureget.Minutes>=StructQwAdr[d]->startM)) // Если файл пора перезаписывать
						{
								taskENTER_CRITICAL();
							f_err = f_close(StructQwAdr[d]->file1);
								taskEXIT_CRITICAL();
							StructQwAdr[d]->filestatus = 0;
						}
				}
					osSemaphoreRelease(SemFiles);
				}
			}
			
			osDelay(50); // Инетрвал между циклами
			for(int d = 0;d < NumQw;d++)
			{
				if(StructQwAdr[d]->log)
				{
					if(StructQwAdr[d]->timecounter < 10) // Если до след записи осталось меньше 10 мс, то считаем, что пора записывать, иначае дикрементируем счетчик на 10 мс
					{
						StructQwAdr[d]->timecounter = 0;
					}
					else
					{
						StructQwAdr[d]->timecounter = StructQwAdr[d]->timecounter - 50; 
					}
				}
				
			}
			
			
		}
		else
		{
			LOGTask = 0;
			osThreadTerminate(NULL);
		}
	}
}

/*
Функция кодирования win1251 to utf8
*/
void win1251_to_utf8 (unsigned char win[17][51],unsigned char utf[17][100])
{
	unsigned short int buf,a=53200,b=53392,len;
	unsigned char k = 0;
	
	for(int i =0;i<17;i++)
	{
		len = strlen(&win[i][0]);
		memset(&utf[i][0], 0,200);
		k = 0;
		for(int j =0;j<len;j++)
		{
			if(win[i][j]>=192&&win[i][j]<=239)
			{
				buf = win[i][j]+a;
				utf[i][k] = 0;
				utf[i][k] = (buf & 0xff00)>>8;
				k++;
				utf[i][k] = buf & 0x00ff;
				k++;
			}
			else if(win[i][j]>=240&&win[i][j]<=255)
			{
				buf = win[i][j]+b;
				utf[i][k] = 0;
				utf[i][k] = (buf & 0xff00)>>8;
				k++;
				utf[i][k] = buf & 0x00ff;
				k++;
			}
			else
			{
				utf[i][k] = win[i][j];
				k++;
			}
			
		}
	}
	
}





 /*
Непонятная структура взятая с сайта.
Нужна для кодирования. Мб не полная.
*/
static Letter g_letters[] = {
        {0x82, 0x201A}, // SINGLE LOW-9 QUOTATION MARK
        {0x83, 0x0453}, // CYRILLIC SMALL LETTER GJE
        {0x84, 0x201E}, // DOUBLE LOW-9 QUOTATION MARK
        {0x85, 0x2026}, // HORIZONTAL ELLIPSIS
        {0x86, 0x2020}, // DAGGER
        {0x87, 0x2021}, // DOUBLE DAGGER
        {0x88, 0x20AC}, // EURO SIGN
        {0x89, 0x2030}, // PER MILLE SIGN
        {0x8A, 0x0409}, // CYRILLIC CAPITAL LETTER LJE
        {0x8B, 0x2039}, // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
        {0x8C, 0x040A}, // CYRILLIC CAPITAL LETTER NJE
        {0x8D, 0x040C}, // CYRILLIC CAPITAL LETTER KJE
        {0x8E, 0x040B}, // CYRILLIC CAPITAL LETTER TSHE
        {0x8F, 0x040F}, // CYRILLIC CAPITAL LETTER DZHE
        {0x90, 0x0452}, // CYRILLIC SMALL LETTER DJE
        {0x91, 0x2018}, // LEFT SINGLE QUOTATION MARK
        {0x92, 0x2019}, // RIGHT SINGLE QUOTATION MARK
        {0x93, 0x201C}, // LEFT DOUBLE QUOTATION MARK
        {0x94, 0x201D}, // RIGHT DOUBLE QUOTATION MARK
        {0x95, 0x2022}, // BULLET
        {0x96, 0x2013}, // EN DASH
        {0x97, 0x2014}, // EM DASH
        {0x99, 0x2122}, // TRADE MARK SIGN
        {0x9A, 0x0459}, // CYRILLIC SMALL LETTER LJE
        {0x9B, 0x203A}, // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
        {0x9C, 0x045A}, // CYRILLIC SMALL LETTER NJE
        {0x9D, 0x045C}, // CYRILLIC SMALL LETTER KJE
        {0x9E, 0x045B}, // CYRILLIC SMALL LETTER TSHE
        {0x9F, 0x045F}, // CYRILLIC SMALL LETTER DZHE
        {0xA0, 0x00A0}, // NO-BREAK SPACE
        {0xA1, 0x040E}, // CYRILLIC CAPITAL LETTER SHORT U
        {0xA2, 0x045E}, // CYRILLIC SMALL LETTER SHORT U
        {0xA3, 0x0408}, // CYRILLIC CAPITAL LETTER JE
        {0xA4, 0x00A4}, // CURRENCY SIGN
        {0xA5, 0x0490}, // CYRILLIC CAPITAL LETTER GHE WITH UPTURN
        {0xA6, 0x00A6}, // BROKEN BAR
        {0xA7, 0x00A7}, // SECTION SIGN
        {0xA8, 0x0401}, // CYRILLIC CAPITAL LETTER IO
        {0xA9, 0x00A9}, // COPYRIGHT SIGN
        {0xAA, 0x0404}, // CYRILLIC CAPITAL LETTER UKRAINIAN IE
        {0xAB, 0x00AB}, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
        {0xAC, 0x00AC}, // NOT SIGN
        {0xAD, 0x00AD}, // SOFT HYPHEN
        {0xAE, 0x00AE}, // REGISTERED SIGN
        {0xAF, 0x0407}, // CYRILLIC CAPITAL LETTER YI
        {0xB0, 0x00B0}, // DEGREE SIGN
        {0xB1, 0x00B1}, // PLUS-MINUS SIGN
        {0xB2, 0x0406}, // CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I
        {0xB3, 0x0456}, // CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I
        {0xB4, 0x0491}, // CYRILLIC SMALL LETTER GHE WITH UPTURN
        {0xB5, 0x00B5}, // MICRO SIGN
        {0xB6, 0x00B6}, // PILCROW SIGN
        {0xB7, 0x00B7}, // MIDDLE DOT
        {0xB8, 0x0451}, // CYRILLIC SMALL LETTER IO
        {0xB9, 0x2116}, // NUMERO SIGN
        {0xBA, 0x0454}, // CYRILLIC SMALL LETTER UKRAINIAN IE
        {0xBB, 0x00BB}, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
        {0xBC, 0x0458}, // CYRILLIC SMALL LETTER JE
        {0xBD, 0x0405}, // CYRILLIC CAPITAL LETTER DZE
        {0xBE, 0x0455}, // CYRILLIC SMALL LETTER DZE
        {0xBF, 0x0457} // CYRILLIC SMALL LETTER YI
};
 
 
 
 
 
 /*
Функция кодирования utf8 to win1251
*/
int convert_utf8_to_windows1251(char utf8[300], char windows1251[300])
{
        int i = 0;
        int j = 0,n;
	char buf[300]={0};
	
	//memset(windows1251,0,300);
		n = strlen(utf8);
	
	for(i=0;i<n;i++)
	{
		if(utf8[i]=='%')
		{
			
			i++;

				switch (utf8[i])
				{
					
				case 'D':
				buf[j]=0x0D<<4;
				break;
					
				case 'B':
				buf[j]=0x0B<<4;
				break;
					
				case 'A':
				buf[j]=0x0A<<4;
				break;
					
				case '9':
				buf[j]=0x09<<4;
				break;
					
				case '8':
				buf[j]=0x08<<4;
				break;
				
				case '2':
				buf[j]=0x02<<4;
				break;

				}
				
				i++;
				
				
				switch (utf8[i])
				{
					
				case '0':
				buf[j]|=0x00;
				break;
					
				case '1':
				buf[j]|=0x01;
				break;
					
				case '2':
				buf[j]|=0x02;
				break;
					
				case '3':
				buf[j]|=0x03;
				break;
					
				case '4':
				buf[j]|=0x04;
				break;
					
				case '5':
				buf[j]|=0x05;
				break;
				
				case '6':
				buf[j]|=0x06;
				break;
				
				case '7':
				buf[j]|=0x07;
				break;
								
				case '8':
				buf[j]|=0x08;
				break;
												
				case '9':
				buf[j]|=0x09;
				break;
																
				case 'A':
				buf[j]|=0x0A;
				break;
																				
				case 'B':
				buf[j]|=0x0B;
				break;
																								
				case 'C':
				buf[j]|=0x0C;
				break;
																												
				case 'D':
				buf[j]|=0x0D;
				break;
																																
				case 'E':
				buf[j]|=0x0E;
				break;
				
				case 'F':
				buf[j]|=0x0F;
				break;
				}
				j++;
			}
		else if(utf8[i]=='+')
		{
				buf[j] = 0x20;
			j++;
		}
		else
		{
			buf[j]=utf8[i];
			j++;
		}
			
	}
		
	// Далее идет кусок взятый и интернетов
	n = strlen(buf);
		i=0;
		j=0;
		
        for(i=0; i < (int)n && buf[i] != 0; ++i) {
                char prefix = buf[i];
                char suffix = buf[i+1];
                if ((prefix & 0x80) == 0) {
                        windows1251[j] = (char)prefix;
                        ++j;
                } else if ((~prefix) & 0x20) {
                        int first5bit = prefix & 0x1F;
                        first5bit <<= 6;
                        int sec6bit = suffix & 0x3F;
                        int unicode_char = first5bit + sec6bit;
 
 
                        if ( unicode_char >= 0x410 && unicode_char <= 0x44F ) {
                                windows1251[j] = (char)(unicode_char - 0x350);
                        } else if (unicode_char >= 0x80 && unicode_char <= 0xFF) {
                                windows1251[j] = (char)(unicode_char);
                        } else if (unicode_char >= 0x402 && unicode_char <= 0x403) {
                                windows1251[j] = (char)(unicode_char - 0x382);
                        } else {
                                int count = sizeof(g_letters) / sizeof(Letter);
                                for (int k = 0; k < count; ++k) {
                                        if (unicode_char == g_letters[k].unicode) {
                                                windows1251[j] = g_letters[k].win1251;
                                                goto NEXT_LETTER;
                                        }
                                }
                                // can't convert this char
                                return 0;
                        }
NEXT_LETTER:
                        ++i;
                        ++j;
                } else {
                        // can't convert this chars
                        return 0;
                }
        }
        windows1251[j] = 0;
        return 1;
}


 /*
Функция упаковки WEB utf8 в char utf8
*/
int convert_utf8(char utf8w[300], char utf8[25])
{
        int i = 0;
        int j = 0,n;
	char buf[25]={0};
	
	memset(utf8,0,25);
		n = strlen(utf8w);
	
	for(i=0;i<n;i++)
	{
		if(utf8w[i]=='%')
		{
			
			i++;

				switch (utf8w[i])
				{
					
				case 'D':
				buf[j]=0x0D<<4;
				break;
					
				case 'B':
				buf[j]=0x0B<<4;
				break;
					
				case 'A':
				buf[j]=0x0A<<4;
				break;
					
				case '9':
				buf[j]=0x09<<4;
				break;
					
				case '8':
				buf[j]=0x08<<4;
				break;
				
				case '2':
				buf[j]=0x02<<4;
				break;
				
				case '3':
				buf[j]=0x03<<4;
				break;

				}
				
				i++;
				
				
				switch (utf8w[i])
				{
					
				case '0':
				buf[j]|=0x00;
				break;
					
				case '1':
				buf[j]|=0x01;
				break;
					
				case '2':
				buf[j]|=0x02;
				break;
					
				case '3':
				buf[j]|=0x03;
				break;
					
				case '4':
				buf[j]|=0x04;
				break;
					
				case '5':
				buf[j]|=0x05;
				break;
				
				case '6':
				buf[j]|=0x06;
				break;
				
				case '7':
				buf[j]|=0x07;
				break;
								
				case '8':
				buf[j]|=0x08;
				break;
												
				case '9':
				buf[j]|=0x09;
				break;
																
				case 'A':
				buf[j]|=0x0A;
				break;
																				
				case 'B':
				buf[j]|=0x0B;
				break;
																								
				case 'C':
				buf[j]|=0x0C;
				break;
																												
				case 'D':
				buf[j]|=0x0D;
				break;
																																
				case 'E':
				buf[j]|=0x0E;
				break;
				
				case 'F':
				buf[j]|=0x0F;
				break;
				}
				j++;
			}
		else if(utf8w[i]=='+')
		{
				buf[j] = 0x20;
			j++;
		}
		else
		{
			buf[j]=utf8w[i];
			j++;
		}
			
	}
	
strcpy(utf8,buf);
	
}


/*
Супер функция по выгрузке файлов на FTP через 3G
*/

unsigned char buf123[2000];

/*
Ф-я записи файла в память модема
*/
int WriteFileToGSM(char *filename, unsigned int filesize)
{
	char* SFSAopen = "AT^SFSA=\"open\", a:/%s, 2";
	char* SFSAwrite = "AT^SFSA=\"write\",0,1024";
	char* SFSAclose = "AT^SFSA=\"close\", 0";
	char* SFSAstat = "AT^SFSA=\"gstat\"";
	char cmd[50];
	
	SendGSM(SFSAstat, strlen(SFSAstat));
	osDelay(1000);
	sprintf(cmd,"AT^SFSA=\"open\", a:/%s, 8",filename);
	SendGSM(cmd, strlen(cmd));
	osDelay(5000);
	for(int i=0; i<1024; i++)
	{
		memset(data_com3_IN,0,300);
	SendGSM(SFSAwrite, strlen(SFSAwrite));
	while(strstr(data_com3_IN, "CONNECT")==0)
		{
			osDelay(10);
		}
		memset(data_com3_IN,0,300);
		SendGSMTrans(data_com3_OUT,1024);
			while(strstr(data_com3_IN, "^SFSA: 0")==0)
		{
			osDelay(10);
		}
	}
SendGSM(SFSAclose, strlen(SFSAclose));
	
	
	return 0;
}

/*
Ф-я отправки файлом без использования прозрачного (Transparent) режима
*/
int UploadToFTP(char *filename)
{
FIL file2;
	FRESULT f_err;
	char len;
	unsigned char buf2[20];
	char* OK;
	char *cmgf="at+cmgf=1";
	char *cmee = "at+cmee=1";
	char *cfun = "at+cfun=1";
	char *sxrat = "at^sxrat=2";
	char *sist = "at^sist=5";
	char *ate = "ATE0";
	char *scfg = "at+creg=2";
	char *creg = "at+creg?";
	int error = 0;
	
	char cmd[24][50];
	strcpy(&cmd[0][0],"at^sics=5,conType,GPRS0");
	strcpy(&cmd[1][0],"at^sics=5,apn,internet");
	strcpy(&cmd[2][0],"at^sics=5,dns1,8.8.8.8");
	strcpy(&cmd[3][0],"at^sics=5,user,gdata");
	strcpy(&cmd[4][0],"at^sics=5,passwd,gdata");
	strcpy(&cmd[5][0],"at^sics=5,alphabet,0");
	strcpy(&cmd[6][0],"at^sics=5,inactTO,20");
	
	strcpy(&cmd[7][0],"at^siss=5,\"srvType\",\"Ftp\"");
	strcpy(&cmd[8][0],"at^siss=5,\"alphabet\",\"1\"");
	strcpy(&cmd[9][0],"at^siss=5,\"conId\",\"5\"");
	strcpy(&cmd[10][0],"at^siss=5,\"address\",\"ftp://92.53.96.8/nir/Reg\"");
	strcpy(&cmd[11][0],"at^siss=5,\"tcpMR\",\"3\"");
	strcpy(&cmd[12][0],"at^siss=5,\"tcpOT\",\"3000\"");
	strcpy(&cmd[13][0],"at^siss=5,\"user\",\"npptik_nir\"");
	strcpy(&cmd[14][0],"at^siss=5,\"passwd\",\"iF0wUAqhD4o0wuBmtH0J\"");
	strcpy(&cmd[15][0],"at^siss=5,\"path\",\"file:///a:\"");
	strcpy(&cmd[16][0],"at^siss=5,\"files\",\"");
	strcat(&cmd[16][0],filename);
	strcat(&cmd[16][0],"\"");
	strcpy(&cmd[17][0],"at^siss=5,\"cmd\",\"put\"");
	strcpy(&cmd[18][0],"at^siss=5,\"ftMode\",\"bin\"");
	strcpy(&cmd[19][0],"at^siss=5,\"hcMethod\",\"0\"");
	
	strcpy(&cmd[20][0],"at^siso=5");
	strcpy(&cmd[23][0],"at^sisc=5");
	
	SendGSM(cmgf, strlen(cmgf));
	osDelay(500);
	SendGSM(cmee, strlen(cmee));
	osDelay(500);
	SendGSM(ate, strlen(ate));

for(int i=0; i<20; i++) 
	{
	osDelay(200);
	SendGSM(&cmd[i][0], strlen(&cmd[i][0]));
	}
	
	osDelay(1000);
	SendGSM(&cmd[20][0], strlen(&cmd[20][0])); 
	
	
	osDelay(7000);
	unsigned int b=0,c,d=0;
	////GSM_LED_ON;
	
	
	for(;;)
	{
	
		taskENTER_CRITICAL();
	f_err2 = f_open(&file2, filename, FA_READ);
		taskEXIT_CRITICAL();
	
							taskENTER_CRITICAL();
						f_err = f_lseek(&file2, b);
							taskEXIT_CRITICAL();
	
 	if(f_err2 == FR_OK)
	{
		error = 0;
		for(;;)
		{

			//osDelay(1000);
			osSemaphoreWait(SemFiles, portMAX_DELAY);
				taskENTER_CRITICAL();
			f_err2 = f_read(&file2, buf123, 512,&c);
				taskEXIT_CRITICAL();
			osSemaphoreRelease(SemFiles);
			
			
			if(f_err2 != FR_OK)
				{
					error = 100;
					break;
				}
				
				b = b + c;
			
			sprintf((char*)buf2,"at^sisw=5,%d",c);
			SendGSM(buf2, strlen(buf2));
				
				//osDelay(200);

				d=0;
			do{
				osDelay(100);
					if(strstr(data_com3_IN, "ERROR")!=0)
					{
						d=1234;
						break;
					}
					
					OK = strstr(data_com3_IN, "^SISW: 5,");
					
					d++;
					
					if(d>2000)
					{
						//error = 1000;
						break;
					}
				}
				while(OK==0);
			
			if(d==1234) 
			{
				error = 1234;
				break;
			}
			
			
			SendGSM(buf123, c);
//osDelay(200);
			
				d=0;
			do{
				osDelay(100);
					if(strstr(data_com3_IN, "ERROR")!=0)
					{
						d=1234;
						break;
					}
					OK = strstr(data_com3_IN, "OK");
					
						d++;
					if(d>2000)
					{
						//error = 1000;
						break;
					}
				}
				while(OK==0);
				
			if(d==1234) 
			{
				error = 1234;
				break;
			}
			
			if(c<512)
			{
				break;
			}
		}
	}
	else
	{
		error = 50;
	}
	
	
		
	
		taskENTER_CRITICAL();
	f_close(&file2);
		taskEXIT_CRITICAL();
	
		if((error == 0) || (f_size(&file2) == b))
		{
			break;
		}
	
	}
	//GSM_LED_OFF;
	osDelay(1000);

		SendGSM(&cmd[23][0], strlen(&cmd[23][0]));
	return error;
}

/*
Однажды это заработает...
*/
/*
Прототип ф-ии сервера (так-то не очень нужно пока)
*/
void TCP_GSM_Listener()
{
	
		char len;
	unsigned char buf2[20];
	char* OK;
	char *cmgf="at+cmgf=1";
	char *cmee = "at+cmee=1";
	char *cfun = "at+cfun=1";
	char *sxrat = "at^sxrat=2";
	char *sist = "at^sist=5";
	char *ate = "ATE0";
	
		char cmd[24][100];
	strcpy(&cmd[0][0],"at^sics=5,conType,GPRS0");
	strcpy(&cmd[1][0],"at^sics=5,apn,internet");
	strcpy(&cmd[2][0],"at^sics=5,dns1,8.8.8.8");
	strcpy(&cmd[3][0],"at^sics=5,user,gdata");
	strcpy(&cmd[4][0],"at^sics=5,passwd,gdata");
	strcpy(&cmd[5][0],"at^sics=5,alphabet,0");
	strcpy(&cmd[6][0],"at^sics=5,inactTO,20");
	
	strcpy(&cmd[7][0],"at^siss=5,\"srvType\",\"Socket\"");
	strcpy(&cmd[8][0],"at^siss=5,\"conId\",\"5\"");
	strcpy(&cmd[9][0],"at^siss=5,\"address\",\"socktcp://178.47.129.82:502;etx;timer=200\"");
	strcpy(&cmd[10][0],"at^siss=5,\"tcpMR\",\"3\"");
	strcpy(&cmd[11][0],"at^siss=5,\"tcpOT\",\"5555\"");
	strcpy(&cmd[12][0],"at^siss=5,\"hcMethod\",\"0\"");
	strcpy(&cmd[13][0],"at^sisc=5");
		strcpy(&cmd[14][0],"+++");
	
	strcpy(&cmd[20][0],"at^siso=5");
	strcpy(&cmd[21][0],"at^siso=5,1");
	
	SendGSM(cmgf, strlen(cmgf));
	osDelay(1000);
	SendGSM(cfun, strlen(cfun));
	osDelay(1000);
	SendGSM(sxrat, strlen(sxrat));
	osDelay(1000);
	SendGSM(cmee, strlen(cmee));
	osDelay(1000);
	SendGSM(ate, strlen(ate));

for(int i=0; i<13; i++) 
	{
	osDelay(300);
	SendGSM(&cmd[i][0], strlen(&cmd[i][0]));
	}
	
	for(;;)
	{
		osDelay(2000);
		SendGSM(&cmd[20][0], strlen(&cmd[20][0])); 
		
			osDelay(5000);
		SendGSM(sist, strlen(sist));
		
		osDelay(5000);
		
		if(strstr(data_com3_IN, "CONNECT")!=0)
		{
			
		GSMTCP_On = 1;
		
//		xTaskCreate((pdTASK_CODE)ModbusGSM,
//										 "GSM_M_TCP",
//										 256,
//										 NULL,
//										 osPriorityNormal,
//										 NULL);
		
		}
		
		while(GSMTCP_On)
		{
			if(strstr(data_com3_IN, "closed")!=0 || strstr(data_com3_IN, "^SISR: ")!=0)
				{
					GSMTCP_On = 0;
					break;
				}
			osDelay(10000);
		}
		
		SendGSMTrans(&cmd[14][0], strlen(&cmd[14][0]));
		
			osDelay(2000);
		SendGSM(&cmd[13][0], strlen(&cmd[13][0])); 
			osDelay(2000);
		}
}




/*
Ф-я отправки файлов с использованием прозрачного (Transparent) режима
*/
unsigned char TCP_GSM(char *filename)
{
	FIL file2;
	char len, exist = 0;
	unsigned char buf2[20], error = 0, flagE = 0;
	unsigned char ipPASV[4];
	unsigned short int portPASV;
	char* token_buff;
	char* OK;
	char *cmgf="at+cmgf=1";
	char *cmee = "at+cmee=1";
	char *cfun = "at+cfun=1";
	char *sxrat = "at^sxrat=2";
	char *sist = "at^sist=5";
	char *sist3 = "at^sist=3";
	char *ate = "ATE0";
	
	int c, size, ftpsize;
	
	
	char* USER = "USER npptik_nir";
	char* PASS = "PASS iF0wUAqhD4o0wuBmtH0J";
	char* SYST = "SYST";
	char* FEAT = "FEAT";
	char* PASV = "PASV";
	char* BIN = "TYPE I";
	char* QUIT = "QUIT";


		char cmd[24][100];
	
	strcpy(&cmd[7][0],"at^siss=5,\"srvType\",\"Socket\"");
	strcpy(&cmd[8][0],"at^siss=5,\"conId\",\"5\"");
	sprintf(&cmd[9][0],"at^siss=5,\"address\",\"socktcp://%s;etx;timer=200\"",&smsMSG[1][0]);
	strcpy(&cmd[10][0],"at^siss=5,\"tcpMR\",\"3\"");
	strcpy(&cmd[11][0],"at^siss=5,\"tcpOT\",\"5555\"");
	strcpy(&cmd[12][0],"at^siss=5,\"hcMethod\",\"0\"");
	strcpy(&cmd[13][0],"at^sisc=5");
	strcpy(&cmd[14][0],"+++");
	strcpy(&cmd[15][0],"at^siso=3");
	strcpy(&cmd[16][0],"at^sisc=3");
	
	strcpy(&cmd[20][0],"at^siso=5");
	strcpy(&cmd[21][0],"at^siso=5,1");
	
	sprintf(&cmd[17][0],"SIZE %s",filename);
	
		if(strlen(&smsMSG[4][0])!=0)
		{
			sprintf(&cmd[18][0],"CWD %s",&smsMSG[4][0]);
		}
	
	sprintf(&cmd[2][0],"USER %s",&smsMSG[2][0]);
	
	sprintf(&cmd[3][0],"PASS %s",&smsMSG[3][0]);
	
		for(int i=7; i<13; i++) 
		{
			osDelay(300);
			SendGSM(&cmd[i][0], strlen(&cmd[i][0]));
		}

	
	osDelay(1000);
	SendGSM(&cmd[20][0], strlen(&cmd[20][0])); 
	

	osDelay(5000);
	
	TransMode = 5;
	SendGSM(sist, strlen(sist));
	
	
	
	osDelay(3000);
	SendGSM(&cmd[2][0], strlen(&cmd[2][0]));
	
		osDelay(1000);
	SendGSM(&cmd[3][0], strlen(&cmd[3][0]));
	
		osDelay(1000);
	SendGSM(&cmd[18][0], strlen(&cmd[18][0]));
	

	
	osDelay(1000);
	SendGSM(BIN, strlen(BIN));
	
	osDelay(1000);
	SendGSM(PASV, strlen(PASV));
	
	
		osDelay(5000);
		//227 Entering Passive Mode (92,53,96,8,173,1).
		
						strtok_r(data_com3_IN,"(",&token_buff);
						ipPASV[0] = atoi((char*)strtok_r(NULL,",",&token_buff));
						ipPASV[1] = atoi((char*)strtok_r(NULL,",",&token_buff));
						ipPASV[2] = atoi((char*)strtok_r(NULL,",",&token_buff));
						ipPASV[3] = atoi((char*)strtok_r(NULL,",",&token_buff));
						
						portPASV = (atoi((char*)strtok_r(NULL,",",&token_buff))& 0xFF) << 8;
						portPASV |= atoi((char*)strtok_r(NULL,")",&token_buff))& 0xFF;
	
	SendGSMTrans(&cmd[14][0], strlen(&cmd[14][0])); 
	TransMode = 0;
	
	strcpy(&cmd[7][0],"at^siss=3,\"srvType\",\"Socket\"");
	strcpy(&cmd[8][0],"at^siss=3,\"conId\",\"5\"");
	sprintf(&cmd[9][0],"at^siss=3,\"address\",\"socktcp://%d.%d.%d.%d:%d;etx;timer=200\"",ipPASV[0],ipPASV[1],ipPASV[2],ipPASV[3],portPASV);
	strcpy(&cmd[10][0],"at^siss=3,\"tcpMR\",\"3\"");
	strcpy(&cmd[11][0],"at^siss=3,\"tcpOT\",\"5555\"");
	strcpy(&cmd[12][0],"at^siss=3,\"hcMethod\",\"0\"");

	osDelay(1000);
		for(int i=7; i<13; i++)
		{
			osDelay(300);
			SendGSM(&cmd[i][0], strlen(&cmd[i][0]));
		}
	
		
	osDelay(2000);
	TransMode = 5;
	SendGSM(sist, strlen(sist));
	
	
	
		osDelay(8000);
	SendGSM(&cmd[17][0], strlen(&cmd[17][0])); 
		osDelay(5000);
	SendGSM(&cmd[17][0], strlen(&cmd[17][0])); 
		osDelay(5000);
	
	exist = 0;
	
		if(atoi((char*)strtok_r(data_com3_IN," ",&token_buff)) == 550)
		{
			exist = 0;
			sprintf(&cmd[22][0],"STOR %s",filename);
		}
		else
		{
			exist = 1;
			ftpsize = atoi((char*)strtok_r(NULL,"\r",&token_buff));
			sprintf(&cmd[22][0],"APPE %s",filename);
		}
	
	
	osDelay(2000);
	SendGSM(&cmd[22][0], strlen(&cmd[22][0]));
	
	osDelay(5000);
	SendGSMTrans(&cmd[14][0], strlen(&cmd[14][0])); 
	TransMode = 0;
	
	osDelay(2000);
	SendGSM(&cmd[15][0], strlen(&cmd[15][0]));
	
		osDelay(2000);
	osSemaphoreWait(SemSMS, portMAX_DELAY);
	TransMode = 3;
		SendGSM(sist3, strlen(sist3));
	osDelay(1500);
	
	
		if(strcmp(filename,"LogEVENT.csv") == 0)
		{
			// ждем семафор
			osSemaphoreWait(SemEventWork, 20000);
		}
	
	osSemaphoreWait(SemFiles, portMAX_DELAY);
		
				TIM5->CNT = 0;
		Open1TimeCount = 0;
		
		taskENTER_CRITICAL();
	f_err2 = f_open(&file2, filename, FA_READ);
		taskEXIT_CRITICAL();
		
		Open1TimeCount = TIM5->CNT;
		
	osSemaphoreRelease(SemFiles);		
		if(f_err2 == FR_OK)
		{
			size = file2.obj.objsize;
			for(;;)
			{
				if(exist)
				{
					if(size <= ftpsize)
					{
						error = 9;
						break;
					}
					else
					{
						TIM5->CNT = 0;
						
							taskENTER_CRITICAL();
						f_err2 = f_lseek(&file2,ftpsize);
							taskEXIT_CRITICAL();
						
						LseekTimeCount = TIM5->CNT;
						
							if(file2.fptr != ftpsize)
							{
								break;
							}
							
						exist = 0;
					}
				}
				
				if(size != file2.obj.objsize)
				{
					ftpsize = ftpsize;
				}
				
						osSemaphoreWait(SemFiles, portMAX_DELAY);
				TIM5->CNT = 0;
					taskENTER_CRITICAL();
//					HAL_NVIC_DisableIRQ(USART2_IRQn);
				f_err2 = f_read(&file2, buf123, 512,&c);
//					HAL_NVIC_EnableIRQ(USART2_IRQn);
					taskEXIT_CRITICAL();
				ReadTimeCount = TIM5->CNT;
						osSemaphoreRelease(SemFiles);
				
				if(f_err2 == FR_OK)
				{
					TIM5->CNT = 0;
					SendGSMTrans(buf123, c);
					
					SendTimeCount = TIM5->CNT;
					//osDelay(10);

					if(c<512)
					{
						break;
					}
				}
				else
				{
					error = 2;
					break;
				}
				
			}
					taskENTER_CRITICAL();
				f_err2 = f_close(&file2);
					taskEXIT_CRITICAL();
						
		}
		else
		{
			error = 1;
		}

		if(strcmp(filename,"LogEVENT.csv") == 0)
		{
			// отдаем семафор
			osSemaphoreRelease(SemEventWork);
		}
		
	osDelay(5000);
	SendGSMTrans(&cmd[14][0], strlen(&cmd[14][0])); 
		TransMode = 0;
		
		
	osDelay(2000);
	SendGSM(&cmd[16][0], strlen(&cmd[16][0])); 
		
		osDelay(5000);
		osSemaphoreRelease(SemSMS);
		TransMode = 5;
	SendGSM(sist, strlen(sist));
		
		osDelay(8000);
	SendGSM(&cmd[17][0], strlen(&cmd[17][0])); 
		osDelay(5000);
	SendGSM(&cmd[17][0], strlen(&cmd[17][0])); 
		osDelay(5000);
		
		c = 0;
		strtok_r(data_com3_IN," ",&token_buff);
		c = atoi((char*)strtok_r(NULL,"\r",&token_buff));
		
		if(error == 9)
		{
			error = 0;
		}
		else if(size!=c)
		{
			error = 3;
		}
		
	SendGSM(QUIT, strlen(QUIT));
			
			osDelay(5000);
	SendGSMTrans(&cmd[14][0], strlen(&cmd[14][0])); 
		TransMode = 0;
	
	osDelay(2000);
	SendGSM(&cmd[13][0], strlen(&cmd[13][0])); 
		osDelay(2000);
		return error;
}


/*
Обновление
*/
uint8_t CheckUpdates()
{
	uint8_t error = 0;
	DeleteGSMFile("boot.bin");
	
	error = DownloadBoot();
	if(error)
	{
		error = DownloadFromGSMtoSD();
	}

	return error;
}

/*
Удаляет файл с gsm flash
*/
unsigned char DeleteGSMFile(char* filename)
{
	char cmdDel[20] = {0};  
	uint8_t timeout = 100, time = 0;
		sprintf(cmdDel,"AT^SFSA=\"remove\", \"a:/%s\"",filename);
	
	SendGSM(cmdDel, strlen(cmdDel));
	osDelay(1000);
	while(1)
	{
		osDelay(50);
		time++;
		if(strstr(data_com3_IN, "OK")!=0)
		{
			return 1;
		}
		else if(strstr(data_com3_IN, "ERROR")!=0)
		{
			return 0;
		}
		
		if(time > timeout)
		{
			return 2;
		}
	}
}

/*
Загружает прошивку с внешнего ftp на gsm flash
*/
unsigned char DownloadBoot(void)
{
	char *cmgf="at+cmgf=1";
	char *cmee = "at+cmee=1";
	char *cfun = "at+cfun=1";
	char *sxrat = "at^sxrat=2";
	char *sist = "at^sist=5";
	char *ate = "ATE0";
	char *scfg = "at+creg=2";
	char *creg = "at+creg?";
	char err = 0;
	uint32_t timeout = 2*60*1000, time = 0;
	
		char cmd[24][100];
	strcpy(&cmd[0][0],"at^sics=5,conType,GPRS0");
	strcpy(&cmd[1][0],"at^sics=5,apn,internet");
	strcpy(&cmd[2][0],"at^sics=5,dns1,8.8.8.8");
	strcpy(&cmd[3][0],"at^sics=5,user,gdata");
	strcpy(&cmd[4][0],"at^sics=5,passwd,gdata");
	strcpy(&cmd[5][0],"at^sics=5,alphabet,0");
	strcpy(&cmd[6][0],"at^sics=5,inactTO,20");
	
	strcpy(&cmd[7][0],"at^siss=5,\"srvType\",\"Ftp\"");
	strcpy(&cmd[8][0],"at^siss=5,\"alphabet\",\"1\"");
	strcpy(&cmd[9][0],"at^siss=5,\"conId\",\"5\"");
	strcpy(&cmd[10][0],"at^siss=5,\"address\",\"ftp://92.53.96.203/nir/Reg\"");
	strcpy(&cmd[11][0],"at^siss=5,\"tcpMR\",\"3\"");
	strcpy(&cmd[12][0],"at^siss=5,\"tcpOT\",\"3000\"");
	strcpy(&cmd[13][0],"at^siss=5,\"user\",\"npptik\"");
	strcpy(&cmd[14][0],"at^siss=5,\"passwd\",\"6zHJuQk7\"");
	strcpy(&cmd[15][0],"at^siss=5,\"path\",\"file:///a:/\"");
	strcpy(&cmd[16][0],"at^siss=5,\"files\",\"boot.bin\"");
	strcpy(&cmd[17][0],"at^siss=5,\"cmd\",\"fget\"");
	strcpy(&cmd[18][0],"at^siss=5,\"ftMode\",\"bin\"");
	strcpy(&cmd[19][0],"at^siss=5,\"hcMethod\",\"0\"");
	
	strcpy(&cmd[20][0],"at^siso=5");
	strcpy(&cmd[23][0],"at^sisc=5");
	
	SendGSM(cmgf, strlen(cmgf));
	osDelay(500);
	SendGSM(cmee, strlen(cmee));
	osDelay(500);
	SendGSM(ate, strlen(ate));

for(int i=7; i<20; i++) 
	{
	osDelay(200);
	SendGSM(&cmd[i][0], strlen(&cmd[i][0]));
	}
	
	osDelay(1000);
	SendGSM(&cmd[20][0], strlen(&cmd[20][0])); 
	
	while(1)
	{
		osDelay(500);
		time++;
		
		if(((strstr(data_com3_IN, "SIS: 5,0,2100,\"FGET: boot.bin\"")!=0)||(strstr(data_com3_IN, "SIS: 5,0,2100,\"fget boot.bin\"")!=0))&&(strstr(data_com3_IN, "Goodbye")!=0))
		{
			err = 1;
			break;
		}
		else if(strstr(data_com3_IN, "ERROR")!=0)
		{
			err = 0;
			break;
		}
		
		if (time>timeout)
		{
			err = 2;
			break;
		}
	}
	osDelay(1000);
	SendGSM(&cmd[23][0], strlen(&cmd[23][0])); 
	osDelay(1000);
	return err;	
}

/*
Перекидывает файл прошивки с gsm flash на sd flash
*/
unsigned char DownloadFromGSMtoSD(void)
{
	FIL file2;
	FRESULT f_err;
	uint32_t readedbytes, bytes;
	uint8_t fgsm_err;
	uint32_t size = 0;
	uint8_t buf[512] = {0};
	
	size = fgsm_filesize("boot.bin");
		
	if(size!=0)
	{
	
	fgsm_err = fgsm_open("boot.bin");
		if(fgsm_err)
		{
		
		
			uint32_t numblocks = size/512;
			uint32_t lastblock = size - 512*numblocks;
			
				osSemaphoreWait(SemFiles, 20000);
					taskENTER_CRITICAL();
						f_err = f_open(&file2, "boot.bin", FA_CREATE_ALWAYS | FA_WRITE);
					taskEXIT_CRITICAL();	
				osSemaphoreRelease(SemFiles);
			if(f_err == FR_OK)
			{
				for(int i = 0;i < numblocks+1; i++)
				{
					if(i == numblocks)
					{
						bytes = lastblock;
					}
					else
					{
						bytes = 512;
					}
					
					fgsm_read(buf,bytes);
					
					osSemaphoreWait(SemFiles, 20000);
										taskENTER_CRITICAL();
									f_err = f_write(&file2, buf, bytes, &readedbytes);
									f_err = f_sync(&file2);
										taskEXIT_CRITICAL();
					osSemaphoreRelease(SemFiles);

				}

				
					taskENTER_CRITICAL();
				f_err = f_close(&file2);
					taskEXIT_CRITICAL();
				fgsm_close();
				return 1;
			}
			else
			{
				fgsm_close();
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}


/*
Возвращает размер файла на gsm
*/
unsigned int fgsm_filesize(char* filename)
{
		//char *sfsa = "AT^SFSA=\"ls\", \"a:/\"";
	char *sfsa1 = "AT^SFSA=\"stat\",\"a:/%s\"";
	char cmdSize[40] = {0};
	uint8_t timeout = 100, time = 0;
	
	sprintf(cmdSize,sfsa1,filename);
	SendGSM(cmdSize, strlen(cmdSize));
	osDelay(1000);
		while(1)
	{
		osDelay(50);
		time++;
		if(strstr(data_com3_IN, "OK")!=0)
		{
			return atoi(&data_com3_IN[9]);
		}
		else if(strstr(data_com3_IN, "ERROR")!=0)
		{
			return 0;
		}
		
		if(time > timeout)
		{
			return 0;
		}
	}
}
/*
Открывает файл на gsm flash
*/
unsigned char fgsm_open(char* filename)
{
		char* fgopen = "AT^SFSA=\"open\", \"a:/%s\", 0";
	char cmdOpen[40] = {0};
	uint8_t timeout = 100, time = 0;
	sprintf(cmdOpen,fgopen,filename);
		SendGSM(cmdOpen, strlen(cmdOpen));
	while(1)
	{
		osDelay(50);
		time++;
		if(strstr(data_com3_IN, "OK")!=0)
		{
			return 1;
		}
		else if(strstr(data_com3_IN, "ERROR")!=0)
		{
			return 0;
		}
		
		if(time > timeout)
		{
			return 2;
		}
	}
}

/*
Чтение len байт из gsm flash
*/
unsigned char fgsm_read(char* buf, unsigned int len)
{
		char* fgread = "AT^SFSA=\"read\", 0, %d";
	char cmdRead[40] = {0},start[40] = {0};
	uint32_t timeout = 20000, time=0;
	sprintf(start, "\r\n^SFSA: %d,0\r\n", len);
	sprintf(cmdRead,fgread,len);
		SendGSM(cmdRead, strlen(cmdRead));
	while(1)
	{
		osDelay(10);
		time++;
		uint32_t slen = strlen(start);
		if(strstr(&data_com3_IN[len+slen], "OK\r\n")!=0)
		{
			memcpy(buf,&data_com3_IN[slen],len);
			return 1;
		}
		else if(strstr(data_com3_IN, "\r\nERROR\r\n")!=0)
		{
			return 0;
		}
		
		if(time > timeout)
		{
			return 2;
		}
	}

}

/*
Закрывает файл на gsm flash
*/
unsigned char fgsm_close(void)
{
		char* fgclose = "AT^SFSA=\"close\", 0";
	uint8_t timeout = 100, time = 0;
		SendGSM(fgclose, strlen(fgclose));
	while(1)
	{
		osDelay(50);
		time++;
		if(strstr(data_com3_IN, "OK")!=0)
		{
			return 1;
		}
		else if(strstr(data_com3_IN, "ERROR")!=0)
		{
			return 0;
		}
		
		if(time > timeout)
		{
			return 2;
		}
	}
}




/*
Парсинг строки логируемых регистров на пары регистров
*/
char ParsingAddr(char* regs, unsigned short int Patern[20][2])
{
	char* token_buff;	
	unsigned short int a,b,c, time;
	u32_t ad1,ad2;
	char regbuf[300], NumQ = 0;
	
NumQ = 0;
	if(strlen(regs)!=0)
	{
		
		strcpy(regbuf, regs);
		
		if((strchr(regs,',')==0)&&(strchr(regs,'-')!=0)) // Если просто диапазон регистров через тире
		{
			
			Patern[NumQ][0] = atoi((char*)strtok_r(regbuf,"-",&token_buff));
			Patern[NumQ][1] = atoi((char*)token_buff);
			NumQ++;
			
		}
		else if((strchr(regs,'-')==0)&&(strchr(regs,',')==0)) //Только одно число
		{
			a = atoi(regs);
					Patern[NumQ][0] = a;
					Patern[NumQ][1] = a;
					NumQ++;
		}
		else if(strchr(regs,'-')==0)  // Все регистры через запятую
		{
				a = atoi((char*)strtok_r(regbuf,",",&token_buff));
				c = a;
				while(1)
				{
					char* check = strchr((char*)token_buff,',');
					if(check==0) // Проверяем, есть ли еще разделитель, если больше нет, то все
						{
							b = atoi((char*)token_buff);
								
								if(b==(a+1)) // Последний регистр подряд
									{
										c = b;
										Patern[NumQ][0] = a;
										Patern[NumQ][1] = b;
										NumQ++;
									}
									else // Не подряд
									{
										Patern[NumQ][0] = a;
										Patern[NumQ][1] = c;
										NumQ++;
										Patern[NumQ][0] = b;
										Patern[NumQ][1] = b;
										NumQ++;
									}

						break;
						}
						else // Еще есть разделители
						{
							b = atoi((char*)strtok_r(NULL,",",&token_buff));
						}
					
					if(b==(c+1)) // Если регистры подряд
					{
						c = b;
					}
					else // не подряд
					{
					Patern[NumQ][0] = a;
					Patern[NumQ][1] = c;
					NumQ++;
						
					a = b;
					c = b;
					}
				
				}
				
		}
		else 	// И диапазоны через тире и через запятую, тот еще гемор парсить, но что поделать...
		{
			token_buff = regbuf;
			char* buf;
			buf = (char*)strtok_r(NULL,",",&token_buff);
			char* check = strchr((char*)buf,'-');
			
			if(check==0)
			{
				a = atoi((char*)buf);
				c = a;
			}
			else
			{
				a = atoi((char*)strtok_r(NULL,"-",&buf));
				b = atoi((char*)buf);
				c = b;
			}
			
			while(1)
			{
					check = strchr((char*)token_buff,',');
					if(check==0) // Проверяем, есть ли еще разделитель, если больше нет, то все
						{
							check = strchr((char*)token_buff,'-');
							
							if(check!=0)
							{
								b = atoi((char*)strtok_r(NULL,"-",&token_buff));
								
								if(b==(c+1)) // Если регистры подряд
										{
											b = atoi((char*)token_buff);
											Patern[NumQ][0] = a;
											Patern[NumQ][1] = b;
											NumQ++;
										}
										else
										{
											Patern[NumQ][0] = a;
											Patern[NumQ][1] = c;
											NumQ++;
											Patern[NumQ][0] = b;
											Patern[NumQ][1] = atoi((char*)token_buff);
											NumQ++;
										}
							}
							else
							{
								b = atoi((char*)token_buff);
								
								if(b==(c+1)) // Если регистры подряд
										{
											Patern[NumQ][0] = a;
											Patern[NumQ][1] = b;
											c = b;
										}
										else // не подряд
										{
										Patern[NumQ][0] = a;
										Patern[NumQ][1] = c;
										NumQ++;
										Patern[NumQ][0] = b;
										Patern[NumQ][1] = b;
										NumQ++;
										}
								
							}
						
						break;
						}
						else // Еще есть разделители
						{
							buf = (char*)strtok_r(NULL,",",&token_buff);
						
							check = strchr((char*)buf,'-');
							
							if(check!=0)
							{
								b = atoi((char*)strtok_r(NULL,"-",&buf));
								
								if(b==(c+1)) // Если регистры подряд
										{
											b = atoi((char*)buf);
											c = b;
										}
										else
										{
											Patern[NumQ][0] = a;
											Patern[NumQ][1] = c;
											NumQ++;
												
											a = b;
											b = atoi((char*)buf);
											c = b;
										}
							}
							else
							{
								b = atoi((char*)buf);
								
								if(b==(c+1)) // Если регистры подряд
										{
											c = b;
										}
										else // не подряд
										{
										Patern[NumQ][0] = a;
										Patern[NumQ][1] = c;
										NumQ++;
											
										a = b;
										c = b;
										}
								
							}
						}
			}
		
		}
	}
	return NumQ;
}


