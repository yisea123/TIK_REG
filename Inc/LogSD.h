#include "main.h"
#include "string.h"



int findNULL(char bufer[100]);
void MSlaveTask(struct device *dev);
void win1251_to_utf8 (unsigned char win[17][51],unsigned char utf[17][100]);
int convert_utf8_to_windows1251(char utf8[300], char windows1251[300]);
void CreateMSlaveTask(struct device *bis);
int convert_utf8(char utf8w[300], char utf8[25]);
void MBusMasterPlaner();
void MBusTCPMasterTask(struct device *dev);
char ParsingAddr(char* regs, unsigned short int Patern[20][2]);
int UploadToFTP(char *filename);
void TCP_GSM_Listener();
unsigned char TCP_GSM(char *filename);
int WriteFileToGSM(char *filename, unsigned int filesize);
unsigned char DeleteGSMFile(char* filename);
unsigned char DownloadBoot(void);
unsigned char DownloadFromGSMtoSD(void);
unsigned int fgsm_filesize(char* filename);
unsigned char fgsm_open(char* filename);
unsigned char fgsm_read(char* buf, unsigned int len);
unsigned char fgsm_close(void);

