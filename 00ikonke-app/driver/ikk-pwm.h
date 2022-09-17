#ifndef __IKONKE_MODULE_PWM_H_______________________________
#define __IKONKE_MODULE_PWM_H_______________________________


#include "app/framework/include/af.h"
#include "../general/ikk-module-def.h"
#include "em_timer.h"

#define PWM_OBJS_SUPPORT_MAXN			6

#define PWM_UNKNOW_ID					0	// Valid[1,254], 255 for Operate all at the same time!
#define PWM_ALLOPT_ID					255	// Valid[1,254], 255 for Operate all at the same time!

#define PWM_TIMER_MAX_DUTY_CYCLE_NUM 	65535
#define PWM_TIMER_MID_DUTY_CYCLE_NUM 	500
#define PWM_TIMER_MIN_DUTY_CYCLE_NUM 	0

typedef enum {ELP_MIN = 0, ELP_MID,ELP_ANY, ELP_MAX, ELP_TO_BRIGHT/* min to max */, ELP_TO_DARKEN/* max to min */, ELP_OPT_DEFAULT=0xff }PwmGradientDirectionEnum;
typedef enum {KK_TIMER_CC0=0, KK_TIMER_CC1, KK_TIMER_CC2, KK_TIMER_CC3}PwmTimerChannelEnum;
typedef enum {KTIMER0=0, KTIMER1, KTIMER2, KTIMER3}TIMER_ENUM;

typedef struct {
	GPIO_Port_TypeDef port;
	unsigned int pin;
	TIMER_ENUM pwmTimer;
//	TIMER_TypeDef *pwmTimer;
	PwmTimerChannelEnum pwmChannel;
	uint32_t timerChannalLocation; //just for EFR32MG13 chip, need check datasheet,search "location"
}PwmGpioListSt;

// led configuration struct
typedef struct tag_pwm_config_st {
	unsigned char u8PwmId; 	// 0 ~ 254
	PwmGpioListSt gpioInfo;
	//TIMER_TypeDef timer;
	//uint8_t pwmChannel;
}PwmConfSt;

extern TIMER_TypeDef *ktimer[];
extern CMU_Clock_TypeDef kclock[];

//funtion callback when pwm duty cycle is changed to max or min
typedef void (*pPwmActionDoneCallback)(unsigned char id, PwmGradientDirectionEnum opt);

/*	DESP: get the pwm index by led_id.
 * 	Auth: maozj.20200211.
 * */
uint8_t kPwmGetIndexByID(uint8_t pwm_id );
/*	DESP: Check whether all pwm have stopped action detection.
 * 	Auth: maozj.20200211.
 * */
bool kPwmGradientChangeIsGoing(void);

/*	DESP: Check if the specified pwm  is changing.
 * 	Auth: maozj.20200211.
 * */
bool kPwmIsChanging(uint8_t pwm_id );
/*	DESP: registration the system pwm driver processing function interface.
	Auth: maozj.20200211.
*/
void kPwmDriverhandler(uint8_t pwm_index, PwmGradientDirectionEnum st, uint16_t value);

/*	DESP: pwm frequence init interface.
 * 	Auth: maozj.20200212.
 * */
void kPwmFrequencySetInit(uint16_t frequency_hz, uint16_t init_duty_cycle, PwmGpioListSt gpioInfo);

/*	DESP: pwm control module Initialize configuration interface.
 * 	Note: pwm initialization interface supports one-time registration of multiple pwm channel!
 * 	Auth: maozj.20200219.
 * */
kk_err_t kPwmModuleInit(PwmConfSt pwmlist[], unsigned char pwm_number, pPwmActionDoneCallback pCallback);

/*	DESP: Indicator pwm trigger behavior control interface.
	Auth: maozj.20200211.
*/
void kPwmOptTrigger(uint8_t id, uint32_t start_value, uint32_t end_value, uint32_t duration_time );

/*	DESP: clear pwm gradient counter interface by pwm id.

 * 	Auth: maozj.20200212.
 * */
bool kPwmClearGradientCounterById(uint8_t pwm_id);
#endif

