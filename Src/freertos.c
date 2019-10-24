/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"

/* USER CODE BEGIN Includes */    
#include "ff.h"
#include "lwip\tcpip.h"
#include "lwip\mem.h"
#include "lwip\pbuf.h"
#include "lwip\ip.h"
#include "lwip\tcp.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "stm32_ub_fatfs.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "ModBus_master.h"
#include "LogSD.h"
#include <stdlib.h>
#include <stdarg.h>
#include "string.h"
#include <stdio.h>
#include "httpserver-netconn.h"
#include "FTP.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include "stdio.h"
#include "LogSD.h"
#include "string.h"
#include "fatfs.h"
#include "Event.h"
#include "mem.h"
#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_cdc.h"

/* USER CODE END Includes */

/* Variables -----------------------------------------------------------------*/

/* USER CODE BEGIN Variables */
osThreadId defaultTaskHandle;
osThreadId myTask02Handle;
osThreadId myTask03Handle;
osThreadId myTask04Handle;
osThreadId myTask05Handle;
osThreadId myTask06Handle;
osThreadId myTask07Handle;
osThreadId myTask08Handle;
osThreadId myTask09Handle;
osThreadId myTask10Handle;
osThreadId myTask11Handle;
osThreadId myTask12Handle;
osThreadId myTask13Handle;
osThreadId myTask14Handle;

xSemaphoreHandle xMbusS;
xSemaphoreHandle xMbusM;
xSemaphoreHandle xMbusG;
xSemaphoreHandle SemGateway;
xSemaphoreHandle SemFiles;
xSemaphoreHandle SemMbus;
xSemaphoreHandle SemMem;
xSemaphoreHandle SemSMS;
xSemaphoreHandle SemGSM;
xSemaphoreHandle SemPlan;
xSemaphoreHandle SemSaveDev;
xSemaphoreHandle SemSaveEv;
xSemaphoreHandle SemFlash;
xSemaphoreHandle SemRR;
xSemaphoreHandle SemSending;
xSemaphoreHandle s_xSemaphore;
xSemaphoreHandle SemEventWork;

extern SD_HandleTypeDef hsd;

extern TIM_HandleTypeDef htim6;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

extern RTC_HandleTypeDef hrtc;

extern IWDG_HandleTypeDef hiwdg;

extern TIM_HandleTypeDef htim6;

extern USBH_HandleTypeDef hUsbHostHS;

//FIL file2;
extern FATFS SDFatFs;
FRESULT ff_err;
extern char SDPath[4]; /* SD card logical drive path */
extern unsigned char   data_com3[2000];
extern unsigned int    n_com3,n_com33;

unsigned char SMSbuf[com_max];
extern char* smsnum;

extern char smsMSG[17][51];

extern unsigned short int first, second, period;
extern char flagLOG;

extern uint8_t IP_ADDRESS[4];
extern uint8_t NETMASK_ADDRESS[4];
extern uint8_t GATEWAY_ADDRESS[4];
extern uint8_t MACAddr[6];

extern char MSREGADR;

unsigned int SpeedSl, SpeedMs, LOADING=0;


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

extern struct events
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


	extern struct events *StructEventAdr[50];
	extern char NumEvents;
	extern struct devices *StructQwAdr[10];
	extern char NumQw;
u8_t GatewayDev = 0,GatewayON = 0;



unsigned int Timeout = 500;

u32_t BASEADRCONF = 0x080A0000;
u32_t BASEADRDEV = 0x080C0000;
u32_t BASEADREVENT = 0x080E0000;

unsigned char AUTORIZED = 0;

extern uint32_t FileErr;

char DelLog = 0;

extern unsigned char CutTime;
char *SendingFile = 0;

char ServiceMode = 0;

unsigned int SendEventTimer = 0;
char ReadyToSendEvent = 1;

float percent = 0;
char SMSfullFLASH = 0;
char FirstTurnOn = 0;

unsigned char NeedToUpdate = 0;

unsigned char SendingEnable = 0;

unsigned char SendFromSector = 0;
unsigned int StartSector = 0, EndSector = 0xFFFFFFFF;

extern unsigned char NumPhones;
extern unsigned char Phones[10][13];

unsigned char exConf = 0, exDev = 0, exEv = 0;

/* USER CODE END Variables */

/* Function prototypes -------------------------------------------------------*/

/* USER CODE BEGIN FunctionPrototypes */

void StartDefaultTask(void const * argument);
void LoadingConf(void const * argument);
void MbusRTUMaster(void const * argument);
void SavingDevices(void const * argument);
void MbusRTUSlave(void const * argument);
void StartGSM(void const * argument);
void RoundRobin(void const * argument);
void ModbusTCP_Slave(void const * argument);
void SaveEvents(void const * argument);
void HTTP_Server(void const * argument);
void MX_FREERTOS_Init(void);
void SavingConf(void const * argument);
void SetByModbus(void const * argument);
void ReadingSMS(void const * argument);


/* USER CODE END FunctionPrototypes */

/* Hook prototypes */

/* USER CODE BEGIN Application */


void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
       
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
		vSemaphoreCreateBinary(xMbusM);
		vSemaphoreCreateBinary(xMbusS);
		vSemaphoreCreateBinary(xMbusG);
		vSemaphoreCreateBinary(SemGateway);
		vSemaphoreCreateBinary(SemFiles);
		vSemaphoreCreateBinary(SemMbus);
		vSemaphoreCreateBinary(SemMem);
		vSemaphoreCreateBinary(SemSMS);
		vSemaphoreCreateBinary(SemGSM);
		vSemaphoreCreateBinary(SemPlan);
		vSemaphoreCreateBinary(SemSaveDev);
		vSemaphoreCreateBinary(SemSaveEv);
		vSemaphoreCreateBinary(SemFlash);
		vSemaphoreCreateBinary(SemRR);
		vSemaphoreCreateBinary(SemSending);
		vSemaphoreCreateBinary(s_xSemaphore);
		vSemaphoreCreateBinary(SemEventWork);
		
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
		MX_TIM6_Init();
		HAL_TIM_Base_Start_IT(&htim6);
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityAboveNormal, 0, 512);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of myTask02 */
  osThreadDef(LOADING, LoadingConf, osPriorityNormal, 0, 1024);
  myTask02Handle = osThreadCreate(osThread(LOADING), NULL);
	  
		/* definition and creation of myTask03 */
  osThreadDef(MosbusRTU_Mo, MbusRTUMaster, osPriorityNormal, 0, 128);
  myTask03Handle = osThreadCreate(osThread(MosbusRTU_Mo), NULL);

  /* definition and creation of myTask04 */
	if(GSMMODE) // Если устройство с установленным модемом, то запускаем задачу
	{
		osThreadDef(StartingGSM, StartGSM, osPriorityNormal, 0, 1536);//128
		myTask04Handle = osThreadCreate(osThread(StartingGSM), NULL);
	}
	  /* definition and creation of myTask05 */
  osThreadDef(ModbusRTU_S, MbusRTUSlave, osPriorityNormal, 0, 128);
  myTask05Handle = osThreadCreate(osThread(ModbusRTU_S), NULL);
	
		  /* definition and creation of myTask06 */
  osThreadDef(SavingDev, SavingDevices, osPriorityNormal, 0, 256);
  myTask06Handle = osThreadCreate(osThread(SavingDev), NULL);
	  
		osThreadDef(CurcleBuffer, RoundRobin, osPriorityNormal, 0, 1024);//67
	myTask07Handle = osThreadCreate(osThread(CurcleBuffer), NULL);
	
		  /* definition and creation of myTask08 */
 osThreadDef(ModbusTCP_S, ModbusTCP_Slave, osPriorityNormal, 0, 128);
 myTask07Handle = osThreadCreate(osThread(ModbusTCP_S), NULL);
			 
				/* definition and creation of myTask09 */
  osThreadDef(SavingEv, SaveEvents, osPriorityNormal, 0, 128);
  myTask09Handle = osThreadCreate(osThread(SavingEv), NULL);
	
	  osThreadDef(HTTP, HTTP_Server, osPriorityAboveNormal, 0, 1536);
  myTask10Handle = osThreadCreate(osThread(HTTP), NULL);
	
		  osThreadDef(SavingConfig, SavingConf, osPriorityNormal, 0, 128);
  myTask11Handle = osThreadCreate(osThread(SavingConfig), NULL);
	
//	osThreadDef(GSM_M_TCP, ModbusGSM, osPriorityNormal, 0, 256);
//  myTask12Handle = osThreadCreate(osThread(GSM_M_TCP), NULL);

			osThreadDef(SettingsM, SetByModbus, osPriorityNormal, 0, 512);
  myTask13Handle = osThreadCreate(osThread(SettingsM), NULL);
			
	if(GSMMODE) // Если устройство с установленным модемом, то запускаем задачу
	{
			osThreadDef(ReadingSMS, ReadingSMS, osPriorityNormal, 0, 256);
  myTask14Handle = osThreadCreate(osThread(ReadingSMS), NULL);
	}

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
	
	
}

/*
Инит IP, SD, FTP
Зажег светодиода, тип включился
*/
void StartDefaultTask(void const * argument)
{
	// Предзагрузка параметров для сетевой настройки
	/*if((HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_15)) || (!HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_2))) // Если при запуске зажата кнопка или DI, то запускается с дефолтными сетевыми настройками
	{
		*/
	if((!HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_12))||(!HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_2))) // Если при запуске зажата кнопка или DI, то запускается с дефолтными сетевыми настройками
	{
			IP_ADDRESS[0]			 = 192;
				IP_ADDRESS[1]			 = 168;
				IP_ADDRESS[2]			 = 5;
				IP_ADDRESS[3]			 = 241;
				NETMASK_ADDRESS[0] = 255;
				NETMASK_ADDRESS[1] = 255;
				NETMASK_ADDRESS[2] = 255;
				NETMASK_ADDRESS[3] = 0;
				GATEWAY_ADDRESS[0] = 192;
				GATEWAY_ADDRESS[1] = 168;
				GATEWAY_ADDRESS[2] = 5;
				GATEWAY_ADDRESS[3] = 240;
				MACAddr[0] = 2;
				MACAddr[1] = 0;
				MACAddr[2] = 0;
				MACAddr[3] = 0;
				MACAddr[4] = 0;
				MACAddr[5] = 0;
		
		POWER_LED_ON;
		MB_LED_ON;
		REL_LED_ON;
		
		ServiceMode = 1;
	
	}
	else
	{
			unsigned int adrcount = BASEADRCONF;
				IP_ADDRESS[0]			 = FLASH_Read(adrcount++);
				IP_ADDRESS[1]			 = FLASH_Read(adrcount++);
				IP_ADDRESS[2]			 = FLASH_Read(adrcount++);
				IP_ADDRESS[3]			 = FLASH_Read(adrcount++);
				NETMASK_ADDRESS[0] = FLASH_Read(adrcount++);
				NETMASK_ADDRESS[1] = FLASH_Read(adrcount++);
				NETMASK_ADDRESS[2] = FLASH_Read(adrcount++);
				NETMASK_ADDRESS[3] = FLASH_Read(adrcount++);
				GATEWAY_ADDRESS[0] = FLASH_Read(adrcount++);
				GATEWAY_ADDRESS[1] = FLASH_Read(adrcount++);
				GATEWAY_ADDRESS[2] = FLASH_Read(adrcount++);
				GATEWAY_ADDRESS[3] = FLASH_Read(adrcount++);
				MACAddr[0] = FLASH_Read(adrcount++);
				MACAddr[1] = FLASH_Read(adrcount++);
				MACAddr[2] = FLASH_Read(adrcount++);
				MACAddr[3] = FLASH_Read(adrcount++);
				MACAddr[4] = FLASH_Read(adrcount++);
				MACAddr[5] = FLASH_Read(adrcount++);
			
	}
	
	if((FLASH_Read(BASEADRCONF + 0x172)) != 0x55) // Проверка на первое влючение
					{
						FirstTurnOn = 1;
						IP_ADDRESS[0]			 = 192;
						IP_ADDRESS[1]			 = 168;
						IP_ADDRESS[2]			 = 5;
						IP_ADDRESS[3]			 = 241;
						NETMASK_ADDRESS[0] = 255;
						NETMASK_ADDRESS[1] = 255;
						NETMASK_ADDRESS[2] = 255;
						NETMASK_ADDRESS[3] = 0;
						GATEWAY_ADDRESS[0] = 192;
						GATEWAY_ADDRESS[1] = 168;
						GATEWAY_ADDRESS[2] = 5;
						GATEWAY_ADDRESS[3] = 240;
						MACAddr[0] = 2;
						MACAddr[1] = 0;
						MACAddr[2] = 0;
						MACAddr[3] = 0;
						MACAddr[4] = 0;
						MACAddr[5] = 0;
					}

	MX_LWIP_Init();
  MX_FATFS_Init();
	FTP_Init();
	

	POWER_LED_ON;

  for(;;)
  {
		osThreadTerminate(NULL);
		osDelay(10000);
  }

}

/*
Загрузка параметров работы
*/
void LoadingConf(void const * argument)
{
	unsigned int c, b;
	FIL* file;
	
		struct device NewDev;
		struct devices DevBuf;

		struct event NewEv;
		struct events EvBuf;

	osDelay(10000);

	
	
	strcpy(&smsMSG[0][0],smsnum);
	smsnum	= (char*)(&smsMSG[0][0]);
	file = pvPortMalloc(sizeof(FIL));
	osSemaphoreWait(SemMem, portMAX_DELAY); 
	osSemaphoreWait(SemFiles, portMAX_DELAY);
	
	if(file!=0)
	{
		ff_err = f_open(file, "config.txt", FA_OPEN_EXISTING | FA_READ);
		
		if(ff_err==FR_OK)
		{
			
			
		ff_err = f_read(file, &Holding_reg[IPREG1], 1, &c);
		ff_err = f_read(file, &Holding_reg[IPREG2], 1, &c);
		ff_err = f_read(file, &Holding_reg[IPREG3], 1, &c);
		ff_err = f_read(file, &Holding_reg[IPREG4], 1, &c);

		ff_err = f_read(file, &Holding_reg[MREG1], 1, &c);
		ff_err = f_read(file, &Holding_reg[MREG2], 1, &c);
		ff_err = f_read(file, &Holding_reg[MREG3], 1, &c);
		ff_err = f_read(file, &Holding_reg[MREG4], 1, &c);
			
		ff_err = f_read(file, &Holding_reg[GREG1], 1, &c);
		ff_err = f_read(file, &Holding_reg[GREG2], 1, &c);
		ff_err = f_read(file, &Holding_reg[GREG3], 1, &c);
		ff_err = f_read(file, &Holding_reg[GREG4], 1, &c);
			
		ff_err = f_read(file, &Holding_reg[MAC1], 1, &c);
		ff_err = f_read(file, &Holding_reg[MAC2], 1, &c);
		ff_err = f_read(file, &Holding_reg[MAC3], 1, &c);
		ff_err = f_read(file, &Holding_reg[MAC4], 1, &c);
		ff_err = f_read(file, &Holding_reg[MAC5], 1, &c);
		ff_err = f_read(file, &Holding_reg[MAC6], 1, &c);
			
			
		IP_ADDRESS[0]			 = Holding_reg[IPREG1];
		IP_ADDRESS[1]			 = Holding_reg[IPREG2];
		IP_ADDRESS[2]			 = Holding_reg[IPREG3];
		IP_ADDRESS[3]			 = Holding_reg[IPREG4];
		NETMASK_ADDRESS[0] = Holding_reg[MREG1];
		NETMASK_ADDRESS[1] = Holding_reg[MREG2];
		NETMASK_ADDRESS[2] = Holding_reg[MREG3];
		NETMASK_ADDRESS[3] = Holding_reg[MREG4];
		GATEWAY_ADDRESS[0] = Holding_reg[GREG1];
		GATEWAY_ADDRESS[1] = Holding_reg[GREG2];
		GATEWAY_ADDRESS[2] = Holding_reg[GREG3];
		GATEWAY_ADDRESS[3] = Holding_reg[GREG4];
		MACAddr[0] = Holding_reg[MAC1];
		MACAddr[1] = Holding_reg[MAC2];
		MACAddr[2] = Holding_reg[MAC3];
		MACAddr[3] = Holding_reg[MAC4];
		MACAddr[4] = Holding_reg[MAC5];
		MACAddr[5] = Holding_reg[MAC6];
		
		


		ff_err = f_read(file, &Holding_reg[MsREG], 1, &c);

				if(Holding_reg[MsREG]==0)
		{
			Holding_reg[MsREG] = 1;
		}

		MSREGADR = Holding_reg[MsREG];
			

		ff_err = f_read(file, smsnum, 12, &c);

		ff_err = f_read(file, &SpeedSl, 4, &c);

		ff_err = f_read(file, &SpeedMs, 4, &c);
	
		ff_err = f_read(file, &GatewayON, 1, &c);
	
		ff_err = f_read(file, &smsMSG[1][0], 51, &c);
		
		ff_err = f_read(file, &smsMSG[2][0], 51, &c);
		
		ff_err = f_read(file, &smsMSG[3][0], 51, &c);
		
		ff_err = f_read(file, &smsMSG[4][0], 51, &c);
		
		ff_err = f_read(file, &CutTime, 1, &c);
		
		ff_err = f_read(file, &DelLog, 1, &c);
		
		ff_err = f_read(file, &Timeout, 4, &c);
		
		ff_err = f_read(file, &SendingEnable, 1, &c);
	
				f_close(file);
				
				exConf = 1;
	
				osSemaphoreRelease(SemMem);

			}
			else
			{
				
				unsigned int adrcount = BASEADRCONF, checkadr;
				checkadr = FLASH_Read(BASEADRCONF + 0x172);
				if(checkadr == 0x55) // Проверка на первое влючение
					{
							Holding_reg[IPREG1] = FLASH_Read(adrcount++);
							Holding_reg[IPREG2] = FLASH_Read(adrcount++);
							Holding_reg[IPREG3] = FLASH_Read(adrcount++);
							Holding_reg[IPREG4] = FLASH_Read(adrcount++);
							
							Holding_reg[MREG1] = FLASH_Read(adrcount++);
							Holding_reg[MREG2] = FLASH_Read(adrcount++);
							Holding_reg[MREG3] = FLASH_Read(adrcount++);
							Holding_reg[MREG4] = FLASH_Read(adrcount++);
							
							Holding_reg[GREG1] = FLASH_Read(adrcount++);
							Holding_reg[GREG2] = FLASH_Read(adrcount++);
							Holding_reg[GREG3] = FLASH_Read(adrcount++);
							Holding_reg[GREG4] = FLASH_Read(adrcount++);
							
							Holding_reg[MAC1] = FLASH_Read(adrcount++);
							Holding_reg[MAC2] = FLASH_Read(adrcount++);
							Holding_reg[MAC3] = FLASH_Read(adrcount++);
							Holding_reg[MAC4] = FLASH_Read(adrcount++);
							Holding_reg[MAC5] = FLASH_Read(adrcount++);
							Holding_reg[MAC6] = FLASH_Read(adrcount++);

							
							IP_ADDRESS[0]			 = Holding_reg[IPREG1];
							IP_ADDRESS[1]			 = Holding_reg[IPREG2];
							IP_ADDRESS[2]			 = Holding_reg[IPREG3];
							IP_ADDRESS[3]			 = Holding_reg[IPREG4];
							NETMASK_ADDRESS[0] = Holding_reg[MREG1];
							NETMASK_ADDRESS[1] = Holding_reg[MREG2];
							NETMASK_ADDRESS[2] = Holding_reg[MREG3];
							NETMASK_ADDRESS[3] = Holding_reg[MREG4];
							GATEWAY_ADDRESS[0] = Holding_reg[GREG1];
							GATEWAY_ADDRESS[1] = Holding_reg[GREG2];
							GATEWAY_ADDRESS[2] = Holding_reg[GREG3];
							GATEWAY_ADDRESS[3] = Holding_reg[GREG4];
							MACAddr[0] = Holding_reg[MAC1];
							MACAddr[1] = Holding_reg[MAC2];
							MACAddr[2] = Holding_reg[MAC3];
							MACAddr[3] = Holding_reg[MAC4];
							MACAddr[4] = Holding_reg[MAC5];
							MACAddr[5] = Holding_reg[MAC6];
				
							Holding_reg[MsREG] = FLASH_Read(adrcount++);
				
							if(Holding_reg[MsREG]==0)
							{
								Holding_reg[MsREG] = 1;
							}

							MSREGADR = Holding_reg[MsREG];
							NumPhones = FLASH_Read(adrcount++);
							for(int i =0;i<10;i++)
								{
								for(int j =0;j<13;j++)
											{
												Phones[i][j] = FLASH_Read(adrcount++);
											}
								}
										
							
							SpeedSl = 0;
							SpeedSl |= FLASH_Read(adrcount++)<<24;
							SpeedSl |= FLASH_Read(adrcount++)<<16;
							SpeedSl |= FLASH_Read(adrcount++)<<8;
							SpeedSl |= FLASH_Read(adrcount++);
							
							SpeedMs |= FLASH_Read(adrcount++)<<24;
							SpeedMs |= FLASH_Read(adrcount++)<<16;
							SpeedMs |= FLASH_Read(adrcount++)<<8;
							SpeedMs |= FLASH_Read(adrcount++);
							
							GatewayON = FLASH_Read(adrcount++);
											
											for(int i =0;i<51;i++)
											{
												smsMSG[1][i] = FLASH_Read(adrcount++);
											}
											
											for(int i =0;i<51;i++)
											{
												smsMSG[2][i] = FLASH_Read(adrcount++);
											}
											
											for(int i =0;i<51;i++)
											{
												smsMSG[3][i] = FLASH_Read(adrcount++);
											}
											
											for(int i =0;i<51;i++)
											{
												smsMSG[4][i] = FLASH_Read(adrcount++);
											}
											
											CutTime = FLASH_Read(adrcount++);
											
											DelLog = FLASH_Read(adrcount++);
											
							Timeout |= FLASH_Read(adrcount++)<<24;
							Timeout |= FLASH_Read(adrcount++)<<16;
							Timeout |= FLASH_Read(adrcount++)<<8;
							Timeout |= FLASH_Read(adrcount++);
											
							SendingEnable = FLASH_Read(adrcount++);
								if(SendingEnable == 0x55)
								{
									SendingEnable = 1;
								}
					}
					else
					{
						
						Holding_reg[IPREG1] = 192;
							Holding_reg[IPREG2] = 168;
							Holding_reg[IPREG3] = 5;
							Holding_reg[IPREG4] = 241;
							
							Holding_reg[MREG1] = 255;
							Holding_reg[MREG2] = 255;
							Holding_reg[MREG3] = 255;
							Holding_reg[MREG4] = 0;
							
							Holding_reg[GREG1] = 192;
							Holding_reg[GREG2] = 168;
							Holding_reg[GREG3] = 5;
							Holding_reg[GREG4] = 240;
							
							Holding_reg[MAC1] = 2;
							Holding_reg[MAC2] = 0;
							Holding_reg[MAC3] = 0;
							Holding_reg[MAC4] = 0;
							Holding_reg[MAC5] = 0;
							Holding_reg[MAC6] = 0;

							
							IP_ADDRESS[0]			 = Holding_reg[IPREG1];
							IP_ADDRESS[1]			 = Holding_reg[IPREG2];
							IP_ADDRESS[2]			 = Holding_reg[IPREG3];
							IP_ADDRESS[3]			 = Holding_reg[IPREG4];
							NETMASK_ADDRESS[0] = Holding_reg[MREG1];
							NETMASK_ADDRESS[1] = Holding_reg[MREG2];
							NETMASK_ADDRESS[2] = Holding_reg[MREG3];
							NETMASK_ADDRESS[3] = Holding_reg[MREG4];
							GATEWAY_ADDRESS[0] = Holding_reg[GREG1];
							GATEWAY_ADDRESS[1] = Holding_reg[GREG2];
							GATEWAY_ADDRESS[2] = Holding_reg[GREG3];
							GATEWAY_ADDRESS[3] = Holding_reg[GREG4];
							MACAddr[0] = Holding_reg[MAC1];
							MACAddr[1] = Holding_reg[MAC2];
							MACAddr[2] = Holding_reg[MAC3];
							MACAddr[3] = Holding_reg[MAC4];
							MACAddr[4] = Holding_reg[MAC5];
							MACAddr[5] = Holding_reg[MAC6];
				
							Holding_reg[MsREG] = 1;
				
							MSREGADR = Holding_reg[MsREG];
							strcpy(smsnum,"+7XXXXXXXXXX");
							
							SpeedSl = 115200;
							SpeedMs = 115200;
														
							GatewayON = 0;
							
							strcpy(&smsMSG[1][0],"0.0.0.0:0");
							
							strcpy(&smsMSG[2][0]," ");
							
							strcpy(&smsMSG[3][0]," ");
							
							strcpy(&smsMSG[4][0]," ");
				
											CutTime = 60;
											
											DelLog = 0;
											
							Timeout = 500;
						
						osSemaphoreRelease(SemMem);
					}
	}
}
	
	MX_USART2_UART_Init(SpeedMs);
  MX_USART1_UART_Init(SpeedSl);
	
	/////////////////////////////////////////////////////////////////////
	//Загрузка девайсов
	osSemaphoreWait(SemSaveDev, portMAX_DELAY);

if(!ServiceMode)
{
		if(file!=0)
			{
				taskENTER_CRITICAL();
				ff_err = f_open(file, "DevConf.txt", FA_OPEN_EXISTING | FA_READ);
				taskEXIT_CRITICAL();
				
				if(ff_err==FR_OK)
				{
					unsigned char NumDev = 0;
					
					taskENTER_CRITICAL();
					ff_err = f_read(file, &NumDev, 1, &c);
					taskEXIT_CRITICAL();
					
					if(NumDev!=0)
					{
						for(int i=0;i<NumDev;i++)
						{
							memset(&NewDev,0,sizeof(NewDev));
							memset(&DevBuf,0,sizeof(DevBuf));
							b =  sizeof(struct devices);
							
							taskENTER_CRITICAL();
							ff_err = f_read(file, &DevBuf, b, &c);
							taskEXIT_CRITICAL();
							if(c==b)
							{
								
								
								if(DevBuf.type3 == 3)
								{
									strcpy(NewDev.regs,DevBuf.regsH);
									strcpy(NewDev.floats,DevBuf.floatsH);
									
									NewDev.addr =  DevBuf.addr;
									NewDev.interface =  DevBuf.interface;
									NewDev.ipadd[0] =  DevBuf.ipadd[0];
									NewDev.ipadd[1] =  DevBuf.ipadd[1];
									NewDev.ipadd[2] =  DevBuf.ipadd[2];
									NewDev.ipadd[3] =  DevBuf.ipadd[3];
									NewDev.port = DevBuf.port;
									NewDev.time = DevBuf.time;
									NewDev.log =  DevBuf.log;
									NewDev.type3 =  DevBuf.type3;
									NewDev.type4 =  0;
									
									CreateMSlaveTask(&NewDev);
									osDelay(1500);
								}
								
								osDelay(200);
								if(DevBuf.type4 == 4)
								{
									strcpy(NewDev.regs,DevBuf.regsI);
									strcpy(NewDev.floats,DevBuf.floatsI);
									
									NewDev.addr =  DevBuf.addr;
									NewDev.interface =  DevBuf.interface;
									NewDev.ipadd[0] =  DevBuf.ipadd[0];
									NewDev.ipadd[1] =  DevBuf.ipadd[1];
									NewDev.ipadd[2] =  DevBuf.ipadd[2];
									NewDev.ipadd[3] =  DevBuf.ipadd[3];
									NewDev.port = DevBuf.port;
									NewDev.time = DevBuf.time;
									NewDev.log =  DevBuf.log;
									NewDev.type3 =  0;
									NewDev.type4 =  DevBuf.type4;
									
									CreateMSlaveTask(&NewDev);
									osDelay(1500);
								}
								osDelay(200);
							}
						}
					}
					f_close(file);
					
					exDev = 1;
					
					osSemaphoreRelease(SemSaveDev);
				}
				else
				{
					//считываем из внутреннего flash
					unsigned char NumDev = 0;
					
							NumDev = FLASH_Read(BASEADRDEV);
					if(NumDev!=0&&NumDev<0xFF)
					{
						unsigned int adrcounter = BASEADRDEV+1;
						for(int i = 0;i<NumDev;i++)
						{
							memset(&NewDev,0,sizeof(NewDev));
							memset(&DevBuf,0,sizeof(DevBuf));
						
								DevBuf.addr = FLASH_Read(adrcounter++);
								DevBuf.interface = FLASH_Read(adrcounter++);
								DevBuf.ipadd[0] = FLASH_Read(adrcounter++);
								DevBuf.ipadd[1] = FLASH_Read(adrcounter++);
								DevBuf.ipadd[2] = FLASH_Read(adrcounter++);
								DevBuf.ipadd[3] = FLASH_Read(adrcounter++);
								DevBuf.port = 0;
								DevBuf.port |= FLASH_Read(adrcounter++)<<8;
								DevBuf.port |= FLASH_Read(adrcounter++);
								DevBuf.time = 0;
								DevBuf.time |= FLASH_Read(adrcounter++)<<8;
								DevBuf.time |= FLASH_Read(adrcounter++);
								DevBuf.log = FLASH_Read(adrcounter++);
								DevBuf.type3 = FLASH_Read(adrcounter++);
								DevBuf.type4 = FLASH_Read(adrcounter++);
						
						for(int j=0;j<51;j++)
						{
							DevBuf.regsH[j]=FLASH_Read(adrcounter++);
						}
						for(int j=0;j<51;j++)
						{
							DevBuf.regsI[j]=FLASH_Read(adrcounter++);
						}
						for(int j=0;j<300;j++)
						{
							DevBuf.floatsH[j]=FLASH_Read(adrcounter++);
						}
						for(int j=0;j<300;j++)
						{
							DevBuf.floatsI[j]=FLASH_Read(adrcounter++);
						}
						
								if(DevBuf.type3 == 3)
								{
									strcpy(NewDev.regs,DevBuf.regsH);
									strcpy(NewDev.floats,DevBuf.floatsH);
									
									NewDev.addr =  DevBuf.addr;
									NewDev.interface =  DevBuf.interface;
									NewDev.ipadd[0] =  DevBuf.ipadd[0];
									NewDev.ipadd[1] =  DevBuf.ipadd[1];
									NewDev.ipadd[2] =  DevBuf.ipadd[2];
									NewDev.ipadd[3] =  DevBuf.ipadd[3];
									NewDev.port = DevBuf.port;
									NewDev.time = DevBuf.time;
									NewDev.log =  DevBuf.log;
									NewDev.type3 =  DevBuf.type3;
									NewDev.type4 =  0;
										
									CreateMSlaveTask(&NewDev);
									osDelay(300);
								}
								
								osDelay(200);
								if(DevBuf.type4 == 4)
								{
									strcpy(NewDev.regs,DevBuf.regsI);
									strcpy(NewDev.floats,DevBuf.floatsI);
									
									NewDev.addr =  DevBuf.addr;
									NewDev.interface =  DevBuf.interface;
									NewDev.ipadd[0] =  DevBuf.ipadd[0];
									NewDev.ipadd[1] =  DevBuf.ipadd[1];
									NewDev.ipadd[2] =  DevBuf.ipadd[2];
									NewDev.ipadd[3] =  DevBuf.ipadd[3];
									NewDev.port = DevBuf.port;
									NewDev.time = DevBuf.time;
									NewDev.log =  DevBuf.log;
									NewDev.type3 =  0;
									NewDev.type4 =  DevBuf.type4;
									
									CreateMSlaveTask(&NewDev);
									osDelay(1500);
								}
								osDelay(200);
							}
						
						
					}
				}
			}
		}
	/////////////////////////////////////////////////////////////////////
	//Закгрузка событий
	osSemaphoreWait(SemSaveEv, portMAX_DELAY);
		if(!ServiceMode)
{
		
	if(file!=0)
	{
	taskENTER_CRITICAL();
	ff_err = f_open(file, "EventConf.txt", FA_OPEN_EXISTING | FA_READ);
	taskEXIT_CRITICAL();
	if(ff_err==FR_OK)
	{
		unsigned char NumEv = 0;
		
		taskENTER_CRITICAL();
		ff_err = f_read(file, &NumEv, 1, &c);
		taskEXIT_CRITICAL();
		
		if(NumEv!=0)
		{
			for(int i=0;i<NumEv;i++)
			{
				memset(&NewEv,0,sizeof(NewEv));
				memset(&EvBuf,0,sizeof(EvBuf));
				b =  sizeof(struct events);
				
				//taskENTER_CRITICAL();
				ff_err = f_read(file, &EvBuf, b, &c);
				//taskEXIT_CRITICAL();
				if(c==b)
				{
					NewEv.dadr = EvBuf.dadr;
					NewEv.f1 = EvBuf.f1;
					NewEv.f2 = EvBuf.f2;
					NewEv.interface = EvBuf.interface;
					NewEv.log = EvBuf.log;
					NewEv.Nreg = EvBuf.Nreg;
					NewEv.param = EvBuf.param;
					NewEv.rele = EvBuf.rele;
					NewEv.sms = EvBuf.sms;
					NewEv.sr = EvBuf.sr;
					NewEv.t1 = EvBuf.t1;
					NewEv.t2 = EvBuf.t2;
					NewEv.tsr = EvBuf.tsr;
					strcpy(NewEv.smstext,EvBuf.smstext);
					strcpy(NewEv.name,EvBuf.name);
					
					CreatingEventTask(&NewEv);
					osDelay(1000);
				}
				osDelay(200);
			}
		}
		f_close(file);
		
		exEv = 1;
		
		osSemaphoreRelease(SemSaveEv);
	}
	else
	{
		//считываем из внутреннего flash
				unsigned char NumEv = 0;
				unsigned int intbuf;
		
				NumEv = FLASH_Read(BASEADREVENT);
		if(NumEv!=0&&NumEv<0xFF)
		{
				memset(&NewEv,0,sizeof(NewEv));
				memset(&EvBuf,0,sizeof(EvBuf));
			
			unsigned int adrcounter = BASEADREVENT+1;
			for(int i = 0;i<NumEv;i++)
			{
					NewEv.dadr = FLASH_Read(adrcounter++);
					NewEv.f1 = FLASH_Read(adrcounter++);
					NewEv.f2 = FLASH_Read(adrcounter++);
					NewEv.interface = FLASH_Read(adrcounter++);
					NewEv.log = FLASH_Read(adrcounter++);
					NewEv.Nreg = 0;
					NewEv.Nreg |= FLASH_Read(adrcounter++)<<8;
					NewEv.Nreg |= FLASH_Read(adrcounter++);
					NewEv.param = 0;
					intbuf = 0;
					intbuf |= FLASH_Read(adrcounter++)<<24;
					intbuf |= FLASH_Read(adrcounter++)<<16;
					intbuf |= FLASH_Read(adrcounter++)<<8;
					intbuf |= FLASH_Read(adrcounter++);
					NewEv.param = *(float*)&intbuf;
					NewEv.rele = FLASH_Read(adrcounter++);
					NewEv.sms = FLASH_Read(adrcounter++);
					NewEv.sr = FLASH_Read(adrcounter++);
					NewEv.t1 = FLASH_Read(adrcounter++);
					NewEv.t2 = FLASH_Read(adrcounter++);
					NewEv.tsr = FLASH_Read(adrcounter++);
					
								for(int j=0;j<51;j++)
								{
									NewEv.smstext[j]=FLASH_Read(adrcounter++);
								}
								for(int j=0;j<25;j++)
								{
									NewEv.name[j]=FLASH_Read(adrcounter++);
								}
				
					CreatingEventTask(&NewEv);
					osDelay(1000);
			}
			
		}

	}
	}
}

	vPortFree(file);
	/////////////////////////////////////////////////////////////////////
	osSemaphoreRelease(SemFiles);
	
	LOADING = 1;
	
  /* USER CODE BEGIN StartTask02 */
  /* Infinite loop */
  for(;;)
  {
		osThreadTerminate(NULL);
		osDelay(10000);
  }
  /* USER CODE END StartTask02 */
}


/*
Обработка ответного пакета ModbusRTU Master
*/
void MbusRTUMaster(void const * argument)
{
	portBASE_TYPE taskWoken = pdTRUE; 
  for(;;)
  {
		osSemaphoreWait(xMbusM, portMAX_DELAY);
		//xSemaphoreTakeFromISR(xMbusM, &taskWoken);
		CLEARBIT(bit_M);
		RTU_M();
  }
}

/*
Запуск GSM модема
*/
void StartGSM(void const * argument)
{
  /* USER CODE BEGIN StartTask04 */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6,0);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7,0);
	//osDelay(10000);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10,1); // Действия, направленные на запуск GSM модема
	//HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13,1);
	osDelay(50);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7,1);
	osDelay(300);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6,1);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7,0);
osDelay(2000);
	
	// reset
	//osDelay(30);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13,0);
	osDelay(10);
//HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13,1);  //

//ON
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10,0);

	for(uint16_t i=0;i<2200;i++)
	{
		
	}
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10,1);// Конец включения GSM модема

	
	//osDelay(12000);
	//GSM_LED_OFF;
  /* Infinite loop */
	osDelay(12000);
	

					FRESULT file_err;
					err_t err;
					DIR fdir;
					FILINFO fno;
					uint32_t size;
					char *fn;
					#if _USE_LFN // Если настроено LFN, то выделяем буфер для LFN
						static TCHAR lfn[_MAX_LFN + 1]={0};
					strcpy(fno.fname,lfn);
					//fno.fname = lfn;
						fno.fsize = sizeof lfn;
					#endif
		
	while(!LOADING)
	{
		osDelay(1000);
	}
	
	osSemaphoreWait(SemSending, portMAX_DELAY);
	
	//MX_USB_HOST_Init();
	
	InitGSM(); // Первичная инициализация модема
	
	// МБ заменить кейсом
	if(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR10) == 0x3822) // Успешно прошилось
	{
		HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR10,0x0000);
// Отправляем СМС
								SendSMS5("Updating SUCCESS");

	}
	else if(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR10) == 0x8835) // Не корректный файл прошивки
	{
		HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR10,0x0000);
// Отправляем СМС

								SendSMS5("Updating ERROR. Wrong file.");

	}
	else
	{
		HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR10,0x0000);
	}
	osSemaphoreRelease(SemSending);
	
		//WriteFileToGSM("123.txt",1024);
	osDelay(5000);
	

						unsigned int SectorCounter = 0;
						unsigned int IterationStatus = 0;

	//TCP_GSM_Listener();
//	TCP_GSM();
  for(;;)
  {
			if(SDFatFs.fs_type != 0)
					{

					file_err = f_opendir(&fdir, "\\");
					if ( (file_err == FR_OK) && (SendingEnable || SendFromSector))
					{
						if((IterationStatus == 1) && (SectorCounter == 0)&&(SendFromSector==1))
						{
							SendFromSector = 0;
						}
						IterationStatus = 0;
						SectorCounter = 0;
						for ( ;; )
						{
							osSemaphoreWait(SemRR, portMAX_DELAY);
							
							if(NeedToUpdate)
							{
								osSemaphoreWait(SemSending, portMAX_DELAY);
								uint8_t errU = CheckUpdates();
									if(errU)
									{
										NeedToUpdate = 0;
										HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR10,0x0707); // Это мы говорим бутлодеру, что прошивка была из интернета, пока хз зачем
										NVIC_SystemReset();
									}
									else
									{
										osSemaphoreRelease(SemSending);
									}
							}
							
							
								taskENTER_CRITICAL();
							file_err = f_readdir(&fdir, &fno);
								taskEXIT_CRITICAL();
							if (file_err != FR_OK || fno.fname[0] == 0 || !(SendingEnable || SendFromSector))
							{
								if(file_err == FR_DISK_ERR)
								{
									FileErr++;
								}
								osSemaphoreRelease(SemRR);
								break;
							}
							
							if (fno.fname[0] == '.') continue;
						
							uint8_t excurrent = 0;
							for(int i = 0;i < NumQw;i++)
							{
								if((strcmp(StructQwAdr[i]->filename,&fno.fname[0]) == 0)) // Если файл, который еще пишется, то далее не будем отправлять
								{
									excurrent = 1;
									break;
								}
							}
							
							if((strcmp(&fno.fname[0],"LogEVENT.csv") == 0) && !ReadyToSendEvent) // Если журнал событий не готов к отправке
							{
								excurrent = 1;
							}
							
							if(strcmp(&fno.fname[0],"boot.bin") == 0)
							{
								excurrent = 1;
							}
						
							
							
										FILINFO fno1;
										//	taskENTER_CRITICAL();
										ff_err = f_stat(&fno.fname[0],&fno1);
										//	taskEXIT_CRITICAL();
											if(file_err == FR_DISK_ERR)
												{
													FileErr++;
												}
							
									if(SendFromSector) // Если файл надо отправить файл из временного сектора, то отправляем
									{
										unsigned int fulltime = 0;
										u32_t d,mon,y,h,min,s;
											y = ((fno.fdate & 0xFE00)>>9)-20;
											mon = (fno.fdate & 0x01E0)>>5;
											d = (fno.fdate & 0x001F);
											
											h = (fno.ftime & 0xF800)>>11;
											min = (fno.ftime & 0x07E0)>>5;
										
											y = ((fno.fdate & 0xFE00)>>9)+1980-2000;
											mon = (fno.fdate & 0x01E0)>>5;
											d = (fno.fdate & 0x001F);
											
											h = (fno.ftime & 0xF800)>>11;
											min = (fno.ftime & 0x07E0)>>5;
										
										
										fulltime  = (((DWORD)y+20) << 25)
															| ((DWORD)mon << 21)
															| ((DWORD)d << 16)
															| ((DWORD)h << 11)
															| ((DWORD)min << 5)
															| ((DWORD)0 >> 1);
										if(fulltime >= StartSector && fulltime <= EndSector)
										{
											excurrent = 0;
										}
										else
										{
											excurrent = 1;
										}
									}
							IterationStatus = 1;
							if((excurrent == 0) && ((fno1.fattrib & AM_ARC) == AM_ARC)) // Нужно ли отправлять файл
							{
							osSemaphoreWait(SemSending, portMAX_DELAY);
								
								if(SendFromSector)
								{
									SectorCounter++;
								}
								
								char err;
								SendingFile = &fno.fname[0];
								err = TCP_GSM(&fno.fname[0]); // Отправка файла
								SendingFile = 0;
								
								if(strcmp(&fno.fname[0],"LogEVENT.csv") == 0)
									 {
											ReadyToSendEvent = 0;
									 }
								
							osSemaphoreRelease(SemSending);
								
								if(err == 0)
								{
									 if(strcmp(&fno.fname[0],"LogEVENT.csv") != 0)
									 {
										 if(DelLog)	// Если надо удалить, то удаляем, если нет, то меняем атрибут файла, чтоб убрать из очереди на отправку
										 {
												taskENTER_CRITICAL();
											ff_err = f_unlink(&fno.fname[0]);
												taskEXIT_CRITICAL();
										 }
										 else
										 {
												taskENTER_CRITICAL();
											 file_err = f_chmod(&fno.fname[0],0,AM_ARC);//изменение атрибута файла
												taskEXIT_CRITICAL();
										 }
									 }
									 else
									 {
													taskENTER_CRITICAL();
											 file_err = f_chmod("LogEVENT.csv",0,AM_ARC);//изменение атрибута файла
													taskEXIT_CRITICAL();
									 }
									
								}
							}
							osSemaphoreRelease(SemRR);
						}
					}
					else
					{
						if(file_err != FR_OK)
						{
							FileErr++;
						}
						
						/*
								if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
								{

									Error_Handler();
								}
						*/
					}
			
		//osThreadTerminate(NULL);
		osDelay(30000); // 60000
				}
				else
				{
					osDelay(1000);
				}
  }
	
  /* USER CODE END StartTask04 */
}


/*
Обработка принятого пакета ModbusRTU Slave
*/
void MbusRTUSlave(void const * argument)
{
  for(;;)
  {
    osSemaphoreWait(xMbusS, portMAX_DELAY); 
		CLEARBIT(bit_S);
		RTU_S();
  }
}


/*
Сохранение устройств
*/
void SavingDevices(void const * argument)
{
	
	while(!LOADING)
	{
		osDelay(1000);
	}

	for(;;)
	{

			// Ждем флага на сохранение параметров
			osSemaphoreWait(SemSaveDev, portMAX_DELAY);
			osSemaphoreWait(SemFlash, portMAX_DELAY);
			//Пишем устройства во внутреннюю flash
				HAL_StatusTypeDef	flash_ok = HAL_ERROR ; //HAL_ERROR
	
				do
				{
				flash_ok = HAL_FLASH_Unlock();
				}
				while(flash_ok != HAL_OK);
				FLASH_Erase_Sector(FLASH_SECTOR_10, VOLTAGE_RANGE_3);
				osDelay(3000);
		
				/////сохранение
				flash_ok = HAL_ERROR;
	flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, BASEADRDEV, NumQw);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, BASEADRDEV, NumQw);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, BASEADRDEV, NumQw);
	//while(flash_ok!=HAL_OK);

				
				if(NumQw > 0)
				{
					int adrcount = BASEADRDEV+1;

					for(int i = 0;i<NumQw;i++)
					{

								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->addr);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->interface);
								//while(flash_ok!=HAL_OK);
						
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->ipadd[0]);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->ipadd[1]);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->ipadd[2]);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->ipadd[3]);
								//while(flash_ok!=HAL_OK);
								
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (StructQwAdr[i]->port&0xFF00)>>8);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->port&0x00FF);
								//while(flash_ok!=HAL_OK);
						
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (StructQwAdr[i]->time&0xFF00)>>8);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->time&0x00FF);
								//while(flash_ok!=HAL_OK);
								
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->log);
								//while(flash_ok!=HAL_OK);
								
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->type3);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->type4);
								//while(flash_ok!=HAL_OK);
								
								for(int j=0;j<51;j++)
								{
									flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->regsH[j]);
									//while(flash_ok!=HAL_OK);
								}
								
								for(int j=0;j<51;j++)
								{
									flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->regsI[j]);
									//while(flash_ok!=HAL_OK);
								}
								
								for(int j=0;j<300;j++)
								{
									flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->floatsH[j]);
									//while(flash_ok!=HAL_OK);
								}
								
								for(int j=0;j<300;j++)
								{
									flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructQwAdr[i]->floatsI[j]);
									//while(flash_ok!=HAL_OK);
								}
						
					}
				}

				
				do
				{
				flash_ok = HAL_FLASH_Lock();
				}
				while(flash_ok != HAL_OK);
				
				osSemaphoreRelease(SemFlash);
				
				if(exDev)
				{
					taskENTER_CRITICAL();
				ff_err = f_unlink("DevConf.txt");
					taskEXIT_CRITICAL();
					
					exDev = 0;
				}

	}
}


/*
Циклический буффер SD карты по фиксированному проценту заполнения
*/
void RoundRobin(void const * argument)
{
	DWORD fre_clust;
	float fre_sect, tot_sect;
	FATFS *fs = &SDFatFs;
	FRESULT ff_err1;
	
					 FRESULT file_err;
					err_t err;
					DIR fdir;
					FILINFO fno;
					uint32_t size,lasttime = 0xffffffff;
					char *fn;   /* This function is assuming non-Unicode cfg. */
					char smon[4]={0},lastname[40]={0};
					u32_t d,mon,y,h,min,s,t;
					#if _USE_LFN // Если настроено LFN, то выделяем буфер для LFN
						static TCHAR lfn[_MAX_LFN + 1]={0};   /* Buffer to store the LFN */
					strcpy(fno.fname,lfn); 
					//fno.fname = lfn;
						fno.fsize = sizeof lfn;
					#endif
	
	while(!LOADING)
	{
		osDelay(1000);
	}
		
  for(;;)
  {
		// вычисление свободной памяти
		//osSemaphoreWait(SemFiles, portMAX_DELAY);
		osSemaphoreWait(SemRR, portMAX_DELAY);
		
		if(SDFatFs.fs_type != 0)
		{
		
		AGAIN:
			taskENTER_CRITICAL();
    ff_err1 = f_getfree(SDPath, &fre_clust, &fs);
			taskEXIT_CRITICAL();
		
		lasttime = 0xffffffff;
		memset(lastname,0,40);

		if(ff_err1 == FR_OK)
		{
			tot_sect = (fs->n_fatent - 2) * fs->csize;
			fre_sect = fre_clust * fs->csize;
			percent = 100*fre_sect/tot_sect;

			sprintf(&smsMSG[10][0],"%f %%",percent);
			if(percent < 10)
			{
				//Если объем флэши меньше 10%, надо запускать процедуру очистки
						SMSfullFLASH = 1;
					file_err = f_opendir(&fdir, "\\");
						
					if ( file_err == FR_OK )
					{
						for ( ;; )
						{
								taskENTER_CRITICAL();	
							file_err = f_readdir(&fdir, &fno);
								taskEXIT_CRITICAL();
							if (file_err != FR_OK || fno.fname[0] == 0) break;
							if (fno.fname[0] == '.') continue;
							
							
								y = ((fno.fdate & 0xFE00)>>9)+1980-2000;
								mon = (fno.fdate & 0x01E0)>>5;
								d = (fno.fdate & 0x001F);
								
								h = (fno.ftime & 0xF800)>>11;
								min = (fno.ftime & 0x07E0)>>5;
							
								t = 0;
								t =	(((DWORD)y+20) << 25)
								| ((DWORD)mon << 21)
								| ((DWORD)d << 16)
								| ((DWORD)h << 11)
								| ((DWORD)min << 5);
							
							if(lasttime>t)
							{
								memset(&lastname,0,40);
								strcpy(lastname,fno.fname);
								lasttime = t;
							}
							
						}
					}
					
							uint8_t excurrent = 0;
					for(int i = 0;i < NumQw;i++) // Если логируемый файл, то пропускаем
							{
								if((strcmp(StructQwAdr[i]->filename,&lastname[0]) == 0))
								{
									excurrent = 1;
									break;
								}
							}
							
							if((strcmp(SendingFile,&lastname[0])==0) || (strcmp("LogEVENT.csv",&lastname[0])==0)) // Если файл находится в отправке, то пропускаем
							{
									excurrent = 1;
									break;
							}
						
							if(excurrent==0)
							{
								taskENTER_CRITICAL();
							ff_err = f_unlink(lastname);
								taskEXIT_CRITICAL();
							}
				osDelay(500);
				goto AGAIN;
			}
		}
		else if(ff_err1 == FR_DISK_ERR)
		{
				if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
				{
					Error_Handler();
				}
				goto AGAIN;
		}
		
		}
		
		osSemaphoreRelease(SemRR);
		//osSemaphoreRelease(SemFiles);
		osDelay(3600000); //60000
  }
}


/*
Обработка входящего подключения ModbusTCP Slave
*/
void ModbusTCP_Slave(void const * argument)
{
	struct netconn *con, *newcon;
  err_t err, accept_err;
	int errM;
	
  con = netconn_new(NETCONN_TCP);

  if (con!= NULL)
  {
    err = netconn_bind(con, NULL, 300);
    if (err == ERR_OK)
    {
      netconn_listen(con);
  
      while(1) 
      {
        accept_err = netconn_accept(con, &newcon);
        if(accept_err == ERR_OK)
        {
					xTaskCreate((pdTASK_CODE)Modbus_TCP_Task,
                   "Modbus_TCP2",
                   512,
                   newcon,
                   osPriorityAboveNormal,
                   NULL
                  );
						osDelay(1000);
						//netconn_delete(newcon);
        }
      }
    }
  }
}



/*
Сохраниене событий
*/
void SaveEvents(void const * argument)
{
	while(!LOADING)
	{
		osDelay(1000);
	}
	
	for(;;)
	{
			// Ждем флага на сохранение параметров
			osSemaphoreWait(SemSaveEv, portMAX_DELAY);
			osSemaphoreWait(SemFlash, portMAX_DELAY);
			//сохранение событий во внутреннюю flash
			HAL_StatusTypeDef	flash_ok = HAL_ERROR ; //HAL_ERROR
	
				do
				{
				flash_ok = HAL_FLASH_Unlock();
				}
				while(flash_ok != HAL_OK);
				FLASH_Erase_Sector(FLASH_SECTOR_11, VOLTAGE_RANGE_3);
				osDelay(3000);
		
				/////сохранение
				flash_ok = HAL_ERROR;
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, BASEADREVENT, NumEvents);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, BASEADREVENT, NumEvents);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, BASEADREVENT, NumEvents);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, BASEADREVENT, NumEvents);


				
				if(NumEvents > 0)
				{
				int adrcount = BASEADREVENT+1;
					unsigned int intbuf;
					
					for(int i = 0;i<NumEvents;i++)
					{
						
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->dadr);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->f1);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->f2);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->interface);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->log);
								//while(flash_ok!=HAL_OK);
						
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (StructEventAdr[i]->Nreg&0xFF00)>>8);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->Nreg&0x00FF);
								//while(flash_ok!=HAL_OK);
						
								intbuf = *(unsigned int*)&StructEventAdr[i]->param;
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (intbuf&0xFF000000)>>24);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (intbuf&0x00FF0000)>>16);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (intbuf&0x0000FF00)>>8);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, intbuf&0x000000FF);
								//while(flash_ok!=HAL_OK);
						
						
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->rele);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->sms);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->sr);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->t1);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->t2);
								//while(flash_ok!=HAL_OK);
								flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->tsr);
								//while(flash_ok!=HAL_OK);
								
								for(int j=0;j<51;j++)
								{
									flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->smstext[j]);
									//while(flash_ok!=HAL_OK);
								}
								
								for(int j=0;j<25;j++)
								{
									flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, StructEventAdr[i]->name[j]);
									//while(flash_ok!=HAL_OK);
								}
						
						
					}
				}
				
				do
				{
				flash_ok = HAL_FLASH_Lock();
				}
				while(flash_ok != HAL_OK);
				
				osSemaphoreRelease(SemFlash);
				
				if(exEv)
				{
					taskENTER_CRITICAL();
				ff_err = f_unlink("EventConf.txt");
				taskEXIT_CRITICAL();
					
					exEv = 0;
				}
			
				

	}
}

/*
WEB Server
*/ 
void HTTP_Server(void const * argument)
{
	osDelay(1000);
	
	struct netconn *conn, *newconn;
  err_t err, accept_err;
	uint32_t regvalue = 0;

  conn = netconn_new(NETCONN_TCP);
  if (conn!= NULL)
  {
    err = netconn_bind(conn, NULL, 80);
    if (err == ERR_OK)
    {
			netconn_listen(conn);
  
      while(1) 
      {
				accept_err = netconn_accept(conn, &newconn);
        if(accept_err == ERR_OK)
        {
          http_server_serve(newconn);
          netconn_delete(newconn);
        }
      }
    }
  }
}



/*
Сохрание конфигурации устройства
*/
void SavingConf(void const * argument)
{
	
	while(!LOADING)
	{
		osDelay(1000);
	}
	
	for(;;)
	{
		osSemaphoreWait(SemMem, portMAX_DELAY); 
		osSemaphoreWait(SemFlash, portMAX_DELAY);
		
			HAL_StatusTypeDef	flash_ok = HAL_ERROR ; //HAL_ERROR
	
			do
			{
			flash_ok = HAL_FLASH_Unlock();
			}
			while(flash_ok != HAL_OK);
			FLASH_Erase_Sector(FLASH_SECTOR_9, VOLTAGE_RANGE_3);
			osDelay(3000);
			flash_ok = HAL_ERROR;

int adrcount = BASEADRCONF;
			
			flash_ok = HAL_ERROR;
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[IPREG1]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[IPREG2]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[IPREG3]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[IPREG4]);
				//while(flash_ok!=HAL_OK);

			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[MREG1]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[MREG2]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[MREG3]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[MREG4]);
				//while(flash_ok!=HAL_OK);

			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[GREG1]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[GREG2]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[GREG3]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[GREG4]);
				//while(flash_ok!=HAL_OK);
				
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[MAC1]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[MAC2]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[MAC3]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[MAC4]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[MAC5]);
				//while(flash_ok!=HAL_OK);
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[MAC6]);
				//while(flash_ok!=HAL_OK);
					
			flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Holding_reg[MsREG]);
				//while(flash_ok!=HAL_OK);
				
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, NumPhones);
				//while(flash_ok!=HAL_OK);

					for(int j =0;j<10;j++)
								{
								for(int i =0;i<13;i++)
											{
												flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Phones[j][i]);
												//while(flash_ok!=HAL_OK);
											}
								}
				
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (SpeedSl&0xFF000000)>>24);
				//while(flash_ok!=HAL_OK);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (SpeedSl&0x00FF0000)>>16);
				//while(flash_ok!=HAL_OK);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (SpeedSl&0x0000FF00)>>8);
				//while(flash_ok!=HAL_OK);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, SpeedSl&0x000000FF);
				//while(flash_ok!=HAL_OK);
				
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (SpeedMs&0xFF000000)>>24);
				//while(flash_ok!=HAL_OK);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (SpeedMs&0x00FF0000)>>16);
				//while(flash_ok!=HAL_OK);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (SpeedMs&0x0000FF00)>>8);
				//while(flash_ok!=HAL_OK);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, SpeedMs&0x000000FF);
				//while(flash_ok!=HAL_OK);
	
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, GatewayON);
					//while(flash_ok!=HAL_OK);
					
				for(int i = 0;i<51;i++)
				{
					flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, smsMSG[1][i]);
				//while(flash_ok!=HAL_OK);
				}
				
				for(int i = 0;i<51;i++)
				{
					flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, smsMSG[2][i]);
				//while(flash_ok!=HAL_OK);
				}
				
				for(int i = 0;i<51;i++)
				{
					flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, smsMSG[3][i]);
				//while(flash_ok!=HAL_OK);
				}
				
				for(int i = 0;i<51;i++)
				{
					flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, smsMSG[4][i]);
				//while(flash_ok!=HAL_OK);
				}
				
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, CutTime);
				//while(flash_ok!=HAL_OK);
				
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, DelLog);
				//while(flash_ok!=HAL_OK);
				
				
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (Timeout&0xFF000000)>>24);
				//while(flash_ok!=HAL_OK);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (Timeout&0x00FF0000)>>16);
				//while(flash_ok!=HAL_OK);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, (Timeout&0x0000FF00)>>8);
				//while(flash_ok!=HAL_OK);
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, Timeout&0x000000FF);
				//while(flash_ok!=HAL_OK);
				
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, SendingEnable);
				//while(flash_ok!=HAL_OK);
				
				flash_ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adrcount++, 0x55);
				//while(flash_ok!=HAL_OK);
					
					
				do
				{
				flash_ok = HAL_FLASH_Lock();
				}
				while(flash_ok != HAL_OK);

				osSemaphoreRelease(SemFlash);
				
				if(exConf)
				{
					taskENTER_CRITICAL();
				ff_err = f_unlink("config.txt");
					taskEXIT_CRITICAL();
					
					exConf = 0;
				}
	}
}


/*
Настройка через Modbus и квитирование
*/
//char Stat[1500];
void SetByModbus(void const * argument)
{
	FIL file3;
	FRESULT f_err;
	osStatus list;
	RTC_DateTypeDef sdatestructureget;
  RTC_TimeTypeDef stimestructureget;
	
	int c;
	
	while(!LOADING)
	{
		osDelay(1000);
	}
	
	for(;;)
	{
		
		if(Holding_reg[SUBDT])
		{
			SetTime(Holding_reg[HOUR], Holding_reg[MIN]);
			SetDate(Holding_reg[DAY], Holding_reg[MONTH], Holding_reg[YEAR]);
			
			
			Holding_reg[SUBDT] = 0;
			
			Holding_reg[HOUR] = 0;
			Holding_reg[MIN] = 0;
			Holding_reg[DAY] = 0;
			Holding_reg[MONTH] = 0;
			Holding_reg[YEAR] = 0;
		}
		
		if(!(HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_12)) || Holding_reg[KVIT] || !(HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_2)))
		{
			for(int i = 0;i< NumEvents;i++)
			{
				StructEventAdr[i]->StateEvent = 13;
			}
			Holding_reg[KVIT] = 0;
		}
		
		if(ReadyToSendEvent == 0) // Установка флага для отправки журнала событий
		{
			SendEventTimer++;
			if(SendEventTimer > CutTime * 60*3)
			{
				ReadyToSendEvent = 1;
			}
		}
		else
		{
			SendEventTimer = 0;
		}
		
		//припилить лог регистратора
		/*
		memset(Stat,0,1500);
		
		HAL_RTC_GetTime(&hrtc, &stimestructureget, RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc, &sdatestructureget, RTC_FORMAT_BIN);

			u32_t Ms;
			Ms = 1000*(stimestructureget.SecondFraction-stimestructureget.SubSeconds)/(stimestructureget.SecondFraction+1); // Вычисляем милисекунды
					
				sprintf(Stat+strlen(Stat),"%02d.%02d.%02d",sdatestructureget.Date, sdatestructureget.Month, 2000 + sdatestructureget.Year);	
				sprintf(Stat+strlen(Stat)," %02d:%02d:%02d.%03d\n",stimestructureget.Hours, stimestructureget.Minutes, stimestructureget.Seconds, Ms);
		list = osThreadList((unsigned char *)(Stat+strlen(Stat)));
		c = xPortGetFreeHeapSize();
		sprintf(Stat+strlen(Stat),"\nFile Errors: %d\n",FileErr);
	sprintf(Stat+strlen(Stat),"Free Heap Size(byte) / Heap Size(byte): %d  /  %d\n\n",c,configTOTAL_HEAP_SIZE);
			
//		taskENTER_CRITICAL();
//		f_err = f_open(&file3, "LogREG.txt", FA_OPEN_ALWAYS | FA_WRITE);
//			taskEXIT_CRITICAL();
//		if(f_err == FR_OK)
//			{
//				taskENTER_CRITICAL();
//					HAL_NVIC_DisableIRQ(USART2_IRQn);
//					HAL_NVIC_DisableIRQ(USART3_IRQn);
//				f_err = f_lseek(&file3, f_size(&file3));
//				f_err = f_write(&file3, Stat, strlen(Stat), &c);
//				f_err = f_close(&file3);
//					HAL_NVIC_EnableIRQ(USART3_IRQn);
//					HAL_NVIC_EnableIRQ(USART2_IRQn);
//				taskEXIT_CRITICAL();
//				
//			}
//			else
//			{
//				FileErr++;
//			}
		*/
		osDelay(500);
	}
}

/*
Чтение и парсинг входящих СМС
*/
void ReadingSMS(void const * argument)
{
	char SendSDFULL = 0;
	while(!LOADING)
	{
		osDelay(1000);
	}
	
	osDelay(60000);
	for(;;)
	{
		osSemaphoreWait(SemSending, portMAX_DELAY);
		ReadSMS();
		
		if(strstr(data_com3_IN, "KVIT")!=0) // Отправить смс "KVIT" для квитирования реле
		{
			Holding_reg[KVIT] = 1;
								SendSMS5("KVIT IS OK");

		}
		
		if(strstr(data_com3_IN, "Update")!=0) 
		{
			NeedToUpdate = 1;
		}
		
		if(strstr(data_com3_IN, "SendFromSector")!=0) // Отправить файлы из временного сектора (SendFromSector 21.02.19 14:25 11.01.19 15:25)
		{
			// Нужно привести время к 32 битному формату
			uint32_t Year1, Month1, Date1, Hours1, Minutes1, Seconds1 = 0;
			uint32_t Year2, Month2, Date2, Hours2, Minutes2, Seconds2 = 0;
			char *token_buff;
			char *target;
			
			StartSector = 0;
			EndSector = 0;
			target = strstr(data_com3_IN, "SendFromSector");
			strtok_r(target," ",&token_buff);
			Date1 = atoi((char*)strtok_r(NULL,".",&token_buff));
			Month1 = atoi((char*)strtok_r(NULL,".",&token_buff));
			Year1 = atoi((char*)strtok_r(NULL," ",&token_buff));
			Hours1 = atoi((char*)strtok_r(NULL,":",&token_buff));
			Minutes1 = atoi((char*)strtok_r(NULL," ",&token_buff));
			
			Date2 = atoi((char*)strtok_r(NULL,".",&token_buff));
			Month2 = atoi((char*)strtok_r(NULL,".",&token_buff));
			Year2 = atoi((char*)strtok_r(NULL," ",&token_buff));
			Hours2 = atoi((char*)strtok_r(NULL,":",&token_buff));
			Minutes2 = atoi((char*)strtok_r(NULL," ",&token_buff));
			
			StartSector = (((DWORD)Year1+20) << 25)
									| ((DWORD)Month1 << 21)
									| ((DWORD)Date1 << 16)
									| ((DWORD)Hours1 << 11)
									| ((DWORD)Minutes1 << 5)
									| ((DWORD)Seconds1 >> 1);
			
			EndSector = (((DWORD)Year2+20) << 25)
									| ((DWORD)Month2 << 21)
									| ((DWORD)Date2 << 16)
									| ((DWORD)Hours2 << 11)
									| ((DWORD)Minutes2 << 5)
									| ((DWORD)Seconds2 >> 1);
			
			SendFromSector = 1;
		}
		
		if(strstr(data_com3_IN, "StartSending")!=0) // Отправлять файлы на фтп
		{
			SendingEnable = 1;
		}
		
		if(strstr(data_com3_IN, "StopSending")!=0) // Не отправлять файлы на фтп
		{
			SendingEnable = 0;
		}

		
		if(strstr(data_com3_IN, "AddPhoneNum")!=0) // Отправить смс "AddPhoneNum +7*" для добавления номера СМС уведомлений
		{
			char *num, *target;
			target = strstr(data_com3_IN, "AddPhoneNum");
			num = strstr(target, "+7");
			if(num != 0)
			{
				if(NumPhones!=10)
				{
						memcpy(&Phones[NumPhones][0],num,12);
						SendSMS5("SMSNUM IS ADDED");
						NumPhones++;
				}
				else
				{
					memcpy(&Phones[9][0],num,12);
								SendSMS5("SMSNUM 10 IS CHANGED");
				}

			}
		}
		
		if(strstr(data_com3_IN, "DEL EVENTLOG")!=0) // Отправить смс "ChangeNum +7*" для изменения номера СМС уведомлений
		{
								taskENTER_CRITICAL();
							ff_err = f_unlink("LogEVENT.csv");
								taskEXIT_CRITICAL();
			if(ff_err == FR_OK)
			{
								SendSMS5("DELITING IS OK");
			}
			else
			{
								SendSMS5("DELITING IS NOT OK");
			}
			SMSfullFLASH = 0;
			SendSDFULL = 0;
		}
		
		if(SMSfullFLASH == 1 && !SendSDFULL)
		{
								SendSMS5("SD card 10%");
			SendSDFULL = 1;
		}
		
		//Сюда добавлять входящие СМС команды
		
		
		DeleteAllSMS();
		
		//Проверку на уровень gsm сигнала
		Holding_reg[1] = GSMQ();
		
		memset(data_com3_IN,0,300);
		
		osSemaphoreRelease(SemSending);
		osDelay(60000);
	}
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
