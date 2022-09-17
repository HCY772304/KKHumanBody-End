#include "stdint.h"
#include "stdbool.h"
//#include "stack/include/ember-types.h"
#include "em_gpio.h"
#include "../driver/ikk-led.h"

#include "ikk-timer.h"
#include "../general/ikk-debug.h"



#define ONSHOT_MIN_TIMER_US				(10)

#if defined(TIMER2) && defined(TIMER3)
uint8_t kTimerIRQn[] = {TIMER0_IRQn, TIMER1_IRQn, TIMER2_IRQn, TIMER3_IRQn};
#else
uint8_t kTimerIRQn[] = {TIMER0_IRQn, TIMER1_IRQn, TIMER1_IRQn, TIMER1_IRQn};
#endif

typedef struct {
	bool bIsGoing;
	TIMER_ENUM timer;
	uint32_t timerTimeUs;
	bool bIsOneShot;
	pTimerPollCallback pollCallback;
	pTimerPollCallback stopCallback;
}KTimer_ST;

KTimer_ST g_stKtimerBuffer[4] = {{.bIsGoing=false}, {.bIsGoing=false}, {.bIsGoing=false}, {.bIsGoing=false}};

//可用于过零检测延时使用
void kTimerInit(TIMER_ENUM timer, bool bIsOneShot,  uint32_t time_us)
{
	/* Enable clock for TIMER0 module */
	CMU_ClockEnable(kclock[timer], true);
	//CMU_ClockEnable(cmuClock_HCLK, true);   //开启高速外设时钟
	CMU_ClockSelectSet(kclock[timer], cmuSelect_HFXO);

	/* Select TIMER0 parameters */
	TIMER_Init_TypeDef timerInit =
	{
	.enable= false,//by kid
	.debugRun= true,
	.prescale= timerPrescale8,
	.clkSel = timerClkSelHFPerClk,
	.fallAction = timerInputActionNone,
	.riseAction = timerInputActionNone,
	.mode = timerModeUp,
	.dmaClrAct= false,
	.quadModeX4 = false,
	.oneShot = bIsOneShot, //单次或循环
	.sync= false,
	};

	// Configure TIMER3 Compare/Capture for output compare
	//TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;

	//timerCCInit.mode = timerCCModeCompare;
	//timerCCInit.cmoa = timerOutputActionNone; // none output on compare match //by kid


	/* Configure TIMER */
	TIMER_Init(ktimer[timer], &timerInit);
	//TIMER_InitCC(TIMER3, 0, &timerCCInit

	uint32_t g_u32TimerFreq = CMU_ClockFreqGet(kclock[timer]); /// (timerInit.prescale + 1);
#if defined(TIMER2) && defined(TIMER3)
	uint32_t clkFreq = CMU_ClockFreqGet(cmuClock_HCLK);
#else
	uint32_t clkFreq = CMU_ClockFreqGet(cmuClock_HFPER);
#endif
	iKonkeAfSelfPrint("\r\n\r\n######TIMER3 freq = %d  clkFreq = %d #######\r\n", g_u32TimerFreq, clkFreq);
	// Set compare value to the first compare value
	//	lastcount = time_ms;
	//	TIMER_CompareSet(TIMER3, 0, time_ms);
	// Set Top value
	// Note each overflow event constitutes 1/2 the signal period
	g_u32TimerFreq = CMU_ClockFreqGet(kclock[timer])/(timerInit.prescale + 1);
	uint32_t topValue = (uint32_t)(g_u32TimerFreq / 1000000.0 * (time_us));
//	uint32_t topValue = timerFreq * time_us;
	TIMER_TopSet (ktimer[timer], topValue);


	// Enable TIMER3 interrupts
	//TIMER_IntEnable(TIMER3, TIMER_IEN_CC0);
	TIMER_IntEnable(ktimer[timer], TIMER_IF_OF);
	NVIC_EnableIRQ(kTimerIRQn[timer]);
	// Enable the TIMER
	TIMER_Enable(ktimer[timer], true);
}

#if defined(TIMER2) && defined(TIMER3)
void TIMER2_IRQHandler(void)
{
	uint32_t flags = TIMER_IntGet(TIMER2);
	TIMER_IntClear(TIMER2, flags);
	//test_timer3 ++ ;
	//LED_OPT_TOGGLE(5);

	if (g_stKtimerBuffer[KTIMER2].pollCallback){
		g_stKtimerBuffer[KTIMER2].pollCallback();
	}

	g_stKtimerBuffer[KTIMER2].bIsGoing = false;
}

void TIMER3_IRQHandler(void)
{
	uint32_t flags = TIMER_IntGet(TIMER3);
	TIMER_IntClear(TIMER3, flags);
	//test_timer3 ++ ;
	//LED_OPT_TOGGLE(5);

	if (g_stKtimerBuffer[KTIMER3].pollCallback){
		g_stKtimerBuffer[KTIMER3].pollCallback();
	}

	g_stKtimerBuffer[KTIMER3].bIsGoing = false;
}
#endif

void TIMER1_IRQHandler(void)
{
	uint32_t flags = TIMER_IntGet(TIMER1);
	TIMER_IntClear(TIMER1, flags);
	//test_timer3 ++ ;
	//LED_OPT_TOGGLE(5);

	if (g_stKtimerBuffer[KTIMER1].pollCallback){
		g_stKtimerBuffer[KTIMER1].pollCallback();
	}

	g_stKtimerBuffer[KTIMER1].bIsGoing = false;
}

//do not use timer0
void kTimerStart(TIMER_ENUM timer, bool bIsOneShot, uint32_t time_us,  pTimerPollCallback start_callback, pTimerPollCallback stop_callback)
{

	g_stKtimerBuffer[timer].bIsGoing = true;
	g_stKtimerBuffer[timer].pollCallback = start_callback;
	g_stKtimerBuffer[timer].stopCallback = stop_callback;
	kTimerInit(timer, bIsOneShot, time_us);
}

void kTimerStop(TIMER_ENUM timer)
{
	NVIC_ClearPendingIRQ(kTimerIRQn[timer]);
	NVIC_DisableIRQ(kTimerIRQn[timer]);
	// Disable the TIMER
	TIMER_Enable(ktimer[timer], false);
	if (g_stKtimerBuffer[timer].stopCallback){
		g_stKtimerBuffer[timer].stopCallback();
	}
}

bool kTimerIsGoing(TIMER_ENUM timer)
{
	return g_stKtimerBuffer[timer].bIsGoing;
}


