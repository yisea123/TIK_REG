#include "main.h"
#include "lwip/api.h"

void CRC16_M (char* data, unsigned int line); 
void init_M(void);     //Init Modbus slave
int RTU_M(void); 
void zapros_M(unsigned char dadd, unsigned char rez_com, unsigned short ladd, unsigned short hadd);
int Modbus_TCP_Slave_serve(struct netconn *conn);
void RTU_G(void); 
void zapros_G(unsigned char arg);
void ReadSMS(void);
void DeleteAllSMS(void);
void SendSMS(char* phonenum, char* text, char cmd);
void SendSMS1(char cmd, char rele, char state);
void SendSMS3(char* phonenum, char cmd);
void SendSMS4(char cmd, char rele, char state);
//void SendSMS5(char cmd, char* text);
void SendSMS5(char* text);
void Modbus_TCP_Task(struct netconn *conn);
void zapros_M10(unsigned char buf[200]);
void zapros_T(struct netconn *conn, unsigned char dadd, unsigned char rez_com, unsigned short ladd, unsigned short hadd);
void SendGSM(unsigned char *cmd, unsigned int len);
void SendGSMTrans(unsigned char *cmd, unsigned int len);
int GSM_S(void);
void ModbusGSM(void const * argument);
void InitGSM(void);
unsigned short int GSMQ(void);

void SendGSMUSB(unsigned char *cmd);
void SendGSMTransUSB(unsigned char *cmd);
void SendGSMTransUSB2(unsigned char *cmd, unsigned int len);

	
//Connecting pins
#define RTS_SET                 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET)
#define RTS_RESET               HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET)

#define RTS_SET3                 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET)
#define RTS_RESET3               HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET)

//#define RTS_SET3                 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET)
//#define RTS_RESET3               HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET)

extern unsigned int    n_master_com, line_master, n_com3, n_com33, line3;
extern unsigned char   data_master_IN[com_max], data_master_OUT[20],data_com3_IN[1200],data_com3_OUT[2000];

