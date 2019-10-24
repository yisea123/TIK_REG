//--------------------------------------------------------------
// File     : stm32_ub_fatfs.h
//--------------------------------------------------------------

//--------------------------------------------------------------
#ifndef __STM32F4_UB_FATFS_H
#define __STM32F4_UB_FATFS_H


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32f4xx.h"
#include "ff.h"                // FATFS-Funktionen
//#include "stm32_ub_sdcard.h"   // SD-Funktionen



//--------------------------------------------------------------
// Тип карты (в данном примере только MMC)
//--------------------------------------------------------------
typedef enum {
  MMC_0 = 0       // данная библиотека поддерживает только MMC
}MEDIA_t;


//--------------------------------------------------------------
// коды ошибок
//--------------------------------------------------------------
typedef enum {
  FATFS_OK =0,
  FATFS_NO_MEDIA,
  FATFS_MOUNT_ERR,
  FATFS_GETFREE_ERR,
  FATFS_UNLINK_ERR,
  FATFS_OPEN_ERR,
  FATFS_CLOSE_ERR,
  FATFS_PUTS_ERR,
  FATFS_SEEK_ERR,
  FATFS_RD_STRING_ERR,
  FATFS_RD_BLOCK_ERR,
  FATFS_WR_BLOCK_ERR,
  FATFS_EOF,
  FATFS_DISK_FULL
}FATFS_t;


//--------------------------------------------------------------
// Режим для OpenFile
//--------------------------------------------------------------
typedef enum {
  F_RD =0,    // Р·Р°С‡РёС‚Р°С‚СЊ (С‚РѕР»СЊРєРѕ РµСЃР»Рё С„Р°Р№Р» СЃСѓС‰РµСЃС‚РІСѓРµС‚)
  F_WR,       // Р·Р°РїРёСЃР°С‚СЊ (С‚РѕР»СЊРєРѕ РµСЃР»Рё С„Р°Р№Р» СЃСѓС‰РµСЃС‚РІСѓРµС‚) Рё РґРѕР±Р°РІРёС‚СЊ РґР°РЅРЅС‹Рµ
  F_WR_NEW,   // Р·Р°РїРёСЃР°С‚СЊ (СЃРѕР·РґР°С‚СЊ РЅРѕРІС‹Р№ С„Р°Р№Р») Рё РґРѕР±Р°РІРёС‚СЊ РґР°РЅРЅС‹Рµ
  F_WR_CLEAR  // Р·Р°РїРёСЃР°С‚СЊ РїРѕРІРµСЂС… (СѓРґР°Р»РёС‚СЊ СЃС‚Р°СЂС‹Рµ РґР°РЅРЅС‹Рµ)
}FMODE_t;


//--------------------------------------------------------------
// Глобальная функция
//--------------------------------------------------------------
void UB_Fatfs_Init(void);
FATFS_t UB_Fatfs_CheckMedia(MEDIA_t dev);
FATFS_t UB_Fatfs_Mount(MEDIA_t dev);
FATFS_t UB_Fatfs_UnMount(MEDIA_t dev);
FATFS_t UB_Fatfs_DelFile(const char* name);
FATFS_t UB_Fatfs_OpenFile(FIL* fp, const char* name, FMODE_t mode);
FATFS_t UB_Fatfs_CloseFile(FIL* fp);
FATFS_t UB_Fatfs_WriteString(FIL* fp, const char* text);
FATFS_t UB_Fatfs_ReadString(FIL* fp, char* text, uint32_t len);
uint32_t UB_Fatfs_FileSize(FIL* fp);
FATFS_t UB_Fatfs_ReadBlock(FIL* fp, unsigned char* buf, uint32_t len, uint32_t* read);
FATFS_t UB_Fatfs_WriteBlock(FIL* fp, unsigned char* buf, uint32_t len, uint32_t* write);
FATFS_t UB_Fatfs_WriteString2(FIL* fp, const char* text, uint8_t param);
FRESULT scan_files (char* path);


//--------------------------------------------------------------
#endif // __STM32F4_UB_FATFS_H
