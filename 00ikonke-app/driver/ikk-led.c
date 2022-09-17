#include "stdint.h"
#include "stdbool.h"
#include "stack/include/ember-types.h"
#include "em_gpio.h"
//#include "generic-interrupt-control/generic-interrupt-control-efr32.h"
//#include "generic-interrupt-control/generic-interrupt-control.h"

#include "../general/ikk-tick-handler.h"
#include "../general/ikk-debug.h"
#include "ikk-led.h"
#include "ikk-pwm.h"

#define LED_UNUSED_ID		LED_UNKNOW_ID

typedef void (* LedCallHandler)(LedOptEnum eopt );

pLedActionDoneCallback g_pLedActionDoneCallback = NULL;
pLedActionHandleCallback g_pLedActionHandleCallback = NULL;
pLedActionDoneCallback g_pLedBlinkDoneCallback[LED_OBJS_SUPPORT_MAXN] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
pLedActionHandleCallback g_pLedBlinkHandelCallback[LED_OBJS_SUPPORT_MAXN] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

// led controller
static struct tag_led_controller_st {
	uint32_t onCounter;
	uint32_t onDuration;
	uint32_t offCounter;
	uint32_t offDuration;
	uint32_t blink_cnt;

	LedOptEnum start;
	LedOptEnum end;
}g_stLedCtrller[LED_OBJS_SUPPORT_MAXN] = { 0 };

static LedConfSt g_stLedConfList[LED_OBJS_SUPPORT_MAXN] = { 0 };


/*	DESP: get the led index by led_id.
 * 	Auth: dingmz_frc.20191107.
 * */
static uint8_t kLedGetLedIndexByID(uint8_t led_id )
{
	for(int led_index = 0; led_index < LED_OBJS_SUPPORT_MAXN; ++led_index ) {
		if( g_stLedConfList[led_index].u8LedId != LED_UNUSED_ID ) {
			if( g_stLedConfList[led_index].u8LedId == led_id ) {
				return led_index;
			}
		}
		else break;
	}

	return 0XFF;
}

/*	DESP: registration the system led driver processing function interface.
	Auth: dingmz_frc.20191108.
*/
void kLedDriverhandler(uint8_t led_index, LedOptEnum st )
{
	switch(st) {
		case (LED_OFF):
		{
			if( g_stLedConfList[led_index].eActionPolarity == ELP_HIGH ) {
				for(int index = 0; index < g_stLedConfList[led_index].u8GpioMappNum; ++index ) {
					/*begin modify by maozj 20200214*/
					if (g_stLedConfList[led_index].gpios[index].pwmId == PWM_UNKNOW_ID){
						GPIO_PinOutClear(g_stLedConfList[led_index].gpios[index].port
								, g_stLedConfList[led_index].gpios[index].pin);
					}else {
						uint8_t pwm_index = kPwmGetIndexByID(g_stLedConfList[led_index].gpios[index].pwmId);
						if (index != 0xFF){
							kPwmDriverhandler(pwm_index, ELP_MIN, 0);
						}else {
//							iKonkeAfSelfPrint("\r\n&&&kLedDriverhandler OFF HIGH led_index(%d),pwm_index(%d)�� pwm_id(%d)\r\n",\
//									led_index, pwm_index, g_stLedConfList[led_index].gpios[index].pwmId);
						}
					}
					/*end modify by maozj 20200214*/
				}

			}else {
				for(int index = 0; index < g_stLedConfList[led_index].u8GpioMappNum; ++index ) {
					/*begin modify by maozj 20200214*/
					if (g_stLedConfList[led_index].gpios[index].pwmId  == PWM_UNKNOW_ID){
						GPIO_PinOutSet(g_stLedConfList[led_index].gpios[index].port
								, g_stLedConfList[led_index].gpios[index].pin);
					}else {
						uint8_t pwm_index = kPwmGetIndexByID(g_stLedConfList[led_index].gpios[index].pwmId );
						if (index != 0xFF){
							kPwmDriverhandler(pwm_index, ELP_MAX, 0);
						}else {
//							iKonkeAfSelfPrint("\r\n&&&kLedDriverhandler OFF LOW led_index(%d),pwm_index(%d)�� pwm_id(%d)\r\n",\
//															led_index, pwm_index, g_stLedConfList[led_index].gpios[index].pwmId);
						}
					}
					/*end modify by maozj 20200214*/

				}
			}

			break;
		}
		case (LED_ON):
		{
			if( g_stLedConfList[led_index].eActionPolarity == ELP_HIGH ) {
				for(int index = 0; index < g_stLedConfList[led_index].u8GpioMappNum; ++index ) {
					/*begin modify by maozj 20200214*/
					if (g_stLedConfList[led_index].gpios[index].pwmId  == PWM_UNKNOW_ID){
						GPIO_PinOutSet(g_stLedConfList[led_index].gpios[index].port
								, g_stLedConfList[led_index].gpios[index].pin);
					}else {
						uint8_t pwm_index = kPwmGetIndexByID(g_stLedConfList[led_index].gpios[index].pwmId );
						if (index != 0xFF){
							kPwmDriverhandler(pwm_index, ELP_MAX, 0);
						}else {
//							iKonkeAfSelfPrint("\r\n&&&kLedDriverhandler ON HIGH led_index(%d),pwm_index(%d)�� pwm_id(%d)\r\n",\
//															led_index, pwm_index, g_stLedConfList[led_index].gpios[index].pwmId);
						}
					}
					/*end modify by maozj 20200214*/
				}
			}else {
				for(int index = 0; index < g_stLedConfList[led_index].u8GpioMappNum; ++index ) {
					/*begin modify by maozj 20200214*/
					if (g_stLedConfList[led_index].gpios[index].pwmId  == PWM_UNKNOW_ID){
						GPIO_PinOutClear(g_stLedConfList[led_index].gpios[index].port
								, g_stLedConfList[led_index].gpios[index].pin);
					}else {
						uint8_t pwm_index = kPwmGetIndexByID(g_stLedConfList[led_index].gpios[index].pwmId );
						if (index != 0xFF){
							kPwmDriverhandler(pwm_index, ELP_MIN, 0);
						}else {
//							iKonkeAfSelfPrint("\r\n&&&kLedDriverhandler ON LOW led_index(%d),pwm_index(%d)�� pwm_id(%d)\r\n",\
//															led_index, pwm_index, g_stLedConfList[led_index].gpios[index].pwmId);
						}
					}
					/*end modify by maozj 20200214*/

				}
			}
			break;
		}
		//add by mao 20200226
		case (LED_TOGGLE):
		{
			//not support pwm toggle
			for(int index = 0; index < g_stLedConfList[led_index].u8GpioMappNum; ++index ) {
				GPIO_PinOutToggle(g_stLedConfList[led_index].gpios[index].port
					, g_stLedConfList[led_index].gpios[index].pin);
			}
			break;
		}
		default: break;
	}
    if (g_pLedBlinkHandelCallback[led_index]){
        g_pLedBlinkHandelCallback[led_index](g_stLedConfList[led_index].u8LedId);
    }
}

/*	DESP: Check whether all leds have stopped action detection.
 * 	Auth: dingmz_frc.20191107.
 * */
bool kLedModuleActionIsGoing(void )
{
	for(int led_index = 0; led_index < LED_OBJS_SUPPORT_MAXN; ++led_index ) {
		if( g_stLedConfList[led_index].u8LedId != LED_UNUSED_ID ) {
			if( g_stLedCtrller[led_index].blink_cnt > 0 ) {
				return true;
			}
		}else break;
	}

	return false;
}

/*	DESP: Led module action detection task callback interface.
 * 	Auth: dingmz_frc.20191107.
 * */
void kLedModuleActionDetectCallback(void )
{
	// buzzer controller
	for(int led_index = 0; led_index < LED_OBJS_SUPPORT_MAXN; ++led_index ) {
		if( g_stLedConfList[led_index].u8LedId == LED_UNUSED_ID ) {
			break;
		}

		// PROCESS...
		if( g_stLedCtrller[led_index].blink_cnt > 0 ) {
			//iKonkeAfSelfPrint("--->debug(%d:%d) blink_cnt(%u)\r\n", led_index, g_stLedConfList[led_index].u8LedId, g_stLedCtrller[led_index].blink_cnt);

			if( g_stLedCtrller[led_index].onCounter > 0 ) {
				if((--g_stLedCtrller[led_index].onCounter) == 0) {
					// �����˸�����ݼ�
					if( g_stLedCtrller[led_index].start == LED_OFF ) {
						if(--g_stLedCtrller[led_index].blink_cnt == 0 ) {
							kLedDriverhandler(led_index, g_stLedCtrller[led_index].end);
							//may be restore led status of relay
							if (g_pLedBlinkDoneCallback[led_index]){
								g_pLedBlinkDoneCallback[led_index](g_stLedConfList[led_index].u8LedId);
								g_pLedBlinkDoneCallback[led_index] = NULL;
							}
							if (g_pLedBlinkHandelCallback[led_index]){
							    g_pLedBlinkHandelCallback[led_index] = NULL;
							}
						}
					}

					if( g_stLedCtrller[led_index].blink_cnt > 0 ) {
						kLedDriverhandler(led_index, LED_OFF);
						g_stLedCtrller[led_index].offCounter = g_stLedCtrller[led_index].offDuration;
					}
				}
			}else {
				if((--g_stLedCtrller[led_index].offCounter) == 0) {
					// �����˸�����ݼ�
					if(g_stLedCtrller[led_index].start == LED_ON ) {
						if(--g_stLedCtrller[led_index].blink_cnt == 0 ) {
							kLedDriverhandler(led_index, g_stLedCtrller[led_index].end);

							//may be restore led status of relay
							if (g_pLedBlinkDoneCallback[led_index]){
								g_pLedBlinkDoneCallback[led_index](g_stLedConfList[led_index].u8LedId);
								g_pLedBlinkDoneCallback[led_index] = NULL;
							}
							if (g_pLedBlinkHandelCallback[led_index]){
                                g_pLedBlinkHandelCallback[led_index] = NULL;
                            }
						}
					}

					if( g_stLedCtrller[led_index].blink_cnt > 0 ) {
						kLedDriverhandler(led_index, LED_ON);
						g_stLedCtrller[led_index].onCounter = g_stLedCtrller[led_index].onDuration;
					}
				}
			}
		}
	}
}

/*	DESP: Check if the specified led indicator is bliking.
 * 	Auth: dingmz_frc.20191108.
 * */
bool kLedIsBlinking(uint8_t led_id )
{
	uint8_t led_index = kLedGetLedIndexByID(led_id);

	if( led_index != 0XFF ) {
		return (g_stLedCtrller[led_index].blink_cnt > 0);
	}

	return false;
}

/*	DESP: Toggle LED
	Auth: maozj.20200226.
*/
void kLedOptToggle(uint8_t id)
{
	uint8_t led_index = kLedGetLedIndexByID(id);

	if( led_index == 0XFF ) {
		return ;
	}

	kLedDriverhandler(led_index, LED_TOGGLE);
}


/*	DESP: Indicator lamp behavior control interface.
	Auth: dingmz_frc.20191108.
*/
void kLedOptTrigger(uint8_t id, uint32_t duration_on_ms, uint32_t duration_off_ms, uint32_t times, LedOptEnum start, LedOptEnum end )
{
	uint8_t led_index = kLedGetLedIndexByID(id);
	static bool bFlg[LED_OBJS_SUPPORT_MAXN] = {false, false, false, false, false, false, false, false};
	if( led_index == 0XFF ) {
		return ;
	}

	iKonkeAfSelfPrint("LED_TRIGGER[%d]: id(%d), d_on(%d), d_off(%u), times(%u), start(%d), end(%d)\r\n",
		led_index, id, duration_on_ms, duration_off_ms, times, start, end);

	g_stLedCtrller[led_index].onDuration  = duration_on_ms / TICK_LOOP_NMS;
	g_stLedCtrller[led_index].offDuration = duration_off_ms/ TICK_LOOP_NMS;

	g_stLedCtrller[led_index].start = start;
	g_stLedCtrller[led_index].end = end;

	if( g_stLedCtrller[led_index].start == LED_ON ) {
		kLedDriverhandler(led_index, LED_ON);
		g_stLedCtrller[led_index].onCounter  = g_stLedCtrller[led_index].onDuration;
	}else {
		kLedDriverhandler(led_index, LED_OFF);
		g_stLedCtrller[led_index].offCounter = g_stLedCtrller[led_index].offDuration;
	}

	//ON or OFF maybe set relay led status when led is blinking
	//add by maozj 20200302
	if (duration_on_ms == 0 && duration_off_ms == 0 && times == 0 && kLedIsBlinking(id) == true){
		if (g_pLedBlinkDoneCallback[led_index] && !bFlg[led_index]){
			bFlg[led_index] = true;
			g_pLedBlinkDoneCallback[led_index](id);
		}
	}else {
		bFlg[led_index] = false;
	}

	g_stLedCtrller[led_index].blink_cnt   = times;

	g_pLedBlinkDoneCallback[led_index] = g_pLedActionDoneCallback;
	g_pLedBlinkHandelCallback[led_index] = g_pLedActionHandleCallback;
	//modify by maozj 20200302
	if (g_stLedCtrller[led_index].blink_cnt != 0){
		kTickRunnningTrigger();
	}
}

/*	DESP: single LED instance register interface.
 * 	Auth: dingmz_frc.20191108.
 * */
kk_err_t kLedRegister(uint8_t id, uint8_t gpio_num, LedGpioListSt gpio_list[], LedPolarityEnum ePolarity)
{
	if( LED_UNKNOW_ID == id || LED_ALLOPT_ID == id || NULL == gpio_list) {
		return KET_ERR_INVALID_PARAM;
	}

	int valid_index = LED_OBJS_SUPPORT_MAXN;
	kk_err_t err = KET_OK;

	for(int index = 0; index < LED_OBJS_SUPPORT_MAXN; ++index ) {
		if( g_stLedConfList[index].u8LedId == id ) {
			valid_index = index;
			break;
		}else if((valid_index == LED_OBJS_SUPPORT_MAXN) && (g_stLedConfList[index].u8LedId == LED_UNUSED_ID)) {
			valid_index = index;
		}
	}

	if( valid_index < LED_OBJS_SUPPORT_MAXN ) {
		memset(&g_stLedCtrller[valid_index], 0, sizeof(g_stLedCtrller[valid_index]));

		g_stLedConfList[valid_index].u8LedId = id;

		g_stLedConfList[valid_index].u8GpioMappNum = (gpio_num < LED_GPIO_MAPPING_MAXN)?gpio_num:LED_GPIO_MAPPING_MAXN;

		if( g_stLedConfList[valid_index].u8GpioMappNum > 0 ) {
			memcpy(g_stLedConfList[valid_index].gpios, gpio_list, sizeof(LedGpioListSt) * g_stLedConfList[valid_index].u8GpioMappNum);
		}

		g_stLedConfList[valid_index].eActionPolarity = ePolarity;

		iKonkeAfSelfPrint("LED_REGISTER: id(%d), eActionPolarity(%d)\r\n", g_stLedConfList[valid_index].u8LedId
			, g_stLedConfList[valid_index].eActionPolarity);

		for(int index = 0; index < g_stLedConfList[valid_index].u8GpioMappNum; ++index ) {
//			iKonkeAfSelfPrint("GPIO[%d]: port(%d), pin(%d)\r\n", index, g_stLedConfList[valid_index].gpios[index].port
//				, g_stLedConfList[valid_index].gpios[index].pin);
			//add by maozj 20200214
			//if (g_stLedConfList[valid_index].pwmIds[index] != PWM_UNKNOW_ID)
			{
				GPIO_PinModeSet(g_stLedConfList[valid_index].gpios[index].port, g_stLedConfList[valid_index].gpios[index].pin
						, gpioModePushPull, (g_stLedConfList[valid_index].eActionPolarity == ELP_HIGH)?0:1);
			}
		}

		// init default off status!
		kLedDriverhandler(valid_index, LED_OFF);
	}else {
		err = KET_ERR_INSUFFICIENT_SPACE;
	}

	return err;
}

/*	DESP: led control module Initialize configuration interface.
 * 	Note: Led initialization interface supports one-time registration of multiple LEDs!
 * 	Auth: dingmz_frc.20191108.
 * */
kk_err_t kLedModuleInit(LedConfSt *ledlist, unsigned char led_number, pLedActionDoneCallback pActionDoneCallback, pLedActionHandleCallback pActionHandleCallback)
{
	kk_err_t err = KET_OK;

	memset(g_stLedConfList, LED_UNUSED_ID, sizeof(g_stLedConfList));
	memset(g_stLedCtrller, 0, sizeof(g_stLedCtrller));

	if( ledlist && (led_number > 0)) {
		led_number = (led_number < LED_OBJS_SUPPORT_MAXN)?led_number:LED_OBJS_SUPPORT_MAXN;

		for(int index = 0; index < led_number; ++index ) {
			err = kLedRegister(ledlist[index].u8LedId, ledlist[index].u8GpioMappNum, ledlist[index].gpios, ledlist[index].eActionPolarity);
		}
	}
	//init callback function when led blink over
	g_pLedActionDoneCallback = pActionDoneCallback;
	g_pLedActionHandleCallback = pActionHandleCallback; 

	return err;
}
/*	DESP: led control module Initialize modify by led id.
 * 	Note: Led initialization interface
 * 	Auth: maozj.20191226.
 * */
kk_err_t kLedModuleInfoModifyGpioById(uint8_t id, LedConfSt ledconf)
{
	kk_err_t err = KET_OK;

	uint8_t ledIndex = kLedGetLedIndexByID(id);
	if (ledIndex == 0xFF){
//		iKonkeAfSelfPrint("Led Id Get Failed\r\n");
		return KET_FAILED;
	}
	kLedRegister(id,ledconf.u8GpioMappNum, ledconf.gpios, ledconf.eActionPolarity);
	return err;
}

