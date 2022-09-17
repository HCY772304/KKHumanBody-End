#ifndef __IKONKE_MODULE_LED_H_______________________________
#define __IKONKE_MODULE_LED_H_______________________________


#include "app/framework/include/af.h"
#include "../general/ikk-module-def.h"


#define LED_OBJS_SUPPORT_MAXN	8
// the number of physical gpios that can be mapped for a single LED control instance!
#define LED_GPIO_MAPPING_MAXN	4

#define LED_UNKNOW_ID			0	// Valid[1,254], 255 for Operate all at the same time!
#define LED_ALLOPT_ID			255	// Valid[1,254], 255 for Operate all at the same time!

#define BLINK_DEAD				0xffffffff
#define BLINK_NONE				0x0

// LED STATUS
typedef enum { LED_OFF = 0,	LED_ON = 1,	LED_TOGGLE, LED_IGNORE = 2 }LedOptEnum;
typedef enum { ELP_LOW = 0/* low to on */, ELP_HIGH/* high to on */ }LedPolarityEnum;

typedef struct {
	GPIO_Port_TypeDef port;
	unsigned int pin;
	unsigned int pwmId; //0 :not use pwm
}LedGpioListSt;

// led configuration struct
typedef struct tag_led_config_st {
	unsigned char u8LedId; 	// 0 ~ 254

	unsigned char u8GpioMappNum;
	LedGpioListSt gpios[LED_GPIO_MAPPING_MAXN];

	LedPolarityEnum eActionPolarity;
	//uint8_t pwmIds[LED_GPIO_MAPPING_MAXN]; //band pwm id
}LedConfSt;

typedef void (*pLedActionDoneCallback)(unsigned char id);
typedef void (*pLedActionHandleCallback)(unsigned char id);

/*	DESP: Indicator lamp behavior control interface.
	Auth: dingmz_frc.20191108.
*/
void kLedOptTrigger(uint8_t id, uint32_t duration_on_ms, uint32_t duration_off_ms, uint32_t times, LedOptEnum start, LedOptEnum end );
/*	DESP: Toggle LED
	Auth: maozj.20200226.
*/
void kLedOptToggle(uint8_t id);

// Turn ON
#define LED_OPT_ON(id)	do { \
	kLedOptTrigger(id, 0, 0, 0, LED_ON, LED_IGNORE); \
}while(0)

// Turn OFF
#define LED_OPT_OFF(id)	do { \
	kLedOptTrigger(id, 0, 0, 0, LED_OFF, LED_IGNORE); \
}while(0)

// Turn TOGGLE add by maozj 20200226
#define LED_OPT_TOGGLE(id)	do { \
		kLedOptToggle(id); \
}while(0)

/*	DESP: Check if the specified led indicator is bliking.
 * 	Auth: dingmz_frc.20191108.
 * */
bool kLedIsBlinking(uint8_t led_id );
/*	DESP: single LED instance register interface.
 * 	Auth: dingmz_frc.20191108.
 * */
kk_err_t kLedRegister(uint8_t id, uint8_t gpio_num, LedGpioListSt gpio_list[], LedPolarityEnum ePolarity);
/*	DESP: led control module Initialize configuration interface.
 * 	Note: Led initialization interface supports one-time registration of multiple LEDs!
 * 	Auth: dingmz_frc.20191108.
 * */
kk_err_t kLedModuleInit(LedConfSt *ledlist, unsigned char led_number, pLedActionDoneCallback pCallback, pLedActionHandleCallback pActionHandleCallback);
/*	DESP: led control module Initialize modify by led id.
 * 	Note: Led initialization interface
 * 	Auth: maozj.20191226.
 * */
kk_err_t kLedModuleInfoModifyGpioById(uint8_t id, LedConfSt ledconf);
#endif

