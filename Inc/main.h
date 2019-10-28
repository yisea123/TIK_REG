/**
  ******************************************************************************
  * File Name          : main.h
  * Description        : This file contains the common defines of the application
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/

/* USER CODE BEGIN Includes */
#define HTTPD_USE_CUSTOM_FSDATA 1
#define LWIP_NETIF_LINK_CALLBACK 1

#define SENDREPORT 0

#define VERSION 112

#define GSMMODE 1

#define swFloat 1

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/

/* USER CODE BEGIN Private defines */
#define HOLDING_REG_COUNT       110            // ????? ??????? ?????????  
#define Input_REG_COUNT         110            // ????? ??????? ??????? ?????????
#define HOLDING_M_REG_COUNT 		200
#define Input_M_REG_COUNT       200 
//extern uint8_t          buffer[900];
	 
	 
#define com_max         300                   
	 
#define SETBIT(b)       (bites |= (1<<b) )      // ????????? ???? b ? ????? ?
#define CLEARBIT(b)     (bites &= ~(1<<b))      // ????? ???? b ? ????? ?
#define CHECKBIT(b)     (bites & (1<<b))        // ???????? ???? b ? ????? ?  ?? 1

#define bit_S           0
#define bit_M           1
#define bit_tim         2
#define bit_save        3
#define bit_kl          4
#define bit_kl2         5
#define bit_H           6
#define bit_sob         7
#define bit_zap         8
#define bit_in_1        9
#define bit_in_2        10
#define bit_G        		11


extern unsigned int    bites, timer_S, timer_M, timer_G;

extern int    Holding_reg[HOLDING_REG_COUNT]; //Holding registers array
extern unsigned short int    Input_reg [Input_REG_COUNT];
/* USER CODE END Private defines */

void _Error_Handler(char *, int);

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)


struct device
{
	unsigned char addr, interface;			// адресс слэйв для опроса и нтерфейс
	unsigned char ipadd[4];
	unsigned short int port;
	unsigned char type3, type4;							// 3 или 4
	char log;														// логировать
	unsigned short time;								// период опроса и логирования
	char regs[51];
	char floats[300];
};


struct event
{
	char dadr,t1,t2,f1,f2,sr,tsr,rele,sms,log,interface;
	unsigned int Ntask;
	unsigned short int Nreg;
	float param;
	char smstext[51];
	char name[25];
	char StateEvent;
	unsigned int Rtime;
	//TaskHandle_t THandle;
};




#define	PLC						10 // Адрес PLC
#define	MsREG					11 // Адрес REG RTU
#define	SENDSMS				12 // Отправлять СМС?
#define	KVIT				13 // Квитировние

#define	LOG						20 // Писать лог?
#define	LOGFirst			21 // Первый регистр лога
#define	LOGSecond			22 // Последний регистр лога
#define	LOGTime				23 // Период лога

#define	SAVE					25 // Период лога

#define	HOUR					30 // Адрес PLC
#define	MIN						31 // Адрес RES RTU

#define	DAY						33 // Адрес PLC
#define	MONTH					34 // Адрес RES RTU
#define	YEAR					35 // Адрес RES RTU

#define	SUBDT					36 // Подтвердить изменения времени и даты

#define	IPREG1					40 // IP1
#define	IPREG2					41 // IP2
#define	IPREG3					42 // IP3
#define	IPREG4					43 // IP4

#define	MREG1					44 // M1
#define	MREG2					45 // M2
#define	MREG3					46 // M3
#define	MREG4					47 // M4

#define	GREG1					48 // G1
#define	GREG2					49 // G2
#define	GREG3					50 // G3
#define	GREG4					51 // G4

#define	MAC1					52 // M1
#define	MAC2					53 // M2
#define	MAC3					54 // M3
#define	MAC4					55 // M4
#define	MAC5					56 // M5
#define	MAC6					57 // M6



#define POWER_LED_ON		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, 1)
#define POWER_LED_OFF		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, 0)

#define REL_LED_ON				HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15,1)
#define REL_LED_OFF			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15,0)

#define MB_LED_ON			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, 1)
#define MB_LED_OFF			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, 0)

#define RELAY_ON				HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0,1);
#define RELAY_OFF				HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0,0);


void MX_USART2_UART_Init(unsigned int speed);
void MX_USART1_UART_Init(unsigned int speed);
void MX_USART3_UART_Init(unsigned int speed);
void MX_TIM6_Init(void);
void MX_TIM7_Init(void);
void MX_TIM2_Init(void);
unsigned char FLASH_Read(unsigned int address);


/**
  * @}
  */ 

/**
  * @}
*/ 

#endif /* __MAIN_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
