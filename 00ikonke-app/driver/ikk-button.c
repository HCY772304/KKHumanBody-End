#include "stdint.h"
#include "stdbool.h"
#include "stack/include/ember-types.h"
#include "em_gpio.h"
//#include "generic-interrupt-control/generic-interrupt-control-efr32.h"
//#include "generic-interrupt-control/generic-interrupt-control.h"
//#include "emdrv/gpiointerrupt/inc/gpiointerrupt.h"
//#include "base/hal/micro/system-timer.h"

#include "../general/ikk-tick-handler.h"
#include "ikk-button.h"
#include "util/plugin/plugin-common/generic-interrupt-control/generic-interrupt-control.h"
#include "platform\emdrv\gpiointerrupt\inc\gpiointerrupt.h"

#define BTN_UNUSED_ID						BTN_UNKNOW_ID
#define BTN_UNKNOW_INTNO					0XFF
//modify by maozj 20200303
#define BTN_CALLBACK_MAXN					(3)

#define SYS_BUTTON_DEFAULT_INDEX			0  //system key defalut index add by maozj 20200313
#define INVALID_TIME_BETWEEN_SYS_BTN_MS		300
#define SYS_BTN_FILTER_TIME_MS				100

// Button jitter timeslot
#define BUTTON_JITTER_SLOT					5	// MS
// Double click timeslot.
#define DCLICK_SLOT_DEF						500	// MS

typedef enum { EBS_UNKNOW = 0, EBS_RELEASED, EBS_PRESSED }BtnStatusEnum;
//Ĭ������0Ϊϵͳ����
//add by maozj 20200313 repair short press other key will effect system key
static uint8_t g_u8TimeMS = 0;
static uint8_t g_u8BtnNum = 0;

static BtnConfSt g_stBtnConfList[BTN_SUPPORT_MAXN]  = { 0 };
// button interrupt mapping table
static uint8_t g_u8BtnIrqMapp[BTN_SUPPORT_MAXN] = { 0 };

// CALLBACs
pBtnActionCallback g_pBtnActionCallback[BTN_CALLBACK_MAXN] = NULL;


typedef struct {
	BtnActionEnum eAction;	// aciton enum
	uint64_t u64ActStamp;	// action occur timestamp
	uint32_t u64ActDuration;// press duration
}ActionRecordSt;

// button click detection controller
typedef struct {
	ActionRecordSt sOldAct;
	ActionRecordSt sNewAct;

	uint64_t stamp;
	BtnStatusEnum status;
	BtnStatusEnum lastStatus;
	bool bValidAction;
	bool bLongPressed;//add by maozj 20200226
	uint32_t u32BtnPressedTimeCount;

	uint32_t u32CountDownCounter;
	bool bStartDect;
}ClickDetectCtrllerSt;

static ClickDetectCtrllerSt g_stClickDetectCtrller[BTN_SUPPORT_MAXN] = {0};
//add by maozj
#define BTN_PRESSED_POLL_TIME_MS	(1000)

/*	DESP: the button[index0] active callback interface.
 * 	Auth: dingmz_frc.20191107.
 * */
static uint8_t kBtnGetButtonIndexByIrqNO(uint8_t irq_no )
{
	for(int index = 0; index < BTN_SUPPORT_MAXN; ++index ) {
		if( g_stBtnConfList[index].u8ButtonId != BTN_UNUSED_ID ) {
			if( g_u8BtnIrqMapp[index] == irq_no ) {
				return index;
			}
		}else {
			break;
		}
	}

	return 0XFF;
}

/*	DESP: the button[index0] active callback interface.
 * 	Auth: maozj.20200325.
 * */
uint8_t kBtnGetButtonIndexById(uint8_t button_id )
{
	for(int index = 0; index < BTN_SUPPORT_MAXN; ++index ) {
		if( g_stBtnConfList[index].u8ButtonId == button_id ) {
			return index;
		}
	}

	return 0XFF;
}

/*	DESP: Get Button Current Status By ID .
 * 	Auth: maozj.20200413.
 * */
uint8_t kBtnGetButtonCurrentStatusById(uint8_t button_id)
{
	for(int index = 0; index < BTN_SUPPORT_MAXN; ++index ) {
		if( g_stBtnConfList[index].u8ButtonId == button_id ) {
			return GPIO_PinInGet(g_stBtnConfList[index].port, g_stBtnConfList[index].pin);
		}
	}

	return 0XFF;
}


/*	DESP: Check whether all keys have stopped action detection.
 * 	Auth: dingmz_frc.20191107.
 * */
bool kBtnModuleActionIsGoing(void )
{
	for(int index = 0; index < BTN_SUPPORT_MAXN; ++index ) {
		if( g_stBtnConfList[index].u8ButtonId != BTN_UNUSED_ID ) {
			if( g_stClickDetectCtrller[index].u32CountDownCounter > 0 ) {
				return true;
			}
		}else {
			break;
		}
	}

	return false;
}

/*	DESP: Register button action callback notification function.
 * 	Auth: dingmz_frc.20191107.
 * */
kk_err_t kBtnaIRQCallbackRegister(pBtnActionCallback callback )
{
	if( NULL == callback ) {
		return KET_ERR_INVALID_PARAM;
	}

	int valid_index = BTN_CALLBACK_MAXN;

	for(int index = 0; index < BTN_CALLBACK_MAXN; ++index ) {
		if( g_pBtnActionCallback[index] == callback ) {
			valid_index = index;
			break;
		}else if((g_pBtnActionCallback[index] == NULL) && (valid_index == BTN_CALLBACK_MAXN)) {
			valid_index = index;
		}
	}

	if( valid_index < BTN_CALLBACK_MAXN ) {
		g_pBtnActionCallback[valid_index] = callback;
		return KET_OK;
	}

	return KET_ERR_INSUFFICIENT_SPACE;
}

/*	DESP: Recall all registered buttons action callback interface.
 * 	Auth: dingmz_frc.20191107.
 * */
void kBtnaIRQCallbackRecall(unsigned char btnIndex, BtnActionEnum action )
{
	for(int index = 0; index < BTN_CALLBACK_MAXN; ++index ) {
		if( g_pBtnActionCallback[index] != NULL ) {
			g_pBtnActionCallback[index](btnIndex, action);
		}else {
			break;
		}
	}
}

/*	DESP: Button module action detection task callback interface.
 * 	Auth: dingmz_frc.20191107.
 * */
void kBtnModuleActionDetectCallback(void )
{
	static uint64_t u64RecordTimeMS = 0;
	uint64_t u64CurrentTimeMS =0;
	static BtnStatusEnum eBtnStatusBuf[BTN_SUPPORT_MAXN];
	static bool bDclickDetectWaitStatus[BTN_SUPPORT_MAXN];

	if( kBtnModuleActionIsGoing() == false ) { // enable detection.
		//add by maozj 20200313
		for(int index = 0; index < BTN_SUPPORT_MAXN; ++index ) {
			g_stClickDetectCtrller[index].bStartDect = false;
			g_stClickDetectCtrller[index].bLongPressed = false;
			g_stClickDetectCtrller[index].u32BtnPressedTimeCount = 0;//add by maozj 20200414
			bDclickDetectWaitStatus[index] = false;
		}
		return ;
	}

	for(int index = 0; index < BTN_SUPPORT_MAXN; ++index ) {

		if( g_stBtnConfList[index].u8ButtonId == BTN_UNUSED_ID || g_stClickDetectCtrller[index].u32CountDownCounter  == 0) {
			continue ;
		}

		if ((g_stClickDetectCtrller[index].u32CountDownCounter > 0) && (g_stClickDetectCtrller[index].status == EBS_PRESSED || bDclickDetectWaitStatus[index])){
			--g_stClickDetectCtrller[index].u32CountDownCounter;
		}

		if((g_stClickDetectCtrller[index].u32CountDownCounter > 0) && (g_stClickDetectCtrller[index].status == EBS_PRESSED)) {

//			--g_stClickDetectCtrller[index].u32CountDownCounter;

			// check button status
			unsigned int gpio_st = GPIO_PinInGet(g_stBtnConfList[index].port, g_stBtnConfList[index].pin);

			eBtnStatusBuf[index] = (g_stBtnConfList[index].eActionPolarity == EBP_HIGH)
					?((gpio_st == 1)?EBS_PRESSED:EBS_RELEASED)
					:((gpio_st == 0)?EBS_PRESSED:EBS_RELEASED);
			//filter�˲�
			if (g_stClickDetectCtrller[index].bStartDect != true && eBtnStatusBuf[index] == EBS_PRESSED ){
				g_stClickDetectCtrller[index].bStartDect = true;
				uint32_t u32CurrentTimeMS = halCommonGetInt32uMillisecondTick();
				iKonkeAfSelfPrint("\r\n##########BTN 	g_stClickDetectCtrller[%d].bStartDect = true time(%d)\r\n", index, u32CurrentTimeMS);
				//add by maozj 20200415
				kBtnaIRQCallbackRecall(g_stBtnConfList[index].u8ButtonId, EBA_PRESSED);
				continue;
			}
			u64CurrentTimeMS = halCommonGetInt64uMillisecondTick();
			uint64_t time_slot = u64CurrentTimeMS - g_stClickDetectCtrller[index].stamp;

			//iKonkeAfSelfPrint("BB: button_st(%d), time_slot(%d)\r\n", button_st, time_slot);

			if( eBtnStatusBuf[index] == EBS_PRESSED) { // BUTTON PRESSED
				// whether long press time is reached!!
				if( time_slot >= g_stBtnConfList[index].u32LongPressDuration && g_stClickDetectCtrller[index].bLongPressed != true ) {
					// record aciton
					memcpy(&g_stClickDetectCtrller[index].sOldAct, &g_stClickDetectCtrller[index].sNewAct, sizeof(ActionRecordSt));

					g_stClickDetectCtrller[index].sNewAct.eAction 		= EBA_LONGPRESS;
					g_stClickDetectCtrller[index].sNewAct.u64ActStamp 	= halCommonGetInt64uMillisecondTick();
					g_stClickDetectCtrller[index].sNewAct.u64ActDuration= time_slot;

					g_stClickDetectCtrller[index].bValidAction = true;
					g_stClickDetectCtrller[index].u32CountDownCounter = 0;
					//delete by maozj202020227
					//button_st = EBS_RELEASED;
					//virtual release button add by maozj 20200227
					g_stClickDetectCtrller[index].bLongPressed = true;
					iKonkeAfSelfPrint("#########Long Click\r\n");
				}else if (g_stClickDetectCtrller[index].bLongPressed == true && g_stBtnConfList[index].bSupportPressedSend == true){
					//interval poll button pressed message,must keep not 0 add by maozj
					g_stClickDetectCtrller[index].u32CountDownCounter = (g_stBtnConfList[index].u32LongPressDuration + 1000) / TICK_LOOP_NMS;

				}
				//add by maozj
				if (g_stBtnConfList[index].bSupportPressedSend == true && g_stClickDetectCtrller[index].u32BtnPressedTimeCount++ >= BTN_PRESSED_POLL_TIME_MS / TICK_LOOP_NMS){
					g_stClickDetectCtrller[index].u32BtnPressedTimeCount = 0;
					//interval 1S poll button pressed message
					//add by maozj 20200226
					kBtnaIRQCallbackRecall(g_stBtnConfList[index].u8ButtonId, EBA_PRESSED);
				}
				g_stClickDetectCtrller[index].lastStatus = EBS_PRESSED;

				//add by maozj 20200313
				uint64_t u64DiffTimeMS = u64CurrentTimeMS - u64RecordTimeMS;
				if (u64DiffTimeMS < INVALID_TIME_BETWEEN_SYS_BTN_MS && g_u8BtnNum  > 1 && index == SYS_BUTTON_DEFAULT_INDEX){
					g_u8TimeMS = SYS_BTN_FILTER_TIME_MS;
					iKonkeAfSelfPrint("\r\n/*****BTN_REGISTER: SYS_BTN_FILTER_TIME_MS index(%d)*******************/\r\n", index);
				}else {
					//iKonkeAfSelfPrint("\r\n/*****BTN_REGISTER:  000000 index(%d)*******************/\r\n", index);
					g_u8TimeMS = 0;
				}

			}else if( eBtnStatusBuf[index] == EBS_RELEASED/* && g_stClickDetectCtrller[index].lastStatus == EBS_PRESSED*/) { // BUTTON RELEASED
				//bool bDClickDetect = false; //add by maozj 20200414 repair will sent released message firstly when double key detect
				// detect click, dclick or longpress?
				if( time_slot > (BUTTON_JITTER_SLOT + g_u8TimeMS)) {
					// record action
					memcpy(&g_stClickDetectCtrller[index].sOldAct, &g_stClickDetectCtrller[index].sNewAct, sizeof(ActionRecordSt));
					g_stClickDetectCtrller[index].sNewAct.u64ActStamp 	= halCommonGetInt64uMillisecondTick();
					g_stClickDetectCtrller[index].sNewAct.u64ActDuration = time_slot;
					g_stClickDetectCtrller[index].u32CountDownCounter = 0;

					// click or dclick
					if( time_slot < g_stBtnConfList[index].u32LongPressDuration && g_stClickDetectCtrller[index].bLongPressed != true ) {
						// If it is less than double-click detection time slot, it is necessary to determine whether it is double-click based on the previous action!!
						uint64_t detslot = g_stClickDetectCtrller[index].sNewAct.u64ActStamp - g_stClickDetectCtrller[index].sOldAct.u64ActStamp;

						if(	g_stBtnConfList[index].bSupportDClick) {
							if ((detslot < DCLICK_SLOT_DEF) && (g_stClickDetectCtrller[index].sNewAct.u64ActDuration < DCLICK_SLOT_DEF) \
								&& (g_stClickDetectCtrller[index].sOldAct.eAction == EBA_CLICK) \
								&& (g_stClickDetectCtrller[index].sOldAct.u64ActDuration < DCLICK_SLOT_DEF)) {
								g_stClickDetectCtrller[index].sOldAct.eAction = EBA_IDLE;
								g_stClickDetectCtrller[index].sNewAct.eAction = EBA_DCLICK;
								g_stClickDetectCtrller[index].bValidAction = true;//add by maozj 20200325
								g_stClickDetectCtrller[index].u32CountDownCounter = 0;//add by maozj 20200416
								bDclickDetectWaitStatus[index] = false;
								iKonkeAfSelfPrint("#########Double Click\r\n");
							}else {
								// detect next dlick action!
								if(	!bDclickDetectWaitStatus[index] \
									/*&& (g_stClickDetectCtrller[index].sNewAct.u64ActDuration < DCLICK_SLOT_DEF)*/ ) {
									g_stClickDetectCtrller[index].sNewAct.eAction = EBA_CLICK;
									g_stClickDetectCtrller[index].bValidAction = true;
									bDclickDetectWaitStatus[index] = true;
									//wait time slot timeout, can judge whether  action is single click
									g_stClickDetectCtrller[index].u32CountDownCounter = DCLICK_SLOT_DEF / TICK_LOOP_NMS;
									iKonkeAfSelfPrint("###Single Click Waiting When Support Double Click\r\n");
								}
							}
						}else {
							//modify by maozj 20200325 avoid send click event after sent longpressed
							if (g_stClickDetectCtrller[index].bLongPressed != true){
								g_stClickDetectCtrller[index].sNewAct.eAction = EBA_CLICK;
								g_stClickDetectCtrller[index].bValidAction = true;
								g_stClickDetectCtrller[index].u32CountDownCounter = 0;
								bDclickDetectWaitStatus[index] = true;
								iKonkeAfSelfPrint("#########Single Click\r\n");
							}
						}

//						g_stClickDetectCtrller[index].bValidAction = true;
					}else {
						//delete by maozj202020227
						iKonkeAfSelfPrint("\r\nBTN 	g_stClickDetectCtrller[index].u32CountDownCounter = 0;\r\n");
						g_stClickDetectCtrller[index].bValidAction = false;
						g_stClickDetectCtrller[index].u32CountDownCounter = 0;
					}
				}

				if (g_stClickDetectCtrller[index].bStartDect ){

					//add by maozj 20200226
					kBtnaIRQCallbackRecall(g_stBtnConfList[index].u8ButtonId, EBA_RELEASED);
					g_stClickDetectCtrller[index].bLongPressed = false;
					//g_stClickDetectCtrller[index].u32CountDownCounter = 0;
					g_stClickDetectCtrller[index].lastStatus = EBS_RELEASED;
					iKonkeAfSelfPrint("\r\n/*****EBA_RELEASED g_stClickDetectCtrller[%d].bStartDect = %d valid = %d****/\r\n\r\n",\
							index, g_stClickDetectCtrller[index].bStartDect, g_stClickDetectCtrller[index].bValidAction );
				}
			}
			//record current button status
			g_stClickDetectCtrller[index].status = eBtnStatusBuf[index];
		}//else

		//sent message for single click or long pressed
		if( g_stClickDetectCtrller[index].u32CountDownCounter == 0 ) { // detection over!!

			if( g_stClickDetectCtrller[index].bValidAction && g_stClickDetectCtrller[index].bStartDect) {
				//g_stClickDetectCtrller[index].bValidAction = false;
				kBtnaIRQCallbackRecall(g_stBtnConfList[index].u8ButtonId, g_stClickDetectCtrller[index].sNewAct.eAction);
				if (g_stClickDetectCtrller[index].bLongPressed != true){
					g_stClickDetectCtrller[index].bStartDect = false;
				}
				g_stClickDetectCtrller[index].bValidAction = false;

				//when key short press except system key,increase time slot
				if (g_stClickDetectCtrller[index].sNewAct.eAction == EBA_CLICK && g_u8BtnNum  > 1 && index != SYS_BUTTON_DEFAULT_INDEX){
					u64RecordTimeMS = halCommonGetInt64uMillisecondTick();
				}
			}
			//add by maozj 20200226
			if (g_stClickDetectCtrller[index].bLongPressed == true && g_stBtnConfList[index].bSupportPressedSend == true){
				//must be not 0,because need to poll button pressing status.
				g_stClickDetectCtrller[index].u32CountDownCounter = (g_stBtnConfList[index].u32LongPressDuration + 1000) / TICK_LOOP_NMS;
				g_stClickDetectCtrller[index].bValidAction = false;
			}else if (g_stClickDetectCtrller[index].bLongPressed == true && g_stBtnConfList[index].bSupportPressedSend != true){
				//sleep device
				g_stClickDetectCtrller[index].bStartDect = false;
				g_stClickDetectCtrller[index].bLongPressed = false;
				g_stClickDetectCtrller[index].u32BtnPressedTimeCount = 0;//add by maozj 20200414
			}
		}
	}
}

/*	DESP: the button[index0] active callback interface.
 * 	Auth: dingmz_frc.20191107.
 * */
static void kBtnaIRQCallback(uint8_t irq_no )
{
	uint8_t button_index = kBtnGetButtonIndexByIrqNO(irq_no);
	static bool flg = false;
	static BtnConfSt stBtnConfList[BTN_SUPPORT_MAXN]  = { 0 };

	if( button_index != 0XFF ) {
		//iKonkeAfSelfPrint("BUTTON Actived[%d]: ButtonID(%d)!\r\n", button_index, g_stBtnConfList[button_index].u8ButtonId);

		// trigger running button click detection.
		g_stClickDetectCtrller[button_index].status = EBS_PRESSED;
		g_stClickDetectCtrller[button_index].stamp  = halCommonGetInt64uMillisecondTick();
		//g_stClickDetectCtrller[button_index].bValidAction = false;
		//g_stClickDetectCtrller[button_index].bLongPressed = false;//add by maozj 20200226
		g_stClickDetectCtrller[button_index].u32BtnPressedTimeCount = 0; //add by maozj 20200226
		g_stClickDetectCtrller[button_index].u32CountDownCounter
			= (g_stBtnConfList[button_index].u32LongPressDuration + 1000) / TICK_LOOP_NMS;
		//g_stClickDetectCtrller[button_index].bStartDect = false;
		g_stClickDetectCtrller[button_index].lastStatus = EBS_UNKNOW;
		if (flg != true){
			flg = true;
			memcpy(stBtnConfList, g_stBtnConfList, sizeof(stBtnConfList));
		}

		//change irq  ege
		if (stBtnConfList[button_index].eActionPolarity == EBP_BOTH){
			bool status = GPIO_PinInGet(g_stBtnConfList[button_index].port, g_stBtnConfList[button_index].pin);
			g_stBtnConfList[button_index].eActionPolarity  = status?EBP_HIGH:EBP_LOW;
		}
		kTickRunnningTrigger();
		//add by maozj 20200919 for sleep device wake up  recv data uart event
		if (g_stBtnConfList[button_index].bTriggerUartRecvEvent == true) {
			kUartActiveTrigger();
		}
	}
}

/*	DESP: Assign an unused interrupt number.
 * 	Auth: dingmz_frc.20191108.
 * */
uint8_t kBtnAssignIRQNO(uint8_t btn_index, unsigned int pin )
{
#define GENERAL_PIN_MAXN	(GPIO_PIN_MAX + 1)
#define INT_SECTION_LENS	(4)

	// Auto assign the interrupt number, warning, interrupt number cannot be repeated!!
	// section 0: INT_NO[0 , 3] for PIN[0 , 3]
	// section 1: INT_NO[4 , 7] for PIN[4 , 7]
	// section 2: INT_NO[8 ,11] for PIN[8 ,11]
	// section 3: INT_NO[12,15] for PIN[12,15]
	const uint8_t irq_section[GENERAL_PIN_MAXN] = { 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3 };
	uint8_t irq_no_used[GENERAL_PIN_MAXN] = { 0 };

	if (btn_index >= BTN_SUPPORT_MAXN){
//		iKonkeAfSelfPrint("BTN Index is not valid\r\n");
		return BTN_UNKNOW_INTNO;
	}
	//have exist interrupt number
	if (g_u8BtnIrqMapp[btn_index] != BTN_UNKNOW_INTNO){
//		iKonkeAfSelfPrint("BTN Index is existed\r\n");
		return g_u8BtnIrqMapp[btn_index];
	}

	g_u8BtnIrqMapp[btn_index] = BTN_UNKNOW_INTNO;

	// Auto assign the interrupt number, warning, interrupt number cannot be repeated!!
	for(int index = 0; index < BTN_SUPPORT_MAXN; ++index ) {
		if( g_u8BtnIrqMapp[index] != BTN_UNKNOW_INTNO ) {
			//1 is used
			irq_no_used[g_u8BtnIrqMapp[index]] = 1;
		}
	}

	if( irq_no_used[pin] == 1 ) {
		uint8_t section_id = irq_section[pin], index_j = 0;

		for( ; index_j < INT_SECTION_LENS; ++index_j ) {
			if( irq_no_used[section_id * INT_SECTION_LENS + index_j] == 0 ) {
				g_u8BtnIrqMapp[btn_index] = section_id * INT_SECTION_LENS + index_j;
				break;
			}
		}
	}else {
		g_u8BtnIrqMapp[btn_index] = pin;
	}

	return g_u8BtnIrqMapp[btn_index];
}

/*	DESP:Get usefull assign irq index for other interrupt gpio include button
 * 	Auth: maozj.20200325.
 * */
uint8_t kBtnGetAssignUsefullIndex(void)
{
	for(int index = 0; index < BTN_SUPPORT_MAXN; ++index ) {
		if( g_u8BtnIrqMapp[index] == BTN_UNKNOW_INTNO ) {
			return index;
		}
	}
	return 0xFF;
}

/*	DESP: single Button instance register interface.
 * 	Auth: dingmz_frc.20191108. modify by maozj 20200326
 * */
kk_err_t kBtnRegister(uint8_t id, GPIO_Port_TypeDef port, unsigned int pin, BtnPolarityEnum ePolarity, bool bSupportDClick, bool bSupportPressedSend, bool bTriggerUartRecvEvent,  uint32_t u32LongPressDuration )
{
	if( BTN_UNKNOW_ID == id || BTN_ALLOPT_ID == id ) {
		return KET_ERR_INVALID_PARAM;
	}

	int valid_index = BTN_SUPPORT_MAXN;
	kk_err_t err = KET_OK;

	for(int index = 0; index < BTN_SUPPORT_MAXN; ++index ) {
		if( g_stBtnConfList[index].u8ButtonId == id ) {
			valid_index = index;
			break;
		}else if((valid_index == BTN_SUPPORT_MAXN) && (g_stBtnConfList[index].u8ButtonId == BTN_UNUSED_ID)) {
			valid_index = index;
		}
	}

	if( valid_index < BTN_SUPPORT_MAXN ) { // GET A VALID CHUNK.



		// try to assign interrupt number.
		g_u8BtnIrqMapp[valid_index] = kBtnAssignIRQNO(valid_index, pin);

		if( g_u8BtnIrqMapp[valid_index] != BTN_UNKNOW_INTNO ) {
			memset(&g_stClickDetectCtrller[valid_index], 0, sizeof(g_stClickDetectCtrller[valid_index]));

			g_stBtnConfList[valid_index].u8ButtonId = id;
			g_stBtnConfList[valid_index].port = port;
			g_stBtnConfList[valid_index].pin  = pin;
			g_stBtnConfList[valid_index].eActionPolarity = ePolarity;
			g_stBtnConfList[valid_index].bSupportDClick  = bSupportDClick;
			g_stBtnConfList[valid_index].bSupportPressedSend = bSupportPressedSend;//add by maozj 20200326
			g_stBtnConfList[valid_index].bTriggerUartRecvEvent = bSupportPressedSend;//add by maozj 20200919
			g_stBtnConfList[valid_index].u32LongPressDuration = u32LongPressDuration;

//			iKonkeAfSelfPrint("BTN_REGISTER: id(%d), port(%d), pin(%d), intno(%d), eActionPolarity(%d), bDclick(%d), longSlot(%d)\r\n"
//				, g_stBtnConfList[valid_index].u8ButtonId, g_stBtnConfList[valid_index].port, g_stBtnConfList[valid_index].pin, g_u8BtnIrqMapp[valid_index]
//				, g_stBtnConfList[valid_index].eActionPolarity, g_stBtnConfList[valid_index].bSupportDClick
//				, g_stBtnConfList[valid_index].u32LongPressDuration
//				);

			#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
			GPIO_Mode_TypeDef mode = gpioModeInput;
			#else
			GPIO_Mode_TypeDef mode = gpioModeInputPull;
			//GPIO_Mode_TypeDef mode = gpioModeInput;
			#endif
			GPIO_PinModeSet(g_stBtnConfList[valid_index].port, g_stBtnConfList[valid_index].pin, mode
					, (g_stBtnConfList[valid_index].eActionPolarity == EBP_HIGH)?0:1);
			/* Register callbacks before setting up and enabling pin interrupt. */
			GPIOINT_CallbackRegister(g_u8BtnIrqMapp[valid_index], kBtnaIRQCallback);

			if( g_stBtnConfList[valid_index].eActionPolarity == EBP_HIGH ) {
				/* Set rising and falling edge interrupts */
				GPIO_ExtIntConfig(g_stBtnConfList[valid_index].port, g_stBtnConfList[valid_index].pin, g_u8BtnIrqMapp[valid_index]
					, true, false, true);
			}else  if (g_stBtnConfList[valid_index].eActionPolarity == EBP_LOW) {
				/* Set rising and falling edge interrupts */
				GPIO_ExtIntConfig(g_stBtnConfList[valid_index].port, g_stBtnConfList[valid_index].pin, g_u8BtnIrqMapp[valid_index]
					, false, true, true);
			}else {
				GPIO_ExtIntConfig(g_stBtnConfList[valid_index].port, g_stBtnConfList[valid_index].pin, g_u8BtnIrqMapp[valid_index]
					, true, true, true);
			}
		}else {
			err = KET_ERR_NO_PERMISSIONS;
		}
	}else {
		err = KET_ERR_INSUFFICIENT_SPACE;
	}

	return err;
}

/*	DESP: button module Initialize configuration interface.
 * 	Note: Button initialization interface supports one-time registration of multiple buttons!
 * 	Auth: dingmz_frc.20191107.
 * */
kk_err_t kBtnModuleInit(BtnConfSt *btnlist, unsigned char btn_number, pBtnActionCallback pBtnActionCallback )
{
	kk_err_t err = KET_OK;

	/* Initialize GPIO interrupt dispatcher */
	GPIOINT_Init();
	/* Enable GPIO in CMU */
	#if !defined(_SILICON_LABS_32B_SERIES_2)
	CMU_ClockEnable(cmuClock_HFPER, true);
	CMU_ClockEnable(cmuClock_GPIO, true);
	#endif

	memset(g_u8BtnIrqMapp, BTN_UNKNOW_INTNO, sizeof(g_u8BtnIrqMapp));
	memset(g_stBtnConfList, BTN_UNUSED_ID, sizeof(g_stBtnConfList));

	if( btnlist && (btn_number > 0)) {
		btn_number = (btn_number < BTN_SUPPORT_MAXN)?btn_number:BTN_SUPPORT_MAXN;

		for(int index = 0; index < btn_number; ++index ) {
			err = kBtnRegister(btnlist[index].u8ButtonId, btnlist[index].port, btnlist[index].pin, btnlist[index].eActionPolarity
				, btnlist[index].bSupportDClick, btnlist[index].bSupportPressedSend, btnlist[index].bTriggerUartRecvEvent, btnlist[index].u32LongPressDuration);
		}

		kBtnaIRQCallbackRegister(pBtnActionCallback);
	}
	g_u8BtnNum = btn_number;
	return err;
}
#if 0
/*	DESP: get button information by button id
 * 	Auth: maozj.20191123.
 * */
kk_err_t kBtnGetInfoByButtonId(uint8_t id, BtnConfSt *btnConf)
{
	if( BTN_UNKNOW_ID == id || BTN_ALLOPT_ID == id ) {
		return KET_ERR_INVALID_PARAM;
	}

	int valid_index = BTN_SUPPORT_MAXN;
	kk_err_t err = KET_OK;

	for(int index = 0; index < BTN_SUPPORT_MAXN; ++index ) {
		if( g_stBtnConfList[index].u8ButtonId == id ) {
			valid_index = index;
			break;
		}
	}

	if( valid_index < BTN_SUPPORT_MAXN ) { // GET A VALID CHUNK.
		memcoy(btnConf, &g_stBtnConfList[valid_index], sizeof(btnConf));
	}else {
		err = KET_ERR_INSUFFICIENT_SPACE;
	}
	return kk_err_t;
}
#endif
