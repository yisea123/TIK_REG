/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/httpser-netconn.c 
  * @author  MCD Application Team
  * @version V1.4.0
  * @date    17-February-2017
  * @brief   Basic http server implementation using LwIP netconn API  
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
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
#include "lwip/api.h"
#include "lwip/apps/fs.h"
#include "string.h"
#include "httpserver-netconn.h"
#include "cmsis_os.h"
#include "task.h"
#include <stdio.h>
#include "main.h"
#include "LogSD.h"
#include "stm32_ub_fatfs.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "ff.h"
#include "Event.h"
#include "lwip.h"
#include "lwip/ip4_addr.h"
#include "ModBus_master.h"

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


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define WEBSERVER_THREAD_PRIO    ( osPriorityAboveNormal )

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
u32_t nPageHits = 0;
/* Format of dynamic web page: the page header */ 
char *PAGE_OKJ = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\nConnection: close\r\n\r\n";



char *PAGE_ADDED_JS = "<script type=\"text/javascript\">alert(\"Добавлено\")</script>";
char *PAGE_WRONG_LOGPASS_JS = "<script type=\"text/javascript\">alert(\"Неверный логин или пароль\")</script>";
char *PAGE_OK = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\nConnection: close\r\n\r\n";
char *PAGE_TEXT = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\nConnection: close\r\n\r\n";
char *PAGE_IMAGE = "HTTP/1.1 200 OK\r\nContent-type: text/image\r\nConnection: close\r\n\r\n";

char *PAGE_TASK = "<!DOCTYPE html><html><head><title>STM32F4xxTASKS</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><meta http-equiv=\"refresh\" content=\"1\"><style =\"font-weight: normal; font-family: Verdana;\"></style></head><body><h4 style=\"background: url(logo.png); background-repeat: no-repeat; background-size: contain;\" align=\"center\"><small style=\"font-family: Verdana;\"><small><big><big><big style=\"font-weight: bold;\"><big><strong><em><span style=\"font-style: italic;\">ДИСПЕТЧЕР ЗАДАЧ РЕГИСТРАТОРА</span></em></strong></big></big></big></big></small></small></h4><hr style=\"width: 100%; height: 2px;\"><span style=\"font-weight: bold;\"></span><span style=\"font-weight: bold;\"><table style=\"width: 920px; height: 30px;\" border=\"1\" cellpadding=\"2\" cellspacing=\"2\" align=\"center\"><tbody><tr><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xx.html\"><span style=\"color: white;\">Главная</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxTASKS.html\"><span style=\"color: white;\">Список задач</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxMBUS.html\"><span style=\"color: white;\">MODBUS</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxEVENT.html\"><span style=\"color: white;\">Список событий</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxQWEST.html\"><span style=\"color: white;\">Список опросов</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxELOG.html\"><span style=\"color: white;\">Журнал событий</span></a></small></td></tr></tbody></table><div style=\"width: 920px; margin-right: auto;margin-left: auto;\"><br></span><span style=\"font-weight: bold;\"></span><small><span style=\"font-family: Verdana;\">ВЕРСИЯ ПО:";

char *PAGE_MAIN = "<!DOCTYPE html><html><head><title>MAIN</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><style =\"font-weight: normal; font-family: Verdana;\"></style></head><body><h4 style=\"background: url(logo.png); background-repeat: no-repeat; background-size: contain;\" align=\"center\"><small style=\"font-family: Verdana;\"><small><big><big><big style=\"font-weight: bold;\"><big><strong><em><span style=\"font-style: italic;\">Главная страница регистратора</span></em></strong></big></big></big></big></small></small></h4><hr style=\"width: 100%; height: 2px;\"><span style=\"font-weight: bold;\"></span><span style=\"font-weight: bold;\"><table style=\"width: 920px; height: 30px;\" border=\"1\" cellpadding=\"2\" cellspacing=\"2\" align=\"center\"><tbody><tr><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xx.html\"><span style=\"color: white;\">Главная</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"></a><small><a href=\"/STM32F4xxTASKS.html\"><span style=\"color: white;\">Список задач</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxMBUS.html\"><span style=\"color: white;\">MODBUS</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxEVENT.html\"><span style=\"color: white;\">Список событий</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxQWEST.html\"><span style=\"color: white;\">Список опросов</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxELOG.html\"><span style=\"color: white;\">Журнал событий</span></a></small></td></tr></tbody></table><div style=\"width: 920px; margin-right: auto;margin-left: auto;\"><br></span>";

char *PAGE_MODBUS = "<!DOCTYPE html><html><head><title>MODBUS</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><style =\"font-weight: normal; font-family: Verdana;\"></style></head><body><h4 style=\"background: url(logo.png); background-repeat: no-repeat; background-size: contain;\" align=\"center\"><small style=\"font-family: Verdana;\"><small><big><big><big style=\"font-weight: bold;\"><big><strong><em><span style=\"font-style: italic;\">MODBUS</span></em></strong></big></big></big></big></small></small></h4><hr style=\"width: 100%; height: 2px;\"><span style=\"font-weight: bold;\"></span><span style=\"font-weight: bold;\"><table style=\"width: 920px; height: 30px;\" border=\"1\" cellpadding=\"2\" cellspacing=\"2\" align=\"center\"><tbody><tr><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xx.html\"><span style=\"color: white;\">Главная</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxTASKS.html\"><span style=\"color: white;\">Список задач</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxMBUS.html\"><span style=\"color: white;\">MODBUS</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxEVENT.html\"><span style=\"color: white;\">Список событий</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxQWEST.html\"><span style=\"color: white;\">Список опросов</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxELOG.html\"><span style=\"color: white;\">Журнал событий</span></a></small></td></tr></tbody></table><div style=\"width: 920px; margin-right: auto;margin-left: auto;\">";
//char *PAGE_MODBUS2 = "<br><hr style=\"width: 100%; height: 2px;\"><h2 align=\"center\">Параметры записи лога</h2><form method=\"get\" action=\"/SetLog\"><input name=\"adr\" type=\"text\" required> *Адрес устройства<br><input name=\"reg\" type=\"text\" required> *Адреса Регистров<br><input name=\"period\" type=\"text\" required> *Период опроса, мс<br><input value=\"Сохранить\" type=\"submit\"></form><br>";
char *PAGE_MODBUS3 = "<br><hr style=\"width: 100%; height: 2px;\"><h2 align=\"center\">Настройка режима шлюза</h2><form method=\"get\" action=\"GatewayON\"><input type=\"submit\" value=\"Включить режим шлюза\"></form>"; 

char *GATEWAY_ON = "<br><hr style=\"width: 100%; height: 2px;\"><h2 align=\"center\">Настройка режима шлюза</h2> ШЛЮЗ ВКЛЮЧЕН <form method=\"get\" action=\"GatewayOFF\"><input type=\"submit\" value=\"Выключить режим шлюза\"></form>";

char *PAGE_EVENTLIST = "<!DOCTYPE html><html><head><title>Список событий</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><style =\"font-weight: normal; font-family: Verdana;\"></style></head><body><h4 style=\"background: url(logo.png); background-repeat: no-repeat; background-size: contain;\" align=\"center\"><small style=\"font-family: Verdana;\"><small><big><big><big style=\"font-weight: bold;\"><big><strong><em><span style=\"font-style: italic;\">Список событий</span></em></strong></big></big></big></big></small></small></h4><hr style=\"width: 100%; height: 2px;\"><span style=\"font-weight: bold;\"></span><span style=\"font-weight: bold;\"><table style=\"width: 920px; height: 30px;\" border=\"1\" cellpadding=\"2\" cellspacing=\"2\" align=\"center\"><tbody><tr><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xx.html\"><span style=\"color: white;\">Главная</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxTASKS.html\"><span style=\"color: white;\">Список задач</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxMBUS.html\"><span style=\"color: white;\">MODBUS</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxEVENT.html\"><span style=\"color: white;\">Список событий</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxQWEST.html\"><span style=\"color: white;\">Список опросов</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxELOG.html\"><span style=\"color: white;\">Журнал событий</span></a></small></td></tr></tbody></table><div style=\"width: 920px; margin-right: auto;margin-left: auto;\"><br></span>";
char *PAGE_EVENT = "<br><hr style=\"width: 100%; height: 2px;\"><h2 align=\"center\">Настройка события</h2><form method=\"get\" action=\"Event\"><input type=\"text\" name=\"Name\" maxlength=\"10\" autocomplete=\"off\" required> *Имя события<Br>Интерфейс: <input type=\"radio\" name=\"int\" checked=\"\" value=\"rs\">RS485<input type=\"radio\" name=\"int\" value=\"eth\">ETH<Br><input type=\"text\" name=\"DADR\" pattern=\"^[ 0-9]+$\" required> *Адресс устройства<Br><input type=\"text\" name=\"RADD\" pattern=\"^[ 0-9]+$\" required> *Адрес регистра<input type=\"checkbox\" name=\"float\">Float<input type=\"radio\" name=\"treg\" checked=\"\" value=\"H\">H<input type=\"radio\" name=\"treg\" value=\"I\">I<Br><input type=\"text\" name=\"if\" list=\"vir\" autocomplete=\"off\" required><datalist id=\"vir\"><option>=</option><option>></option><option><</option><option>&</option></datalist> *Выражение<input type=\"checkbox\" name=\"typeif\">*Сравнить с другим регистром<Br><input type=\"text\" name=\"param\" pattern=\"\\d+(\\.\\d{0,6})?\" required> *Параметр или адрес регистра<input type=\"checkbox\" name=\"float2\">Float<input type=\"radio\" name=\"treg2\" checked=\"\" value=\"H\">H<input type=\"radio\" name=\"treg2\" value=\"I\">I<Br><input type=\"checkbox\" name=\"logging\">Логировать событие<Br><input type=\"checkbox\" name=\"rele\">Реле <input type=\"text\" name=\"Rtime\" maxlength=\"10\" autocomplete=\"off\"> Время удержания реле, мс <Br><input type=\"checkbox\" name=\"sms\">СМС <input type=\"text\" name=\"smstext\" size=\"55\" maxlength=\"50\" autocomplete=\"off\"> Текст смс<Br><input type=\"submit\" value=\"Добавить событие\"></form>";

char *PAGE_QW = "<h2 align=\"center\">Настройка опроса</h2><form method=\"get\" action=\"Qwest\">Интерфейс: <input type=\"radio\" name=\"int\" checked=\"\" value=\"rs\">RS485<input type=\"radio\" name=\"int\" value=\"eth\">ETH<Br><input type=\"text\" name=\"DADR\" pattern=\"^[ 0-9]+$\" required> *Адрес устройства<Br><input type=\"radio\" name=\"treg\" checked=\"\" value=\"H\">H<input type=\"radio\" name=\"treg\" value=\"I\">I<Br><input type=\"text\" name=\"RADD\" required> *Регистры (Как в модскане)<Br><input type=\"text\" name=\"period\" pattern=\"^[ 0-9]+$\" required> *Период опроса, мс<Br><input type=\"checkbox\" name=\"log\"> Логировать<br><input type=\"submit\" value=\"Добавить опрос\"></form>";

char *PAGE_QWLIST = "<!DOCTYPE html><html><head><title>Список опросов</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><style =\"font-weight: normal; font-family: Verdana;\"></style></head><body><h4 style=\"background: url(logo.png); background-repeat: no-repeat; background-size: contain;\" align=\"center\"><small style=\"font-family: Verdana;\"><small><big><big><big style=\"font-weight: bold;\"><big><strong><em><span style=\"font-style: italic;\">Список опросов</span></em></strong></big></big></big></big></small></small></h4><hr style=\"width: 100%; height: 2px;\"><span style=\"font-weight: bold;\"></span><span style=\"font-weight: bold;\"><table style=\"width: 920px; height: 30px;\" border=\"1\" cellpadding=\"2\" cellspacing=\"2\" align=\"center\"><tbody><tr><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xx.html\"><span style=\"color: white;\">Главная</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxTASKS.html\"><span style=\"color: white;\">Список задач</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxMBUS.html\"><span style=\"color: white;\">MODBUS</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxEVENT.html\"><span style=\"color: white;\">Список событий</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxQWEST.html\"><span style=\"color: white;\">Список опросов</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxELOG.html\"><span style=\"color: white;\">Журнал событий</span></a></small></td></tr></tbody></table><div style=\"width: 920px; margin-right: auto;margin-left: auto;\"><br></span>";
char *PAGE_EVENTLOG = "<!DOCTYPE html><html><head><title>Журнал событий</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><style =\"font-weight: normal; font-family: Verdana;\"></style></head><body><h4 style=\"background: url(logo.png); background-repeat: no-repeat; background-size: contain;\" align=\"center\"><small style=\"font-family: Verdana;\"><small><big><big><big style=\"font-weight: bold;\"><big><strong><em><span style=\"font-style: italic;\">Журнал событий</span></em></strong></big></big></big></big></small></small></h4><hr style=\"width: 100%; height: 2px;\"><span style=\"font-weight: bold;\"></span><span style=\"font-weight: bold;\"><table style=\"width: 920px; height: 30px;\" border=\"1\" cellpadding=\"2\" cellspacing=\"2\" align=\"center\"><tbody><tr><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xx.html\"><span style=\"color: white;\">Главная</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxTASKS.html\"><span style=\"color: white;\">Список задач</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxMBUS.html\"><span style=\"color: white;\">MODBUS</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxEVENT.html\"><span style=\"color: white;\">Список событий</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxQWEST.html\"><span style=\"color: white;\">Список опросов</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxELOG.html\"><span style=\"color: white;\">Журнал событий</span></a></small></td></tr></tbody></table><div style=\"width: 920px; margin-right: auto;margin-left: auto;\"><br></span>";


char *MAIN_IP = "<h2 align=\"center\">Настройка IPv4</h2><form method=\"get\" action=\"IPSettings\"><input type=\"text\" name=\"ip\" pattern=\"\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\" value=\"%d.%d.%d.%d\" required> IP адресс устройства<Br><input type=\"text\" name=\"mask\" pattern=\"\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\" value=\"%d.%d.%d.%d\" required> Маска подсети<Br><input type=\"text\" name=\"ipgate\" pattern=\"\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\" value=\"%d.%d.%d.%d\" required> Шлюз<Br><input type=\"text\" name=\"macadr\" value=\"%d:%d:%d:%d:%d:%d\" required> MAC адрес<Br><input type=\"submit\" value=\"Изменить\"></form><br>";
char *MAIN_MADD = "<hr style=\"width: 100%; height: 2px;\"><h2 align=\"center\">Настройка параметров Modbus</h2>Настройка адреса Modbus<form method=\"get\" action=\"MaddSet\"><input type=\"text\" name=\"MADD\" value=\"%d\" required> Модбас адрес регистратора<Br><input type=\"submit\" value=\"Изменить\"></form><br>";
char *MAIN_DATE = "<hr style=\"width: 100%; height: 2px;\"><h2 align=\"center\">Настройка даты и времени</h2><form method=\"get\" action=\"DateSet\"><input type=\"text\" name=\"DATE\" value=\"%d.%02d.%02d\" required> Дата<Br><input type=\"submit\" value=\"Изменить\"></form>";
char *MAIN_TIME = "<form method=\"get\" action=\"TimeSet\"><input type=\"text\" name=\"TIME\" value=\"%02d:%02d\" required> Время<Br><input type=\"submit\" value=\"Изменить\"></form><br>";
char *MAIN_FTP = "<hr style=\"width: 100%; height: 2px;\"><h2 align=\"center\">Настройка внешнего FTP</h2><form method=\"get\" action=\"FTPSet\"><input type=\"text\" name=\"ADDR\" autocomplete=\"off\" value=\"%s\" required> Адрес:Порт<Br><input type=\"text\" name=\"LOGIN\" autocomplete=\"off\" required> Логин<Br><input type=\"text\" name=\"PASS\" autocomplete=\"off\" required> Пароль<Br><input type=\"text\" name=\"FOLDER\" autocomplete=\"off\"> Внутренняя директория<Br><input type=\"submit\" value=\"Сохранить\"></form><br>";
char *MAIN_FTP2 = "<form method=\"get\" action=\"FTP2Set\"><input type=\"text\" name=\"Period\" value=\"%d\" autocomplete=\"off\" > Период записи, мин<Br><input type=\"checkbox\" name=\"delete\" %s>Удалять после успешной отправки на внешний FTP<Br><input type=\"submit\" value=\"Сохранить\"></form><br>";
char *MAIN_FTP3 = "<form method=\"get\" action=\"FTP3Set\"><input type=\"checkbox\" name=\"sending\" %s>Отправлять файлы на внешний FTP<Br><input type=\"submit\" value=\"Сохранить\"></form><br>";
char *PhoneScript = "<script type=\"text/javascript\">var xhr = new XMLHttpRequest();xhr.open('GET', 'Phones', false);xhr.send();var s = xhr.responseText;var numbers = s.split(',');console.log(s);console.log(numbers);var div = document.getElementById('container');var form = document.createElement('form');    if ((numbers.length > 0)&& (numbers[0].length == 12)) {        var label = document.createElement('label');        label.innerHTML = 'Список номеров:';        div.appendChild(label);        var ul = document.createElement('ul');        for (var i = 0; i < numbers.length; i++) {            var li = document.createElement('li');            li.innerHTML = numbers[i] + '<form method=\"get\" action=\"deletenum' + i + '\"><input type=\"submit\" value=\"Удалить\"></form>';            ul.appendChild(li);        }        div.appendChild(ul);    } else {        var label = document.createElement('label');        label.innerHTML = 'Нет добавленных номеров';    div.appendChild(label);}</script>";

//char *AUTORIZATION = "<!DOCTYPE html><html><head><title>Авторизация</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><style =\"font-weight: normal; font-family: Verdana;\"></style></head><body><h4 style=\"background: url(logo.png); background-repeat: no-repeat; background-size: contain;\" align=\"center\"><small style=\"font-family: Verdana;\"><small><big><big><big style=\"font-weight: bold;\"><big><strong><em><span style=\"font-style: italic;\">Авторизация</span></em></strong></big></big></big></big></small></small></h4><hr style=\"width: 100%; height: 2px;\"><span style=\"font-weight: bold;\"></span><span style=\"font-weight: bold;\"><div align=\"center\"><form method=\"get\" action=\"AUTORIZ\"><input type=\"text\" name=\"login\" autocomplete=\"off\" required><br><input type=\"password\" name=\"passw\" required><br><input type=\"submit\" value=\"ВХОД\"></form></div><div id=\"container\" style=\"min-width: 310px; height: 400px; margin: 0 auto\"></div><script type=\"text/javascript\">var years = prompt('Сколько вам лет?', '');alert('Вам ' + years + ' лет!')</script></body></html>";
char *AUTORIZATION = "<!DOCTYPE html><html><head><title>Авторизация</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><style =\"font-weight: normal; font-family: Verdana;\"></style></head><body><h4 style=\"background: url(logo.png); background-repeat: no-repeat; background-size: contain;\" align=\"center\"><small style=\"font-family: Verdana;\"><small><big><big><big style=\"font-weight: bold;\"><big><strong><em><span style=\"font-style: italic;\">Авторизация</span></em></strong></big></big></big></big></small></small></h4><hr style=\"width: 100%; height: 2px;\"><span style=\"font-weight: bold;\"></span><span style=\"font-weight: bold;\"><div align=\"center\"><form method=\"get\" action=\"AUTORIZ\"><input type=\"text\" name=\"login\" autocomplete=\"off\" required><br><input type=\"password\" name=\"passw\" required><br><input type=\"submit\" value=\"ВХОД\"></form></div></body></html>";

char *SHOW_GRAPH = "<!DOCTYPE html><html><head><title>График</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><style =\"font-weight: normal; font-family: Verdana;\"></style><script src=\"jquery.min.js\"></script><script src=\"highcharts.js\"></script><script src=\"exporting.js\"></script><script src=\"export.js\"></script></head><body><h4 style=\"background: url(logo.png); background-repeat: no-repeat; background-size: contain;\" align=\"center\"><small style=\"font-family: Verdana;\"><small><big><big><big style=\"font-weight: bold;\"><big><strong><em><span style=\"font-style: italic;\">График</span></em></strong></big></big></big></big></small></small></h4><hr style=\"width: 100%; height: 2px;\"><span style=\"font-weight: bold;\"></span><span style=\"font-weight: bold;\"><table style=\"width: 920px; height: 30px;\" border=\"1\" cellpadding=\"2\" cellspacing=\"2\" align=\"center\"><tbody><tr><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xx.html\"><span style=\"color: white;\">Главная</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxTASKS.html\"><span style=\"color: white;\">Список задач</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxMBUS.html\"><span style=\"color: white;\">MODBUS</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxEVENT.html\"><span style=\"color: white;\">Список событий</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxQWEST.html\"><span style=\"color: white;\">Список опросов</span></a></small></td><td style=\"font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(0, 146, 161); text-align: center;\"><small><a href=\"/STM32F4xxELOG.html\"><span style=\"color: white;\">Журнал событий</span></a></small></td></tr></tbody></table><div style=\"width: 920px; margin-right: auto;margin-left: auto;\"><br></span><div id=\"container\" style=\"min-width: 310px; height: 400px; margin: 0 auto\"></div>";
char *GRAPH_J ="<script type=\"text/javascript\">Highcharts.setOptions({global:{useUTC: false}});Highcharts.chart('container', {   chart: {    type: 'spline',  animation: {duration: %d}, marginRight: 10,  events: { load: function () {  var series = this.series[0]; setInterval(function () {  var xhr = new XMLHttpRequest();xhr.open('GET', 'someparam', false);xhr.send();  var x = (new Date()).getTime(),  y = Number(xhr.responseText); series.addPoint([x, y], true, true);  }, %d);  }  } }, title: {   text: 'Динамический график устройства %d' },  xAxis: { type: 'datetime',   tickPixelInterval: 150  },    yAxis: { title: {  text: 'Значение'  }, plotLines: [{  value: 0, width: 1, color: '#808080'  }] }, tooltip: {    formatter: function () { return '<b>' + this.series.name + '</b><br/>' + Highcharts.dateFormat('%%Y-%%m-%%d %%H:%%M:%%S', this.x) + '<br/>' + Highcharts.numberFormat(this.y, 2);  }  },  legend: {  enabled: false },  exporting: {  enabled: false },series: [{ name: 'Значение',   data: (function () {   var data = [], time = (new Date()).getTime(), i; for (i = -%d; i <= 0; i += 1) { data.push({ x: time + i * %d,y: 0 }); } return data; }())  }]});</script>"; 
unsigned short int  first, second, period;
char tryCreateLog = 0, address, flagEV;
//char a1[1500];

unsigned short int NumG, RegG = 1, NRegG = 0, TypeG = 4, PeriodG = 1000, TimeG = 20, NumPointsG = 20;
unsigned char Gfloat = 0;
char Gbuf[2500] = {0};

	extern char flagLOG;
	extern char KillLOG;
	extern unsigned char smsMSG[17][51];
	extern unsigned char smsMSGutf8[17][100];
	
	extern xSemaphoreHandle SemMem;
	extern xSemaphoreHandle SemSaveDev;
	extern xSemaphoreHandle SemSaveEv;
	extern xSemaphoreHandle SemSMS;
	extern xSemaphoreHandle SemGSM;
	
	extern RTC_HandleTypeDef hrtc;
	
	extern struct events *StructEventAdr[50];
	extern char NumEvents;
	extern struct devices *StructQwAdr[10];
	extern char NumQw;
	
	//char MSGnames[17][100];
	
FIL file3;
extern FATFS SDFatFs;
FRESULT f_err3;

extern uint8_t IP_ADDRESS[4];
extern uint8_t NETMASK_ADDRESS[4];
extern uint8_t GATEWAY_ADDRESS[4];
extern uint8_t MACAddr[6];

extern char MSREGADR;

extern unsigned char StateReley;

struct device newDev;

extern char* smsnum;
extern unsigned int SpeedSl, SpeedMs;
extern unsigned int LOADING;

extern uint32_t FileErr;
extern u8_t GatewayDev,GatewayON;

extern unsigned char AUTORIZED;
char* login = "tik";
extern char* password = "123";

extern unsigned char CutTime;
	
extern char DelLog;

extern unsigned int Timeout;

extern unsigned char TransMode;
extern char ServiceMode;

extern unsigned char SendingEnable;

extern unsigned char NumPhones;
extern unsigned char Phones[10][13];


char log3g[30];
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief serve tcp connection  
  * @param conn: pointer on connection structure 
  * @retval None
  */
void http_server_serve(struct netconn *conn) 
{
  struct netbuf *inbuf;
  err_t recv_err;
  char* buf;
  u16_t buflen;
  struct fs_file file;
	
	char* token_buff;
	char MSGbuf[300],err;
	int num;
	
  /* Read the data from the port, blocking if nothing yet there. 
   We assume the request (the part we care about) is in one netbuf */
  recv_err = netconn_recv(conn, &inbuf);
  
  if (recv_err == ERR_OK)
  {
    if (netconn_err(conn) == ERR_OK)
    {
      netbuf_data(inbuf, (void**)&buf, &buflen);
			
			/*
			if ((buflen >= 5) && (strncmp(buf, "POST /", 6) == 0))
      {
				for(;;)
			{
			memset(a1,0,1500);
    strncpy(a1,buf,buflen);
				recv_err = netconn_recv(conn, &inbuf);
				if (recv_err == ERR_OK)
				{
					netbuf_data(inbuf, (void**)&buf, &buflen);
					if(buflen == 0)
					{
						break;
					}
				}
				else
				{
					break;	
				}
	
			}
		}
		*/
		
      /* Is this an HTTP GET command? (only check the first 5 chars, since
      there are other formats for GET, and we're keeping it very simple )*/
      if ((buflen >= 5) && (strncmp(buf, "GET /", 5) == 0))
      {
				if(AUTORIZED)
				{
					if((strncmp(buf, "GET /STM32F4xx.html", 19) == 0) || (strncmp(buf, "GET / ", 6) == 0)) // Запрос главной страницы
					{
						MAIN(conn);
					}
					else if(strncmp(buf, "GET /STM32F4xxTASKS.html", 24) == 0) // Запрос страницы диспетчера задач
					{
						DynWebPage(conn);
					}
					else if(strncmp(buf, "GET /DevConf.txt", 16) == 0) // Скачивание файла настройки опросов
					{
						unsigned int c;
						
						netconn_write(conn, PAGE_TEXT, strlen(PAGE_TEXT), NETCONN_COPY);
						netconn_write(conn, &NumQw, 1, NETCONN_COPY);
						
						if(NumQw>0)
						{
							for(int i = 0;i<NumQw;i++)
							{
								netconn_write(conn, StructQwAdr[i], sizeof(struct devices), NETCONN_COPY);
							}
						}
					}
					else if(strncmp(buf, "GET /EventConf.txt", 18) == 0) // Скачивание файла настройки событий
					{
						unsigned int c;
						
						netconn_write(conn, PAGE_TEXT, strlen(PAGE_TEXT), NETCONN_COPY);
						netconn_write(conn, &NumEvents, 1, NETCONN_COPY);
						
						if(NumEvents>0)
						{
							for(int i = 0;i<NumEvents;i++)
							{
								netconn_write(conn, StructEventAdr[i], sizeof(struct events), NETCONN_COPY);
							}
						}
					}
					else if(strncmp(buf, "GET /config.txt", 15) == 0) // Скачивание файла настройки
					{
						unsigned int c;
						
						netconn_write(conn, PAGE_TEXT, strlen(PAGE_TEXT), NETCONN_COPY);
						
						netconn_write(conn, &Holding_reg[IPREG1], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[IPREG2], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[IPREG3], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[IPREG4], 1, NETCONN_COPY);
						
						netconn_write(conn, &Holding_reg[MREG1], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[MREG2], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[MREG3], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[MREG4], 1, NETCONN_COPY);
						
						netconn_write(conn, &Holding_reg[GREG1], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[GREG2], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[GREG3], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[GREG4], 1, NETCONN_COPY);
						
						netconn_write(conn, &Holding_reg[MAC1], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[MAC2], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[MAC3], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[MAC4], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[MAC5], 1, NETCONN_COPY);
						netconn_write(conn, &Holding_reg[MAC6], 1, NETCONN_COPY);

		
						netconn_write(conn, &Holding_reg[MsREG], 1, NETCONN_COPY);
						netconn_write(conn, &NumPhones, 1, NETCONN_COPY);

							for(int j =0;j<10;j++)
								{
								for(int i =0;i<13;i++)
											{
												netconn_write(conn, &Phones[j][i], 13, NETCONN_COPY);
											}
								}

						netconn_write(conn, smsnum, 12, NETCONN_COPY);

						netconn_write(conn, &SpeedSl, 4, NETCONN_COPY);

						netconn_write(conn, &SpeedMs, 4, NETCONN_COPY);

						netconn_write(conn, &GatewayON, 1, NETCONN_COPY);

						netconn_write(conn, &smsMSG[1][0], 51, NETCONN_COPY);
						netconn_write(conn, &smsMSG[2][0], 51, NETCONN_COPY);
						netconn_write(conn, &smsMSG[3][0], 51, NETCONN_COPY);
						netconn_write(conn, &smsMSG[4][0], 51, NETCONN_COPY);

						netconn_write(conn, &CutTime, 1, NETCONN_COPY);
						netconn_write(conn, &DelLog, 1, NETCONN_COPY);

						netconn_write(conn, &Timeout, 4, NETCONN_COPY);


					}
					else if(strncmp(buf, "GET /Reboot", 11) == 0) // Очевидно ребут
					{
						NVIC_SystemReset();
						MAIN(conn);
					}
					else if(strncmp(buf, "GET /SendTESTSMS", 16) == 0) // Отправка тестового СМС
					{

					
						MAIN(conn);
						
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
									
										SendSMS5("Test SMS 123");
									
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
					else if(strncmp(buf, "GET /ChangeSlSpeed", 18) == 0) // Изменение слэйв скорости
					{
						strtok_r(buf,"=",&token_buff);
						SpeedSl = atoi((char*)strtok_r(NULL,"=",&token_buff));
						MX_USART1_UART_Init(SpeedSl);
					
						MAIN(conn);
						osSemaphoreRelease(SemMem); //Запись на флешку
					}
					else if(strncmp(buf, "GET /ChangeMsSpeed", 18) == 0) // Измененеи мастер скорости
					{
						strtok_r(buf,"=",&token_buff);
						SpeedMs = atoi((char*)strtok_r(NULL," ",&token_buff));
						MX_USART2_UART_Init(SpeedMs);
  
						MAIN(conn);
						osSemaphoreRelease(SemMem); //Запись на флешку
					}
					else if(strncmp(buf, "GET /ChangeTimeout", 18) == 0) // Изменение таймаута слэйв устройств
					{
						strtok_r(buf,"=",&token_buff);
						Timeout = atoi((char*)strtok_r(NULL," ",&token_buff));

						MAIN(conn);
						osSemaphoreRelease(SemMem); //Запись на флешку
					}
					else if(strncmp(buf, "GET /CheckRelay", 15) == 0) //Проверка реле
					{
						RELAY_ON;
						REL_LED_ON;
						MAIN(conn);
						osDelay(1000);
						RELAY_OFF;
						REL_LED_OFF;
					}
					else if(strncmp(buf, "GET /WriteSMSNum", 16) == 0) // Изменение номера для СМС уведослений
					{
						memset(MSGbuf,0,300);
						atoi((char*)strtok_r(buf,"=",&token_buff));
						strcpy(MSGbuf,(char*)strtok_r(NULL," ",&token_buff));

						// декод UTF8 в 1251 используя соотвествующие буфферы и хранилища
						if(NumPhones != 10)
						{
							err = convert_utf8_to_windows1251(MSGbuf, &Phones[NumPhones][0]);
							NumPhones++;
						}
						else
						{
							err = convert_utf8_to_windows1251(MSGbuf, &Phones[9][0]);
						}

						MAIN(conn);
						
						osSemaphoreRelease(SemMem); //Запись на флешку
					}
					else if(strncmp(buf, "GET /STM32F4xxMBUS.html", 23) == 0) // Запрос страницы настройки опросов и событий
					{
						 MODBUS(conn);
					}
					else if(strncmp(buf, "GET /STM32F4xxEVENT.html", 23) == 0)  // Запрос страницы списка событий
					{
						 EVENTLIST(conn);
					}
					else if(strncmp(buf, "GET /Kvit", 9) == 0)  // Запрос страницы списка событий
					{
						 Holding_reg[KVIT] = 1;
						 EVENTLIST(conn);
					}
					else if(strncmp(buf, "GET /KillEvent", 14) == 0) // Удаление события
					{
						TaskHandle_t THandle;
						strtok_r(buf,"?",&token_buff);
						THandle = (void*)atoi((char*)strtok_r(NULL,"=",&token_buff));
						
						for(int i=0;i<NumEvents;i++)
						{
							if(StructEventAdr[i]->THandle == THandle)
							{
								if((StructEventAdr[i]->rele)&&(StructEventAdr[i]->StateEvent)) // Если событие было активным при удалении и использовало реле, то вычетаем влияние этого события на реле
								{
									StateReley--;
								}
								
								for(int j=i;j<NumEvents;j++)
								{
								StructEventAdr[j] = StructEventAdr[j+1];
								}
								
								break;
							}
						}
						
						NumEvents--;
						
						if(NumEvents==0||StateReley==0) // Если удаляемое событие последним использовало реле, то сбрасываем
						{
							HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0,0);
						}
						
						osThreadTerminate(THandle);
						
						osSemaphoreRelease(SemSaveEv);

						EVENTLIST(conn);
					}
					else if(strncmp(buf, "GET /Float", 10) == 0) // Проверка на float регистры при логировании
					{
						//теперь тут придется парсить эту сраную строку с отметками float
						atoi((char*)strtok_r(buf,"=",&token_buff));
						if(token_buff[0]!=0x20) // проверка, есть ли floats
						{
							convert_utf8_to_windows1251((char*)strtok_r(NULL," ",&token_buff), newDev.floats);
						}
										
						flagLOG = 1;
						tryCreateLog = 0;

						CreateMSlaveTask(&newDev); // Создаем задачу логировния
					
						MODBUS(conn);
					}
					else if(strncmp(buf, "GET /Event", 10) == 0) // Создание события
					{
						struct event newEvent;
						memset(newEvent.smstext,0,51);
						memset(newEvent.name,0,25);
						//char nameEvent8[25]={0};
						
						strtok_r(buf,"=",&token_buff);
						memset(MSGbuf,0,300);
						strcpy(MSGbuf,(char*)strtok_r(NULL,"&",&token_buff));
						convert_utf8(MSGbuf, newEvent.name);
						strtok_r(NULL,"=",&token_buff);
						
						if((strncmp(token_buff, "rs", 2) == 0))
						{
							newEvent.interface = 0;
						}
						if((strncmp(token_buff, "eth", 3) == 0))
						{
							newEvent.interface = 1;
						}
						
						strtok_r(NULL,"=",&token_buff);
						newEvent.dadr = atoi((char*)strtok_r(NULL,"&",&token_buff));
						strtok_r(NULL,"=",&token_buff);
						newEvent.Nreg = atoi((char*)strtok_r(NULL,"&",&token_buff));
						
						if((strncmp(token_buff, "float", 5) == 0))
						{
							strtok_r(NULL,"&",&token_buff);
							newEvent.f1 = 1;
						}
						else
						{
							newEvent.f1 = 0;
						}
						
						strtok_r(NULL,"=",&token_buff);
						
						if((strncmp(token_buff, "H", 1) == 0))
						{
							newEvent.t1 = 3;
						}
						else if((strncmp(token_buff, "I", 1) == 0))
						{
							newEvent.t1 = 4;
						}
						
						strtok_r(NULL,"=",&token_buff);
						
						if((strncmp(token_buff, "%3D", 3) == 0)) // равно, но мб имеет смысл добавить параметр "Битовая маска"
						{
							newEvent.sr = 1;
						}
						else if((strncmp(token_buff, "%3E", 3) == 0)) // больше
						{
							newEvent.sr = 2;
						}
						else if((strncmp(token_buff, "%3C", 3) == 0))	// меньше
						{
							newEvent.sr = 3;
						}
						else if((strncmp(token_buff, "%26", 3) == 0))	// Битовая маска
						{
							newEvent.sr = 4;
						}
						
						strtok_r(NULL,"&",&token_buff);
						
						if((strncmp(token_buff, "typeif", 6) == 0))
						{
							strtok_r(NULL,"=",&token_buff);
							if((strncmp(token_buff, "on", 2) == 0))
							{
								newEvent.tsr = 1;
							}
						}
						else
						{
						newEvent.tsr = 0;
						}
						
						strtok_r(NULL,"=",&token_buff);
						
						// Если разделитель дробной части не соответствует формату
						char par[20] = {0};
						char *pa = (char*)strtok_r(NULL,"&",&token_buff);
						if(strchr(pa,'%') != 0)
						{
							convert_utf8((char*)pa,(char*)par);
							if(strchr(par,',') != 0)
							{
								*strchr(par,',') = '.';
							}
							pa = (char*)par;
						}
						newEvent.param = atof(pa);
						
						
						if((strncmp(token_buff, "float2", 6) == 0))
						{
							strtok_r(NULL,"&",&token_buff);
							newEvent.f2 = 1;
						}
						else
						{
							newEvent.f2 = 0;
						}
						
						strtok_r(NULL,"=",&token_buff);
						
						if((strncmp(token_buff, "H", 1) == 0))
						{
							newEvent.t2 = 3;
						}
						else if((strncmp(token_buff, "I", 1) == 0))
						{
							newEvent.t2 = 4;
						}
						
						strtok_r(NULL,"&",&token_buff);
						
						if((strncmp(token_buff, "logging", 7) == 0))
						{
							newEvent.log = 1;
							strtok_r(NULL,"&",&token_buff);
						}
						else
						{
							newEvent.log = 0;
						}
						
						if((strncmp(token_buff, "rele", 4) == 0))
						{
							newEvent.rele = 1;
							strtok_r(NULL,"&",&token_buff);
							strtok_r(NULL,"=",&token_buff);
							if((strncmp(token_buff, "&", 1) == 0))
							{
								token_buff = token_buff+1;
								newEvent.Rtime = 0;
							}
							else
							{
								newEvent.Rtime = atoi((char*)strtok_r(NULL,"&",&token_buff));
							}
						}
						else
						{
							newEvent.rele = 0;
							newEvent.Rtime = 0;
							strtok_r(NULL,"&",&token_buff);
						}

						if((strncmp(token_buff, "sms", 3) == 0)&&(strncmp(token_buff, "smstext", 7)!=0))
						{
							strtok_r(NULL,"&",&token_buff);
							newEvent.sms = 1;
							strtok_r(NULL,"=",&token_buff);
					
							convert_utf8_to_windows1251((char*)strtok_r(NULL," ",&token_buff), newEvent.smstext);
						}
						else
						{
							newEvent.sms = 0;
						}
						
						CreatingEventTask(&newEvent); // Создаем задачу события
						flagEV = 1;
											
						MODBUS(conn);
					}
					else if(strncmp(buf, "GET /Qwest", 10) == 0) // Создание опроса
					{
						// Структура newDev объявлена выше
						memset(&newDev,0,sizeof(newDev));
						strtok_r(buf,"=",&token_buff);
						if((strncmp(token_buff, "rs", 2) == 0))
						{
							newDev.interface = 0;
							strtok_r(NULL,"=",&token_buff);
							newDev.addr = atoi((char*)strtok_r(NULL,"&",&token_buff));
						}
						if((strncmp(token_buff, "eth", 3) == 0))
						{
							newDev.interface = 1;
							strtok_r(NULL,"=",&token_buff);
							newDev.ipadd[0] = atoi((char*)strtok_r(NULL,".",&token_buff));
							newDev.ipadd[1] = atoi((char*)strtok_r(NULL,".",&token_buff));
							newDev.ipadd[2] = atoi((char*)strtok_r(NULL,".",&token_buff));
							newDev.ipadd[3] = atoi((char*)strtok_r(NULL,"%",&token_buff));
							strtok_r(NULL,"A",&token_buff);
							newDev.port = atoi((char*)strtok_r(NULL,"&",&token_buff));
						}
						
						strtok_r(NULL,"=",&token_buff);
						
						if((strncmp(token_buff, "H", 1) == 0))
						{
							newDev.type3 = 3;
							strtok_r(NULL,"=",&token_buff);
							convert_utf8_to_windows1251((char*)strtok_r(NULL,"&",&token_buff), newDev.regs);
						}
						else if((strncmp(token_buff, "I", 1) == 0))
						{
							newDev.type4 = 4;
							strtok_r(NULL,"=",&token_buff);
							convert_utf8_to_windows1251((char*)strtok_r(NULL,"&",&token_buff), newDev.regs);
						}
						strtok_r(NULL,"=",&token_buff);
						newDev.time = atoi((char*)strtok_r(NULL,"&",&token_buff));
						
						// тут уже можно стартовать опрос
						
						
						if((strncmp(token_buff, "log=on", 6) == 0))
						{
							//тут установить флаг добавления лога и т.д.
							tryCreateLog = 1;
							newDev.log = 1;
						}
						else
						{
							flagLOG=1;
							tryCreateLog = 0;
							newDev.log = 0;
							CreateMSlaveTask(&newDev);
						}

						MODBUS(conn);
					}
					else if(strncmp(buf, "GET /TimeSet", 12) == 0) // Настройка времени
					{
						char HH, MM;
						strtok_r(buf,"=",&token_buff);
						HH = atoi((char*)strtok_r(NULL,"%",&token_buff));
						strtok_r(NULL,"A",&token_buff);
						MM = atoi((char*)strtok_r(NULL," ",&token_buff));
						SetTime(HH, MM);
						
						MAIN(conn);
					}
					else if(strncmp(buf, "GET /DateSet", 12) == 0) // Настройка даты
					{
						char Da, Mo, Ye;
						strtok_r(buf,"=",&token_buff);
						Da = atoi((char*)strtok_r(NULL,".",&token_buff));
						Mo = atoi((char*)strtok_r(NULL,".",&token_buff));
						Ye = atoi((char*)strtok_r(NULL," ",&token_buff));
						SetDate(Da, Mo, Ye);
						
						MAIN(conn);
					}
					else if(strncmp(buf, "GET /IPSettings", 15) == 0) // Настройка параметров сети
					{
						
						strtok_r(buf,"=",&token_buff);
						Holding_reg[IPREG1] = atoi((char*)strtok_r(NULL,".",&token_buff));
						Holding_reg[IPREG2] = atoi((char*)strtok_r(NULL,".",&token_buff));
						Holding_reg[IPREG3] = atoi((char*)strtok_r(NULL,".",&token_buff));
						Holding_reg[IPREG4] = atoi((char*)strtok_r(NULL,"&",&token_buff));
						
						strtok_r(NULL,"=",&token_buff);
						Holding_reg[MREG1] = atoi((char*)strtok_r(NULL,".",&token_buff));
						Holding_reg[MREG2] = atoi((char*)strtok_r(NULL,".",&token_buff));
						Holding_reg[MREG3] = atoi((char*)strtok_r(NULL,".",&token_buff));
						Holding_reg[MREG4] = atoi((char*)strtok_r(NULL,"&",&token_buff));
						
						strtok_r(NULL,"=",&token_buff);
						Holding_reg[GREG1] = atoi((char*)strtok_r(NULL,".",&token_buff));
						Holding_reg[GREG2] = atoi((char*)strtok_r(NULL,".",&token_buff));
						Holding_reg[GREG3] = atoi((char*)strtok_r(NULL,".",&token_buff));
						Holding_reg[GREG4] = atoi((char*)strtok_r(NULL,"&",&token_buff));
						
						strtok_r(NULL,"=",&token_buff);
						Holding_reg[MAC1] = atoi((char*)strtok_r(NULL,"%",&token_buff));
						strtok_r(NULL,"A",&token_buff);
						Holding_reg[MAC2] = atoi((char*)strtok_r(NULL,"%",&token_buff));
						strtok_r(NULL,"A",&token_buff);
						Holding_reg[MAC3] = atoi((char*)strtok_r(NULL,"%",&token_buff));
						strtok_r(NULL,"A",&token_buff);
						Holding_reg[MAC4] = atoi((char*)strtok_r(NULL,"%",&token_buff));
						strtok_r(NULL,"A",&token_buff);
						Holding_reg[MAC5] = atoi((char*)strtok_r(NULL,"%",&token_buff));
						strtok_r(NULL,"A",&token_buff);
						Holding_reg[MAC6] = atoi((char*)strtok_r(NULL," ",&token_buff));
						
						osSemaphoreRelease(SemMem);
						
						MAIN(conn);
					}
					else if(strncmp(buf, "GET /MaddSet", 12) == 0) // Изменение адреса регистратора
					{
						
						strtok_r(buf,"=",&token_buff);
						Holding_reg[MsREG] = atoi((char*)strtok_r(NULL," ",&token_buff));
						osSemaphoreRelease(SemMem);
						
						MAIN(conn);
					}
					else if(strncmp(buf, "GET /STM32F4xxQWEST.html", 23) == 0) // Запрос страницы списка опросов
					{
						 QWESTLIST(conn);
					}
					else if(strncmp(buf, "GET /KillQw", 11) == 0) // Удаление опроса
					{
						TaskHandle_t THandle;
						TaskHandle_t LTHandle;
						struct devices *Some;
						strtok_r(buf,"?",&token_buff);
						u32_t sometime;
						
						Some = (struct devices*)atoi((char*)strtok_r(NULL,"=",&token_buff));
						
						for(int i=0;i<NumQw;i++) // Поиск устройста
						{
							if(StructQwAdr[i] == Some)
							{
								if(StructQwAdr[i]->interface == 0) // RS485 устройство
								{
									if(StructQwAdr[i]->log == 1) // Если устройство логируется, то отправляем код для удаления в задаче логирования
									{
										vPortFree(StructQwAdr[i]->file1);
										vPortFree(StructQwAdr[i]);
										
										for(int j=i;j<NumQw;j++)
											{
											StructQwAdr[j] = StructQwAdr[j+1];
											}
										
											NumQw--;
										osSemaphoreRelease(SemSaveDev);
									}
									else if(StructQwAdr[i]->log == 0) // Если устройство не логируется, удаляем тут
									{
										vPortFree(StructQwAdr[i]);
										
										for(int j=i;j<NumQw;j++)
											{
											StructQwAdr[j] = StructQwAdr[j+1];
											}
										
											NumQw--;
										osSemaphoreRelease(SemSaveDev);
									}
								}
								else if(StructQwAdr[i]->interface == 1) // TCP устройство
								{
									if(StructQwAdr[i]->log == 1) // Если устройство логируется, то отправляем код для удаления в задаче логирования
									{
										THandle = StructQwAdr[i]->THandle;
										vPortFree(StructQwAdr[i]->file1);
										vPortFree(StructQwAdr[i]);
										for(int j=i;j<NumQw;j++)
											{
											StructQwAdr[j] = StructQwAdr[j+1];
											}
										
										osThreadTerminate(THandle);
										
											NumQw--;
										
										osSemaphoreRelease(SemSaveDev);
									}
									else if(StructQwAdr[i]->log == 0)// Если устройство не логируется, удаляем тут
									{
										THandle = StructQwAdr[i]->THandle;
										vPortFree(StructQwAdr[i]);
										for(int j=i;j<NumQw;j++)
											{
											StructQwAdr[j] = StructQwAdr[j+1];
											}
										
										osThreadTerminate(THandle);
										
											NumQw--;
										
										osSemaphoreRelease(SemSaveDev);
									}
								}

								break;
							}
						}

						QWESTLIST(conn);
					}
					else if(strncmp(buf, "GET /STM32F4xxELOG.html", 22) == 0) // Запрос страницы журнала событий
					{
						 EVENTLOG(conn);
					}
					else if(strncmp(buf, "GET /DeleteELog", 15) == 0) // Удаление журнала событий
					{
						f_err3 = f_unlink("LogEVENT.csv");
						EVENTLOG(conn);
					}
					else if(strncmp(buf, "GET /LogEVENT.txt", 13) == 0) // Скачивание журнала событий
					{
						unsigned int c;
						char logbuf[410];

						netconn_write(conn, PAGE_TEXT, strlen(PAGE_TEXT), NETCONN_COPY);
						
							taskENTER_CRITICAL();
						f_err3 = f_open(&file3, "LogEVENT.csv", FA_OPEN_EXISTING | FA_READ);
							taskEXIT_CRITICAL();
						
						if(f_err3 == FR_OK)
						{
							for(;;)
							{
										taskENTER_CRITICAL();
									f_err3 = f_read(&file3, logbuf, 400,&c);
										taskEXIT_CRITICAL();
									netconn_write(conn, logbuf, c, NETCONN_COPY);
									if (c<400) 
									{
										break;
									}
							}
								taskENTER_CRITICAL();
							f_close(&file3);
								taskEXIT_CRITICAL();
						}
					}
					else if(strncmp(buf, "GET /GatewayON", 14) == 0) // Включение шлюза
					{
						GatewayON = 1;
						MODBUS(conn);
						osSemaphoreRelease(SemMem);
					}
					else if(strncmp(buf, "GET /GatewayOFF", 15) == 0) // Отключение шлюза
					{
						GatewayON = 0;
						MODBUS(conn);
						osSemaphoreRelease(SemMem);
					}
					else if(strncmp(buf, "GET /FTPSet", 11) == 0) // Настройка внешнего FTP
					{
						memset(MSGbuf,0,300);

						strtok_r(buf,"=",&token_buff);
						strcpy(MSGbuf,(char*)strtok_r(NULL,"&",&token_buff));
						err = convert_utf8(MSGbuf, &smsMSG[1][0]);
						
						strtok_r(NULL,"=",&token_buff);
						strcpy(MSGbuf,(char*)strtok_r(NULL,"&",&token_buff));
						err = convert_utf8(MSGbuf, &smsMSG[2][0]);
						
						strtok_r(NULL,"=",&token_buff);
						strcpy(MSGbuf,(char*)strtok_r(NULL,"&",&token_buff));
						err = convert_utf8(MSGbuf, &smsMSG[3][0]);
						
						strtok_r(NULL,"=",&token_buff);
						if(token_buff[0] != 0x20)
						{
							strcpy(MSGbuf,(char*)strtok_r(NULL," ",&token_buff));
							err = convert_utf8(MSGbuf, &smsMSG[4][0]);
						}
						
						MAIN(conn);
						osSemaphoreRelease(SemMem);
					}
					else if(strncmp(buf, "GET /FTP2Set", 12) == 0) // Настройка: удалять файл после успешной отправки или нет
					{
						strtok_r(buf,"=",&token_buff);
						
						if(strstr(token_buff,"delete=on")==0)
						{
							DelLog = 0;
							
							CutTime = atoi((char*)strtok_r(NULL," ",&token_buff));
						}
						else
						{
							DelLog = 1;
							
							CutTime = atoi((char*)strtok_r(NULL,"&",&token_buff));
						}
					
						MAIN(conn);
						osSemaphoreRelease(SemMem);
					}
					else if(strncmp(buf, "GET /FTP3Set", 12) == 0) // Настройка: удалять файл после успешной отправки или нет
					{
						strtok_r(buf,"?",&token_buff);
						
						if(strstr(token_buff,"sending=on")!=0)
						{
							SendingEnable = 1;
						}
						else
						{
							SendingEnable = 0;
						}
					
						MAIN(conn);
						osSemaphoreRelease(SemMem);
					}
					else if(strncmp(buf, "GET /EXIT", 9) == 0) // Разлогирование
					{
						AUTORIZED = 0;
						AUTORIZ(conn);
					}
					else if(strncmp(buf, "GET /ChangeGraph", 16) == 0) //ChangeGraph?NumPoints=30&period=500&reg=2
					{
						strtok_r(buf,"=",&token_buff);
						NumPointsG = atoi((char*)strtok_r(NULL,"&",&token_buff));
						
						strtok_r(NULL,"=",&token_buff);
						PeriodG = atoi((char*)strtok_r(NULL,"&",&token_buff));
						
						strtok_r(NULL,"=",&token_buff);
						RegG = atoi((char*)strtok_r(NULL," ",&token_buff));
						
						int k = 0;
							for(int i = 0;i < StructQwAdr[NumG]->NumQI;i++)
							{
								for(int j = StructQwAdr[NumG]->PatternI[i][0]; j <= StructQwAdr[NumG]->PatternI[i][1]; j++)
								{
									if(j==RegG)
									{
										Gfloat = 0;
										for(int p = 0;p<100;p++)
										{
											if(StructQwAdr[NumG]->fl2[p] == j)
											{
												Gfloat = 1;
												break;
											}
										}
										NRegG = k;
										k=-1;
										break;
									}
									k++;
								}
								if(k==-1)
								{
									break;
								}
							}
						
						SHOWGRAPH(conn);
					}
					else if(strncmp(buf, "GET /ShowGraph", 14) == 0) // Страница отображения графика
					{
						
						strtok_r(buf,"?",&token_buff);
						NumG = atoi((char*)strtok_r(NULL," ",&token_buff));
						
						SHOWGRAPH(conn);
					}
					else if(strncmp(buf, "GET /logo.png", 13) == 0) // Запрос логотипа
					{
						fs_open(&file, "/logo.png"); 
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}
					else if(strncmp(buf, "GET /favicon.ico", 16) == 0) // Запрос favicon
					{
						fs_open(&file, "/favicon.ico"); 
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}
					else if(strncmp(buf, "GET /export.js", 14) == 0)
					{
						fs_open(&file, "/export.js"); 
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}
					else if(strncmp(buf, "GET /exporting.js", 17) == 0)
					{
						fs_open(&file, "/exporting.js"); 
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}
					else if(strncmp(buf, "GET /highcharts.js", 18) == 0)
					{
						fs_open(&file, "/highcharts.js"); 
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}
					else if(strncmp(buf, "GET /jquery.min.js", 18) == 0)
					{
						fs_open(&file, "/jquery.min.js"); 
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}
					else if(strncmp(buf, "GET /Phones", 11) == 0)
					{
						netconn_write(conn, PAGE_OKJ, strlen(PAGE_OKJ), NETCONN_COPY);
						for(int i = 0; i < NumPhones; i++)
						{
							netconn_write(conn, &Phones[i][0], 12, NETCONN_COPY);
							if(i<(NumPhones-1))
							{
								netconn_write(conn, ",", 1, NETCONN_COPY);
							}
						}
						
					}
					else if(strncmp(buf, "GET /deletenum", 14) == 0)
					{
						unsigned char Num = atoi((char*)&buf[14]);
						
						for(int i = Num; i < NumPhones;i++)
						{
							strncpy(&Phones[i][0],&Phones[i+1][0],12);
						}
							
						NumPhones--;
						
						MAIN(conn);
						
						osSemaphoreRelease(SemMem); //Запись на флешку
					}
					else if(strncmp(buf, "GET /HardReset", 14) == 0) // Создание опроса
					{
						HAL_StatusTypeDef	flash_ok = HAL_ERROR;
						
													
													flash_ok = HAL_FLASH_Unlock();
													
												HAL_Delay(1000);
													FLASH_Erase_Sector(FLASH_SECTOR_9, VOLTAGE_RANGE_3);
												HAL_Delay(1000);
													FLASH_Erase_Sector(FLASH_SECTOR_10, VOLTAGE_RANGE_3);
												HAL_Delay(1000);
													FLASH_Erase_Sector(FLASH_SECTOR_11, VOLTAGE_RANGE_3);
												HAL_Delay(1000);
													HAL_Delay(1000);
												flash_ok = HAL_FLASH_Lock();
												HAL_Delay(1000);
												
												
						NVIC_SystemReset();
					}
					else if(strncmp(buf, "GET /someparam", 14) == 0) // ответ на java запрос для динамического графика
					{
						netconn_write(conn, PAGE_OKJ, strlen(PAGE_OKJ), NETCONN_COPY);
						char aaa[10] = {0};
						
						if(Gfloat)
						{
							float holdf;
							unsigned int hi;
							
							if(swFloat)
							{
								hi=0;
								hi = StructQwAdr[NumG]->Input[NRegG];
								hi |= StructQwAdr[NumG]->Input[NRegG+1]<<16;
								holdf = *(float*)&hi;
								sprintf(aaa,"%f",holdf);
							}
							else
							{
								hi=0;
								hi = StructQwAdr[NumG]->Input[NRegG]<<16;
								hi |= StructQwAdr[NumG]->Input[NRegG+1];
								holdf = *(float*)&hi;
								sprintf(aaa,"%f",holdf);
							}
						}
						else
						{
							sprintf(aaa,"%d",StructQwAdr[NumG]->Input[NRegG]);
						}
						
						netconn_write(conn, aaa, strlen(aaa), NETCONN_COPY);
					}
					else // 404
					{
						fs_open(&file, "/404.html");
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}
				}
				else // Не авторизованы 
				{
					if(strncmp(buf, "GET /AUTORIZ", 12) == 0) // Авторизация
					{
						strtok_r(buf,"=",&token_buff);
						char* log = strtok_r(NULL,"&",&token_buff);
						strtok_r(NULL,"=",&token_buff);
						char* pass = strtok_r(NULL," ",&token_buff);
						if((strcmp(log,login) == 0) && (strcmp(pass,password) == 0))
						{
							AUTORIZED = 1;
							MAIN(conn);
						}
						else
						{
							AUTORIZED = 3;
							AUTORIZ(conn);
						}
					}
					else if(strncmp(buf, "GET /logo.png", 13) == 0) // Запрос логотипа
					{
						fs_open(&file, "/logo.png"); 
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}
					else if(strncmp(buf, "GET /favicon.ico", 16) == 0)// Запрос favicon
					{
						fs_open(&file, "/favicon.ico"); 
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}
					else // Страница авторизации
					{
						AUTORIZ(conn);
					}
				}
      }
    }
  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);
  
  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}

/*
Генерация страницы авторизации
*/
void AUTORIZ(struct netconn *conn)
{
	netconn_write(conn, PAGE_OK, strlen(PAGE_OK), NETCONN_COPY);
  netconn_write(conn, AUTORIZATION, strlen(AUTORIZATION), NETCONN_COPY);
	if(AUTORIZED == 3)
	{
		netconn_write(conn, PAGE_WRONG_LOGPASS_JS, strlen(PAGE_WRONG_LOGPASS_JS), NETCONN_COPY);
		AUTORIZED = 0;
	}
	
}

/*
Генерация страницы диспетчера задач
*/
void DynWebPage(struct netconn *conn)
{
	osStatus list;
	u32_t size;
  char PAGE_BODY[1500],buf[100];
  portCHAR pagehits[10] = {0};
  memset(PAGE_BODY, 0,1500);
	float ver;
	
	netconn_write(conn, PAGE_OK, strlen(PAGE_OK), NETCONN_COPY);
  netconn_write(conn, PAGE_TASK, strlen(PAGE_TASK), NETCONN_COPY);
	
	
	memset(log3g,0,30);
	memcpy(log3g,data_com3_IN,29);
	
	for(int i = 0;i<29;i++)
	{
		if(log3g[i]==0)
		{
			log3g[i] = '_';
		}
	}
	
	
  /* Update the hit count */

  ver = (float)VERSION/100;
  sprintf(pagehits, " %.2f", ver);
  strcat(PAGE_BODY, pagehits);
  strcat((char *)PAGE_BODY, "<pre><br>Name          State  Priority  Stack   Num" );
  strcat((char *)PAGE_BODY, "<br>---------------------------------------------<br>");
    
  /* The list of tasks and their status */
	list = osThreadList((unsigned char *)(PAGE_BODY + strlen(PAGE_BODY)));
  
	strcat((char *)PAGE_BODY, "<br><br>---------------------------------------------");
  strcat((char *)PAGE_BODY, "<br>Легенда: B - Blocked, R - Ready, D - Deleted, S - Suspended<br><br>");
	size = xPortGetFreeHeapSize();
	sprintf(buf,"Free Heap Size(byte) / Heap Size(byte): %d / %d<br><br>",size,configTOTAL_HEAP_SIZE);
	strcat((char *)PAGE_BODY,buf);
	sprintf(buf,"<pre><br>Name            Abs Time        %% Time");
	strcat((char *)PAGE_BODY,buf);
		strcat((char *)PAGE_BODY, "<br>---------------------------------------------<br>");
	netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
	memset(PAGE_BODY, 0,1500);
	vTaskGetRunTimeStats(PAGE_BODY);
	strcat((char *)PAGE_BODY,"<br><br>3g LOG:<br>");
	strcat((char *)PAGE_BODY, log3g);
	strcat((char *)PAGE_BODY,"</div></body></html>");
	/* Send the dynamically generated page */
	
  netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);

}


/*
Генерация страницы настройки опросов и событий
*/
void MODBUS (struct netconn *conn)
{
  char PAGE_BODY[720];
	char buf[512];
	memset(PAGE_BODY, 0,512);
	
		netconn_write(conn, PAGE_OK, strlen(PAGE_OK), NETCONN_COPY);
		netconn_write(conn, PAGE_MODBUS, strlen(PAGE_MODBUS), NETCONN_COPY);
		netconn_write(conn, PAGE_QW, strlen(PAGE_QW), NETCONN_COPY);

if(tryCreateLog) // Процесс создания опроса с  логированием
{
	sprintf(buf,"<p style=\"color:#ff0000\">Создание лога регистров. Отметьте регистры, которые должны быть в формате FLOAT.<br>Не отмеченые будут записываться в формате INT</p><form method=\"get\" action=\"/Float\"><input name=\"floats\" type=\"textbox\"><br><input value=\"Начать запись лога\" type=\"submit\"></form>");
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	tryCreateLog = 0;
}

if(flagLOG==1) // Опрос создан
{
	netconn_write(conn, PAGE_ADDED_JS, strlen(PAGE_ADDED_JS), NETCONN_COPY);
	flagLOG=0;
}

if(flagEV==1) // Событие создано
{
	netconn_write(conn, PAGE_ADDED_JS, strlen(PAGE_ADDED_JS), NETCONN_COPY);
	flagEV=0;
}

	netconn_write(conn, PAGE_EVENT, strlen(PAGE_EVENT), NETCONN_COPY);
	
if(GatewayON == 0)
	{
		netconn_write(conn, PAGE_MODBUS3, strlen(PAGE_MODBUS3), NETCONN_COPY);
	}
	else
	{
		netconn_write(conn, GATEWAY_ON, strlen(GATEWAY_ON), NETCONN_COPY);
	}
	
	strcat((char *)PAGE_BODY, "</div></body></html>");
  netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
}

/*
Генерация страницы списка опросов
*/
void EVENTLIST(struct netconn *conn)
{
	char PAGE_BODY[512];
	char buf[500]= {0};
	memset(PAGE_BODY, 0,512);
	char* rs = "RS485";
	char* eth = "ETH";
	char* interface;
	
	netconn_write(conn, PAGE_OK, strlen(PAGE_OK), NETCONN_COPY);
	netconn_write(conn, PAGE_EVENTLIST, strlen(PAGE_EVENTLIST), NETCONN_COPY);
	if(NumEvents==0)
	{
		sprintf(buf,"Нет созданых событий<br>");
		netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	}
	else
	{
		sprintf(buf,"<a href=\"EventConf.txt\" download style=\"text-decoration: none;\"><input value=\"Скачать конфигурацию событий\" type=\"button\"></a><br>");
		netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
		
		sprintf(buf,"<big>Количество cозданных событий: %d</big><br><br>", NumEvents);
		netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
		
		sprintf(buf,"<form method=\"get\" action=\"/Kvit\"><input value=\"КВИТИРОВАТЬ\" type=\"submit\"></form><br>", NumEvents);
		netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
		
		for(int i=0;i<NumEvents;i++)
		{
			
			switch (StructEventAdr[i]->interface)
			{
				case 0:
					interface = rs;
				break;
				
				case 1:
					interface = eth;
				break;
			}
			
			sprintf(buf,"<u><big>Событие %d</big></u><br>Интерфейс: %s<br>Устройство: %d<br>Имя события: %s <br>Состояние: %d <br>Счетчик: %d <br>", i+1, interface, StructEventAdr[i]->dadr, StructEventAdr[i]->name, StructEventAdr[i]->StateEvent, StructEventAdr[i]->counter);
			netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
			sprintf(buf,"<form method=\"get\" action=\"/KillEvent\"><input value=\"Удалить событие\" type=\"submit\" name=\"%d\"><br><br>", StructEventAdr[i]->THandle);
			netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
			
		}
	}
	
	strcat((char *)PAGE_BODY, "</div></body></html>");
	netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
	
}

/*
Генерация главной страницы
*/
void MAIN (struct netconn *conn)
{
	RTC_DateTypeDef sdatestructureget;
  RTC_TimeTypeDef stimestructureget;
	
	char PAGE_BODY[300] = {0};
	char buf[800] = {0};

	HAL_RTC_GetTime(&hrtc, &stimestructureget, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sdatestructureget, RTC_FORMAT_BIN);
	
	netconn_write(conn, PAGE_OK, strlen(PAGE_OK), NETCONN_COPY);
  netconn_write(conn, PAGE_MAIN, strlen(PAGE_MAIN), NETCONN_COPY);

	sprintf(buf, MAIN_IP, IP_ADDRESS[0], IP_ADDRESS[1], IP_ADDRESS[2], IP_ADDRESS[3], NETMASK_ADDRESS[0], NETMASK_ADDRESS[1], NETMASK_ADDRESS[2], NETMASK_ADDRESS[3], GATEWAY_ADDRESS[0], GATEWAY_ADDRESS[1], GATEWAY_ADDRESS[2], GATEWAY_ADDRESS[3],MACAddr[0],MACAddr[1],MACAddr[2],MACAddr[3],MACAddr[4],MACAddr[5]);
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	
	sprintf(buf, MAIN_MADD, MSREGADR);
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);

	sprintf(buf,"Скорость RS485 регистратора:<form method=\"get\" action=\"/ChangeSlSpeed\"><input name=\"Speed\" pattern=\"[0-9]{4,}\" maxlength=\"10\" value=\"%d\" autocomplete=\"off\" required><br><input value=\"Изменить\" type=\"submit\"></form>",SpeedSl);
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	sprintf(buf,"Скорость подключаемых устройств по RS485:<form method=\"get\" action=\"/ChangeMsSpeed\"><input name=\"Speed\" pattern=\"[0-9]{4,}\" maxlength=\"10\" value=\"%d\" autocomplete=\"off\" required><br><input value=\"Изменить\" type=\"submit\"></form>",SpeedMs);
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	sprintf(buf,"Таймаут:<form method=\"get\" action=\"/ChangeTimeout\"><input name=\"Timeout\" pattern=\"[0-9]{3,}\" value=\"%d\" autocomplete=\"off\" required><br><input value=\"Изменить\" type=\"submit\"></form><br>",Timeout);
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	
	sprintf(buf, MAIN_DATE, sdatestructureget.Date,sdatestructureget.Month,sdatestructureget.Year);
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	sprintf(buf, MAIN_TIME, stimestructureget.Hours,stimestructureget.Minutes);
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	
	sprintf(buf, MAIN_FTP, &smsMSG[1][0]);
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	
	
	if(DelLog)
	{
		sprintf(buf, MAIN_FTP2, CutTime, "checked");
	}
	else
	{
		sprintf(buf, MAIN_FTP2, CutTime, " ");
	}
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);	
	
	if(SendingEnable)
	{
		sprintf(buf, MAIN_FTP3, "checked");
	}
	else
	{
		sprintf(buf, MAIN_FTP3, " ");
	}
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	

	sprintf(buf,"<hr style=\"width: 100%; height: 2px;\"><h2 align=\"center\">Настройка СМС уведомлений</h2><div id=\"container\"></div><Br><form method=\"get\" action=\"/WriteSMSNum\"><input type=\"tel\" name=\"Tel\" pattern=\"[+][7][0-9]{10}\" maxlength=\"12\" autocomplete=\"off\" required> Номер мобильного телефона для уведомлений<br><input value=\"Добавить\" type=\"submit\"></form><form method=\"get\" action=\"/SendTESTSMS\"><input value=\"Отправить тестовое СМС\" type=\"submit\"></form><br><br>");
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	
	netconn_write(conn, PhoneScript, strlen(PhoneScript), NETCONN_COPY);
	
	if(ServiceMode == 0)
	{
		sprintf(buf,"<hr style=\"width: 100%; height: 2px;\"><h2 align=\"center\">Другое</h2><div style=\"float:left;\">Ошибки SD: %d<Br>Свободно памяти на SD: %s<Br>Качаство мобильной связи: %d<Br><Br><form method=\"get\" action=\"/EXIT\"><input value=\"ВЫЙТИ\" type=\"submit\"></form></div><div style=\"float:right;\"><form method=\"get\" action=\"Reboot\"><input type=\"submit\" value=\"Перезагрузить\"></form><br><br><a href=\"config.txt\" download style=\"text-decoration: none;\"><input value=\"Скачать конфигурацию прибора\" type=\"button\"></a></div>", FileErr, &smsMSG[10][0], Holding_reg[1]);
	}
	else
	{
		sprintf(buf,"<hr style=\"width: 100%; height: 2px;\"><h2 align=\"center\">Другое</h2><div style=\"float:left;\"><form method=\"get\" action=\"/CheckRelay\"><input value=\"РЕЛЕ\" type=\"submit\"></form><form method=\"get\" action=\"/HardReset\"><input value=\"СБРОС ДО ЗАВОДСКИХ\" type=\"submit\"></form>Ошибки SD: %d<Br>Свободно памяти на SD: %s<Br>Качаство мобильной связи: %d<Br><Br><form method=\"get\" action=\"/EXIT\"><input value=\"ВЫЙТИ\" type=\"submit\"></form></div><div style=\"float:right;\"><form method=\"get\" action=\"Reboot\"><input type=\"submit\" value=\"Перезагрузить\"></form><br><br><a href=\"config.txt\" download style=\"text-decoration: none;\"><input value=\"Скачать конфигурацию прибора\" type=\"button\"></a></div>", FileErr, &smsMSG[10][0], Holding_reg[1]);
	}
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	
	/*
	sprintf(buf,"<form method=\"post\" action=\"/config\"><input type=\"file\" name=\"file123.txt\"><input type=\"submit\" formenctype=\"multipart/form-data\" formmethod=\"post\"></form>");
	netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	*/
	
	strcat((char *)PAGE_BODY, "</div></body></html>");
  netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
}

/*
Генерация страницы списка опросов
*/
void QWESTLIST(struct netconn *conn)
{
	char PAGE_BODY[200];
	char buf[500]= {0};
	memset(PAGE_BODY, 0,200);
	
	char* rs = "RS485";
	char* eth = "ETH";
	char* interface;
	
	char* l1 = "ведется";
	char* l2 = "не ведется";
	char* logex;
	
	netconn_write(conn, PAGE_OK, strlen(PAGE_OK), NETCONN_COPY);
	netconn_write(conn, PAGE_QWLIST, strlen(PAGE_QWLIST), NETCONN_COPY);
	
	
	if(NumQw==0)
	{
		sprintf(buf,"Нет созданых опросов<br>");
		netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	}
	else
	{
		sprintf(buf,"<a href=\"DevConf.txt\" download style=\"text-decoration: none;\"><input value=\"Скачать конфигурацию опросов\" type=\"button\"></a><br>");
		netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
		
		sprintf(buf,"<big>Количество cозданных опросов: %d</big><br><br>", NumQw);
		netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
		
		for(int i=0;i<NumQw;i++)
		{
			switch (StructQwAdr[i]->log)
			{
				
				case 1:
				logex = l1;
				break;
								
				case 0x00:
				logex = l2;
				break;
				
				
				default:
				break;
			}
			
			switch (StructQwAdr[i]->interface)
			{
				case 0:
					interface = rs;
					sprintf(buf,"<u><big>Опрос %d</big></u><form method=\"get\" action=\"/ShowGraph\"><input value=\"Открыть график\" type=\"submit\" name=\"%d\"></form>Интерфейс: %s<br>Устройство: %d<br>", i+1, i, interface, StructQwAdr[i]->addr);


				break;
				
				case 1:
					interface = eth;
					sprintf(buf,"<u><big>Опрос %d</big></u><form method=\"get\" action=\"/ShowGraph\"><input value=\"Открыть график\" type=\"submit\" name=\"%d\"></form>Интерфейс: %s<br>Устройство: %d.%d.%d.%d:%d<br>", i+1,i, interface, StructQwAdr[i]->ipadd[0],StructQwAdr[i]->ipadd[1],StructQwAdr[i]->ipadd[2],StructQwAdr[i]->ipadd[3],StructQwAdr[i]->port);
				
				break;
			}
		
				
					if(StructQwAdr[i]->type3 == 3)
					{
					sprintf(PAGE_BODY,"Регистры H: %s <br>",StructQwAdr[i]->regsH);
					strcat(buf,PAGE_BODY);
					}
					if(StructQwAdr[i]->type4 == 4)
					{
					sprintf(PAGE_BODY,"Регистры I: %s <br>",StructQwAdr[i]->regsI);
					strcat(buf,PAGE_BODY);
					}
					
					sprintf(PAGE_BODY,"Период: %d <br>Логирование: %s <br>",StructQwAdr[i]->time, logex);
					strcat(buf,PAGE_BODY);
					
					sprintf(PAGE_BODY,"Ошибки связи/Кол-во запросов: %d/%d<br>",StructQwAdr[i]->BadPackets, StructQwAdr[i]->Packets);
					strcat(buf,PAGE_BODY);
			
			
			netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
			sprintf(buf,"<form method=\"get\" action=\"/KillQw\"><input value=\"Удалить опрос\" type=\"submit\" name=\"%d\"></form><br><br>", StructQwAdr[i]);
			netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
			
		}
	}
	
	sprintf((char *)PAGE_BODY, "</div></body></html>");
	netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
}

/*
Генерация страницы журнала событий
*/
void EVENTLOG(struct netconn *conn)
{
	char PAGE_BODY[500];
	char buf[500]= {0};
	memset(PAGE_BODY, 0,500);
	unsigned int c;
	char* enter;
	
	netconn_write(conn, PAGE_OK, strlen(PAGE_OK), NETCONN_COPY);
	netconn_write(conn, PAGE_EVENTLOG, strlen(PAGE_EVENTLOG), NETCONN_COPY);
	
//	taskENTER_CRITICAL();
	f_err3 = f_open(&file3, "LogEVENT.csv", FA_OPEN_EXISTING | FA_READ);
//	taskEXIT_CRITICAL();
	if(f_err3!= FR_OK)
	{
		sprintf(buf,"Журнал не создан");
		netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
	}
	else if(f_err3 == FR_OK)
	{
		if(file3.obj.objsize == 0)
		{
		sprintf(buf,"Журнал пуст");
		netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
		}
		else
		{
			sprintf(buf,"<div style=\"width: 300px;\"><div style=\"float: left;\"><a href=\"LogEVENT.csv\" download style=\"text-decoration: none;\"><input value=\"Скачать журнал\" type=\"button\"></a></div><div style=\"float: right;\"><form method=\"get\" action=\"/DeleteELog\"><input value=\"Удалить журнал\" type=\"submit\"></form></div></div><br><br>");
			netconn_write(conn, buf, strlen(buf), NETCONN_COPY);
			 for(;;)
            {
						//	taskENTER_CRITICAL();
              f_err3 = f_read(&file3, buf, 400,&c);
						//	taskEXIT_CRITICAL();
							
							while(1)
							{
								enter = strstr(buf, "  \r\n");
								if(enter != 0)
								{
									strncpy(enter,"<br>",4);
								}
								else
								{
									break;
								}
							}
							
							netconn_write(conn, buf, c, NETCONN_COPY);
							if (c<400) 
							{
								break;
							}
            }
		}
	//	taskENTER_CRITICAL();
		f_close(&file3);
	//	taskEXIT_CRITICAL();
	}
	
	strcat((char *)PAGE_BODY, "</div></body></html>");
	netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
	
}

/*
Отображение графика
*/
void SHOWGRAPH(struct netconn *conn)
{
	memset(Gbuf,0,2500);
	char buflist[50] = {0};
	netconn_write(conn, PAGE_OK, strlen(PAGE_OK), NETCONN_COPY);
	
	netconn_write(conn, SHOW_GRAPH, strlen(SHOW_GRAPH), NETCONN_COPY);
	
	sprintf(Gbuf,GRAPH_J, PeriodG/2, PeriodG, NumG+1, NumPointsG-1, PeriodG);
	
	netconn_write(conn, Gbuf, strlen(Gbuf), NETCONN_COPY);
	
	sprintf(Gbuf, "<form method=\"get\" action=\"ChangeGraph\"><input type=\"text\" name=\"NumPoints\" value=\"%d\" autocomplete=\"off\" required> Число точек<br><input type=\"text\" name=\"period\" value=\"%d\" autocomplete=\"off\" required> Период, мс<br><input type=\"text\" name=\"reg\" list=\"regs\" autocomplete=\"off\" value=\"%d\" required><datalist id=\"regs\">", NumPointsG, PeriodG, RegG);
	
	netconn_write(conn, Gbuf, strlen(Gbuf), NETCONN_COPY);
	
	for(int i = 0;i < StructQwAdr[NumG]->NumQI;i++)
	{
		for(int j = StructQwAdr[NumG]->PatternI[i][0]; j <= StructQwAdr[NumG]->PatternI[i][1]; j++)
		{
			sprintf(buflist,"<option>%d</option>",j);
			netconn_write(conn, buflist, strlen(buflist), NETCONN_COPY);
		}
	}
	
	sprintf(Gbuf,"</datalist> Номер регистра<br><input type=\"submit\" value=\"Применить\"></form></body></html>");
	
	netconn_write(conn, Gbuf, strlen(Gbuf), NETCONN_COPY);
}


/*
Ф-я установки времени
*/
void SetTime(char H, char M)
{
	RTC_TimeTypeDef sTime;
	
		if(H<10)
	{
		H = H;
	}
	else if(H>=10&&H<20)
	{
		H = H+6;
	}
		else if(H>=20&&H<30)
	{
		H = H+12;
	}
	
	
		if(M<10)
	{
		M = M;
	}
	else if(M>=10&&M<20)
	{
		M = M+6;
	}
	else if(M>=20&&M<30)
	{
		M = M+12;
	}
	else if(M>=30&&M<40)
	{
		M = M+18;
	}
		else if(M>=40&&M<50)
	{
		M = M+24;
	}
		else if(M>=50&&M<0x60)
	{
		M = M+30;
	}

	
	sTime.Hours = H;
  sTime.Minutes = M;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD);

	
}

/*
Ф-я установки даты
*/
void SetDate(char D, char M, char Y)
{
	RTC_DateTypeDef sDate;
	
	if(D<10)
	{
		D = D;
	}
	else if(D>=10&&D<20)
	{
		D = D+6;
	}
	else if(D>=20&&D<30)
	{
		D = D+12;
	}
	else if(D>=30&&D<40)
	{
		D = D+18;
	}

	
	
		if(M<10)
	{
		M = M;
	}
	else if(M>=10&&M<20)
	{
		M = M+6;
	}

	
	
		if(Y<10)
	{
		Y = Y;
	}
	else if(Y>=10&&Y<20)
	{
		Y = Y+6;
	}
	else if(Y>=20&&Y<30)
	{
		Y = Y+12;
	}
	else if(Y>=30&&Y<40)
	{
		Y = Y+18;
	}
		else if(Y>=40&&Y<50)
	{
		Y = Y+24;
	}
		else if(Y>=50&&Y<0x60)
	{
		Y = Y+30;
	}
	
	sDate.WeekDay = RTC_WEEKDAY_WEDNESDAY;
	sDate.Date = D;
	sDate.Month = M;
  sDate.Year = Y;

  HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD);
}

