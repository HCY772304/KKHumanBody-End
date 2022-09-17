#include "stdint.h"
#include "stdbool.h"
#include "stack/include/ember-types.h"
#include "em_gpio.h"

//#include "generic-interrupt-control/generic-interrupt-control-efr32.h"
//#include "generic-interrupt-control/generic-interrupt-control.h"

#include "../general/ikk-tick-handler.h"
#include "../general/ikk-debug.h"
#include "ikk-pwm.h"


#define PWM_UNUSED_ID					PWM_UNKNOW_ID

#define PWM_TICK_LOOP_NMS				10	// MS


#define KK_PWM_FREQUENCY          		(1000U)
#define KK_CLOCK_FREQUENCY        		38400000
#define KK_PWM_INVERT_OUTPUT       		false
#define KK_TIMER_DEFAULT_INIT            \
  {                                   \
    .eventCtrl = timerEventEveryEdge, \
    .edge = timerEdgeBoth,            \
    .prsSel = timerPRSSELCh0,         \
    .cufoa = timerOutputActionNone,   \
    .cofoa = timerOutputActionNone,   \
    .cmoa = timerOutputActionToggle,  \
    .mode = timerCCModePWM,           \
    .filter = false,                  \
    .prsInput = false,                \
    .coist = false,                   \
    .outInvert = KK_PWM_INVERT_OUTPUT,   \
  }


//typedef void (* LedCallHandler)(LedOptEnum eopt );

pPwmActionDoneCallback g_pPwmActionDoneCallback = NULL;
// led controller
static struct tag_pwm_controller_st {
	uint32_t actionCounter; //event couter
	uint32_t intervalDutyCycle; //+ / - every times
	uint32_t durationMs;
	uint16_t startDutyCycle;
	uint16_t endDutyCycle;
	uint16_t startDutyCycleValue;
	PwmGpioListSt gpioInfo;
}g_stPwmCtrller[PWM_OBJS_SUPPORT_MAXN] = { 0 };

static PwmConfSt g_stPwmConfList[PWM_OBJS_SUPPORT_MAXN] = {PWM_UNKNOW_ID};
static uint8_t g_u8PwmChannelNum = 0;
EmberEventControl kPwmGradientChangeEventControl;


#if defined(TIMER2) && defined(TIMER3)
	TIMER_TypeDef *ktimer[] = {TIMER0, TIMER1, TIMER2, TIMER3};
	CMU_Clock_TypeDef kclock[] = {cmuClock_TIMER0, cmuClock_TIMER1, cmuClock_TIMER2, cmuClock_TIMER3};
#else
	TIMER_TypeDef *ktimer[] = {TIMER0, TIMER1, TIMER1, TIMER1};
	CMU_Clock_TypeDef kclock[] = {cmuClock_TIMER0, cmuClock_TIMER1, cmuClock_TIMER1, cmuClock_TIMER1};
#endif
// ---------- GLOBAL VARIABLES ----------
static uint16_t ticksPerPeriod;
static uint16_t pwmFrequency;


/*	DESP: get the pwm index by led_id.
 * 	Auth: maozj.20200211.
 * */
uint8_t kPwmGetIndexByID(uint8_t pwm_id )
{
	for(int index = 0; index < PWM_OBJS_SUPPORT_MAXN; ++index ) {
//		iKonkeAfSelfPrint("####kPwmGetIndexByID index[%d] pwm_id[%d]\r\n",index,  g_stPwmConfList[index].u8PwmId);
		if( g_stPwmConfList[index].u8PwmId != PWM_UNUSED_ID ) {
			if( g_stPwmConfList[index].u8PwmId == pwm_id ) {
				return index;
			}
		}
		else break;
	}

	return 0XFF;
}

/*	DESP: registration the system pwm driver processing function interface.
	Auth: maozj.20200211.
*/
void kPwmDriverhandler(uint8_t pwm_index, PwmGradientDirectionEnum st, uint16_t value)
{
	emberEventControlSetInactive(kPwmGradientChangeEventControl);

	if (pwm_index >= PWM_OBJS_SUPPORT_MAXN){
//		iKonkeAfSelfPrint("####kPwmDriverhandler index is too large\r\n");
		return;
	}

	uint8_t timer = g_stPwmConfList[pwm_index].gpioInfo.pwmTimer;

//	iKonkeAfSelfPrint("######kPwmDriverhandler index(%d), direction(%d), value(%d)\r\n", pwm_index, st, value);
	if (timer != KTIMER0 && timer != KTIMER1 ){
		return;
	}

	switch(st) {
		case (ELP_MIN):
		{
			//emberEventControlSetInactive(kPwmGradientChangeEventControl);
			TIMER_CompareBufSet(timer == KTIMER0?TIMER0:TIMER1, \
					g_stPwmConfList[pwm_index].gpioInfo.pwmChannel, PWM_TIMER_MIN_DUTY_CYCLE_NUM);
			break;
		}
		case (ELP_MID):
		{
			//emberEventControlSetInactive(kPwmGradientChangeEventControl);
			TIMER_CompareBufSet(timer == KTIMER0?TIMER0:TIMER1, \
						g_stPwmConfList[pwm_index].gpioInfo.pwmChannel, PWM_TIMER_MID_DUTY_CYCLE_NUM);
			break;
		}

		case (ELP_ANY):
		{
			//emberEventControlSetInactive(kPwmGradientChangeEventControl);
			//must use param[value] when pwm set a value
			TIMER_CompareBufSet(timer == KTIMER0?TIMER0:TIMER1, \
						g_stPwmConfList[pwm_index].gpioInfo.pwmChannel, value);
			break;
		}
		case (ELP_MAX):
		{
			//emberEventControlSetInactive(kPwmGradientChangeEventControl);
			TIMER_CompareBufSet(timer == KTIMER0?TIMER0:TIMER1, \
						g_stPwmConfList[pwm_index].gpioInfo.pwmChannel, PWM_TIMER_MAX_DUTY_CYCLE_NUM);
			break;
		}
		case (ELP_TO_BRIGHT):
		{
			//start pwm gradient change
			//emberEventControlSetDelayMS(kPwmGradientChangeEventControl, 100);
			//TIMER_CompareBufSet(timer == 0?TIMER0:TIMER1, \
			//					g_stPwmConfList[pwm_index].gpioInfo.pwmChannel, PWM_TIMER_MIN_DUTY_CYCLE_NUM);
			break;
		}
		case (ELP_TO_DARKEN):
		{
			//start pwm gradient change
			//emberEventControlSetDelayMS(kPwmGradientChangeEventControl, 100);
			//TIMER_CompareBufSet(timer == 0?TIMER0:TIMER1, \
			//					g_stPwmConfList[pwm_index].gpioInfo.pwmChannel, PWM_TIMER_MAX_DUTY_CYCLE_NUM);
			break;
		}
		default: break;
	}
}

/*	DESP: Check whether all pwm have stopped action detection.
 * 	Auth: maozj.20200211.
 * */
bool kPwmGradientChangeIsGoing(void )
{
	for(int pwm_index = 0; pwm_index < g_u8PwmChannelNum; ++pwm_index ) {
		if( g_stPwmCtrller[pwm_index].actionCounter > 0 ) {
			return true;
		}
		//else break;
	}

	return false;
}

/*	DESP: Check if the specified pwm  is changing.
 * 	Auth: maozj.20200211.
 * */
bool kPwmIsChanging(uint8_t pwm_id )
{
	uint8_t pwm_index = kPwmGetIndexByID(pwm_id);

	if( pwm_index != 0XFF ) {
		return (g_stPwmCtrller[pwm_index].actionCounter > 0);
	}

	return false;
}

/*	DESP: Indicator pwm trigger behavior control interface.
	Auth: maozj.20200211.
*/
void kPwmOptTrigger(uint8_t id, uint32_t start_value, uint32_t end_value, uint32_t duration_time )
{
	uint8_t pwm_index = kPwmGetIndexByID(id);

	if( pwm_index == 0XFF || (start_value == end_value) ) {
//		iKonkeAfSelfPrint("#####PWM value is invalid index(%d)id(%d)start(%d)end(%d) \r\n", pwm_index, id, start_value, end_value);
		return ;
	}
	if (g_stPwmConfList[pwm_index].gpioInfo.pwmTimer != KTIMER0 && g_stPwmConfList[pwm_index].gpioInfo.pwmTimer != KTIMER1 ){
//		iKonkeAfSelfPrint("#####PWM Timer is invalid %d\r\n", g_stPwmConfList[pwm_index].gpioInfo.pwmTimer);
		return;
	}

//	iKonkeAfSelfPrint("PWM_TRIGGER: pwm_index[%d], id(%d), start_value(%d), end_value(%d), duration_time(%d)\r\n",
//			pwm_index, id, start_value, end_value, duration_time);

	g_stPwmCtrller[pwm_index].actionCounter  = duration_time / PWM_TICK_LOOP_NMS;
	g_stPwmCtrller[pwm_index].intervalDutyCycle = abs(start_value - end_value) \
													/ g_stPwmCtrller[pwm_index].actionCounter;

	g_stPwmCtrller[pwm_index].startDutyCycle = start_value;
	g_stPwmCtrller[pwm_index].endDutyCycle = end_value;
	g_stPwmCtrller[pwm_index].startDutyCycleValue = start_value;
	memcpy(&g_stPwmCtrller[pwm_index].gpioInfo, &g_stPwmConfList[pwm_index].gpioInfo, sizeof(PwmGpioListSt));

	TIMER_CompareBufSet(g_stPwmConfList[pwm_index].gpioInfo.pwmTimer == KTIMER0?TIMER0:TIMER1, \
			g_stPwmConfList[pwm_index].gpioInfo.pwmChannel, start_value);

	if(!emberEventControlGetActive(kPwmGradientChangeEventControl)) {
		emberEventControlSetDelayMS(kPwmGradientChangeEventControl, 50);
	}
}
#if 0
/*	DESP: pwm frequence init interface.
 * 	Auth: maozj.20200212.
 * */
void kPwmFrequencyInit(PwmConfSt config_list[], uint8_t size)
{
	bool bUseTimer0 = false;
	bool bUseTimer1 = false;
    bool flg1 = false;
    bool flg2 = false;
	if ( NULL == config_list || size == 0) {
		return;
	}

	  TIMER_InitCC_TypeDef timerCCInit = KK_TIMER_DEFAULT_INIT;

	  /* Select timer parameters */
	  TIMER_Init_TypeDef timerInit = {
	    .enable = true,
	    .debugRun = true,
	    .prescale = timerPrescale1,
	    .clkSel = timerClkSelHFPerClk,
	    .fallAction = timerInputActionNone,
	    .riseAction = timerInputActionNone,
	    .mode = timerModeUp,
	    .dmaClrAct = false,
	    .quadModeX4 = false,
	    .oneShot = false,
	    .sync = false,
	  };

	uint32_t ticksPerPeriod32;

	pwmFrequency = KK_PWM_FREQUENCY;

	ticksPerPeriod32 = (uint32_t) KK_CLOCK_FREQUENCY;
	ticksPerPeriod32 /= (uint32_t) pwmFrequency;

	ticksPerPeriod = (uint16_t) ticksPerPeriod32;

	for(int index = 0; index < size; ++index ) {
		if (config_list[index].gpioInfo.pwmTimer == KTIMER0){
			if (flg1 != true){
				flg1 = true;
				bUseTimer0 = true;
				CMU_ClockEnable(cmuClock_TIMER0, true);
			}
			if (config_list[index].gpioInfo.pwmChannel == KK_TIMER_CC0){
			  TIMER_InitCC(TIMER0, 0, &timerCCInit);
			//  TIMER0->ROUTEPEN |= TIMER_ROUTEPEN_CC0PEN;
			//  TIMER0->ROUTELOC0 |= (BSP_BULBPWM_CC0_LOC << _TIMER_ROUTELOC0_CC0LOC_SHIFT);
			  GPIO->TIMERROUTE[0].ROUTEEN  |= GPIO_TIMER_ROUTEEN_CC0PEN;
			  GPIO->TIMERROUTE[0].CC0ROUTE = (config_list[index].gpioInfo.port << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
									| (config_list[index].gpioInfo.pin << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
			  TIMER_CompareBufSet(TIMER0, KK_TIMER_CC0, 0);//modi by zyb
			  iKonkeAfSelfPrint("LED 0 init.\r\n");
			} else if (config_list[index].gpioInfo.pwmChannel == KK_TIMER_CC1){
			  TIMER_InitCC(TIMER0, 1, &timerCCInit);
			//  TIMER0->ROUTEPEN |= TIMER_ROUTEPEN_CC1PEN;
			//  TIMER0->ROUTELOC0 |= (BSP_BULBPWM_CC1_LOC << _TIMER_ROUTELOC0_CC1LOC_SHIFT);
			  GPIO->TIMERROUTE[0].ROUTEEN  |= GPIO_TIMER_ROUTEEN_CC1PEN;
				GPIO->TIMERROUTE[0].CC1ROUTE = (config_list[index].gpioInfo.port << _GPIO_TIMER_CC1ROUTE_PORT_SHIFT)
									  | (config_list[index].gpioInfo.pin << _GPIO_TIMER_CC1ROUTE_PIN_SHIFT);
			  TIMER_CompareBufSet(TIMER0, KK_TIMER_CC1, 0);//modi by zyb
			  iKonkeAfSelfPrint("LED 1 init.\r\n");
			} else if (config_list[index].gpioInfo.pwmChannel == KK_TIMER_CC2){
			  TIMER_InitCC(TIMER0, 2, &timerCCInit);
			//  TIMER0->ROUTEPEN |= TIMER_ROUTEPEN_CC2PEN;
			//  TIMER0->ROUTELOC0 |= (BSP_BULBPWM_CC2_LOC << _TIMER_ROUTELOC0_CC2LOC_SHIFT);
			  GPIO->TIMERROUTE[0].ROUTEEN  |= GPIO_TIMER_ROUTEEN_CC2PEN;
				GPIO->TIMERROUTE[0].CC2ROUTE = (config_list[index].gpioInfo.port << _GPIO_TIMER_CC2ROUTE_PORT_SHIFT)
									  | (config_list[index].gpioInfo.pin << _GPIO_TIMER_CC2ROUTE_PIN_SHIFT);
			  TIMER_CompareBufSet(TIMER0, KK_TIMER_CC2, 0);//modi by zyb
			}
		}
		if (bUseTimer0 == true){
			  // Set Top Value
			  TIMER_TopSet(TIMER0, ticksPerPeriod);

			  // Set compare value starting at 0 - it will be incremented in the interrupt
			  // handler
			  TIMER_Init(TIMER0, &timerInit);
		}
	}

	for(int index = 0; index < size; ++index ) {
		if (config_list[index].gpioInfo.pwmTimer == KTIMER1){
			if (flg2 != true){
				flg2 = true;
				bUseTimer1 = true;
				CMU_ClockEnable(cmuClock_TIMER1, true);
			}

			if (config_list[index].gpioInfo.pwmChannel == KK_TIMER_CC0){
				TIMER_InitCC(TIMER1, 0, &timerCCInit);
				//  TIMER1->ROUTEPEN |= TIMER_ROUTEPEN_CC0PEN;
				//  TIMER1->ROUTELOC0 |= (BSP_BULBPWM_COLOR_CC0_LOC << _TIMER_ROUTELOC0_CC0LOC_SHIFT);
				GPIO->TIMERROUTE[1].ROUTEEN  |= GPIO_TIMER_ROUTEEN_CC0PEN;
				GPIO->TIMERROUTE[1].CC0ROUTE = (config_list[index].gpioInfo.port << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
									| (config_list[index].gpioInfo.pin << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
				TIMER_CompareBufSet(TIMER1, KK_TIMER_CC0, 0);//modi by zyb
				iKonkeAfSelfPrint("LED 2 init.\r\n");
			} else if (config_list[index].gpioInfo.pwmChannel == KK_TIMER_CC1){
				TIMER_InitCC(TIMER1, 1, &timerCCInit);
				//  TIMER1->ROUTEPEN |= TIMER_ROUTEPEN_CC1PEN;
				//  TIMER1->ROUTELOC0 |= (BSP_BULBPWM_COLOR_CC1_LOC << _TIMER_ROUTELOC0_CC1LOC_SHIFT);
				GPIO->TIMERROUTE[1].ROUTEEN  |= GPIO_TIMER_ROUTEEN_CC1PEN;
				GPIO->TIMERROUTE[1].CC1ROUTE = (config_list[index].gpioInfo.port << _GPIO_TIMER_CC1ROUTE_PORT_SHIFT)
									  | (config_list[index].gpioInfo.pin << _GPIO_TIMER_CC1ROUTE_PIN_SHIFT);
				TIMER_CompareBufSet(TIMER1, KK_TIMER_CC1, 0);
				iKonkeAfSelfPrint("LED 3 init.\r\n");
			} else if (config_list[index].gpioInfo.pwmChannel == KK_TIMER_CC2){
				TIMER_InitCC(TIMER1, 2, &timerCCInit);
				//TIMER1->ROUTEPEN |= TIMER_ROUTEPEN_CC2PEN;
				//TIMER1->ROUTELOC0 |= (BSP_BULBPWM_COLOR_CC2_LOC << _TIMER_ROUTELOC0_CC2LOC_SHIFT);
				GPIO->TIMERROUTE[1].ROUTEEN  |= GPIO_TIMER_ROUTEEN_CC2PEN;
				GPIO->TIMERROUTE[1].CC0ROUTE = (config_list[index].gpioInfo.port << _GPIO_TIMER_CC2ROUTE_PORT_SHIFT)
									 | (config_list[index].gpioInfo.pin << _GPIO_TIMER_CC2ROUTE_PIN_SHIFT);
				TIMER_CompareBufSet(TIMER1, KK_TIMER_CC2, 0);
			}
			if (bUseTimer1 == true){
				// Set Top Value
				TIMER_TopSet(TIMER1, ticksPerPeriod);

				// Set compare value starting at 0 - it will be incremented in the interrupt
				// handler
				TIMER_Init(TIMER1, &timerInit);
			}
		}
	}
}
#endif
/*	DESP: pwm frequence init interface.
 * example:gpioInfo.timerChannalLocation = TIMER_ROUTELOC0_CC0LOC_LOC0
 * 	Auth: maozj.20200212.
 * */
void kPwmFrequencySetInit(uint16_t frequency_hz, uint16_t init_duty_cycle, PwmGpioListSt gpioInfo)
{
	if ( frequency_hz == 0) {
//		iKonkeAfSelfPrint("#Error:Freq = 0hz\r\n");
		return;
	}

	TIMER_ENUM eTimer = gpioInfo.pwmTimer;
	PwmTimerChannelEnum eChannel = gpioInfo.pwmChannel;
#if defined(TIMER2) && defined(TIMER3)
	if (eTimer != KTIMER0 && eTimer != KTIMER1 && eTimer != KTIMER2 && eTimer != KTIMER3)	{
#else
	if (eTimer != KTIMER0 && eTimer != KTIMER1)	{
#endif
//		iKonkeAfSelfPrint("#Error:timer is invalid \r\n", eTimer);
		return;
	}

#if defined(TIMER2) && defined(TIMER3)
	if (eChannel != KK_TIMER_CC0 && eChannel != KK_TIMER_CC1 && eChannel != KK_TIMER_CC2){
#else
	if (eChannel != KK_TIMER_CC0 && eChannel != KK_TIMER_CC1 && eChannel != KK_TIMER_CC2 && eChannel != KK_TIMER_CC3){
#endif
//		iKonkeAfSelfPrint("#Error:timer channel %d is invalid \r\n", eChannel);
		return;
	}


#if defined(TIMER2) && defined(TIMER3)
	uint8_t kroutePen[] = {GPIO_TIMER_ROUTEEN_CC0PEN, GPIO_TIMER_ROUTEEN_CC1PEN, GPIO_TIMER_ROUTEEN_CC2PEN};
	uint8_t kroutePort[] = {_GPIO_TIMER_CC0ROUTE_PORT_SHIFT, _GPIO_TIMER_CC1ROUTE_PORT_SHIFT, _GPIO_TIMER_CC2ROUTE_PORT_SHIFT};
	uint8_t kroutePin[] = {_GPIO_TIMER_CC0ROUTE_PIN_SHIFT, _GPIO_TIMER_CC1ROUTE_PIN_SHIFT, _GPIO_TIMER_CC2ROUTE_PIN_SHIFT};
	//uint32_t route[] = {GPIO->TIMERROUTE[eChannel].CC0ROUTE, GPIO->TIMERROUTE[eChannel].CC1ROUTE, GPIO->TIMERROUTE[eChannel].CC2ROUTE};
#else
	uint8_t kroutePen[] = {TIMER_ROUTEPEN_CC0PEN, TIMER_ROUTEPEN_CC1PEN, TIMER_ROUTEPEN_CC2PEN, TIMER_ROUTEPEN_CC3PEN};
	// Enable GPIO and clock
	CMU_ClockEnable(cmuClock_GPIO, true);

	// Configure GPIO as output
	GPIO_PinModeSet(gpioInfo.port, gpioInfo.pin, gpioModePushPull, 0);
#endif

	TIMER_InitCC_TypeDef timerCCInit = KK_TIMER_DEFAULT_INIT;

	  /* Select timer parameters */
	  TIMER_Init_TypeDef timerInit = {
	    .enable = true,
	    .debugRun = true,
	    .prescale = timerPrescale1,
	    .clkSel = timerClkSelHFPerClk,
	    .fallAction = timerInputActionNone,
	    .riseAction = timerInputActionNone,
	    .mode = timerModeUp,
	    .dmaClrAct = false,
	    .quadModeX4 = false,
	    .oneShot = false,
	    .sync = false,
	  };

	uint32_t ticksPerPeriod32 = 0;

	//pwmFrequency = KK_PWM_FREQUENCY;

	ticksPerPeriod32 = (uint32_t)KK_CLOCK_FREQUENCY;
	ticksPerPeriod32 /= (uint32_t)frequency_hz;

	ticksPerPeriod = (uint16_t)ticksPerPeriod32;

	CMU_ClockEnable(kclock[eTimer], true);


	TIMER_InitCC(ktimer[eTimer], eChannel, &timerCCInit);
	//  TIMER0->ROUTEPEN |= TIMER_ROUTEPEN_CC0PEN;
	//  TIMER0->ROUTELOC0 |= (BSP_BULBPWM_CC0_LOC << _TIMER_ROUTELOC0_CC0LOC_SHIFT);
#if defined(TIMER2) && defined(TIMER3)
	GPIO->TIMERROUTE[eTimer].ROUTEEN  |= kroutePen[eChannel];
	if (eChannel == KK_TIMER_CC0){
		GPIO->TIMERROUTE[eTimer].CC0ROUTE = (gpioInfo.port << kroutePort[eChannel])
						| (gpioInfo.pin <<  kroutePin[eChannel]);
	}else if(eChannel == KK_TIMER_CC1){
		GPIO->TIMERROUTE[eTimer].CC1ROUTE = (gpioInfo.port << kroutePort[eChannel])
						| (gpioInfo.pin <<  kroutePin[eChannel]);
	}else if(eChannel == KK_TIMER_CC2){
		GPIO->TIMERROUTE[eTimer].CC2ROUTE = (gpioInfo.port << kroutePort[eChannel])
						| (gpioInfo.pin <<  kroutePin[eChannel]);
	}
#else
	ktimer[eTimer]->ROUTEPEN |=  kroutePen[eChannel];
	ktimer[eTimer]->ROUTELOC0 |= gpioInfo.timerChannalLocation;
#endif

	TIMER_CompareBufSet(ktimer[eTimer], eChannel, init_duty_cycle);//Duty cycle set
//	iKonkeAfSelfPrint("#PWM timer %d ,channel %d init.\r\n", eTimer, eChannel);

	//top value
	TIMER_TopSet(ktimer[eTimer], ticksPerPeriod); //freq set

	// Set compare value starting at 0 - it will be incremented in the interrupt
	// handler
	TIMER_Init(ktimer[eTimer], &timerInit);
}

/*	DESP: single pwm instance register interface.
 * 	Auth: maozj.20200219.
 * */
kk_err_t kPwmRegister(uint8_t pwm_id, PwmGpioListSt gpio_list )
{
	if( PWM_UNKNOW_ID == pwm_id || PWM_ALLOPT_ID == pwm_id \
			|| (gpio_list.pwmTimer != KTIMER0 && gpio_list.pwmTimer != KTIMER1)) {
		return KET_ERR_INVALID_PARAM;
	}

	int valid_index = PWM_OBJS_SUPPORT_MAXN;
	kk_err_t err = KET_OK;

	for(int index = 0; index < PWM_OBJS_SUPPORT_MAXN; ++index ) {
		if( g_stPwmConfList[index].u8PwmId == pwm_id ) {
			valid_index = index;
			break;
		}else if((valid_index == PWM_OBJS_SUPPORT_MAXN) && (g_stPwmConfList[index].u8PwmId == PWM_UNUSED_ID)) {
			valid_index = index;
		}
	}

	if( valid_index < PWM_OBJS_SUPPORT_MAXN ) {
		memset(&g_stPwmCtrller[valid_index], 0, sizeof(g_stPwmCtrller[valid_index]));

		g_stPwmConfList[valid_index].u8PwmId = pwm_id;
		memcpy(&g_stPwmConfList[valid_index].gpioInfo, &gpio_list, sizeof(PwmGpioListSt));
		//g_stPwmConfList[valid_index].gpioInfo.timer = gpio_list.timer;
		//g_stPwmConfList[valid_index].gpioInfo.pwmChannel = gpio_list.pwmChannel;

//		iKonkeAfSelfPrint("PWM_REGISTER:index(%d),id(%d), pwmChannel(%d)\r\n",valid_index,  g_stPwmConfList[valid_index].u8PwmId
//			, g_stPwmConfList[valid_index].gpioInfo.pwmChannel);


		GPIO_PinModeSet(g_stPwmConfList[valid_index].gpioInfo.port, g_stPwmConfList[valid_index].gpioInfo.pin, gpioModePushPull, 0);
	}else {
		err = KET_ERR_INSUFFICIENT_SPACE;
	}

	return err;
}

/*	DESP: pwm control module Initialize configuration interface.
 * 	Note: pwm initialization interface supports one-time registration of multiple pwm channel!
 * 	Auth: maozj.20200219.
 * */
kk_err_t kPwmModuleInit(PwmConfSt pwmlist[], unsigned char pwm_number, pPwmActionDoneCallback pCallback)
{
	kk_err_t err = KET_OK;

	//memset(g_stPwmConfList, PWM_UNUSED_ID, sizeof(g_stPwmConfList));
	memset(g_stPwmCtrller, 0, sizeof(g_stPwmCtrller));

	if(pwmlist && (pwm_number > 0)) {
		g_u8PwmChannelNum = (pwm_number < PWM_OBJS_SUPPORT_MAXN)?pwm_number:PWM_OBJS_SUPPORT_MAXN;
		 // GPIO setup
		 CMU_ClockEnable(cmuClock_GPIO, true);

		for(int index = 0; index < g_u8PwmChannelNum; ++index ) {
			err = kPwmRegister(pwmlist[index].u8PwmId, pwmlist[index].gpioInfo);
			kPwmFrequencySetInit(KK_PWM_FREQUENCY, 0, pwmlist[index].gpioInfo);
		}

		//kPwmFrequencyInit(pwmlist, g_u8PwmChannelNum);
	}
	//init callback function when led blink over
	g_pPwmActionDoneCallback = pCallback;

	return err;
}
///*	DESP: led control module Initialize modify by led id.
// * 	Note: Led initialization interface
// * 	Auth: maozj.20191226.
// * */
//kk_err_t kLedModuleInfoModifyGpioById(uint8_t id, LedConfSt ledconf)
//{
//	kk_err_t err = KET_OK;
//
//	uint8_t ledIndex = kLedGetLedIndexByID(id);
//	if (ledIndex == 0xFF){
//		iKonkeAfSelfPrint("Led Id Get Failed\r\n");
//		return KET_FAILED;
//	}
//	// GPIO setup
//	CMU_ClockEnable(cmuClock_GPIO, true);
//	kLedRegister(id,ledconf.u8GpioMappNum, ledconf.gpios, ledconf.eActionPolarity);
//	return err;
//}

void kPwmGradientChangeEventHandler(void)
{
	//static uint8_t count = 0;
	emberEventControlSetInactive(kPwmGradientChangeEventControl);
#if 1
	for (uint8_t i = 0; i < g_u8PwmChannelNum; i++){
		if (g_stPwmCtrller[i].actionCounter > 0){
			if (g_stPwmCtrller[i].startDutyCycle > g_stPwmCtrller[i].endDutyCycle){
				g_stPwmCtrller[i].actionCounter--;
				g_stPwmCtrller[i].startDutyCycleValue -= g_stPwmCtrller[i].intervalDutyCycle;
				//Duty cycle set
				TIMER_CompareBufSet(g_stPwmCtrller[i].gpioInfo.pwmTimer == KTIMER0?TIMER0:TIMER1, \
							g_stPwmCtrller[i].gpioInfo.pwmChannel, g_stPwmCtrller[i].startDutyCycleValue);
			} else if (g_stPwmCtrller[i].startDutyCycle < g_stPwmCtrller[i].endDutyCycle){
				g_stPwmCtrller[i].actionCounter--;
				g_stPwmCtrller[i].startDutyCycleValue += g_stPwmCtrller[i].intervalDutyCycle;
				//Duty cycle set
				TIMER_CompareBufSet(g_stPwmCtrller[i].gpioInfo.pwmTimer == KTIMER0?TIMER0:TIMER1, \
							g_stPwmCtrller[i].gpioInfo.pwmChannel, g_stPwmCtrller[i].startDutyCycleValue);
			}
//			iKonkeAfSelfPrint("BREATH index(%d) count(%d)\r\n", i, g_stPwmCtrller[i].actionCounter);
			//����PWM_ID�����ص�
			if (g_stPwmCtrller[i].actionCounter == 0){
				//may be end duty cycle not equal, restore end duty cycle
				TIMER_CompareBufSet(g_stPwmCtrller[i].gpioInfo.pwmTimer == KTIMER0?TIMER0:TIMER1, \
											g_stPwmCtrller[i].gpioInfo.pwmChannel, g_stPwmCtrller[i].endDutyCycle);

				if (g_pPwmActionDoneCallback){
					PwmGradientDirectionEnum opt = ELP_OPT_DEFAULT;
					if (g_stPwmCtrller[i].startDutyCycle > g_stPwmCtrller[i].endDutyCycle){
						opt = ELP_TO_DARKEN;
					}else if (g_stPwmCtrller[i].startDutyCycle < g_stPwmCtrller[i].endDutyCycle){
						opt = ELP_TO_BRIGHT;
					}

					if (opt != ELP_OPT_DEFAULT){
						//user custom  implement
						g_pPwmActionDoneCallback(g_stPwmConfList[i].u8PwmId, opt);
					}
				}
			}
		}
	}

	//all pwm channel is operate done,must exit current event
	if (kPwmGradientChangeIsGoing() != true){
		if (g_pPwmActionDoneCallback){
			g_pPwmActionDoneCallback(PWM_ALLOPT_ID, ELP_OPT_DEFAULT);
		}
//		iKonkeAfSelfPrint("#############PWM event exit\r\n");
		return;
	}

#endif
	emberEventControlSetDelayMS(kPwmGradientChangeEventControl, PWM_TICK_LOOP_NMS);
}

/*	DESP: clear pwm gradient counter interface by pwm id.

 * 	Auth: maozj.20200212.
 * */
bool kPwmClearGradientCounterById(uint8_t pwm_id)
{
	if (pwm_id == PWM_UNUSED_ID){
//		iKonkeAfSelfPrint("#Err: kPwmClearGradientCounterById pwm id is invalid\r\n");
		return false;
	}

	for (int index = 0; index < PWM_OBJS_SUPPORT_MAXN; ++index ) {
		//iKonkeAfSelfPrint("####kPwmGetIndexByID index[%d] pwm_id[%d]\r\n",index,  g_stPwmConfList[index].u8PwmId);

		if( g_stPwmConfList[index].u8PwmId == pwm_id ){
//			iKonkeAfSelfPrint("###kPwmClearGradientCounterById success index(%d) id(%d)\r\n", index, pwm_id);
			g_stPwmCtrller[index].actionCounter = 0;
			return true;
		}
	}

	return false;
}
