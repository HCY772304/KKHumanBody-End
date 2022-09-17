//#include "app/framework/plugin/network-steering/network-steering.h"
//#include "app/framework/plugin/network-steering/network-steering-internal.h"
#include "ikk-ota.h"
#include "ikk-debug.h"
#include "../driver/ikk-led.h"


#define LED_SLOW_BLINK_ON_TIME_MS		800
#define LED_SLOW_BLINK_OFF_TIME_MS		800
#define LED_FAST_BLINK_ON_TIME_MS		200
#define LED_FAST_BLINK_OFF_TIME_MS		200

#define LED_SLOW_BLINK_CONTINUE_TIME_MS	(120 * 1000)
#define LED_FAST_BLINK_CONTINUE_TIME_MS	(5 * 1000)



static OTAStatusEnum g_eOTAStatus = OTA_NORMAL;
static uint8_t g_u8OtaLedId = LED_UNKNOW_ID;
pOTACallback g_pOTACallback = NULL;

static bool kOtaGetGpioStatus(GPIO_Port_TypeDef port, unsigned int pin);
static void kOTAMoudleOptCallback(OTAStatusEnum status);
static uint32_t FLASH_ReadWord(uint32_t _addr);
static void FLASH_ReadByte(uint32_t _addr, uint8_t *p, uint16_t Byte_Num);
//void KOTAMoudleKeepGpioStatus(uint8_t num, OTAGpioListSt gpioLisSt[]);


//__no_init OTAGpioStatusST g_stOTAGpioStatus @ 0x2000FFF0;

static OTAGpioStatusST g_stOTAGpioStatus;

OTAStatusEnum kGetOTAStatus(void)
{
	//flash_init();
	return g_eOTAStatus;
}


void kSetOTAStatus(OTAStatusEnum status)
{
	g_eOTAStatus = status;
}

void kOTAMoudleInit(pOTACallback callback, uint8_t led_id)
{
	g_pOTACallback = callback;
	g_u8OtaLedId = led_id;
}

static bool kOtaGetGpioStatus(GPIO_Port_TypeDef port, unsigned int pin)
{
	return GPIO_PinOutGet(port, pin);
}

void kOTAMoudleCallback(OTAStatusEnum status)
{

	//校验成功或失败保存gpio状态，待bootloader中恢复
	if (status == OTA_VERITY_SUCCEED || status == OTA_FAILED){
		iKonkeAfSelfPrint("OTA GPIO KEEP status(%d)\r\n", g_eOTAStatus);
#if 0
		kStoreGpioStatus();



		OTAGpioStatusST stOTAGpioStatus;

		FLASH_ReadByte(OTA_STORE_GPIO_STATUS_START_FALSH_ADDDR, (uint8_t *)&stOTAGpioStatus, sizeof(OTAGpioStatusST));
		iKonkeAfSelfPrint("OTA GPIO Read From Flash valid(%d) gpioNum(%d) gpio[0].port(%d) pin(%d) gpio[1].port(%d) pin(%d)\r\n", \
				stOTAGpioStatus.bIsValid, stOTAGpioStatus.gpioNum, stOTAGpioStatus.gpio[0].port, stOTAGpioStatus.gpio[0].pin,\
				stOTAGpioStatus.gpio[1].port, stOTAGpioStatus.gpio[1].pin);
#endif
	}

	if (g_pOTACallback){
		g_pOTACallback(status);
	}else {
		if (g_u8OtaLedId != LED_UNKNOW_ID){
			kOTAMoudleOptCallback(status);
		}
	}
}

static void kOTAMoudleOptCallback(OTAStatusEnum status)
{
	iKonkeAfSelfPrint("##############kOTAMoudleUserCallback status(%d)##################\r\n", status);
	static bool flg = false;//防止网关断电后,ota失败后会间隔快闪
	switch(status)
	{
		case OTA_NORMAL:
			break;
		case OTA_START:
			kLedOptTrigger(g_u8OtaLedId, 2 * LED_FAST_BLINK_ON_TIME_MS, 14 * LED_FAST_BLINK_ON_TIME_MS,\
					OTA_UPGRADE_CONTINUE_MAX_TIME_MS /(2 * LED_FAST_BLINK_ON_TIME_MS + 14 * LED_FAST_BLINK_ON_TIME_MS), LED_ON, LED_ON);
			g_eOTAStatus = OTA_START;
			flg = false;
			break;
		case OTA_DOWNLOAD_DONE:
		{
			uint8_t buffer[] = {"Konke download disable watchdog.....\r\n"};
		 	  //不能注释掉， 不然无法禁用看门狗，导致第二次连续升级卡死，认为这里是相当于加了延时
		  	emberSerialWriteData((uint8_t)APP_SERIAL, buffer, strlen((char *)buffer));
		  	halInternalDisableWatchDog(MICRO_DISABLE_WATCH_DOG_KEY);//add by maozj 20190308文件快下载好后禁用看门狗
		  	g_eOTAStatus = OTA_DOWNLOAD_DONE;
		  	break;
		}
		case OTA_VERITY_SUCCEED:
			//校验成功，记录继电器状态
			//saveRelayStatusWhenOtaSucceed(relayTotalNum);
//			extern uint8_t generatedDefaults[];
//			uint8_t u8RelayNum = 0;
//			if (generatedDefaults[33+16] == '1'){
//				u8RelayNum = 1;
//			}else if (generatedDefaults[33+16] == '2'){
//
//			}else if (generatedDefaults[33+16] == '3'){
//
//			}
//
//			g_stOTAGpioStatusBuffer.gpioNum = u8RelayNum;
			//常亮5S
			kLedOptTrigger(g_u8OtaLedId, 25 * LED_FAST_BLINK_ON_TIME_MS,  LED_FAST_BLINK_ON_TIME_MS, 1, LED_ON, LED_ON);
			g_eOTAStatus = OTA_NORMAL;
			break;
		case OTA_FAILED:
			if (flg != true){
				halInternalWatchDogEnabled();  //使能看门狗
				emberAfOtaStorageClearTempDataCallback();
				kLedOptTrigger(g_u8OtaLedId, LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_ON_TIME_MS,\
									LED_FAST_BLINK_CONTINUE_TIME_MS /(LED_FAST_BLINK_ON_TIME_MS + LED_FAST_BLINK_ON_TIME_MS), LED_ON, LED_ON);
				g_eOTAStatus = OTA_NORMAL;
			}
			flg = true;
			break;
		default:
			iKonkeAfSelfPrint("##############Err: OTA status is not exist\r\n");
			break;
	}
}
#if 0
void KOTAMoudleKeepGpioStatus(uint8_t num, OTAGpioListSt gpioLisSt[])
{
	if (gpioLisSt == NULL) {
		g_stOTAGpioStatus.gpioNum = 0;
		iKonkeAfSelfPrint("Err:OTA restore gpio set\r\n");
		return;
	}


	g_stOTAGpioStatus.gpioNum = (num >= OTA_GPIO_RESTORE_MAXN)?OTA_GPIO_RESTORE_MAXN : num;
	iKonkeAfSelfPrint("#OTA GPIO gpioNum(%d) , num(%d)\r\n", g_stOTAGpioStatus.gpioNum, num);
	memcpy(g_stOTAGpioStatus.gpio, gpioLisSt, g_stOTAGpioStatus.gpioNum * sizeof(OTAGpioListSt));

	iKonkeAfSelfPrint("OTA GPIO Set valid(%d) gpioNum(%d) gpio[0].port(%d) pin(%d) gpio[1].port(%d) pin(%d) addr(%d)\r\n", \
			g_stOTAGpioStatus.bIsValid, g_stOTAGpioStatus.gpioNum, g_stOTAGpioStatus.gpio[0].port, g_stOTAGpioStatus.gpio[0].pin,\
			g_stOTAGpioStatus.gpio[1].port, g_stOTAGpioStatus.gpio[1].pin, OTA_STORE_GPIO_STATUS_START_FALSH_ADDDR);
}

void kStoreGpioStatus(void)
{
	g_stOTAGpioStatus.bIsValid = true;
	for (uint8_t i = 0; i < g_stOTAGpioStatus.gpioNum; i++){
		if (g_stOTAGpioStatus.gpio[i].port != PORT_UNKNOW && g_stOTAGpioStatus.gpio[i].pin != PIN_UNKNOW){
			g_stOTAGpioStatus.gpio[i].bStatus = kOtaGetGpioStatus(g_stOTAGpioStatus.gpio[i].port , g_stOTAGpioStatus.gpio[i].pin);
		}
	}
	uint32_t buffer[] = {0x12345678, 0x11223344, 0x55667788, 0x1314151617, 0x23242526, 0x34353637, 0x45464748, 0x56575859};
	//flash_write_buf(OTA_STORE_GPIO_STATUS_START_FALSH_ADDDR, (uint32_t *)&g_stOTAGpioStatus, sizeof(OTAGpioStatusST));
	flash_write_buf(262144, (uint32_t *)buffer, 32);
	//uint32_t addr = 262144;
//	while (addr & 0x03)
//	{
//		addr++;
//	}
	//iKonkeAfSelfPrint("### 4 Align addr is (%d)\r\n", addr);
	//MSC_WriteWord(addr, (void *)&g_stOTAGpioStatus, sizeof(OTAGpioStatusST));

}

void kRestoreGpioStatus(void)
{
	OTAGpioStatusST stOTAGpioStatus;
	uint8_t buffer[64];
//	FLASH_ReadByte(OTA_STORE_GPIO_STATUS_START_FALSH_ADDDR, (uint8_t *)&stOTAGpioStatus, sizeof(OTAGpioStatusST));
	FLASH_ReadByte(262144, buffer, 64 );
	iKonkeAfSelfPrint("Get From Flash:");
	iKonkeAfSelfPrintBuffer(buffer, 64, true);
	iKonkeAfSelfPrint(" end \r\n");

	if (stOTAGpioStatus.bIsValid == true){
		for (uint8_t i = 0; i < stOTAGpioStatus.gpioNum; i++){
			if (stOTAGpioStatus.gpio[i].port != PORT_UNKNOW && stOTAGpioStatus.gpio[i].pin != PIN_UNKNOW){
			  GPIO_PinModeSet(stOTAGpioStatus.gpio[i].port, stOTAGpioStatus.gpio[i].pin, \
					gpioModePushPull, stOTAGpioStatus.gpio[i].bStatus);
			  if (stOTAGpioStatus.gpio[i].bStatus){
				  GPIO_PinOutSet(stOTAGpioStatus.gpio[i].port, stOTAGpioStatus.gpio[i].pin);
			  }else {
				  GPIO_PinOutClear(stOTAGpioStatus.gpio[i].port, stOTAGpioStatus.gpio[i].pin);
			  }
			}
		}
	}
}

void kClearStoredGpioStatus(void)
{
	g_stOTAGpioStatus.bIsValid = false;
//	for (uint8_t i = 0; i < OTA_GPIO_RESTORE_MAXN; i++){
//		g_stOTAGpioStatus.gpio[i].port = PORT_UNKNOW;
//		g_stOTAGpioStatus.gpio[i].pin = PIN_UNKNOW;
//	}
}

bool kIsRestoredGpioStatus(void)
{
	return g_stOTAGpioStatus.bIsValid;
}

//FLASH标志读取全字
static uint32_t FLASH_ReadWord(uint32_t _addr)
{
	return *(__IO uint32_t*)(_addr);
}

static void FLASH_ReadByte(uint32_t _addr , uint8_t *p , uint16_t Byte_Num)
{
	while(Byte_Num--)
	{
		*(p++)=*((uint8_t*)_addr++);
	}
}
#endif
