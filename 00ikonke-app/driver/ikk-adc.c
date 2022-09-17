#include <00ikonke-app/driver/ikk-adc.h>
#include "stdint.h"
#include "stdbool.h"
#include "stack/include/ember-types.h"
#include "em_gpio.h"

#include "../general/ikk-common-utils.h"
// *****************************************************************************
// * battery-monitor-efr32.c
// *
// * Code to implement a battery monitor.
// *
// * This code will read the battery voltage during a transmission (in theory
// * when we are burning the most current), and update the battery voltage
// * attribute in the power configuration cluster.
// *
// * Copyright 2016 Silicon Laboratories, Inc.                              *80*
// *****************************************************************************

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include "stack/include/ember-types.h"
//#include "util/silicon_labs/silabs_core/event_control/event.h"
#include "hal/hal.h"
#include "../general/ikk-debug.h"
#include "../general/ikk-network.h"
#include "../general/ikk-opt-tunnel.h"
#include "../general/ikk-cluster.h"
#include <00ikonke-app/ikk-user.h>


#define FIFO_SIZE   			10
#define ADC_CHANNEL_MAX_NUM		4  //adc scan max number, can not more than 4
#define ADC_GET_TIMEOUT_COUNTER	5
#define GETBATTERY_LOOPTIME_SEC    20 * 60
#define TOKEN_BATTERY_VOLTAGE 18
#define TOKEN_BATTERY_VOLTAGE 20

//static float g_fDividerValue = 1.0;
//static uint16_t g_u16ReferenceVoltage = 5000;
//pBatteryVoltageGetCallback g_pBatteryVoltageGetCallback = NULL;
//static uint16_t u16TimeoutCounter = ADC_GET_TIMEOUT_COUNTER;
//static bool g_bAdcScanDone = false;//scan adc covert done in this time
static uint8_t g_u8CurrentUseAdcId = ADC_UNKNOW_ID;

typedef struct {
	uint8_t adcIndex;
	uint8_t endpoint;
	uint8_t getTimes;
	uint8_t currentPositionIndex[2];
	uint8_t fifoInitialized[2];
	uint16_t lastReportedVoltageMilliV;
	uint16_t delayTimeS;
	uint16_t maxVoltageMilliv;
	uint16_t minVoltageMilliv;
	uint32_t getBatteryLockTime;
	uint32_t voltageValueFIFO[2][FIFO_SIZE];
}BatteryInfoTypedef;

BatteryInfoTypedef g_stBatteryInfo = {.adcIndex = ADC_UNKNOW_ID, .fifoInitialized = {false, false}};

typedef struct {
	AdcConfSt adcConf;
	uint16_t adcValue;
	//uint16_t adcValueFIFO[FIFO_SIZE];
	//bool bAdcScanDone;//scan adc covert done in this time
}AdcValueTypedef;

AdcValueTypedef g_stAdcValueList[ADC_CHANNEL_MAX_NUM] = {
		{.adcConf.adcId = ADC_UNKNOW_ID},
		{.adcConf.adcId = ADC_UNKNOW_ID},
		{.adcConf.adcId = ADC_UNKNOW_ID},
		{.adcConf.adcId = ADC_UNKNOW_ID}
};

typedef struct {
	uint8_t adc0Channel;
	GPIO_Port_TypeDef port;
	unsigned int pin;
	uint32_t posSel1; //different bus
	uint32_t posSel2; //different bus
	uint32_t posSel3; //different bus
	bool bCurrentChannelUsed; //just only one channel can be used one bus
}Adc0TableSt;

uint8_t g_u8AdcNum = 0;


// sample FIFO access variables
static uint8_t samplePtr = 0;
//static uint16_t voltageFifo[FIFO_SIZE];
static bool fifoInitialized = false;

// Remember the last reported voltage value from callback, which will be the
// return value if anyone needs to manually poll for data
// static uint16_t lastReportedVoltageMilliV;
// ------------------------------------------------------------------------------
// Forward Declaration
static uint16_t kFilterVoltageSample(uint8_t id, uint16_t sample);
static uint32_t kAdcToMilliV(uint32_t adcVal, uint32_t referenceVoltageMV);
static void kAdcInitialize(uint8_t adc_id);
static void kBatteryMonitorWriteDataCallback(uint8_t endpoint, uint16_t voltageMilliV);
// ------------------------------------------------------------------------------
// Globals

//EmberEventControl emberAfPluginBatteryMonitorReadADCEventControl;
EmberEventControl kGetBatteryVoltageEventControl;

static uint8_t kAdcFindIndexById(uint8_t id);
static uint8_t kFindAdcPosSelUsefullByGpio(GPIO_Port_TypeDef port, unsigned int pin, uint32_t *adcPosSel);
static bool kFindAdcPosSelUsefullByPoseSel(uint32_t adcPosSel);
//check adc channel is used
static bool kCheckAdcPosSelChannelUsedByGpio(GPIO_Port_TypeDef port, unsigned int pin);
static uint8_t kFindAdcPosSelIndexByPosSel(uint32_t adcPosSel);
static uint8_t kGetPosSelByAdcId(uint8_t id, uint32_t *u32Pos1, uint32_t *u32Pos2, uint32_t *u32Pos3);
//static void kAdcStart(void);

#if !defined(TIMER2) && !defined(TIMER3)

// Init to max ADC clock for Series 1
#define adcFreq   16000000
#define GET_REFERENCE_VOLTAGE_MV(type) (type == adcRef5V?5000:(type == adcRef1V25?1250:3300))

// efr32mg1 datasheet
//Table 6.9. ADC0 Bus
Adc0TableSt g_stAdc0Table[] = {
		{1, gpioPortD, 9, adcPosSelAPORT4XCH1, adcPosSelAPORT3YCH1, 0, false},
		{2, gpioPortD, 10, adcPosSelAPORT4YCH2, adcPosSelAPORT3XCH2, 0, false},
		{3, gpioPortD, 11, adcPosSelAPORT4XCH3, adcPosSelAPORT3YCH3, 0, false},
		{4, gpioPortD, 12, adcPosSelAPORT4YCH4, adcPosSelAPORT3XCH4, 0, false},
		{5, gpioPortD, 13, adcPosSelAPORT4XCH5, adcPosSelAPORT3YCH5, 0, false},
		{6, gpioPortD, 14, adcPosSelAPORT4YCH6, adcPosSelAPORT3XCH6, 0, false},
		{6, gpioPortC, 6, adcPosSelAPORT2YCH6, adcPosSelAPORT1XCH6, 0, false},
		{7, gpioPortD, 15, adcPosSelAPORT4XCH7, adcPosSelAPORT3YCH7, 0, false},
		{7, gpioPortC, 7, adcPosSelAPORT2XCH7, adcPosSelAPORT1YCH7, 0, false},
		{8, gpioPortA, 0, adcPosSelAPORT4YCH8, adcPosSelAPORT3XCH8, 0, false},
		{8, gpioPortC, 8, adcPosSelAPORT2YCH8, adcPosSelAPORT1XCH8, 0, false},
		{9, gpioPortA, 1, adcPosSelAPORT4XCH9, adcPosSelAPORT3YCH9, 0, false},
		{9, gpioPortC, 9, adcPosSelAPORT2XCH9, adcPosSelAPORT1YCH9, 0, false},
		{10, gpioPortA, 2, adcPosSelAPORT4YCH10, adcPosSelAPORT3XCH10, 0, false},
		{10, gpioPortC, 10, adcPosSelAPORT2YCH10, adcPosSelAPORT1XCH10, 0, false},
		{11, gpioPortA, 3, adcPosSelAPORT4XCH11, adcPosSelAPORT3YCH11, 0, false},
		{11, gpioPortC, 11, adcPosSelAPORT2XCH11, adcPosSelAPORT1YCH11, 0, false},
		{12, gpioPortA, 4, adcPosSelAPORT4YCH12, adcPosSelAPORT3XCH12, 0, false},
		{13, gpioPortA, 5, adcPosSelAPORT4XCH13, adcPosSelAPORT3YCH13, 0, false},
		{16, gpioPortF, 0, adcPosSelAPORT2YCH16, adcPosSelAPORT1XCH16, 0, false},
		{17, gpioPortF, 1, adcPosSelAPORT2XCH17, adcPosSelAPORT1YCH17, 0, false},
		{18, gpioPortF, 2, adcPosSelAPORT2YCH18, adcPosSelAPORT1XCH18, 0, false},
		{19, gpioPortF, 3, adcPosSelAPORT2XCH19, adcPosSelAPORT1YCH19, 0, false},
		{20, gpioPortF, 4, adcPosSelAPORT2YCH20, adcPosSelAPORT1XCH20, 0, false},
		{21, gpioPortF, 5, adcPosSelAPORT2XCH21, adcPosSelAPORT1YCH21, 0, false},
		{22, gpioPortF, 6, adcPosSelAPORT2YCH22, adcPosSelAPORT1XCH22, 0, false},
		{23, gpioPortF, 7, adcPosSelAPORT2XCH23, adcPosSelAPORT1YCH23, 0, false},
		{27, gpioPortB, 11, adcPosSelAPORT4XCH27, adcPosSelAPORT3YCH27, 0, false},
		{28, gpioPortB, 12, adcPosSelAPORT4YCH28, adcPosSelAPORT3XCH28, 0, false},
		{29, gpioPortB, 13, adcPosSelAPORT4XCH29, adcPosSelAPORT3YCH29, 0, false},
		{30, gpioPortB, 14, adcPosSelAPORT4YCH30, adcPosSelAPORT3XCH30, 0, false},
		{31, gpioPortB, 15, adcPosSelAPORT4XCH31, adcPosSelAPORT3YCH31, 0, false},
};

// Use this macro to check if Battery Monitor plugin is included
#define EMBER_AF_PLUGIN_BATTERY_MONITOR
// User options for plugin Battery Monitor
#define EMBER_AF_PLUGIN_BATTERY_MONITOR_MONITOR_TIMEOUT_M 0
#define EMBER_AF_PLUGIN_BATTERY_MONITOR_SAMPLE_FIFO_SIZE 16

// Default settings used to configured the ADC to read the battery voltage
#define ADC_INITSINGLE_BATTERY_VOLTAGE                                \
  {                                                                   \
    adcPRSSELCh0, /* PRS ch0 (if enabled). */                         \
    adcAcqTime16, /* 1 ADC_CLK cycle acquisition time. */             \
    adcRef5VDIFF, /* V internal reference. */                         \
    adcRes12Bit, /* 12 bit resolution. */                             \
    adcPosSelAVDD, /* Select Vdd as posSel */                         \
    adcNegSelVSS, /* Select Vss as negSel */                          \
    false,       /* Single ended input. */                            \
    false,       /* PRS disabled. */                                  \
    false,       /* Right adjust. */                                  \
    false,       /* Deactivate conversion after one scan sequence. */ \
    false,       /* No EM2 DMA wakeup from single FIFO DVL */         \
    false        /* Discard new data on full FIFO. */                 \
  }

#define ADC_REFERENCE_VOLTAGE_MILLIVOLTS 5000




#else
// When changing GPIO port/pins above, make sure to change xBUSALLOC macro's
// accordingly.
#define IADC_INPUT_ABUS         ABUSALLOC
#define IADC_INPUT_BBUS         BBUSALLOC
#define IADC_INPUT_CDBUS        CDBUSALLOC

#define IADC_INPUT_ABUSALLOC    GPIO_ABUSALLOC_AEVEN0_ADC0
#define IADC_INPUT_BBUSALLOC    GPIO_BBUSALLOC_BEVEN0_ADC0
#define IADC_INPUT_CDBUSALLOC    GPIO_CDBUSALLOC_CDEVEN0_ADC0

//Adc0TableSt g_stAdc0Table[] = {
		//{1, gpioPortA, 0, iadcPosInputPortAPin0, IADC_INPUT_ABUS, IADC_INPUT_ABUSALLOC, false},
	//	{2, gpioPortB, 0, iadcPosInputPortBPin0, IADC_INPUT_BBUS, IADC_INPUT_BBUSALLOC, false},
		//{3, gpioPortC, 0, iadcPosInputPortCPin0, IADC_INPUT_CDBUS, IADC_INPUT_CDBUSALLOC, false},
		//{1, gpioPortD, 0, iadcPosInputPortDPin0, IADC_INPUT_CDBUS, IADC_INPUT_CDBUSALLOC, false},

//};

#define ADC_REFERENCE_VOLTAGE_MILLIVOLTS 1200
#define GET_REFERENCE_VOLTAGE_MV(type) (type == iadcCfgReferenceVddx?3300:(type == iadcCfgReferenceExt1V25?1250:1200))


// Set CLK_ADC to 10MHz
#define CLK_SRC_ADC_FREQ        1000000 // CLK_SRC_ADC
#define CLK_ADC_FREQ            10000   // CLK_ADC

// When changing GPIO port/pins below, make sure to change xBUSALLOC macro's
// accordingly.
//#define IADC_INPUT_0_BUS          CDBUSALLOC
//#define IADC_INPUT_0_BUSALLOC     GPIO_CDBUSALLOC_CDEVEN0_ADC0
//#define IADC_INPUT_1_BUS          CDBUSALLOC
//#define IADC_INPUT_1_BUSALLOC     GPIO_CDBUSALLOC_CDODD0_ADC0
// When changing GPIO port/pins above, make sure to change xBUSALLOC macro's
// accordingly.
//#define IADC_INPUT_BUS         CDBUSALLOC //CDBUSALLOC
//#define IADC_INPUT_BUSALLOC    _GPIO_CDBUSALLOC_CDEVEN0_ADC0 //GPIO_CDBUSALLOC_CDEVEN0_ADC0
/*******************************************************************************
 ***************************   GLOBAL VARIABLES   *******************************
 ******************************************************************************/

static volatile int32_t sample;
static volatile double singleResult; // Volts

/**************************************************************************//**
 * @brief  Initialize IADC function
 *****************************************************************************/
void initIADC (uint8_t adc_id)
{
#if 0
	//g_u16ReferenceVoltage = referenceVoltage;
	uint8_t adcIndex = kAdcFindIndexById(adc_id);

	if (adcIndex == 0xFF) {
		iKonkeAfSelfPrint("Err:kAdcInitialize id(%d)\r\n", adc_id);
		return;
	}
	// Declare init structs
	IADC_Init_t init = IADC_INIT_DEFAULT;
	IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
	IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
	IADC_SingleInput_t initSingleInput = IADC_SINGLEINPUT_DEFAULT;

	// Reset IADC to reset configuration in case it has been modified by
	// other code
	IADC_reset(IADC0);

	// Select clock for IADC
	CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO);  // FSRCO - 20MHz

	// Modify init structs and initialize
	init.warmup = iadcWarmupKeepWarm;

	// Set the HFSCLK prescale value here
	init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);


	// Assign pins to positive and negative inputs in differential mode
	uint32_t posSel1 = 0,  posSel2 = 0, posSel3 = 0;
	kGetPosSelByAdcId(adc_id, &posSel1, &posSel2, &posSel3);
	initSingleInput.posInput = (IADC_SingleInput_t)posSel1;
	initSingleInput.negInput   = iadcNegInputGnd;   // PC05 -> P27 on BRD4001 J102
	// Use unbuffered AVDD as reference
	initAllConfigs.configs[0].reference = (IADC_CfgReference_t)g_stAdcValueList[adcIndex].adcConf.referenceVoltageMV;

	// Configuration 0 is used by both scan and single conversions by default
	// Use unbuffered AVDD as reference
	initAllConfigs.configs[0].reference = iadcCfgReferenceInt1V2;

	// Divides CLK_SRC_ADC to set the CLK_ADC frequency
	initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
											 CLK_ADC_FREQ,
											 0,
											 iadcCfgModeNormal,
											 init.srcClkPrescale);





	// Initialize the IADC
	IADC_init(IADC0, &init, &initAllConfigs);

	// Initialize the Single conversion inputs
	IADC_initSingle(IADC0, &initSingle, &initSingleInput);

	// Allocate the analog bus for ADC0 inputs
	//  GPIO->IADC_INPUT_0_BUS |= IADC_INPUT_0_BUSALLOC;
	//  GPIO->IADC_INPUT_1_BUS |= IADC_INPUT_1_BUSALLOC;
	if (posSel2 == IADC_INPUT_ABUS) {
		GPIO->IADC_INPUT_ABUS |= GPIO_ABUSALLOC_AEVEN0_ADC0;
	} else if (posSel2 == IADC_INPUT_BBUS) {
		GPIO->IADC_INPUT_BBUS |= GPIO_BBUSALLOC_BEVEN0_ADC0;
	} else {
		GPIO-> IADC_INPUT_CDBUS |= IADC_INPUT_CDBUSALLOC;
		GPIO->IADC_INPUT_CDBUS |= GPIO_CDBUSALLOC_CDODD0_ADC0;
	}
#endif
}


#endif

static void kAdcInitialize(uint8_t adc_id)
{
	//g_u16ReferenceVoltage = referenceVoltage;
	uint8_t adcIndex = kAdcFindIndexById(adc_id);

	if (adcIndex == 0xFF) {
		iKonkeAfSelfPrint("Err:kAdcInitialize id(%d)\r\n", adc_id);
		return;
	}
//
//	if (idIndex != 0xFF) {
#if !defined(TIMER2) && !defined(TIMER3)
	uint32_t flags;
	ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
	ADC_InitSingle_TypeDef initAdc = ADC_INITSINGLE_BATTERY_VOLTAGE;
	ADC_Reset(ADC0);
	// Enable ADC clock
	CMU_ClockEnable(cmuClock_ADC0, true);
	uint32_t posSel1 = 0,  posSel2 = 0, posSel3 = 0;
	kGetPosSelByAdcId(adc_id, &posSel1, &posSel2, &posSel3);
	initAdc.posSel = (ADC_PosSel_TypeDef)posSel1;
	initAdc.reference = (ADC_Ref_TypeDef)(g_stAdcValueList[adcIndex].adcConf.referenceVoltageMV);
	iKonkeAfSelfPrint("ADC id(%d) reference(%d)\r\n", adc_id, initAdc.reference);
	// Initialize the ADC peripheral
	ADC_Init(ADC0, &init);

	// Setup ADC for single conversions for reading AVDD with a 5V reference
	ADC_InitSingle(ADC0, &initAdc);

	flags = ADC_IntGet(ADC0);
	ADC_IntClear(ADC0, flags);
	ADC_Start(ADC0, adcStartSingle);

#else
		initIADC(adc_id);
		IADC_command(IADC0, iadcCmdStartSingle);

		// Wait for conversion to be complete
		while(!(IADC0->STATUS & (_IADC_STATUS_CONVERTING_MASK
							| _IADC_STATUS_SINGLEFIFODV_MASK)));

		// Get ADC result
		uint16_t adc = IADC_readSingleData(IADC0);
#endif
//	}else {
//		iKonkeAfSelfPrint("Err: adcId(%d)\r\n", id);
//	}
}

/*	DESP: This function will sample the ADC according to flag whether need to write to attribute
 * 	Auth: maozj.20200325.
 * */
uint16_t kGetVoltageValueById(uint8_t adc_id, uint8_t endpoint, uint8_t sample_mode, bool immediately, bool bIsWriteAttr, uint16_t diffrence)
{


	uint16_t voltageMilliV;
	//uint32_t currentMsTick = halCommonGetInt32uMillisecondTick();
	//uint32_t timeSinceLastMeasureMS = currentMsTick - lastBatteryMeasureTick;

	//Get adc value
	uint32_t vData = kGetAdcSampleValueById(adc_id);

    //kAdcStart();
	uint8_t index = kAdcFindIndexById(adc_id);
	if (index != 0xFF) {
		voltageMilliV = kAdcToMilliV(vData, GET_REFERENCE_VOLTAGE_MV(g_stAdcValueList[index].adcConf.referenceVoltageMV));
		g_stAdcValueList[index].adcValue = vData;
		iKonkeAfSelfPrint("sample ADC vData = %d voltageMilliV = %dmv, time(%ld)\r\n", vData, voltageMilliV, halCommonGetInt64uMillisecondTick() );
		 emberAfPrint(0xFFFF, "sample ADC vData = %d voltageMilliV = %dmv, time(%ld)\r\n", vData, voltageMilliV, halCommonGetInt64uMillisecondTick() );
		uint16_t maxVoltageValue = g_stAdcValueList[index].adcConf.maxSampleVoltageMV;
		// filter the voltage to prevent spikes from overly influencing data
        if (voltageMilliV != 0) {
            voltageMilliV += 20;
        }
		if (immediately != true /*&& voltageMilliV <=  maxVoltageValue*/){
			voltageMilliV = kFilterVoltageSample(sample_mode, voltageMilliV);
		}

		// extern bool batterySampleFlg;
		voltageMilliV = (uint16_t)(voltageMilliV * g_stAdcValueList[index].adcConf.dividerValue);
		voltageMilliV += diffrence;
//		iKonkeAfSelfPrint("Battery Voltage11 = %d\r\n", voltageMilliV);
		//batterySampleFlg = true; 
//		emberAfPrint(0xFFFF, "22time(%d) sample ADC vData = %d voltageMilliV = %dmv\r\n",halCommonGetInt64uMillisecondTick(), vData, voltageMilliV);

		 if (g_stBatteryInfo.lastReportedVoltageMilliV == 0){
			 g_stBatteryInfo.lastReportedVoltageMilliV = maxVoltageValue;
		 }
         voltageMilliV = voltageMilliV > maxVoltageValue?			\
                        maxVoltageValue:voltageMilliV > g_stBatteryInfo.lastReportedVoltageMilliV?		\
                        g_stBatteryInfo.lastReportedVoltageMilliV:voltageMilliV;

		if (bIsWriteAttr){
			g_stBatteryInfo.lastReportedVoltageMilliV = voltageMilliV;
			kBatteryMonitorWriteDataCallback(endpoint, voltageMilliV);
		}
	} else {
		g_stBatteryInfo.lastReportedVoltageMilliV = 0;
		iKonkeAfSelfPrint("Err: kGetVoltageValueById is Invalid id(%d)\r\n", adc_id);
	}

  extern SENSOR_CHECK_STATUS_E g_eSensorCheckStatus;
  extern EmberEventControl kDelaySetLedStatusEventControl;

  if (g_eSensorCheckStatus == SCS_INIT) {
    g_eSensorCheckStatus = SCS_LOCK;
    iKonkeAfSelfPrint("###Sensor Locked After Message Sent 9999\r\n");

    emberEventControlSetDelayMS(kDelaySetLedStatusEventControl, 3.2 * 1000);
  }
	return voltageMilliV;
}

// ------------------------------------------------------------------------------
// Implementation of private functions

// Convert a 12 bit analog ADC reading to a value
// in milliVolts
static uint32_t kAdcToMilliV(uint32_t adcVal, uint32_t referenceVoltageMV)
{
	uint32_t vMax = 0xFFF; // 12 bit ADC maximum
	uint32_t referenceMilliV = referenceVoltageMV;

	float milliVPerBit = (float)referenceMilliV / (float)vMax;

	return (uint32_t)(milliVPerBit * adcVal);
}

// Provide smoothing of the voltage readings by reporting an average over the
// last few values
static uint16_t kFilterVoltageSample(uint8_t mode_index, uint16_t sample)
{
	uint32_t voltageSum;
	#define FILTER_NUM    2 
	uint32_t tmpBuffer[FIFO_SIZE] = {0};
	uint16_t temp_sample[2] = {0}, sample_out = g_stBatteryInfo.maxVoltageMilliv;
	if (g_stBatteryInfo.fifoInitialized[mode_index] == true){
		g_stBatteryInfo.voltageValueFIFO[mode_index][g_stBatteryInfo.currentPositionIndex[mode_index]++] = sample;

		if (g_stBatteryInfo.currentPositionIndex[mode_index] >= FIFO_SIZE) {
			g_stBatteryInfo.currentPositionIndex[mode_index] = 0;
		}
		#if 0
		voltageSum = 0;
		for (i = 0; i < FIFO_SIZE; i++) {
			voltageSum += voltageFifo[i];
		}
		sample = voltageSum / FIFO_SIZE;
		#endif

	}else{
		for (int i = 0; i < FIFO_SIZE; i++) {
			g_stBatteryInfo.voltageValueFIFO[mode_index][i] = sample;
		}
		g_stBatteryInfo.fifoInitialized[mode_index] = true;
	}

	for (int i = 0; i < 2; i++){
		if (true == g_stBatteryInfo.fifoInitialized[i]){
			memcpy(tmpBuffer, g_stBatteryInfo.voltageValueFIFO[i], FIFO_SIZE * sizeof(uint32_t));
			temp_sample[i] = kUtilsGetAverage(tmpBuffer, FIFO_SIZE, FILTER_NUM);
		#if 1
			iKonkeAfSelfPrint("###before(%d) data = ", temp_sample[i]);
			// emberAfPrint(0xFFFF, "###before(%d) data = ", temp_sample[i]);
			for (int j = 0; j < FIFO_SIZE; j++) {
				iKonkeAfSelfPrint("%d ", g_stBatteryInfo.voltageValueFIFO[i][j]);
				// emberAfPrint(0xFFFF, "%d ", g_stBatteryInfo.voltageValueFIFO[i][j]);
			}
			iKonkeAfSelfPrint("\r\n");
			// emberAfPrint(0xFFFF, "\r\n");
		#endif
		}
	}

	if (true == g_stBatteryInfo.fifoInitialized[HIGH_LEVEL]){
		if ((sample > temp_sample[mode_index]) \
			&& ((sample - temp_sample[mode_index]) > ((g_stBatteryInfo.maxVoltageMilliv - g_stBatteryInfo.minVoltageMilliv)/3))){
			//change battery without reset
			g_stBatteryInfo.fifoInitialized[HIGH_LEVEL] = false;
			g_stBatteryInfo.fifoInitialized[LOW_LEVEL] = false;
			emberEventControlSetDelayMS(kGetBatteryVoltageEventControl, DEFAULT_GETBATTERY_DELAY_S * 1000);
			kGetBatteryTriggerSet(10, DEFAULT_GETBATTERY_DELAY_S);
			emberAfPrint(0xffff, "INTO kGetBatteryTriggerSet 111\r\n");
		}

		if (true == g_stBatteryInfo.fifoInitialized[LOW_LEVEL]){
			// uint16_t BenchMark = (g_stBatteryInfo.maxVoltageMilliv - g_stBatteryInfo.minVoltageMilliv) * 100;
			// sample_out = temp_sample[HIGH_LEVEL] * (BenchMark - temp_sample[HIGH_LEVEL] + temp_sample[LOW_LEVEL]) / BenchMark;
			sample_out = (temp_sample[HIGH_LEVEL] + temp_sample[LOW_LEVEL]) / 2;
		}else{
			if (ENS_ONLINE != kNwkGetCurrentStatus()){
				sample_out = temp_sample[HIGH_LEVEL];
			}
		}
	}

	 emberAfPrint(0xFFFF, "###Sample result (%d) \r\n", sample_out);

	return sample_out;
}

#define MILLI_TO_DECI_CONVERSION_FACTOR 100
// Implemented callbacks

static void kBatteryMonitorWriteDataCallback(uint8_t endpoint, uint16_t voltageMilliV)
{
	EmberAfStatus afStatus;
	uint8_t voltageDeciV;
	uint8_t i;

	emberAfAppPrintln("New voltage reading: %d mV", voltageMilliV);

	// convert from mV to 100 mV, which are the units specified by zigbee spec for
	// the power configuration cluster's voltage attribute.
	voltageDeciV = ((uint8_t) (voltageMilliV / MILLI_TO_DECI_CONVERSION_FACTOR));

	// Cycle through all endpoints, check to see if the endpoint has a power
	// configuration server, and if so update the voltage attribute

	if (emberAfContainsServer(endpoint, ZCL_POWER_CONFIG_CLUSTER_ID)) {
		afStatus = emberAfWriteServerAttribute(endpoint,
											 ZCL_POWER_CONFIG_CLUSTER_ID,
											 ZCL_BATTERY_VOLTAGE_ATTRIBUTE_ID,
											 &voltageDeciV,
											 ZCL_INT8U_ATTRIBUTE_TYPE);

		if (EMBER_ZCL_STATUS_SUCCESS != afStatus) {
		emberAfAppPrintln("Power Configuration Server: failed to write value "
						  "0x%x to cluster 0x%x attribute ID 0x%x: error 0x%x",
						  voltageDeciV,
						  ZCL_POWER_CONFIG_CLUSTER_ID,
						  ZCL_BATTERY_VOLTAGE_ATTRIBUTE_ID,
						  afStatus);
		}
	}
}


uint8_t kkGetBatteryPowerPercent(uint16_t currentVoltageMilliV, uint16_t minVoltageMilliV, uint16_t maxVoltageMilliV, bool bIsWriteAttr)
{
	if (minVoltageMilliV >= maxVoltageMilliV){
		return 0;
	}
//	iKonkeAfSelfPrint("##########kkGetBatteryPowerPercent currentVoltage(%dmv) minVolage(%dmv) maxVoltage(%dmv) isWrite(%d)\r\n",
//			currentVoltageMilliV, minVoltageMilliV, maxVoltageMilliV, bIsWriteAttr);
	uint8_t percent = 100 * 2;
	if (currentVoltageMilliV >= maxVoltageMilliV){
		percent =  100 * 2;
	}else if (currentVoltageMilliV <= minVoltageMilliV){
		percent = 1 * 2;
	}else {
		percent = (uint8_t)(( 2 * 100 * (currentVoltageMilliV - minVoltageMilliV)) / (maxVoltageMilliV - minVoltageMilliV));
	}

	if(percent >= 200){
		percent = 200;
	}
	if(percent <= 2){
		percent = 2;
	}
	if (bIsWriteAttr){
		EmberAfStatus err = emberAfWriteServerAttribute(1, ZCL_POWER_CONFIG_CLUSTER_ID, ZCL_BATTERY_PERCENTAGE_REMAINING_ATTRIBUTE_ID, &percent, ZCL_INT8U_ATTRIBUTE_TYPE);
//		iKonkeAfSelfPrint("##########Write Battery Err = 0x%0X, Voltage = %dmv, percent = %d%%\r\n", err, currentVoltageMilliV, (percent/2));
		 emberAfPrint(0xFFFF, "##########Write Battery Err = 0x%0X, Voltage = %dmv, percent = %d%%\r\n", err, currentVoltageMilliV, (percent/2));
	}
//	iKonkeAfSelfPrint("\r\battery percent data = %d%% \r\n", percent);
	return percent;
}

void kAdcModuleInit(AdcConfSt conf_list[], uint8_t adc_num,	uint32_t referenceVoltage)
{
	if( conf_list && (adc_num > 0)) {
		adc_num = (adc_num < ADC_CHANNEL_MAX_NUM)?adc_num:ADC_CHANNEL_MAX_NUM;

		for (uint8_t i = 0; i < adc_num; i++) {
			memcpy(&g_stAdcValueList[i].adcConf, &conf_list[i], sizeof(AdcConfSt));
		}
		//kAdcInitialize(adc_num, referenceVoltage);
//		iKonkeAfSelfPrint("111conf_list[0] isGpio(%d)port(%d)pin(%d)posSel(%d)\r\n", \
//				conf_list[0].directSampleGpio, conf_list[0].port, conf_list[0].pin, conf_list[0].posSel);
//		iKonkeAfSelfPrint("111conf_list[1] isGpio(%d)port(%d)pin(%d)posSel(%d)\r\n", \
//				conf_list[1].directSampleGpio, conf_list[1].port, conf_list[1].pin, conf_list[1].posSel);
//
//		iKonkeAfSelfPrint("222conf_list[0] isGpio(%d)port(%d)pin(%d)posSel(%d)\r\n", \
//				g_stAdcValueList[0].adcConf.directSampleGpio,g_stAdcValueList[0].adcConf.port, g_stAdcValueList[0].adcConf.pin, g_stAdcValueList[0].adcConf.posSel);
//		iKonkeAfSelfPrint("222conf_list[1] isGpio(%d)port(%d)pin(%d)posSel(%d)\r\n", \
//						g_stAdcValueList[1].adcConf.directSampleGpio,g_stAdcValueList[1].adcConf.port, g_stAdcValueList[1].adcConf.pin, g_stAdcValueList[1].adcConf.posSel);
	}else {
//		iKonkeAfSelfPrint("Err:conf_list is NULL or num is 0\r\n");
	}
	g_u8AdcNum = adc_num;
	//g_u16ReferenceVoltage = referenceVoltage;
}



uint32_t kGetAdcSampleValueById(uint8_t id)
{

	uint32_t flags;
	uint32_t vData;

#if 0
	if (g_u8CurrentUseAdcId == id) {
		//not init when current adc is using
//		iKonkeAfSelfPrint("kGetAdcSampleValueById ADC id(%d)is not init\r\n", id);
	}else {
		kAdcInitialize(id);
		g_u8CurrentUseAdcId = id;
	}
#if !defined(TIMER2) && !defined(TIMER3)
    ADC_InitSingle_TypeDef initAdc = ADC_INITSINGLE_BATTERY_VOLTAGE;
	uint32_t posSel1 = 0,  posSel2 = 0, posSel3 = 0;
	uint8_t adcIndex = kGetPosSelByAdcId(id, &posSel1, &posSel2, &posSel3);
	initAdc.posSel = (ADC_PosSel_TypeDef)posSel1;
	if (adcIndex != 0xFF) {
		initAdc.reference = (ADC_Ref_TypeDef)g_stAdcValueList[adcIndex].adcConf.referenceVoltageMV;
	}
  //emberEventControlSetInactive(emberAfPluginBatteryMonitorReadADCEventControl);

  // In case something else in the system was using the ADC, reconfigure it to
  // properly sample the battery voltage
  	ADC_InitSingle(ADC0, &initAdc);

    // The most common and shortest (other than the ACK) transmission is the
    // data poll.  It takes 512 uS for a data poll, which is plenty of time for
    // a 16 cycle conversion
    flags = ADC_IntGet(ADC0);
    ADC_IntClear(ADC0, flags);
    ADC_Start(ADC0, adcStartSingle);

    // wait for the ADC to finish sampling
    while ((ADC_IntGet(ADC0) & ADC_IF_SINGLE) != ADC_IF_SINGLE) {
    }
    vData = ADC_DataSingleGet(ADC0);


#else

    // Start IADC conversion
       IADC_command(IADC0, iadcCmdStartSingle);

       // Wait for conversion to be complete
       while(!(IADC0->STATUS & (_IADC_STATUS_CONVERTING_MASK
               				| _IADC_STATUS_SINGLEFIFODV_MASK)));

       // Get ADC result
       vData = IADC_readSingleData(IADC0);

#endif
#endif

	return vData;
}

static uint8_t kAdcFindIndexById(uint8_t id)
{
	uint8_t index = 0xFF;
	for (uint8_t i = 0; i < ADC_CHANNEL_MAX_NUM; i++) {
		if (g_stAdcValueList[i].adcConf.adcId == id) {
			index = i;
			break;
		}
	}
	return index;
}

static uint8_t kFindAdcPosSelUsefullByGpio(GPIO_Port_TypeDef port, unsigned int pin, uint32_t *adcPosSel)
{
	if (adcPosSel == NULL) {
		iKonkeAfSelfPrint("Err:adcPosSel is NULL\r\n");
		return false;
	}
	uint8_t index = 0xFF;
#if !defined(TIMER2) && !defined(TIMER3)
	for (uint8_t i = 0; i < sizeof(g_stAdc0Table) / sizeof(Adc0TableSt); i++) {
		if (g_stAdc0Table[i].port == port && g_stAdc0Table[i].pin == pin  \
				&& g_stAdc0Table[i].bCurrentChannelUsed != true) {
			*adcPosSel = g_stAdc0Table[i].posSel1; //select sel1, just select one
			return index;
		}
	}
#else

#endif
	return index;
}

static bool kFindAdcPosSelUsefullByPoseSel(uint32_t adcPosSel)
{
#if !defined(TIMER2) && !defined(TIMER3)
	uint8_t channel = 1;
	for (uint8_t i = 0; i < sizeof(g_stAdc0Table) / sizeof(Adc0TableSt); i++) {
		if (g_stAdc0Table[i].posSel1 == adcPosSel || g_stAdc0Table[i].posSel2 == adcPosSel) {
			if (g_stAdc0Table[i].bCurrentChannelUsed != true) {
				channel = g_stAdc0Table[i].adc0Channel;
				break;
			}else {
				return false;
			}
		}
	}

	for (uint8_t j = 0; j < sizeof(g_stAdc0Table) / sizeof(Adc0TableSt); j++) {
		if (g_stAdc0Table[j].adc0Channel == channel && g_stAdc0Table[j].bCurrentChannelUsed == true) {
			return false;
		}
	}
#else

#endif
	return true;
}
//check adc channel is used
static bool kCheckAdcPosSelChannelUsedByGpio(GPIO_Port_TypeDef port, unsigned int pin)
{
#if !defined(TIMER2) && !defined(TIMER3)
	uint8_t channel = 1;

	for (uint8_t i = 0; i < sizeof(g_stAdc0Table) / sizeof(Adc0TableSt); i++) {
		if (g_stAdc0Table[i].port == port && g_stAdc0Table[i].pin == pin) {
			if (g_stAdc0Table[i].bCurrentChannelUsed == true) {
				return true;
			}else {
				channel = g_stAdc0Table[i].adc0Channel;
				break;
			}
		}
	}

	for (uint8_t j = 0; j < sizeof(g_stAdc0Table) / sizeof(Adc0TableSt); j++) {
		if (g_stAdc0Table[j].adc0Channel == channel && g_stAdc0Table[j].bCurrentChannelUsed == true) {
			return true;
		}
	}
#else

#endif
	return false;
}
static uint8_t kFindAdcPosSelIndexByPosSel(uint32_t adcPosSel)
{
	uint8_t index = 0xFF;
#if !defined(TIMER2) && !defined(TIMER3)
	for (uint8_t i = 0; i < sizeof(g_stAdc0Table) / sizeof(Adc0TableSt); i++) {
		if ((g_stAdc0Table[i].posSel1 == adcPosSel || g_stAdc0Table[i].posSel2 == adcPosSel) \
				&& g_stAdc0Table[i].bCurrentChannelUsed != true) {
			return index;
		}
	}
#else

#endif
	return index;
}

static uint8_t kGetPosSelByAdcId(uint8_t id, uint32_t *u32Pos1, uint32_t *u32Pos2, uint32_t *u32Pos3)
{
	uint8_t adcIndex = kAdcFindIndexById(id);
	if (adcIndex == 0xFF || u32Pos1 == NULL || u32Pos2 == NULL || u32Pos2 == NULL) {
//		iKonkeAfSelfPrint("Err: kGetPosSelByAdcId is Invalid id(%d)\r\n", id);
		return 0xFF;
	}
#if !defined(TIMER2) && !defined(TIMER3)
	//not use gpio to find channel
	if (g_stAdcValueList[adcIndex].adcConf.directSampleGpio == false) {

		if (g_stAdcValueList[adcIndex].adcConf.posSel <= adcPosSelAPORT4XCH31) {
			uint8_t tableIndex = kFindAdcPosSelIndexByPosSel(g_stAdcValueList[adcIndex].adcConf.posSel);
			if (tableIndex != 0xFF && kFindAdcPosSelUsefullByPoseSel(g_stAdcValueList[adcIndex].adcConf.posSel) == true) {
			  //set adc channel status change to used
			    g_stAdc0Table[tableIndex].bCurrentChannelUsed = true;
//			    iKonkeAfSelfPrint("####111adcConf.PosSel(0x%x)\r\n", g_stAdcValueList[adcIndex].adcConf.posSel);
			  //ADC_ScanSingleEndedInputAdd(&initScan, adcScanInputGroup0 + i, g_stAdcValueList[i].adcConf.PosSe);
			}else {
//			    iKonkeAfSelfPrint("Err:Direct use channel  is used index(%d)\r\n", tableIndex);
			}
		}else {
//			iKonkeAfSelfPrint("#######Direct use VDD or Temperature\r\n");
			// ADC_ScanSingleEndedInputAdd(&initScan, adcScanInputGroup0 + i, g_stAdcValueList[i].adcConf.PosSe);
		}
		*u32Pos1 = g_stAdcValueList[adcIndex].adcConf.posSel;
	}else {
		uint32_t posSel = 0;
		bool bUsed = kCheckAdcPosSelChannelUsedByGpio(g_stAdcValueList[adcIndex].adcConf.port, g_stAdcValueList[adcIndex].adcConf.pin);
		if (bUsed != true) {

		 bool index =  kFindAdcPosSelUsefullByGpio(g_stAdcValueList[adcIndex].adcConf.port, g_stAdcValueList[adcIndex].adcConf.pin, &posSel);
		 if (index != 0xFF) {
			  // Select ADC input. See README for corresponding EXP header pin.
			  // Add VDD to scan for demonstration purposes
			  //ADC_ScanSingleEndedInputAdd(&initScan, adcScanInputGroup0 + i, posSel);
//			 iKonkeAfSelfPrint("####222adcConf.PosSe(0x%x)\r\n", posSel);
		 }else {
//			 iKonkeAfSelfPrint("Err:111Adc Channel is used id(%d)\r\n", id);
		 }
		}else {
//		  iKonkeAfSelfPrint("Err:222Adc Channel is used id(%d)\r\n", id);
		}
		*u32Pos1 = posSel;
	}
#else
	uint8_t tableIndex = 0;
	if (g_stAdcValueList[adcIndex].adcConf.directSampleGpio == false) {

	//	if (g_stAdcValueList[adcIndex].adcConf.posSel <= iadcPosInputPosRef) {
	//		iKonkeAfSelfPrint("####111adcConf.PosSel(0x%x)\r\n", g_stAdcValueList[adcIndex].adcConf.posSel);
	//	} else {
			//find bus
	//		tableIndex = kFindAdcPosSelIndexByPosSel(g_stAdcValueList[adcIndex].adcConf.posSel);
	//	}
		//uint8_t tableStartIndex = kFindAdcPosSelIndexByPosSel(g_stAdcValueList[adcIndex].adcConf.posSel);
		*u32Pos1 = g_stAdcValueList[adcIndex].adcConf.posSel;
		if (tableIndex != 0xFF) {

		}

	} else {
		uint32_t posSel = 0;
		tableIndex =  kFindAdcPosSelUsefullByGpio(g_stAdcValueList[adcIndex].adcConf.port, g_stAdcValueList[adcIndex].adcConf.pin, &posSel);
		if (tableIndex != 0xFF) {

		}
		*u32Pos1 = posSel;
	}
#endif
	return adcIndex;
}
#if 1
/**************************************************************************//**
 * @brief Convert ADC sample values to celsius.
 * @detail See section 25.3.4.1 in the reference manual for detail on
 *   temperature measurement and conversion.
 * @param adcSample Raw value from ADC to be converted to celsius
 * @return The temperature in degrees celsius.
 *****************************************************************************/
float kGetTemperatureByAdcId(uint8_t id)
{
	uint32_t calTemp0;
	uint32_t calValue0;
	int32_t readDiff;
	float temp;

	uint8_t index = kAdcFindIndexById(id);
	if (index == 0xFF) {
//		iKonkeAfSelfPrint("Err: Get Temperature is invalid id(%d)\r\n", id);
		return 0;
	}
#if !defined(TIMER2) && !defined(TIMER3)
	/* Factory calibration temperature from device information page. */
	calTemp0 = ((DEVINFO->CAL & _DEVINFO_CAL_TEMP_MASK)
			  >> _DEVINFO_CAL_TEMP_SHIFT);
	//Get adc value when 25 degC
	calValue0 = ((DEVINFO->ADC0CAL3
				/* _DEVINFO_ADC0CAL3_TEMPREAD1V25_MASK is not correct in
					current CMSIS. This is a 12-bit value, not 16-bit. */
				& 0xFFF0)
			   >> _DEVINFO_ADC0CAL3_TEMPREAD1V25_SHIFT);

	if ((calTemp0 == 0xFF) || (calValue0 == 0xFFF))
	{
		/* The temperature sensor is not calibrated */
		return -100.0;
	}
	//get adc value
//	kGetVoltageValueById(id, 1, true, false,  0);
	kGetVoltageValueById(id, 1, HIGH_LEVEL, true, false, 0);
	//kAdcStart();
//	u16TimeoutCounter = ADC_GET_TIMEOUT_COUNTER;
//	while (u16TimeoutCounter-- && g_bAdcScanDone != true) {
//		halCommonDelayMicroseconds(2);
//	}
//	if (g_bAdcScanDone == true) {
		/* TGRAD_ADCTH = 1.84 mV/degC (from datasheet)
		*/
		readDiff = calValue0  - g_stAdcValueList[index].adcValue;
		temp     = ((float)readDiff * 1250) / (4096 * 1.84) * (-1);


		/* Calculate offset from calibration temperature */
		temp     = (float)calTemp0 - temp;
//	}
#else

#endif

	iKonkeAfSelfPrint("#calValue0(%d)calTemp0(%d)diff(%d)adcValue(%d)Get Temperature(%d degC) tmp*100(%d)\r\n", \
			calValue0, calTemp0, readDiff, g_stAdcValueList[index].adcValue, (int16_t)temp, (int16_t)(temp * 100));
	return temp;
}
#endif
void kBatteryModuleInit(uint8_t adc_index, uint8_t endpoint, uint16_t minVoltageMilliv, uint16_t maxVoltageMilliv)
{
#if 0
	if (adc_index != ADC_UNKNOW_ID){
		g_stBatteryInfo.adcIndex = adc_index; 
	}
	if (endpoint != 0xff){
		g_stBatteryInfo.endpoint = endpoint;
	}
	g_stBatteryInfo.minVoltageMilliv = minVoltageMilliv;
	g_stBatteryInfo.maxVoltageMilliv = maxVoltageMilliv;
	g_stBatteryInfo.lastReportedVoltageMilliV = maxVoltageMilliv;

	uint16_t temp_value = 0;
	if (0x0601 == halGetExtendedResetInfo()){
		halCommonGetToken(&temp_value, TOKEN_BATTERY_VOLTAGE);
		if (temp_value != 0){
			g_stBatteryInfo.lastReportedVoltageMilliV = temp_value;
		}
	}
	temp_value = 0;
	halCommonSetToken(TOKEN_BATTERY_VOLTAGE, &temp_value);

	emberAfPrint(0xFFFF, "!!!INTO kBatteryModuleInit lastReportedVoltageMilliV = %dmv 111 !!!\r\n",g_stBatteryInfo.lastReportedVoltageMilliV);
	kBatteryMonitorWriteDataCallback(1, g_stBatteryInfo.lastReportedVoltageMilliV);
	kkGetBatteryPowerPercent(g_stBatteryInfo.lastReportedVoltageMilliV, minVoltageMilliv, maxVoltageMilliv, true);

	kGetBatteryTriggerSet(10, DEFAULT_GETBATTERY_DELAY_S);
	emberAfPrint(0xffff, "INTO kGetBatteryTriggerSet 222\r\n");
#endif
}

uint16_t kBatteryGetCurrentVoltage(void)
{
    return g_stBatteryInfo.lastReportedVoltageMilliV;
}

void kBatterySetLockTime(void)
{
	g_stBatteryInfo.getBatteryLockTime = halCommonGetInt32uMillisecondTick();
}

void kBatterySetToken(void)
{
	//halCommonSetToken(TOKEN_BATTERY_VOLTAGE, &g_stBatteryInfo.lastReportedVoltageMilliV);
}

void kBatteryGetVoltageWithMsgSent(void)
{
	uint32_t current_time = halCommonGetInt32uMillisecondTick();
	if (((current_time > g_stBatteryInfo.getBatteryLockTime) && ((current_time - g_stBatteryInfo.getBatteryLockTime)> 40 * 1000)) \
		|| ((current_time < g_stBatteryInfo.getBatteryLockTime) && ((g_stBatteryInfo.getBatteryLockTime - current_time) < (0xffff - 40 * 1000)))){
	    kZclClusterSetPermitReportInfo(1, ZCL_POWER_CONFIG_CLUSTER_ID, true, false, true, false);
		uint16_t u16BatteryVotageMV = kGetVoltageValueById(g_stBatteryInfo.adcIndex, g_stBatteryInfo.endpoint, LOW_LEVEL, false, true, 0);
		kkGetBatteryPowerPercent(u16BatteryVotageMV, g_stBatteryInfo.minVoltageMilliv, g_stBatteryInfo.maxVoltageMilliv, true);
	}

	g_stBatteryInfo.getBatteryLockTime = current_time;
}

void kGetBatteryTriggerSet(uint8_t get_times, uint16_t delay_s)
{
	emberAfPrint(0xffff, "kGetBatteryTriggerSet:times(%d), delay time(%d)\r\n", get_times, delay_s);
	if (g_stBatteryInfo.getTimes < get_times){
		g_stBatteryInfo.getTimes = get_times;
	}

	uint32_t remain_time = emberEventControlGetRemainingMS(kGetBatteryVoltageEventControl);
	//if ((MAX_INT32U_VALUE == remain_time) || (delay_s * 1000 > remain_time)){
		//emberEventControlSetDelayMS(kGetBatteryVoltageEventControl, delay_s * 1000);
	//}
}

void kGetBatteryVoltageEventHandler(void)
{
    emberEventControlSetInactive(kGetBatteryVoltageEventControl);

	if (--g_stBatteryInfo.getTimes == 0){
	    kZclClusterSetPermitReportInfo(1, ZCL_POWER_CONFIG_CLUSTER_ID, true, false, true, false);
		uint16_t u16BatteryVotageMV = kGetVoltageValueById(g_stBatteryInfo.adcIndex, g_stBatteryInfo.endpoint, HIGH_LEVEL, false, true, 0);
		kkGetBatteryPowerPercent(u16BatteryVotageMV, g_stBatteryInfo.minVoltageMilliv, g_stBatteryInfo.maxVoltageMilliv, true);
		g_stBatteryInfo.getTimes = 1;
		g_stBatteryInfo.delayTimeS = GETBATTERY_LOOPTIME_SEC;
		if (ENS_ONLINE == kNwkGetCurrentStatus() && g_stBatteryInfo.fifoInitialized[LOW_LEVEL] == false){
//			kOptTunnelCommonReport(ECA_OPTDATA);
		    // kZclClusterHeartbeatControl(1, ZCL_POWER_CONFIG_CLUSTER_ID, ZCL_BATTERY_PERCENTAGE_REMAINING_ATTRIBUTE_ID);
			//kkPollProceduleTrigger(3, true);
		}
	}else{
		kGetVoltageValueById(g_stBatteryInfo.adcIndex, g_stBatteryInfo.endpoint, HIGH_LEVEL, false, false, 0);
		g_stBatteryInfo.delayTimeS = 1;
	}

    emberEventControlSetDelayMS(kGetBatteryVoltageEventControl, g_stBatteryInfo.delayTimeS * 1000);
}


