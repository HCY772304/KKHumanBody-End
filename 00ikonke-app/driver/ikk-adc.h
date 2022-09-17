#ifndef __IKONKE_MODULE_BATTERY_H_______________________________
#define __IKONKE_MODULE_BATTERY_H_______________________________

#include "app/framework/include/af.h"

#define ADC_UNKNOW_ID				0	// Valid[1,254]

#define ADC_CHANNEL_MAX_NUM		4  //adc scan max number, can not more than 4

#define USE_EFR32MG21_CHIP   false

#if defined(TIMER2) && defined(TIMER3)
//#include EMBER_AF_API_BATTERY_MONITOR
#include EMBER_AF_API_GENERIC_INTERRUPT_CONTROL
#include "em_cmu.h"
#include "em_adc.h"
#include "em_prs.h"

#include "em_iadc.h"
#else
#include "em_adc.h"
#endif

#define HIGH_LEVEL 	0
#define LOW_LEVEL	1

#define	DEFAULT_GETBATTERY_DELAY_S		40

typedef struct {
	uint8_t adcId;//only number
	bool directSampleGpio; //find adc channel by gpio, otherwise direct use PosSel
	GPIO_Port_TypeDef port;
	unsigned int pin;
	uint32_t posSel; //directSampleGpio is false, use it
	float dividerValue; //may be With divider resistor
	uint16_t maxSampleVoltageMV; //max voltage value, ready to write to attribute
	uint32_t referenceVoltageMV;
}AdcConfSt;

typedef void (*pBatteryVoltageGetCallback)(uint16_t battery_voltage_mv);

float kGetTemperatureByAdcId(uint8_t id);
/*	DESP: adc sample init
 * 	Auth: maozj.20200905
 * */
void kAdcModuleInit(AdcConfSt conf_list[], uint8_t adc_num,	uint32_t referenceVoltage);
void kBatteryModuleInit(uint8_t adc_index, uint8_t endpoint, uint16_t minVoltageMilliv, uint16_t maxVoltageMilliv);
//unit:mv
/*	DESP: This function will sample the ADC according to flag whether need to write to attribute
 * 	Auth: maozj.20200325.
 * 	limit_voltage_mv:make sure write battery voltage is normal about actual battery.
 * */
void kBatterySetToken(void);
uint16_t kGetVoltageValueById(uint8_t adc_id, uint8_t endpoint, uint8_t sample_mode, bool immediately, bool bIsWriteAttr, uint16_t diffrence);
uint32_t kGetAdcSampleValueById(uint8_t id);
uint8_t kkGetBatteryPowerPercent(uint16_t currentVoltageMilliV, uint16_t minVoltageMilliV, uint16_t maxVoltageMilliV, bool bIsWriteAttr);
void kGetBatteryTriggerSet(uint8_t get_times, uint16_t delay_s);
void kBatterySetLockTime(void);
#endif
