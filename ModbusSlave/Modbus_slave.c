// Modbus_RTU Functions
#include "main.h"
#include "Modbus_Slave.h"
//#include "usart.h"
#include "stm32f4xx_hal.h"
//#include "gpio.h"
//#include "main.h"
#include "ModBus_master.h"


extern xSemaphoreHandle SemMem;

unsigned int    n_com1, line1;
unsigned char   data_com1[com_max];

extern char MSREGADR;

extern unsigned char smsMSG[17][51];
extern char* smsnum;
extern unsigned char SMSbuf[com_max];

#define MB_ADRESS_M       0x01                   //Modbus device adress
#define BAUD_RATE_M       115200                   //USART baud rate value

//Connecting pins
#define USART1_RTS_PIN          GPIO_Pin_12    //USART RTS pin
#define USART1_TX_PIN           GPIO_Pin_9    //USART TX pin
#define USART1_RX_PIN           GPIO_Pin_10    //USART RX pin
#define USART1_port             GPIOA         //USART port





void CRC16_1 (unsigned int line)         
{
unsigned int j, a, n;
unsigned int reg;

reg=0xffff;   a=line;
for(j=0;j<line;j++)
  {
  reg^=data_com1[j];
  for(n=0; n<8;n++)
      {
      if((reg&0x01)==0x01) reg=(reg>>1)^0xA001;
            else           reg>>=1;
      }
  }
data_com1[a++]=reg;        // L
data_com1[a]=reg>>8;       // H
}

/*
Разбор Slave запроса
*/
void RTU_S(void)  
{
unsigned int ad, n, j; 
unsigned int dat, len;
	char smsbuf[30] = {0};
 
     if(data_com1[0]!= MSREGADR) goto m_eror1;    // 
     ad=n_com1;   
     n_com1=0;   
     if(ad<6) goto m_eror1;             // 
     n=data_com1[ad-1];                 // 
     j=data_com1[ad-2];
     CRC16_1(ad-2);
     if((n!=data_com1[ad-1])||(j!=data_com1[ad-2]))   goto m_eror1;   // 
     
					if(data_com1[1]==0x55)          // Чтение номера телефона
          {  
						strcpy(smsbuf, smsnum);
						data_com1[2] = strlen(smsnum);
						n = data_com1[2]+1;
						for(char i=0;i<n;i++)
						{
							data_com1[i+3] = smsbuf[i];
						}
						
						line1=data_com1[2]+3;
						CRC16_1(line1);
						line1 = line1 + 2; 						
						goto m_ok1;
					}
					
					if(data_com1[1]==0x56)          // Запись номера телефона
          {  
							len = data_com1[2];

							for(int i=0;i<len;i++)
							{
								SMSbuf[i] = data_com1[3+i];
								smsMSG[0][i] = data_com1[3+i];
							}

							line1=15;
							CRC16_1(line1);
							line1 = line1 + 2; 
							osSemaphoreRelease(SemMem);
							goto m_ok1;
					}
     
     if(data_com1[1]==0x03)          // ***********************************
          {  
          ad=data_com1[2]; ad<<=8; 
          ad|=data_com1[3]; ad++;             
          n=data_com1[5]<<1;      
          data_com1[2]=n;  
          j=3;         
          while(n!=0)
            {
            dat=Holding_reg[ad++];            
            data_com1[j++]=dat>>8;
            data_com1[j++]=dat;
            n-=2;
            }
          CRC16_1(j);
          line1=j+2;          
          goto m_ok1;
          }

     if(data_com1[1]==0x04)          
          {
          ad=data_com1[2]; ad<<=8;  
          ad|=data_com1[3];                    
          n=data_com1[5]<<1;                  
          data_com1[2]=n;        
          j=3;  ad++;       

                        while(n!=0)
                              {
                              dat=Input_reg[ad++];          
                              data_com1[j++]=dat>>8;             // H
                              data_com1[j++]=dat;                // L
                              n-=2;
                              }

          CRC16_1(j);
          line1=j+2; 
          goto m_ok1;
          }    

          
     if(data_com1[1]==0x06)          // ******************** 
          {
          n=data_com1[2];    n<<=8;  
          n|=data_com1[3];   n++;
          dat=data_com1[4]; dat<<=8;
          dat|=data_com1[5];
          Holding_reg[n]=dat;
          line1=8; 
          
          
m_ok1:    
          USART1->CR1  &=  ~USART_CR1_RE;              //Off 
          USART1->CR1  |=   USART_CR1_TE;               
          RTS1_SET;
          USART1->DR=data_com1[n_com1++];                
          
          return;
          }
     
m_eror1:  
     n_com1=0;
}



