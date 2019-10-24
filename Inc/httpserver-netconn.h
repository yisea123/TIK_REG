#ifndef __HTTPSERVER_NETCONN_H__
#define __HTTPSERVER_NETCONN_H__

#include "lwip/api.h"
void http_server_serve(struct netconn *conn);
void http_server_netconn_init(void);
void DynWebPage(struct netconn *conn);
void MODBUS (struct netconn *conn);
void SMS (struct netconn *conn);
void EVENTLIST(struct netconn *conn);
void MAIN (struct netconn *conn);
void QWESTLIST(struct netconn *conn);
void EVENTLOG(struct netconn *conn);
void AUTORIZ(struct netconn *conn);
void SHOWGRAPH(struct netconn *conn);

void SetTime(char H, char M);
void SetDate(char D, char M, char Y);
#endif /* __HTTPSERVER_NETCONN_H__ */
