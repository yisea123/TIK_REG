/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"
#include "cmsis_os.h"

/* USER CODE BEGIN 0 */
#include "stm32_ub_fatfs.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "ff.h"
#include "ModBus_master.h"
#include "Modbus_Slave.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "main.h"
unsigned char   timer_tim1, tim_kl, tim_in_1, tim_in_2;
unsigned char   buf_com[10][com_max],step=0;

unsigned int TimeoutCounter = 0;
extern unsigned int Timeout;



extern xSemaphoreHandle xMbusM;
extern xSemaphoreHandle xMbusS;
extern xSemaphoreHandle xMbusG;
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern ETH_HandleTypeDef EthHandle;
extern SD_HandleTypeDef hsd;
extern TIM_HandleTypeDef htim6;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

extern IWDG_HandleTypeDef hiwdg;

extern HCD_HandleTypeDef hhcd_USB_OTG_HS;


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


extern struct devices* WhichDevNowInPlaner;
extern char NumQw;

extern char GSMTCP_On;
/******************************************************************************/
/*            Cortex-M4 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles Non maskable interrupt.
*/
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
* @brief This function handles Hard fault interrupt.
*/
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
		NVIC_SystemReset();
  }
  /* USER CODE BEGIN HardFault_IRQn 1 */

  /* USER CODE END HardFault_IRQn 1 */
}

/**
* @brief This function handles Memory management fault.
*/
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN MemoryManagement_IRQn 1 */

  /* USER CODE END MemoryManagement_IRQn 1 */
}

/**
* @brief This function handles Pre-fetch fault, memory access fault.
*/
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN BusFault_IRQn 1 */

  /* USER CODE END BusFault_IRQn 1 */
}

/**
* @brief This function handles Undefined instruction or illegal state.
*/
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN UsageFault_IRQn 1 */

  /* USER CODE END UsageFault_IRQn 1 */
}

/**
* @brief This function handles Debug monitor.
*/
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
* @brief This function handles System tick timer.
*/
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  osSystickHandler();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
* @brief This function handles USART1 global interrupt.
*/
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */
	if((USART1->SR & USART_SR_RXNE)!=0) {
			//USART1->SR &=  ~USART_SR_RXNE; 
    if(!(CHECKBIT(bit_S)))  
			{
         timer_S=6; //20;  
            if(n_com1>=com_max) 	
							{
									n_com1=0;
							}
						else      
							{
									data_com1[n_com1]=USART1->DR;  
									n_com1++;  
							}
     }
	}
    
if((USART1->SR & USART_SR_TC)!=0)
    {
    USART1->SR &=  ~USART_SR_TC;
    if (n_com1>=line1)
          {
          USART1->CR1  &=  ~USART_CR1_TE;
          USART1->CR1  |=   USART_CR1_RE;
          RTS1_RESET;
					
	  n_com1=0;
          SETBIT(bit_zap);
          }
     else {
          USART1->DR=data_com1[n_com1++];
          }
    }
USART1->SR&=0;
  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/**
* @brief This function handles USART2 global interrupt.
*/
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */
if((USART2->SR & USART_SR_RXNE)!=0) 
		{
			if(!(CHECKBIT(bit_M)))  
				{  
					timer_M=6; //20;  
					if(n_master_com>=com_max) 	
					{
						n_master_com=0;     
					}
					else      
					{
						data_master_IN[n_master_com]=USART2->DR;  
						n_master_com++;  
					}

				}
		}
    
if((USART2->SR & USART_SR_TC)!=0) 
    {
    USART2->SR &=  ~USART_SR_TC;                      
    if (n_master_com>=line_master) 
			{     
				for(int i;i<60;i++)
				{
					
				}
				//сюда бахнуть мемсет
				memset(data_master_IN,0,com_max);
				n_master_com=0; 
				RTS_RESET; 
				MB_LED_OFF;
				for(int i;i<60;i++)
				{
					
				}
				USART2->CR1  &=  ~USART_CR1_TE;              //Off 
				USART2->CR1  |=   USART_CR1_RE;            	  
				
			}
     else 
			{
			USART2->DR=data_master_OUT[n_master_com++];      
			}
    }
//USART2->SR&=0; 
  /* USER CODE END USART2_IRQn 0 */
  HAL_UART_IRQHandler(&huart2);
  /* USER CODE BEGIN USART2_IRQn 1 */

  /* USER CODE END USART2_IRQn 1 */
}

/**
* @brief This function handles USART3 global interrupt.
*/
void USART3_IRQHandler(void)
{
  /* USER CODE BEGIN USART3_IRQn 0 */
if((USART3->SR & USART_SR_RXNE)!=0) 
		{
       if(n_com3>=com_max) 	
			 {
				 /*
				 memcpy(&buf_com[step][0],data_com3_IN,300);
				 step++;
				 n_com33=0;
				 data_com3_IN[n_com33]=USART3->DR; 
				 */
				 n_com33 = 0;
				 n_com3 = 0;
			 }
					else      
							{
							data_com3_IN[n_com33]=USART3->DR;  
							n_com33++;  
							}
                         
		}
    
if((USART3->SR & USART_SR_TC)!=0) 
    {
			USART3->SR &=  ~USART_SR_TC;                      
			if (n_com3>=line3) 
        { 
          USART3->CR1  &=  ~USART_CR1_TE;              //Off 
          USART3->CR1  |=   USART_CR1_RE;            	  
         // RTS_RESET3;
					n_com33 = 0;						
					n_com3=0; 
					step = 0;
         }
				else 
				{
         USART3->DR=data_com3_OUT[n_com3++];      
				}
    }
USART3->SR&=0; 
  /* USER CODE END USART3_IRQn 0 */
  HAL_UART_IRQHandler(&huart3);
  /* USER CODE BEGIN USART3_IRQn 1 */

  /* USER CODE END USART3_IRQn 1 */
}

/**
* @brief This function handles SDIO global interrupt.
*/
void SDIO_IRQHandler(void)
{
  /* USER CODE BEGIN SDIO_IRQn 0 */

  /* USER CODE END SDIO_IRQn 0 */
  HAL_SD_IRQHandler(&hsd);
  /* USER CODE BEGIN SDIO_IRQn 1 */

  /* USER CODE END SDIO_IRQn 1 */
}

/**
* @brief This function handles TIM6 global interrupt, DAC1 and DAC2 underrun error interrupts.
*/
void TIM6_DAC_IRQHandler(void)
{
	
	portBASE_TYPE taskWoken = pdTRUE;
  /* USER CODE BEGIN TIM6_DAC_IRQn 0 */
if(__HAL_TIM_GET_FLAG(&htim6, TIM_FLAG_UPDATE) != RESET)
  {
    if(__HAL_TIM_GET_IT_SOURCE(&htim6, TIM_IT_UPDATE) !=RESET)
    {
      __HAL_TIM_CLEAR_IT(&htim6, TIM_IT_UPDATE);
			
			//HAL_IWDG_Refresh(&hiwdg);
        
        if(timer_S>=2)  timer_S--;
              else      if(timer_S==1)
														{ 
														SETBIT(bit_S);  
														timer_S=0;
														osSemaphoreRelease(xMbusS);
														}
														
														
														
        if(timer_M>=2)  timer_M--;
              else      
								if(timer_M==1)
								{ 
									SETBIT(bit_M); 
									timer_M=0;
									//тут надо установить семафор для задачи разбора пакетов
									osSemaphoreRelease(xMbusM);
								//	xSemaphoreGiveFromISR(xMbusM, &taskWoken);
								}
								
								if(NumQw>0)
								{
									if(TimeoutCounter<(Timeout*10))
									{
										TimeoutCounter++;
									}
									else
									{
										TimeoutCounter = 0;
										
												if(WhichDevNowInPlaner->BadPackets>0x7fffffff)
												{
													WhichDevNowInPlaner->BadPackets=0;
												}
											WhichDevNowInPlaner->BadPackets++;
									
												
									}
								}
								else
								{
									TimeoutCounter = 0;
								}
								
			if(GSMTCP_On)
			{
				if(timer_G>=2)  timer_G--;
				else      
					if(timer_G==1)
					{ 
						SETBIT(bit_G); 
						timer_G=0;
						//тут надо установить семафор для задачи разбора пакетов
					osSemaphoreRelease(xMbusG);
					}
			}

								/*
        if(++timer_tim1>=100)   //      100 ??
            { 
              timer_tim1=0;
              if(timer_tim!=0){if(--timer_tim==0) SETBIT(bit_tim);}
              if(timer_H>=2)  timer_H--;
                 else      
										if(timer_H==1)
										{ 
											SETBIT(bit_H);  
											timer_H=0;}
										}   
*								*/								
 				}
			}
  /* USER CODE END TIM6_DAC_IRQn 0 */
  HAL_TIM_IRQHandler(&htim6);
  /* USER CODE BEGIN TIM6_DAC_IRQn 1 */

  /* USER CODE END TIM6_DAC_IRQn 1 */
}

/**
* @brief This function handles Ethernet global interrupt.
*/
void ETH_IRQHandler(void)
{
  /* USER CODE BEGIN ETH_IRQn 0 */

  /* USER CODE END ETH_IRQn 0 */
  HAL_ETH_IRQHandler(&EthHandle);
  /* USER CODE BEGIN ETH_IRQn 1 */

  /* USER CODE END ETH_IRQn 1 */
}

/* USER CODE BEGIN 1 */
void EXTI9_5_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI15_10_IRQn 0 */

  /* USER CODE END EXTI15_10_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
  /* USER CODE BEGIN EXTI15_10_IRQn 1 */

  /* USER CODE END EXTI15_10_IRQn 1 */
}



void OTG_HS_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_HS_IRQn 0 */

  /* USER CODE END OTG_HS_IRQn 0 */
  HAL_HCD_IRQHandler(&hhcd_USB_OTG_HS);
  /* USER CODE BEGIN OTG_HS_IRQn 1 */

  /* USER CODE END OTG_HS_IRQn 1 */
}


/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
