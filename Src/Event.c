#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "Event.h"
#include "stm32f4xx_hal.h"
#include "LogSD.h"
#include "ModBus_master.h"
#include "stm32_ub_fatfs.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "ff.h"

struct events
{
	char dadr,t1,t2,f1,f2,sr,tsr,rele,sms,log,interface;
	unsigned int Ntask;
	unsigned short int Nreg;
	float param;
	char smstext[51];
	char name[25];
	char StateEvent;
	u32_t counter;
	u32_t Rtime;
	TaskHandle_t THandle;
};

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

unsigned char StateReley=0;
struct events *StructEventAdr[50];
char NumEvents=0;

extern struct devices *StructQwAdr[10];
extern char NumQw;

extern xSemaphoreHandle SemSMS;
extern xSemaphoreHandle SemSaveEv;
extern xSemaphoreHandle SemFiles;
extern xSemaphoreHandle SemGSM;
extern xSemaphoreHandle SemEventWork;

UBaseType_t uxCurrentTaskNumber;

FIL file4;
extern FATFS SDFatFs;
extern char SDPath[4]; 
FRESULT f_err4;

extern RTC_HandleTypeDef hrtc;

extern unsigned int LOADING;

char* EventLogHat = "Время события;Имя события;Адрес усторойства;Условие;Значение;Лог/Реле/СМС;  \r\n"; // мб добавить счетчик событий и интерфейс

extern unsigned char TransMode;

extern uint32_t FileErr;

extern unsigned char NumPhones;
extern unsigned char Phones[10][12];
/*
Создание задачи для события
*/
void CreatingEventTask(struct event *newEvent)
{
	//226 занимает в диспетчере, мб можно уменьшить до 256 или 384.
		xTaskCreate((pdTASK_CODE)EventTask,
                   newEvent->name,
                   384,
                   newEvent,
                   osPriorityNormal,
                   NULL
                  );
	
}


char bufSD[300],buflog[100];

/*
Задача события
*/
void EventTask(struct event *newEvent)
{
	struct events ThisEvent;
	struct devices *ThisDev;
	
	RTC_DateTypeDef sdatestructureget;
	RTC_TimeTypeDef stimestructureget;
		
	char flag=0, flagSMS=0, flagR = 0, flagL = 0;
	unsigned short int Nreg1,Nreg2, a, b,k = 0,j;
	float af,bf,param1;
	unsigned int h,c, Rcounter = 0;
	char buf8[35]={0},buf12[70]={0},EventString[150]={0};
	unsigned char WhichDev = 0, QwEx = 0, find = 0;
	
		char* end = "  \r\n";
		
	TaskHandle_t xHandle;
	TaskStatus_t xTaskDetails;
	
	memset(&ThisEvent,0,sizeof(ThisEvent));
	
	//Узнаем номер задачи, чтоб потом ее убивать!
	
    xHandle = xTaskGetHandle(NULL);
    configASSERT( xHandle );
    vTaskGetInfo(xHandle,
                 &xTaskDetails,
                 pdTRUE,
                 eInvalid );
	
	
	ThisEvent.Ntask = xTaskDetails.xTaskNumber;
	ThisEvent.THandle = xTaskDetails.xHandle;
	
	ThisEvent.dadr = newEvent->dadr;
	ThisEvent.t1 = newEvent->t1;
	ThisEvent.t2 = newEvent->t2;
	ThisEvent.f1 = newEvent->f1;
	ThisEvent.f2 = newEvent->f2;
	ThisEvent.sr = newEvent->sr;
	ThisEvent.tsr = newEvent->tsr;
	ThisEvent.Nreg = newEvent->Nreg;
	ThisEvent.param = newEvent->param;
	ThisEvent.rele = newEvent->rele;
	ThisEvent.log = newEvent->log;
	ThisEvent.sms = newEvent->sms;
	ThisEvent.interface = newEvent->interface;
	ThisEvent.counter = 0;
	ThisEvent.Rtime = newEvent->Rtime;
	strcpy(ThisEvent.name,newEvent->name);
	strcpy(ThisEvent.smstext, newEvent->smstext);
	
	StructEventAdr[NumEvents] = &ThisEvent;
	NumEvents++;
	
	if(LOADING)
	{
		osSemaphoreRelease(SemSaveEv);
	}
	
	if(ThisEvent.sms)
	{
		sprintf(buf8,"Устройство %d. ",ThisEvent.dadr);
		convert_utf8_to_windows1251(buf8, buf12);
		strcat(buf12, ThisEvent.smstext);
	}
	
	
	for(WhichDev=0;WhichDev<20;WhichDev++) // Ищем устройство
	{
		if(StructQwAdr[WhichDev]->addr == ThisEvent.dadr) // Устройство есть
		{ 
			QwEx = 0;
			ThisDev = StructQwAdr[WhichDev];
			//ВЫЧИСЛИТЬ, ГДЕ ЛЕЖИТ НУЖНЫЙ РЕГИСТР

				if(ThisEvent.t1==3)
				{
					for(int i=0;i<ThisDev->NumQH;i++)
					{
						for(j=ThisDev->PatternH[i][0];j<=ThisDev->PatternH[i][1];j++)
						{
							if(j == ThisEvent.Nreg)
							{
								Nreg1 = k;
								find = 1;
								QwEx = 1;
								break;
							}
							k++;
						}
								if(find == 1)
									{
										break;
									}
					}
				}
				else if(ThisEvent.t1==4)
				{
					for(int i=0;i<ThisDev->NumQI;i++)
					{
						for(j=ThisDev->PatternI[i][0];j<=ThisDev->PatternI[i][1];j++)
						{
							if(j == ThisEvent.Nreg)
							{
								Nreg1 = k;
								find = 1;
								QwEx = 1;
								break;
							}
							k++;
						}
								if(find == 1)
									{
										break;
									}
					}
				}
				
				k=0;
				
				
				
				if(ThisEvent.tsr == 1) // Приведение параметра сравнения, в зависимости от настройки.
				{
					find = 0;
					if(ThisEvent.t2 == 3)
					{
						for(int i=0;i<ThisDev->NumQH;i++)
						{
							for(j=ThisDev->PatternH[i][0];j<=ThisDev->PatternH[i][1];j++)
							{
								if(j == ThisEvent.param)
								{
									param1 = k;
									find = 1;
									break;
								}
								k++;
							}
									if(find == 1)
										{
											break;
										}
						}
					}
					else if(ThisEvent.t2 == 4)
					{
						for(int i=0;i<ThisDev->NumQI;i++)
						{
							for(j=ThisDev->PatternI[i][0];j<=ThisDev->PatternI[i][1];j++)
							{
								if(j == ThisEvent.param)
								{
									param1 = k;
									find = 1;
									break;
								}
								k++;
							}
									if(find == 1)
										{
											break;
										}
						}
					}
				}
			break;
		}
		else
		{
			QwEx = 0; // Устройства нет
		}
	}
	
	if((QwEx == 0) && (find == 0))
	{
		// НУЖНО СОЗДАТЬ ОПРОС
		struct device newDev;
		
		if(QwEx == 0)
		{
			memset(&newDev,0,sizeof(newDev));
			newDev.addr = ThisEvent.dadr;
			newDev.time = 250;
			newDev.log = 0;
			newDev.interface = ThisEvent.interface;
				if(ThisEvent.f1)
				{
					sprintf(newDev.regs,"%d-%d",ThisEvent.Nreg,ThisEvent.Nreg+1);
				}
				else
				{
					sprintf(newDev.regs,"%d",ThisEvent.Nreg);
				}
				
			if(ThisEvent.t1==3)
			{
				newDev.type3 = 3;
			}
			else if(ThisEvent.t1==4)
			{
				newDev.type4 = 4;
			}
			
			CreateMSlaveTask(&newDev);
		}
		
			while(1)
			{
				osDelay(1000);
				for(WhichDev=0;WhichDev<20;WhichDev++) // Ищем устройство
				{
					if(StructQwAdr[WhichDev]->addr == ThisEvent.dadr) // Устройство есть
					{
						ThisDev = StructQwAdr[WhichDev];
						
						if((find == 0) && (ThisEvent.tsr == 1))
							{
								memset(&newDev,0,sizeof(newDev));
								newDev.addr = ThisEvent.dadr;
								newDev.time = 250;
								newDev.log = 0;
								newDev.interface = ThisEvent.interface;
								u32_t pInt = ThisEvent.param;
									if(ThisEvent.f2)
									{
										sprintf(newDev.regs,"%d-%d",pInt,pInt+1);
									}
									else
									{
										sprintf(newDev.regs,"%d",pInt);
									}
									
								if(ThisEvent.t2==3)
								{
									newDev.type3 = 3;
								}
								else if(ThisEvent.t2==4)
								{
									newDev.type4 = 4;
								}
								
								CreateMSlaveTask(&newDev);
							}
						///////////////////////////////
			if(ThisEvent.t1==3)
				{
					for(int i=0;i<ThisDev->NumQH;i++)
					{
						for(j=ThisDev->PatternH[i][0];j<=ThisDev->PatternH[i][1];j++)
						{
							if(j == ThisEvent.Nreg)
							{
								Nreg1 = k;
								find = 1;
								QwEx = 1;
								break;
							}
							k++;
						}
								if(find == 1)
									{
										break;
									}
					}
				}
				else if(ThisEvent.t1==4)
				{
					for(int i=0;i<ThisDev->NumQI;i++)
					{
						for(j=ThisDev->PatternI[i][0];j<=ThisDev->PatternI[i][1];j++)
						{
							if(j == ThisEvent.Nreg)
							{
								Nreg1 = k;
								find = 1;
								QwEx = 1;
								break;
							}
							k++;
						}
								if(find == 1)
									{
										break;
									}
					}
				}
				
				k=0;
				
				
				
				if(ThisEvent.tsr == 1)
				{
					find = 0;
					if(ThisEvent.t2 == 3)
					{
						for(int i=0;i<ThisDev->NumQH;i++)
						{
							for(j=ThisDev->PatternH[i][0];j<=ThisDev->PatternH[i][1];j++)
							{
								if(j == ThisEvent.param)
								{
									param1 = k;
									find = 1;
									break;
								}
								k++;
							}
									if(find == 1)
										{
											break;
										}
						}
					}
					else if(ThisEvent.t2 == 4)
					{
						for(int i=0;i<ThisDev->NumQI;i++)
						{
							for(j=ThisDev->PatternI[i][0];j<=ThisDev->PatternI[i][1];j++)
							{
								if(j == ThisEvent.param)
								{
									param1 = k;
									find = 1;
									break;
								}
								k++;
							}
									if(find == 1)
										{
											break;
										}
						}
					}
				}
			
		
						break;
					}
					else
					{
						QwEx = 0; // Устройства нет
					}
				}
					if(QwEx == 1)
						{
							break;
						}
				
			}
	}
	////////////////////////////////сформировать строку для журнала
	if(ThisEvent.log)
	{
		memset(buflog,0,100);
		
		sprintf(EventString,"%s;",ThisEvent.name);
		sprintf(buflog,"%d;",ThisEvent.dadr);
		strcat(EventString,buflog);
		
		sprintf(buflog,"%d",ThisEvent.Nreg);
		if(ThisEvent.t1==3)
		{
			strcat(buflog,"h");
		}
		else if(ThisEvent.t1==4)
		{
			strcat(buflog,"i");
		}
		
		if(ThisEvent.f1==0)
		{
			strcat(buflog,"I ");
		}
		else
		{
			strcat(buflog,"F ");
		}
		
		switch(ThisEvent.sr)
		{
			case 1:
				strcat(buflog,"= ");
			break;
			
			case 2:
				strcat(buflog,"> ");
			break;
			
			case 3:
				strcat(buflog,"< ");
			break;
			
			case 4:
				strcat(buflog,"& ");
			break;
			
			default:
				strcat(buflog,"? ");
			break;
		}
		
		strcat(EventString,buflog);
		
		if(ThisEvent.tsr)
		{
			
		sprintf(buflog,"%d",ThisEvent.Nreg);	
			
			if(ThisEvent.t1==3)
			{
				strcat(buflog,"h");
			}
			else if(ThisEvent.t1==4)
			{
				strcat(buflog,"i");
			}
			
			if(ThisEvent.f1==0)
			{
				strcat(buflog,"I;");
			}
			else
			{
				strcat(buflog,"F;");
			}
		}
		else
		{
			sprintf(buflog,"%f;",ThisEvent.param);
		}
		
		strcat(EventString,buflog);
		strcat(EventString,"%f;");
		
		sprintf(buflog,"%d/%d/%d;",ThisEvent.log,ThisEvent.rele,ThisEvent.sms);
		strcat(EventString,buflog);

		
		
	}
	
	///////////////////////////////
	
	while(!LOADING)
	{
		osDelay(1000);
	}
osDelay(5000);
	
	for(;;)
	{
		// Приведение элементов сравнения
		if(ThisEvent.t1==3)
		{
			if(ThisEvent.f1)
			{
				if(swFloat)
				{
					h = 0;
					h = ThisDev->Holding[Nreg1];
					h |= ThisDev->Holding[Nreg1+1]<<16;
					af = *(float*)&h;
				}
				else
				{
					h = 0;
					h = ThisDev->Holding[Nreg1]<<16;
					h |= ThisDev->Holding[Nreg1+1];
					af = *(float*)&h;
				}
				
			}
			else
			{
				af=0;
				a = ThisDev->Holding[Nreg1];
				af = a;
			}
		}
		else
		{
			if(ThisEvent.f1)
			{
				if(swFloat)
				{
					h = 0;
					h = ThisDev->Input[Nreg1];
					h |= ThisDev->Input[Nreg1+1]<<16;
					af = *(float*)&h;
				}
				else
				{
					h = 0;
					h = ThisDev->Input[Nreg1]<<16;
					h |= ThisDev->Input[Nreg1+1];
					af = *(float*)&h;
				}
				
			}
			else
			{
				af=0;
				a = ThisDev->Input[Nreg1];
				af = a;
			}
		}
		
		if(ThisEvent.tsr)
		{
			Nreg2 = param1;
			if(ThisEvent.t2==3)
			{
				if(ThisEvent.f1)
				{
					if(swFloat)
					{
						h = 0;
						h = ThisDev->Holding[Nreg2];
						h |= ThisDev->Holding[Nreg2+1]<<16;
						bf = *(float*)&h;
					}
					else
					{
						h = 0;
						h = ThisDev->Holding[Nreg2]<<16;
						h |= ThisDev->Holding[Nreg2+1];
						bf = *(float*)&h;
					}
				}
				else
				{
					bf=0;
					b = ThisDev->Holding[Nreg2];
					bf = b;
				}
			}
			else
			{
				if(ThisEvent.f1)
				{
					if(swFloat)
					{
						h = 0;
						h = ThisDev->Input[Nreg2];
						h |= ThisDev->Input[Nreg2+1]<<16;
						bf = *(float*)&h;
					}
					else
					{
						h = 0;
						h = ThisDev->Input[Nreg2]<<16;
						h |= ThisDev->Input[Nreg2+1];
						bf = *(float*)&h;
					}
				}
				else
				{
					bf = 0;
					b = ThisDev->Input[Nreg2];
					bf = b;
				}
			}
		}
		else
		{
			bf = ThisEvent.param;
		}
		
		if(ThisEvent.StateEvent == 13) // Квитирование
		{
			flagL = 0;
			flagR = 0;
			flagSMS = 0;
				Rcounter = 0;
				StateReley = 0;
				ThisEvent.StateEvent = 0;
				RELAY_OFF
				REL_LED_OFF;
			osDelay(2000);
		}
		
		// Сравнение элементов события
		if(ThisEvent.sr==1)
		{
			if(af==bf)
			{
				flag=1;
			}
		}
		else if(ThisEvent.sr==2)
		{
			if(af>bf)
			{
				flag=1;
			}
		}
		else if(ThisEvent.sr==3)
		{
			if(af<bf)
			{
				flag=1;
			}
		}
		else if(ThisEvent.sr==4)
		{
			unsigned int abit = 0, bbit = 0;
			abit = af;
			bbit = bf;
			if((abit & bbit)!=0)
			{
				flag=1;
			}
		}
		

		
		if(flag) // Условие события выполняется
		{
			if(ThisEvent.StateEvent == 0)
			{
				ThisEvent.counter++;
			}
			ThisEvent.StateEvent = 1;
			
			if(ThisEvent.rele&&!flagR) // Для общего управления реле
			{
				flagR = 1;
				Rcounter = 0;
				StateReley++;
				RELAY_ON;
				REL_LED_ON;
			}
			
			
			if(ThisEvent.log&&!flagL)
			{
				//записать в лог

				flagL = 1;

				if(SDFatFs.fs_type != 0)
				{
					osSemaphoreWait(SemEventWork, 20000);
					osSemaphoreWait(SemFiles, portMAX_DELAY);
					
				HAL_RTC_GetTime(&hrtc, &stimestructureget, RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc, &sdatestructureget, RTC_FORMAT_BIN);
				
				u32_t Ms;

					Ms = 1000*(stimestructureget.SecondFraction-stimestructureget.SubSeconds)/(stimestructureget.SecondFraction+1); // Вычисляем милисекунды
					
					
					memset(buflog,0,100);
					memset(bufSD,0,300);
					
					sprintf(buflog,"%02d.%02d.%02d ",sdatestructureget.Date, sdatestructureget.Month, 2000 + sdatestructureget.Year);	// Genius Line 
					strcat(bufSD,buflog);
					
					sprintf(buflog,"%02d:%02d:%02d.%03d;",stimestructureget.Hours, stimestructureget.Minutes, stimestructureget.Seconds,Ms);
					strcat(bufSD,buflog);
				
					sprintf(buflog,EventString,af);
					strcat(bufSD,buflog);
					strcat(bufSD,end);
					
				
					taskENTER_CRITICAL();
				f_err4 = f_open(&file4, "LogEVENT.csv", FA_CREATE_NEW | FA_WRITE);
					taskEXIT_CRITICAL();
				if(f_err4==FR_OK)
				{
					taskENTER_CRITICAL();
						f_err4 = f_write(&file4, EventLogHat, strlen(EventLogHat), &c);
						f_close(&file4);
					taskEXIT_CRITICAL();
					osDelay(250);
				}
				else if(f_err4 == FR_DISK_ERR)
						{
							FileErr++;
						}				


						taskENTER_CRITICAL();
					f_err4 = f_open(&file4, "LogEVENT.csv", FA_OPEN_ALWAYS | FA_WRITE);
						taskEXIT_CRITICAL();
				
				if(f_err4==FR_OK)
				{
					taskENTER_CRITICAL();
						f_err4 = f_lseek(&file4, f_size(&file4));
						f_err4 = f_write(&file4, bufSD, strlen(bufSD), &c);
						f_close(&file4);
					taskEXIT_CRITICAL();

				}
				else if(f_err4 == FR_DISK_ERR)
						{
							FileErr++;
						}
						osSemaphoreRelease(SemFiles);
						osSemaphoreRelease(SemEventWork);
					}
			}
			
			if(ThisEvent.sms&&!flagSMS) // отправка СМС
			{
				flagSMS=1;
				osSemaphoreWait(SemSMS, portMAX_DELAY); // Ожидаем, когда порт модема освободится
				osSemaphoreWait(SemGSM, portMAX_DELAY);
						
				if(TransMode!=0) // Если в прозрачном режиме, выходим на время отправки СМС
							{
								sprintf(&data_com3_OUT[0],"+++");
								line3=3; n_com3=0;
								osDelay(2000);
								USART3->CR1  &=  ~USART_CR1_RE;              
								USART3->CR1  |=   USART_CR1_TE;               
								USART3->DR=data_com3_OUT[n_com3++]; 
								osDelay(5000);				
							}
							
								SendSMS5(buf12);
							
						if(TransMode!=0) // Если надо, возвращаемся в прозрачный режим
							{
								sprintf(&data_com3_OUT[0],"at^sist=%d\r\n",TransMode);
								line3=strlen(&data_com3_OUT[0]); n_com3=0;
								osDelay(2000);
								USART3->CR1  &=  ~USART_CR1_RE;              
								USART3->CR1  |=   USART_CR1_TE;               
								USART3->DR=data_com3_OUT[n_com3++];
								osDelay(2000);	
							}
							
				osSemaphoreRelease(SemGSM);
				osSemaphoreRelease(SemSMS);
			}
		}
		else
		{
			// Сброс состояния события
			ThisEvent.StateEvent = 0;
			flagSMS = 0;
			flagL = 0;
			if(flagR && (ThisEvent.Rtime == 0))
			{
				flagR = 0;
				StateReley--;
				if(StateReley==0)
				{
					RELAY_OFF;
					REL_LED_OFF;
				}
			}
			else if(flagR && (ThisEvent.Rtime > 0))
			{
				Rcounter++;
				if((Rcounter*250) > ThisEvent.Rtime)
				{
					flagR = 0;
					StateReley--;
					if(StateReley==0)
					{
						RELAY_OFF;
						REL_LED_OFF;
					}
				}
			}
		}
		
		
		flag = 0;
	osDelay(250);
	}
	
}

