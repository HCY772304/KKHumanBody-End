#ifndef __IKONKE_MODULE_BUTTON_H_______________________________
#define __IKONKE_MODULE_BUTTON_H_______________________________

#include "app/framework/include/af.h"
#include "../general/ikk-module-def.h"
#include "../general/ikk-debug.h"


// Maximum number of buttons supported!
#define BTN_SUPPORT_MAXN			8

#define BTN_UNKNOW_ID				0	// Valid[1,254], 255 for Operate all at the same time!
#define BTN_ALLOPT_ID				255	// Valid[1,254], 255 for Operate all at the same time!




typedef enum { EBP_LOW = 0, EBP_HIGH, EBP_BOTH }BtnPolarityEnum;

typedef enum { EBA_IDLE = 0, EBA_CLICK, EBA_DCLICK, EBA_LONGPRESS, EBA_PRESSED, EBA_RELEASED}BtnActionEnum;

typedef void (*pBtnActionCallback)(unsigned char btnIndex, BtnActionEnum action );

typedef struct tag_button_configeration {
	unsigned char u8ButtonId;

	GPIO_Port_TypeDef port;
	unsigned int pin;

	BtnPolarityEnum eActionPolarity;
	bool bSupportDClick;
	bool bSupportPressedSend; //true:key always long pressed will send pressed message interval 1 second.used to sensor
	bool bTriggerUartRecvEvent; //true:interrupt coming to trigger uart receive event
	unsigned int u32LongPressDuration;	// MS
}BtnConfSt;

/*	DESP: Assign an unused interrupt number.
 * 	Auth: dingmz_frc.20191108.
 * */
uint8_t kBtnAssignIRQNO(uint8_t btn_index, unsigned int pin );

/*	DESP:Get usefull assign irq index for other interrupt gpio include button
 * 	Auth: maozj.20200325.
 * */
uint8_t kBtnGetAssignUsefullIndex(void);

/*	DESP: single Button instance register interface.
 * 	Auth: dingmz_frc.20191108. modify by maozj 20200326
 * */
kk_err_t kBtnRegister(uint8_t id, GPIO_Port_TypeDef port, unsigned int pin, BtnPolarityEnum ePolarity, bool bSupportDClick, bool bSupportPressedSend, bool bTriggerUartRecvEvent,  uint32_t u32LongPressDuration );
/*	DESP: button module Initialize configuration interface.
 * 	Note: Button initialization interface supports one-time registration of multiple buttons!
 * 	Auth: dingmz_frc.20191107.
 * */
kk_err_t kBtnModuleInit(BtnConfSt *btnlist, unsigned char btn_number, pBtnActionCallback pBtnActionCallback );

/*	DESP: get button information by button id
 * 	Auth: maozj.20191123.
 * */
kk_err_t kBtnGetInfoByButtonId(uint8_t id, BtnConfSt *btnConf);
#endif

