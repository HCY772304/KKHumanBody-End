#ifndef __IKONKE_MODULE_TIMER_H_______________________________
#define __IKONKE_MODULE_TIMER_H_______________________________

#include "app/framework/include/af.h"
//#include "../general/ikk-module-def.h"
#include "em_timer.h"
#include "ikk-pwm.h"
#if 0
#if defined(RTCC_PRESENT) && (RTCC_COUNT == 1)
#define RTCDRV_USE_RTCC
#else
#define RTCDRV_USE_RTC
#endif
#if defined(RTCDRV_USE_RTCC)
#include "em_rtcc.h"
#else
#include "em_rtc.h"
#endif
#endif
//#include "rtcdriver.h"

typedef void (*pTimerPollCallback)(void);

//可用于过零检测延时使用
void kTimerInit(TIMER_ENUM timer, bool bIsOneShot,  uint32_t time_us);
//do not use timer0
void kTimerStart(TIMER_ENUM timer, bool bIsOneShot, uint32_t time_us,  pTimerPollCallback start_callback, pTimerPollCallback stop_callback);
void kTimerStop(TIMER_ENUM timer);
bool kTimerIsGoing(TIMER_ENUM timer);

#endif
