// MODBUS_M
#include "main.h"

void init_S(void);     //Init Modbus slave
void RTU_S(void);  
void CRC16_ (unsigned int line);        

#define RTS1_SET                 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET)
#define RTS1_RESET               HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET)


extern unsigned int    n_com1, line1;
extern unsigned char   data_com1[com_max];

