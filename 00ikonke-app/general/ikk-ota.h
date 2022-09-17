#ifndef __IKONKE_MODULE_OTA_H_______________________
#define __IKONKE_MODULE_OTA_H_______________________

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "KKTemplateCodeProject.h"

#define OTA_UPGRADE_CONTINUE_MAX_TIME_MS   (20*60*1000)

#define OTA_GPIO_RESTORE_MAXN	8


#define OTA_STORE_GPIO_STATUS_START_FALSH_ADDDR	\
	(EMBER_AF_PLUGIN_OTA_STORAGE_SIMPLE_EEPROM_STORAGE_START - (EMBER_AF_PLUGIN_NVM3_FLASH_PAGES * FLASH_PAGE_SIZE) - FLASH_PAGE_SIZE)

typedef enum{OTA_NORMAL=0, OTA_START,OTA_DOWNLOAD_DONE, OTA_VERITY_SUCCEED, OTA_FAILED}OTAStatusEnum;
typedef enum{OTA_GPIO_OFF=0, OAT_GPIO_ON}OTAGpioEnum;

#pragma pack(1)  //按4字节对齐
  typedef struct {
  	GPIO_Port_TypeDef port;
  	unsigned int pin;
  	bool bStatus; // on/off
  }OTAGpioListSt;
#pragma pack()    //取消自定义字节对齐方式

#pragma pack(1)  //按1字节对齐
  typedef struct {
	bool bIsValid;
  	uint8_t gpioNum;
  	OTAGpioListSt gpio[OTA_GPIO_RESTORE_MAXN];
  }OTAGpioStatusST;
#pragma pack()    //取消自定义字节对齐方式


//OTA操作函数，一般操作用来操作led
typedef void (*pOTACallback)(OTAStatusEnum status);

//获取当前产测状态
OTAStatusEnum kGetOTAStatus(void);
void kSetOTAStatus(OTAStatusEnum status);

void kOTAMoudleInit(pOTACallback callback, uint8_t led_id);

void kOTAMoudleCallback(OTAStatusEnum status);
#if 0
void KOTAMoudleKeepGpioStatus(uint8_t num, OTAGpioListSt gpioLisSt[]);

void kStoreGpioStatus(void);

void kRestoreGpioStatus(void);

void kClearStoredGpioStatus(void);
#endif
#endif

