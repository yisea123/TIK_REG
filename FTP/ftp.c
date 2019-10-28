#include "FTP.h"
#include "lwip\tcpip.h"
#include "lwip\mem.h"
#include "lwip\pbuf.h"
#include "lwip\api.h"
#include "lwip\ip.h"
#include "lwip\tcp.h"
#include "ff.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "httpserver-netconn.h"
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
#include <stdlib.h>
#include <stdarg.h>
#include "stm32_ub_fatfs.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"

const char* FTP_REPLY_150 = "150 File status okay.\r\n";
const char* FTP_REPLY_200 = "200 Command okay.\r\n";
const char* FTP_REPLY_202 = "202 Command not implemented, superfluous at this server.\r\n";
const char* FTP_REPLY_215 = "215 UNIX type.\r\n";
const char* FTP_REPLY_220 = "220 Service ready for new user.\r\n";
const char* FTP_REPLY_221 = "221 Service closing control connection.\r\n";
const char* FTP_REPLY_226 = "226 Closing data connection. Requested file action successful.\r\n";
const char* FTP_REPLY_230 = "230 User logged in, proceed. Logged out if appropriate.\r\n";
const char* FTP_REPLY_331 = "331 User name okay, need password.\r\n";
const char* FTP_REPLY_332 = "332 Need account for login.\r\n";
const char* FTP_REPLY_421 = "421 Service not available, closing control connection.\r\n";
const char* FTP_REPLY_425 = "425 Can't open data connection.\r\n";
const char* FTP_REPLY_450 = "450 Requested file action not taken.File busy\r\n";
const char* FTP_REPLY_451 = "451 Requested action aborted. Local error in processing.\r\n";
const char* FTP_REPLY_500 = "500 Syntax error, command unrecognized.\r\n";
const char* FTP_REPLY_501 = "501 Syntax error in parameters or arguments.\r\n";
const char* FTP_REPLY_502 = "502 Command not implemented.\r\n";
const char* FTP_REPLY_503 = "503 Bad sequence of commands.\r\n";
const char* FTP_REPLY_504 = "504 Command not implemented for that parameter.\r\n";
const char* FTP_REPLY_530 = "530 Not logged in.\r\n";
const char* FTP_REPLY_550 = "550 Requested action not taken. File or path not found.\r\n";
const char* FTP_REPLY_551 = "551 Requested action aborted.\r\n";
const char* FTP_REPLY_552 = "552 Requested file action aborted.\r\n";

const char* FTP_REPLY_227_temp = "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).\r\n";

enum
{
  FTP_AUSTATE_USER,      
  FTP_AUSTATE_USER_BAD,  
  FTP_AUSTATE_PASS,      
  FTP_AUSTATE_LOGIN      
}AUTH_STATES;

enum
{
  FTP_CMD_USER,
  FTP_CMD_PASS,
  FTP_CMD_TYPE,
  FTP_CMD_PWD,
  FTP_CMD_SYST,
  FTP_CMD_PORT,
  FTP_CMD_LIST,
  FTP_CMD_CWD,
  FTP_CMD_CDUP,
  FTP_CMD_RETR,
  FTP_CMD_STOR,
  FTP_CMD_DELE,
  FTP_CMD_QUIT,
  FTP_CMD_NOOP,
  FTP_CMD_PASV,
  FTP_CMD_MKD,
  FTP_CMD_RMD,
	FTP_CMD_SIZE,
	FTP_CMD_APPE
}FTP_CMD_ID;

typedef struct ftp_cmd_list_tag
{
  int8_t  name_id;
  char*   name;
  uint8_t access;   // need login
  uint8_t enabled;  // ready
} ftp_cmd_list_t;

const ftp_cmd_list_t ftp_cmd_list[] =
{
  {FTP_CMD_USER,"USER",  0, 1},
  {FTP_CMD_PASS,"PASS",  0, 1},
  {FTP_CMD_TYPE,"TYPE",  1, 1},
  {FTP_CMD_PWD, "PWD",   1, 1},
  {FTP_CMD_SYST,"SYST",  0, 1},
  {FTP_CMD_PORT,"PORT",  1, 1},
  {FTP_CMD_LIST,"LIST",  1, 1},
  {FTP_CMD_CWD, "CWD",   1, 1},
  {FTP_CMD_CDUP,"CDUP",  1, 1},
  {FTP_CMD_RETR,"RETR",  1, 1},
  {FTP_CMD_STOR,"STOR",  1, 1},
  {FTP_CMD_DELE,"DELE",  1, 1},
  {FTP_CMD_QUIT,"QUIT",  0, 1},
  {FTP_CMD_NOOP,"NOOP",  0, 1},
  {FTP_CMD_PASV,"PASV",  1, 1},
  {FTP_CMD_MKD ,"MKD",   1, 1},
  {FTP_CMD_RMD ,"RMD",   1, 1},
	{FTP_CMD_SIZE,"SIZE",  1, 1},
	{FTP_CMD_APPE,"APPE",  1, 1},
};
#define FTP_CMD_COUNT (sizeof(ftp_cmd_list)/sizeof(ftp_cmd_list_t))

typedef struct ftp_pcb_tag
{
  uint32_t au_state;
  ip_addr_t ConnectIP;
  uint16_t  ConnectPort;
  char buff[FTP_COMMON_BUFF_SIZE];
}ftp_pcb_t;

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

xSemaphoreHandle   FTPClientCounter;
xSemaphoreHandle FTPBuffProtect;
extern xSemaphoreHandle SemFiles;

char      FTPReplyBuff[160];
uint8_t IP_ADDR[4];
ip4_addr_t ipad;

FRESULT WHY_ERR;
FIL *file;
FATFS SDFatFs;  /* File system object for SD card logical drive */
char who; /* SD card logical drive path */
extern char SDPath[4];
unsigned short ss;

	extern struct devices *StructQwAdr[20];
	extern char NumQw;

FILINFO fstat;

extern uint8_t IP_ADDRESS[4];
uint16_t lastftpport = 30010;
char FTPmode = 0; // 0 - active; 1 - passive
/******************************************************************************
* FUNCTION:     FTP_SendList
*
* DESCRIPTION:  
* PARAMETERS:   
* RETURNS:      
*
******************************************************************************/
void FTP_SendList(struct netconn *conn, ftp_pcb_t *pcb)
{
  FRESULT file_err;
  err_t err;
  DIR fdir;
  FILINFO fno;
  uint32_t size;
	char *fn;   /* This function is assuming non-Unicode cfg. */
	char smon[4]={0};
	u32_t d,mon,y,h,min,s;
	osSemaphoreWait(SemFiles, 10000);
	#if _USE_LFN // Если настроено LFN, то выделяем буфер для LFN
    static TCHAR lfn[_MAX_LFN + 1]={0};   /* Buffer to store the LFN */
  strcpy(fno.fname,lfn); 
	//fno.fname = lfn;
    fno.fsize = sizeof lfn;
#endif
  HAL_NVIC_DisableIRQ(USART2_IRQn);
		
  file_err = f_opendir(&fdir, pcb->buff);
  if ( file_err == FR_OK )
  {
    for ( ;; )
    {
				taskENTER_CRITICAL();
      file_err = f_readdir(&fdir, &fno);     
				taskEXIT_CRITICAL();			
      if (file_err != FR_OK || fno.fname[0] == 0) 
			{
				break;
			}
			
      if (fno.fname[0] == '.')  
			{
				continue;
			}
			
if(strlen(fno.fname)==0) // Проверка длинны лонг имени файла, если меньше 8.3, то используем SFN
{
	
	y = ((fno.fdate & 0xFE00)>>9)-20;
	mon = (fno.fdate & 0x01E0)>>5;
	d = (fno.fdate & 0x001F);
	
	h = (fno.ftime & 0xF800)>>11;
	min = (fno.ftime & 0x07E0)>>5;
	
	switch (mon)
	{
		case 1:
			strcpy(smon,"Jan");	
		break;
		
		case 2:
			strcpy(smon,"Feb");	
		break;
		
		case 3:
			strcpy(smon,"Mar");	
		break;
		
		case 4:
			strcpy(smon,"Apr");	
		break;
		
		case 5:
			strcpy(smon,"May");	
		break;
		
		case 6:
			strcpy(smon,"Jun");	
		break;
		
		case 7:
			strcpy(smon,"Jul");	
		break;
		
		case 8:
			strcpy(smon,"Aug");	
		break;
		
		case 9:
			strcpy(smon,"Sep");	
		break;
		
		case 10:
			strcpy(smon,"Oct");	
		break;
		
		case 11:
			strcpy(smon,"Nov");	
		break;
		
		case 12:
			strcpy(smon,"Dec");	
		break;
		
		default:
			strcpy(smon,"Jan");	
		break;
	}
	
	
			size = snprintf(pcb->buff,sizeof(pcb->buff),"%crw------- 1 user group %u %02d %02d:%02d %s\r\n",
                      (fno.fattrib & AM_DIR)?'d':'-',
												fno.fsize,
												smon,
												d,
												h,
												min,
												fno.fname                    
                      );
}
else // Иначе используем LFN
{
	y = ((fno.fdate & 0xFE00)>>9)+1980-2000;
	mon = (fno.fdate & 0x01E0)>>5;
	d = (fno.fdate & 0x001F);
	
	h = (fno.ftime & 0xF800)>>11;
	min = (fno.ftime & 0x07E0)>>5;
	
	switch (mon)
	{
		case 1:
			strcpy(smon,"Jan");	
		break;
		
		case 2:
			strcpy(smon,"Feb");	
		break;
		
		case 3:
			strcpy(smon,"Mar");	
		break;
		
		case 4:
			strcpy(smon,"Apr");	
		break;
		
		case 5:
			strcpy(smon,"May");	
		break;
		
		case 6:
			strcpy(smon,"Jun");	
		break;
		
		case 7:
			strcpy(smon,"Jul");	
		break;
		
		case 8:
			strcpy(smon,"Aug");	
		break;
		
		case 9:
			strcpy(smon,"Sep");	
		break;
		
		case 10:
			strcpy(smon,"Oct");	
		break;
		
		case 11:
			strcpy(smon,"Nov");	
		break;
		
		case 12:
			strcpy(smon,"Dec");	
		break;
		
		default:
			strcpy(smon,"Jan");	
		break;
	}

	
      size = snprintf(pcb->buff,sizeof(pcb->buff),"%crw------- 1 user group %u %s %02d %02d:%02d %s\r\n",
                      (fno.fattrib & AM_DIR)?'d':'-',
												fno.fsize,
												smon,
												d,
												h,
												min,
												fno.fname               
                      );
}
      err = netconn_write(conn, pcb->buff, size, NETCONN_COPY);
      if (err != ERR_OK) 
			{
				break;
			}
    }
  }
	HAL_NVIC_EnableIRQ(USART2_IRQn);
	osSemaphoreRelease(SemFiles);
}
/******************************************************************************
* FUNCTION:     FTP_SendReply
*
* DESCRIPTION:
* PARAMETERS:
* RETURNS:
*
******************************************************************************/
inline static void FTP_SendReply(struct netconn *conn, const char *args, ...)
{
  uint32_t len;
  va_list ap;
  //xSemaphoreTake(FTPBuffProtect,portMAX_DELAY);
  va_start(ap, args);
  len = vsnprintf(FTPReplyBuff,sizeof(FTPReplyBuff),args,ap);
  va_end(ap);
  netconn_write(conn, FTPReplyBuff, len, NETCONN_COPY);
  //xSemaphoreGive(FTPBuffProtect);
}
/******************************************************************************
* FUNCTION:  FTP_Execute
*
* DESCRIPTION:
* PARAMETERS:
* RETURNS:
*
******************************************************************************/
struct netconn *data_conn_pasv;
void FTP_Execute(struct netconn *conn, const char* cmd, char* arg, ftp_pcb_t* pcb)
{
  static const char port_delimiters[] = ",";
  uint32_t i,cmd_id;
  char* token_buff;
  struct netconn *data_conn, *newconn;
  FRESULT file_err;
  
  uint32_t fsize;
  err_t err;
  struct pbuf *p,*q;


	cmd_id = -1;
  for(i=0;i<FTP_CMD_COUNT;i++)
  {
    if(strcasecmp(ftp_cmd_list[i].name,cmd)==0)
    {
      if ( !ftp_cmd_list[i].enabled )
      {
        FTP_SendReply(conn, FTP_REPLY_502);
        return;
      }
      cmd_id = ftp_cmd_list[i].name_id;
      if (( pcb->au_state != FTP_AUSTATE_LOGIN) && (ftp_cmd_list[i].access != 0))
      {
        FTP_SendReply(conn, FTP_REPLY_530);
        return;
      }
      break;
    };
  };
	

	
  switch ( cmd_id )
  {
    case FTP_CMD_USER:
      if(arg == NULL) 
      {
        FTP_SendReply(conn,FTP_REPLY_332);
        break;
      }
      pcb->au_state = (strcmp("admin",arg)==0)?FTP_AUSTATE_PASS:FTP_AUSTATE_USER_BAD;
      FTP_SendReply(conn,FTP_REPLY_331); 
      break;
      
    case FTP_CMD_PASS:
      if(arg == NULL) 
      {
        FTP_SendReply(conn,FTP_REPLY_530);
        break;
      };
      if (( pcb->au_state == FTP_AUSTATE_PASS) || (pcb->au_state == FTP_AUSTATE_USER_BAD))
      {
        if (( pcb->au_state == FTP_AUSTATE_PASS) && (strcmp("admin",arg) == 0))
        {
          pcb->au_state = FTP_AUSTATE_LOGIN;
          FTP_SendReply(conn, FTP_REPLY_230);
        }
        else
        {
          pcb->au_state = FTP_AUSTATE_USER;
          FTP_SendReply(conn, FTP_REPLY_530);
        }
      }
      else FTP_SendReply(conn, FTP_REPLY_503);
      break;
      
    case FTP_CMD_TYPE:
      if(arg == NULL) FTP_SendReply(conn, FTP_REPLY_501); 
      if((strcasecmp(arg,"A")==0) || (strcasecmp(arg,"I")==0)) FTP_SendReply(conn,FTP_REPLY_200);
      else FTP_SendReply(conn, FTP_REPLY_504);
      break;
      
    case FTP_CMD_SYST:
      FTP_SendReply(conn,FTP_REPLY_215); 
      break;
    case FTP_CMD_QUIT: 
      FTP_SendReply(conn, FTP_REPLY_221);

      //netconn_close(conn);
      break;
    case FTP_CMD_NOOP: 
      FTP_SendReply(conn, FTP_REPLY_200);
      break;
    case FTP_CMD_PASV:
      //FTP_SendReply(conn,"502 Passive mode of connection is not supported.\r\n");
			char PASVbuf[100]={0};
			//1024-65535
			lastftpport++;
			uint8_t ftpport1 = 0,  ftpport2 = 0; 
			ftpport1 = lastftpport >> 8;
			ftpport2 = lastftpport & 0x00FF;
			
			sprintf(PASVbuf, FTP_REPLY_227_temp, IP_ADDRESS[0], IP_ADDRESS[1], IP_ADDRESS[2], IP_ADDRESS[3], ftpport1, ftpport2);
			
		
			  data_conn_pasv = netconn_new(NETCONN_TCP);
				if (conn!= NULL)
				{
					err = netconn_bind(data_conn_pasv, NULL, lastftpport);
					if (err == ERR_OK)
					{
						
						netconn_listen(data_conn_pasv);
						FTPmode = 1;
						FTP_SendReply(conn,PASVbuf);

						
						
					}
					else
					{
						FTP_SendReply(conn,"502 Passive mode of connection is not supported.\r\n");
					}
				}
				else
				{
					FTP_SendReply(conn,"502 Passive mode of connection is not supported.\r\n");
				}

				
      break;
    case FTP_CMD_PORT:
      if(arg == NULL) FTP_SendReply(conn,FTP_REPLY_501);

			IP_ADDR[0] =  atoi((char*)strtok_r(arg,port_delimiters,&token_buff))  & 0xFF;   
			IP_ADDR[1] = (atoi((char*)strtok_r(NULL,port_delimiters,&token_buff)) & 0xFF);   
			IP_ADDR[2] = (atoi((char*)strtok_r(NULL,port_delimiters,&token_buff)) & 0xFF);   
			IP_ADDR[3] = (atoi((char*)strtok_r(NULL,port_delimiters,&token_buff)) & 0xFF);   
					
			IP4_ADDR(&ipad,IP_ADDR[0], IP_ADDR[1], IP_ADDR[2], IP_ADDR[3]);

			ss = (atoi((char*)strtok_r(NULL,port_delimiters,&token_buff))  & 0xFF) << 8;
			ss |=  atoi((char*)strtok_r(NULL, port_delimiters,&token_buff)) & 0xFF;
			FTP_SendReply(conn, FTP_REPLY_200);
		  
			break;
      
    case FTP_CMD_LIST:
			if(!FTPmode)
			{
				if((ipad.addr == 0) || (ss == 0)) FTP_SendReply(conn,FTP_REPLY_503);
				data_conn = netconn_new(NETCONN_TCP);
				netconn_set_recvtimeout(data_conn,1000*60*2); //2 min timeout
			 // timeout == keep_idle + (keep_intvl * keep_cnt)
				data_conn->pcb.tcp->keep_idle  =  5000;
				data_conn->pcb.tcp->keep_intvl =  1000;
				data_conn->pcb.tcp->keep_cnt   =  5;
				data_conn->pcb.tcp->so_options |= SOF_KEEPALIVE;
			

				err = netconn_connect(data_conn, &ipad,ss);
				if ( err == ERR_OK )
				{

					file_err = f_getcwd(pcb->buff,sizeof(pcb->buff));
					if ( file_err != FR_OK )
					{
						FTP_SendReply(conn,FTP_REPLY_550);
					}
					else
					{
						FTP_SendReply(conn,FTP_REPLY_150);
						FTP_SendList(data_conn, pcb); 
						FTP_SendReply(conn,FTP_REPLY_226);
					}
				}
				else
				{
					FTP_SendReply(conn,FTP_REPLY_425);
				}
				netconn_close(data_conn);
				netconn_delete(data_conn);
			}
			else
			{
						while(1) 
						{
						err_t accept_err;
						accept_err = netconn_accept(data_conn_pasv, &newconn);
							if(accept_err == ERR_OK)
							{
								
								netconn_set_recvtimeout(newconn,1000*2); // Кароч, если за 10 сек нет данных, то отъезжаем
								newconn->pcb.tcp->keep_idle  =  5000;
								newconn->pcb.tcp->keep_intvl =  1000;
								newconn->pcb.tcp->keep_cnt   =  5;
								newconn->pcb.tcp->so_options |= SOF_KEEPALIVE;

								
								break;
							}
						}
						
				file_err = f_getcwd(pcb->buff,sizeof(pcb->buff));
				if ( file_err != FR_OK )
					{
						FTP_SendReply(conn,FTP_REPLY_550);
					}
					else
					{
				FTP_SendReply(conn,FTP_REPLY_150);
						FTP_SendList(newconn, pcb); 
						FTP_SendReply(conn,FTP_REPLY_226);
					}
							//netconn_close(data_conn);
				//netconn_delete(data_conn);
					netconn_close(newconn);
				netconn_delete(newconn);
			}
			
      break;
  
    case FTP_CMD_PWD:

		  file_err =  f_getcwd(pcb->buff,sizeof(pcb->buff));
      if ( file_err == FR_OK )
      {
				
				FTP_SendReply(conn, "257 \x22%s\x22 is current directory.\r\n",pcb->buff);
      }
      else
      {
        FTP_SendReply(conn, FTP_REPLY_551);
      }
      break;
      
    case FTP_CMD_CWD:

      if(arg == NULL) FTP_SendReply(conn, FTP_REPLY_501);
      if ( arg[0] == '/' )
      {
        f_chdir("/");
        i = strlen(arg);
        if ( i > 1)
        {
          arg[i-1] = '\0';
        }
      };
      file_err = f_chdir(arg);
      if ( file_err == FR_OK )
      {
        f_getcwd(pcb->buff,sizeof(pcb->buff));
        FTP_SendReply(conn, "200 CDUP command successful. \x22%s\x22 is current directory.\r\n",pcb->buff); 
      }
      else
      {
        FTP_SendReply(conn, FTP_REPLY_551);
      }
      break;
      
    case FTP_CMD_CDUP:

		
      file_err = f_chdir("..");
      if ( file_err == FR_OK )
      {
        f_getcwd(pcb->buff,sizeof(pcb->buff));
        FTP_SendReply(conn, "200 CDUP command successful. \x22%s\x22 is current directory.\r\n",pcb->buff); 
      }
      else
      {
        FTP_SendReply(conn, FTP_REPLY_551);
      }
      break;
    case FTP_CMD_MKD:
      if(arg == NULL) FTP_SendReply(conn, FTP_REPLY_501);
      file_err = f_mkdir(arg);
      if ( file_err == FR_OK)
      {
        FTP_SendReply(conn, "250 MKD command successful.\r\n");
      }
      else
      {
        FTP_SendReply(conn,FTP_REPLY_550);
      }
      break;
    case FTP_CMD_DELE:
    case FTP_CMD_RMD:
      if(arg == NULL) FTP_SendReply(conn, FTP_REPLY_501);
			
			uint8_t excurrent = 0;
					for(int i = 0;i < NumQw;i++)
					{
						if((strcmp(StructQwAdr[i]->filename,arg) == 0)) // Если файл, который еще пишется, то далее не будем удалять
						{
							excurrent = 1;
							break;
						}
					}
					file_err = FR_OK;
			if(excurrent == 0)
			{
				taskENTER_CRITICAL();
      file_err = f_unlink(arg);
				taskEXIT_CRITICAL(); 
				
				if ( file_err == FR_OK)
      {
        FTP_SendReply(conn, "250 RMD command successful.\r\n");
      }
      else
      {
        FTP_SendReply(conn,FTP_REPLY_550);
      }
			}
			else
			{
				FTP_SendReply(conn,FTP_REPLY_450);
			}
				
      break;
    case FTP_CMD_RETR:
			osSemaphoreWait(SemFiles, portMAX_DELAY);
		if(!FTPmode)
		{
			if((ipad.addr == 0) || (ss == 0)) FTP_SendReply(conn,FTP_REPLY_503);
      if(arg == NULL) FTP_SendReply(conn, FTP_REPLY_501);
      file = pvPortMalloc(sizeof(FIL));
		 
		if (file){
					taskENTER_CRITICAL();
        file_err = f_open(file, arg, FA_OPEN_EXISTING | FA_READ); 
					taskEXIT_CRITICAL();
        if ( file_err == FR_OK)
        {
          data_conn = netconn_new(NETCONN_TCP);
          netconn_set_recvtimeout(data_conn,1000*60*2); //2 min timeout
          //timeout == keep_idle + (keep_intvl * keep_cnt)
          data_conn->pcb.tcp->keep_idle  =  5000;
          data_conn->pcb.tcp->keep_intvl =  1000;
          data_conn->pcb.tcp->keep_cnt   =  5;
          data_conn->pcb.tcp->so_options |= SOF_KEEPALIVE;
          err = netconn_connect(data_conn, &ipad,ss);
          
					if ( err == ERR_OK )
          {
            FTP_SendReply(conn,FTP_REPLY_150); 

            for(;;)
            {
								taskENTER_CRITICAL();
							HAL_NVIC_DisableIRQ(USART2_IRQn);
              file_err = f_read(file, pcb->buff, sizeof(pcb->buff),&fsize);
							HAL_NVIC_EnableIRQ(USART2_IRQn);
								taskEXIT_CRITICAL();
							if ((file_err || fsize) == 0) 
							{
								break;
							}
              err = netconn_write(data_conn, pcb->buff,  fsize, NETCONN_COPY);
              if (err != ERR_OK) 
							{
								break;
							}
            }
            
            if ((file_err == FR_OK) && (err == ERR_OK))
            {
              FTP_SendReply(conn,FTP_REPLY_226);
            }
            else
            {
              FTP_SendReply(conn,FTP_REPLY_451);
            }
          }
          else
          {
            FTP_SendReply(conn,FTP_REPLY_425);
          }
          f_close(file);
          vPortFree(file);
          netconn_close(data_conn);
          netconn_delete(data_conn); 
					osSemaphoreRelease(SemFiles);
        }
        else
        {
          vPortFree(file);
          FTP_SendReply(conn, FTP_REPLY_550);
					osSemaphoreRelease(SemFiles);
        }
      }
      else
      {
        FTP_SendReply(conn,FTP_REPLY_552);
				osSemaphoreRelease(SemFiles);
      }
		}
		else
		{
			while(1) 
						{
						err_t accept_err;
						accept_err = netconn_accept(data_conn_pasv, &newconn);
							if(accept_err == ERR_OK)
							{
								
								netconn_set_recvtimeout(newconn,1000*2); // Кароч, если за 10 сек нет данных, то отъезжаем
								newconn->pcb.tcp->keep_idle  =  5000;
								newconn->pcb.tcp->keep_intvl =  1000;
								newconn->pcb.tcp->keep_cnt   =  5;
								newconn->pcb.tcp->so_options |= SOF_KEEPALIVE;

								FTPmode = 1;
								break;
							}
						}
						
						
			if(arg == NULL) FTP_SendReply(conn, FTP_REPLY_501);
      file = pvPortMalloc(sizeof(FIL));
		 
		if (file){
					taskENTER_CRITICAL();
        file_err = f_open(file, arg, FA_OPEN_EXISTING | FA_READ); 
					taskEXIT_CRITICAL();
        if ( file_err == FR_OK)
        {
        
            FTP_SendReply(conn,FTP_REPLY_150); 

            for(;;)
            {
								taskENTER_CRITICAL();
							HAL_NVIC_DisableIRQ(USART2_IRQn);
              file_err = f_read(file, pcb->buff, sizeof(pcb->buff),&fsize);
							HAL_NVIC_EnableIRQ(USART2_IRQn);
								taskEXIT_CRITICAL();
							if ((file_err || fsize) == 0) 
							{
								break;
							}
              err = netconn_write(newconn, pcb->buff,  fsize, NETCONN_COPY);
              if (err != ERR_OK) 
							{
								break;
							}
            }
            
            if ((file_err == FR_OK) && (err == ERR_OK))
            {
              FTP_SendReply(conn,FTP_REPLY_226);
            }
            else
            {
              FTP_SendReply(conn,FTP_REPLY_451);
            }

          f_close(file);
          vPortFree(file);
          netconn_close(newconn);
          netconn_delete(newconn); 
					osSemaphoreRelease(SemFiles);
        }
        else
        {
          vPortFree(file);
          FTP_SendReply(conn, FTP_REPLY_550);
					osSemaphoreRelease(SemFiles);
        }
      }
      else
      {
        FTP_SendReply(conn,FTP_REPLY_552);
				osSemaphoreRelease(SemFiles);
      }
						
						
						
						
						
						
		}
		
			
      break;
		
		case FTP_CMD_APPE:
    case FTP_CMD_STOR:
		osSemaphoreWait(SemFiles, portMAX_DELAY);
		
		if(!FTPmode)
		{
      if((ipad.addr == 0) || (ss == 0)) FTP_SendReply(conn,FTP_REPLY_503);
      if(arg == NULL) FTP_SendReply(conn, FTP_REPLY_501);
      file = pvPortMalloc(sizeof(FIL));
      
		if ( file )
      {
					taskENTER_CRITICAL();
        file_err = f_open(file, arg, FA_CREATE_NEW | FA_WRITE);
					taskEXIT_CRITICAL();
        if ( file_err == FR_OK )
        {
          data_conn = netconn_new(NETCONN_TCP);
          netconn_set_recvtimeout(data_conn,1000*60*2); //2 min timeout
          //timeout == keep_idle + (keep_intvl * keep_cnt)
          data_conn->pcb.tcp->keep_idle  =  5000;
          data_conn->pcb.tcp->keep_intvl =  1000;
          data_conn->pcb.tcp->keep_cnt   =  5;
          data_conn->pcb.tcp->so_options |= SOF_KEEPALIVE;
          err = netconn_connect(data_conn, &ipad,ss);
          			
					if ( err == ERR_OK )
          {
            FTP_SendReply(conn,FTP_REPLY_150);
            while((err = netconn_recv_tcp_pbuf(data_conn,&p))==ERR_OK)
            {
              for( q=p;q!=NULL;q = q->next )
              {
									taskENTER_CRITICAL();
                file_err = f_write(file, q->payload, q->len, &fsize);
									taskEXIT_CRITICAL();
								if( file_err || q->len < fsize ) break;
              }
              netconn_recved(data_conn, p->tot_len);
              pbuf_free(p);
              if( file_err != FR_OK) break;
            }
            
            if ( file_err != FR_OK )
            {
              FTP_SendReply(conn,FTP_REPLY_451);
						}
            else
            {
              FTP_SendReply(conn,FTP_REPLY_226);
            }
            f_sync(file);
          }
          else
          {
            FTP_SendReply(conn,FTP_REPLY_425);
          }
          f_close(file);
          vPortFree(file);
          netconn_delete(data_conn); 
					osSemaphoreRelease(SemFiles);
        }
        else
        {
          vPortFree(file);
          FTP_SendReply(conn, FTP_REPLY_550);
					osSemaphoreRelease(SemFiles);
        }
      }
      else
      {
        FTP_SendReply(conn,FTP_REPLY_552);
				osSemaphoreRelease(SemFiles);
      }
		}
		else
		{
			
			while(1) 
						{
						err_t accept_err;
						accept_err = netconn_accept(data_conn_pasv, &newconn);
							if(accept_err == ERR_OK)
							{
								
								netconn_set_recvtimeout(newconn,1000*2); // Кароч, если за 10 сек нет данных, то отъезжаем
								newconn->pcb.tcp->keep_idle  =  5000;
								newconn->pcb.tcp->keep_intvl =  1000;
								newconn->pcb.tcp->keep_cnt   =  5;
								newconn->pcb.tcp->so_options |= SOF_KEEPALIVE;

								FTPmode = 1;
								break;
							}
						}
			
			
			if(arg == NULL) FTP_SendReply(conn, FTP_REPLY_501);
      file = pvPortMalloc(sizeof(FIL));
      
		if ( file )
      {
					taskENTER_CRITICAL();
        file_err = f_open(file, arg, FA_CREATE_NEW | FA_WRITE);
					taskEXIT_CRITICAL();
        if ( file_err == FR_OK )
        {

            FTP_SendReply(conn,FTP_REPLY_150);
            while((err = netconn_recv_tcp_pbuf(newconn,&p))==ERR_OK)
            {
              for( q=p;q!=NULL;q = q->next )
              {
									taskENTER_CRITICAL();
                file_err = f_write(file, q->payload, q->len, &fsize);
									taskEXIT_CRITICAL();
								if( file_err || q->len < fsize ) break;
              }
              netconn_recved(newconn, p->tot_len);
              pbuf_free(p);
              if( file_err != FR_OK) break;
            }
            
            if ( file_err != FR_OK )
            {
              FTP_SendReply(conn,FTP_REPLY_451);
						}
            else
            {
              FTP_SendReply(conn,FTP_REPLY_226);
            }
            f_sync(file);
          f_close(file);
          vPortFree(file);
          netconn_delete(newconn); 
					osSemaphoreRelease(SemFiles);
        }
        else
        {
          vPortFree(file);
          FTP_SendReply(conn, FTP_REPLY_550);
					osSemaphoreRelease(SemFiles);
        }
      }
      else
      {
        FTP_SendReply(conn,FTP_REPLY_552);
				osSemaphoreRelease(SemFiles);
      }
		}
      break;
			
			 case FTP_CMD_SIZE:
				 f_stat(arg, &fstat);
				 FTP_SendReply(conn, "213 %d\r\n",fstat.fsize);
			 break;
    default: FTP_SendReply(conn, FTP_REPLY_502);
  };
}
/******************************************************************************
* FUNCTION:     FTP_Command_thread
*
* DESCRIPTION:
* PARAMETERS:
* RETURNS:
*
******************************************************************************/
void FTP_Command_thread(struct netconn *conn)
{
  err_t err;
  static const char cmd_delimiters[]  = " \r\n";
  char  *token_buff, *cmd, *arg, *ptr;
  struct pbuf *p, *q;
  uint32_t  len,indx;
  ftp_pcb_t fpcb;
  memset(&fpcb,0,sizeof(fpcb));
	FTPmode = 0;
  netconn_set_recvtimeout(conn,1000*60*2); //2 min timeout
  //timeout == keep_idle + (keep_intvl * keep_cnt)
  conn->pcb.tcp->keep_idle  =  5000;
  conn->pcb.tcp->keep_intvl =  1000;
  conn->pcb.tcp->keep_cnt   =  5;
  conn->pcb.tcp->so_options |= SOF_KEEPALIVE;
  indx = 0;
  fpcb.buff[0] = '\0';
  fpcb.au_state = FTP_AUSTATE_USER;
  f_chdir("/");
  FTP_SendReply(conn, FTP_REPLY_220);
lastftpport++;
  for (;;)
  {
    err = netconn_recv_tcp_pbuf(conn,&p);
    if ( err == ERR_OK )
    {
      for ( q=p; q!=NULL; q = q->next )
      {
        len = q->len;
        ptr = q->payload;
        while ( len-- )
        {
          if ( *ptr == '\r' )
          {
            ptr++;
            fpcb.buff[indx] = '\0';
            indx = 0;
            cmd = (char*)strtok_r(fpcb.buff,cmd_delimiters,&token_buff);
            arg = (char*)strtok_r(NULL,cmd_delimiters,&token_buff);
            FTP_Execute(conn, cmd, arg, &fpcb);
          }
          else
          {
            fpcb.buff[indx++] = *ptr++;
            if ( indx >= sizeof(fpcb.buff) )
            {
              indx = 0;
              FTP_SendReply(conn, FTP_REPLY_500);
            }
          }
        }
      };
      netconn_recved(conn,p->tot_len);
      pbuf_free(p);
    }
    else
    {
      netconn_delete(conn);
			netconn_delete(data_conn_pasv);
     // xSemaphoreGive(FTPClientCounter);
      vTaskDelete(NULL);
    }
  }
}
/******************************************************************************
* FUNCTION:     FTP_Accept_thread
*
* DESCRIPTION:
* PARAMETERS:
* RETURNS:
*
******************************************************************************/
void FTP_Accept_thread(void* arg)
{
  struct netconn *conn1, *newconn1;
  err_t err;

  conn1 = netconn_new(NETCONN_TCP);
	if (conn1!= NULL)
  {
		err = netconn_bind(conn1, NULL, 21);
		if (err == ERR_OK)
    {
		netconn_listen(conn1);
			for ( ; ; )
			{
				err = netconn_accept(conn1, &newconn1);
				if ( err == ERR_OK )
				{
					if (xTaskCreate((pdTASK_CODE)FTP_Command_thread,
													 "ftpcmd",
													 1024,
													 newconn1,
													 osPriorityAboveNormal,
													 NULL
													 ) != pdTRUE) // Создаем задачу обработки комманд ftp при подключении
					{
						netconn_close(newconn1);
						netconn_delete(newconn1);
						continue;
					}
				}
				else
				{
					netconn_close(newconn1);
					netconn_delete(newconn1);
				}
			}
    }
	}
}

/******************************************************************************
* FUNCTION:     FTP_Init
*
* DESCRIPTION:
* PARAMETERS:
* RETURNS:
*
******************************************************************************/
int FTP_Init(void)
{
	BaseType_t ftp;

	if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK) // Фантастически гениально монтируем флешку в самом подходящем месте
    {
      Error_Handler();
    }
		
	ftp = xTaskCreate((pdTASK_CODE)FTP_Accept_thread,
                   "ftpacc",
                   256,
                   NULL,
                   osPriorityAboveNormal,
									 NULL); // Создаем задачу ожидания подключения по ftp

  return 1;
}

