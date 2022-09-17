#include <00ikonke-app/driver/ikk-relay.h>
#include "stdint.h"
#include "stdbool.h"
#include "stack/include/ember-types.h"
#include "em_gpio.h"
#include "ikk-button.h"
#include "ikk-timer.h"
#include "ikk-pwm.h"
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
//#include "rtcdriver.h"
//#include "generic-interrupt-control/generic-interrupt-control-efr32.h"
#include EMBER_AF_API_GENERIC_INTERRUPT_CONTROL

#include "../general/ikk-tick-handler.h"
#include "../general/ikk-debug.h"
#include "em_gpio.h"
//#include "em_int.h"
#include "ikk-timer.h"

#define RELAY_CHNN_UNUSED_ID		RELAY_CHNN_UNKNOW_ID

//RELAY ZERO DECTED
#define ZERO_TIMEOUT_MS				200
#define ZERO_CHECK_POLL_TIME		500

static RelayChannelConfSt g_stRelayChannelConfList[RELAY_OBJS_SUPPORT_MAXN] = { 0 };
static RelayStatusSt g_stRelayStatusList[RELAY_OBJS_SUPPORT_MAXN] = {{RELAY_CHNN_UNKNOW_ID, EZAO_DEFAULT},{RELAY_CHNN_UNKNOW_ID, EZAO_DEFAULT},{RELAY_CHNN_UNKNOW_ID, EZAO_DEFAULT},{RELAY_CHNN_UNKNOW_ID, EZAO_DEFAULT},{RELAY_CHNN_UNKNOW_ID, EZAO_DEFAULT}};
static RelayStatusSt g_stRelayStatusListBkp[RELAY_OBJS_SUPPORT_MAXN] = {{RELAY_CHNN_UNKNOW_ID, EZAO_DEFAULT},{RELAY_CHNN_UNKNOW_ID, EZAO_DEFAULT},{RELAY_CHNN_UNKNOW_ID, EZAO_DEFAULT},{RELAY_CHNN_UNKNOW_ID, EZAO_DEFAULT},{RELAY_CHNN_UNKNOW_ID, EZAO_DEFAULT}};

static uint32_t g_u32CheckedZeroCount = 0;  //��⵽�������
static bool g_bIsCheckedZeroFlg = false;// ���ϵ�10S�ڻ�û�н�220������Ϊfalse,���ؼ����㼴�ɴ����̵������� ,����true
static uint8_t  g_u8RelayChannelNum = 0;
static uint32_t g_u32RelayDelayOnTimeUS = 0;
static uint32_t g_u32RelayDelayOffTimeUS = 0;
static uint32_t g_u32RelayZeroCheckCountdownCounter = 0;

pRelayOptLedCallback g_pRelayOptLedCallback = NULL;

//EmberEventControl kRelayOnOffEventControl;


static uint8_t kRelayGetChannelIndexByID(uint8_t channel_id );
static void kRelayIRQCallback(uint8_t irq_no );
static void kRelayGpioIRQInit(GPIO_Port_TypeDef port, uint8_t pin, RelayPolarityEnum ePolarity);
static void kRelayGpioInit(GPIO_Port_TypeDef port, uint8_t pin, bool out);

static void judgeCheckZeroIsUsefullPoll(uint16_t poll_time_ms, uint16_t time_out_ms);
static void kRelayGpioControl(GPIO_Port_TypeDef port, uint8_t pin, bool status);
static void kRelayChannelControl(uint8_t channel, bool status);

void kRelayUserCallback( void );
/*	DESP: get the channel index by channel_id.
 * 	Auth: dingmz_frc.20191107.
 * */
static uint8_t kRelayGetChannelIndexByID(uint8_t channel_id )
{
	for(int Relay_index = 0; Relay_index < RELAY_OBJS_SUPPORT_MAXN; ++Relay_index ) {
		if( g_stRelayChannelConfList[Relay_index].u8RelayChannelId != RELAY_CHNN_UNUSED_ID ) {
			if( g_stRelayChannelConfList[Relay_index].u8RelayChannelId == channel_id ) {
				return Relay_index;
			}
		}else break;
	}

	return 0XFF;
}


/*	DESP: Zero-Access channel operate interface.
	Auth: dingmz_frc.20191108.
*/
void kRelayChannelOpt(uint8_t channel_id, RelayOptEnum opt )
{
	uint8_t chnn_index = kRelayGetChannelIndexByID(channel_id);

	if( chnn_index  == 0xFF) {
//		iKonkeAfSelfPrint("Err:Relay OPT\r\n");
		return ;
	}
	//add by maozj 20200708
	g_u32RelayZeroCheckCountdownCounter = MS2COUNT(ZERO_CHECK_POLL_TIME);
	kTickRunnningTrigger();

//	uint32_t u32TimeMS = halCommonGetInt32uMillisecondTick();
//	iKonkeAfSelfPrint("\r\n####Relay-OPT 11111: channel(%d), operate(%d) time(%d), count(%d)\r\n", channel_id, opt, u32TimeMS, g_u32CheckedZeroCount);
	g_stRelayStatusList[chnn_index].opt = opt;
	g_stRelayStatusList[chnn_index].u8RelayChannelId = channel_id;
	uint32_t u32TimeMS = halCommonGetInt32uMillisecondTick();
	if (g_bIsCheckedZeroFlg == true){
		//delay on off relay when  zero detected, on or off delay time is different.
		//emberEventControlSetDelayMS(kRelayOnOffEventControl, ZERO_TIMEOUT_MS);//add by maozj 20200314
//		iKonkeAfSelfPrint("####Relay Module Checked Zero Gpio time(%d) count(%d)\r\n", u32TimeMS, g_u32CheckedZeroCount);
	}else {
		//direct on off relay when not detect zero interrupt
//		iKonkeAfSelfPrint("####Relay Modul Not Checked Zero Gpio time(%d) count(%d)\r\n", u32TimeMS, g_u32CheckedZeroCount);
		memcpy(&g_stRelayStatusListBkp[chnn_index], &g_stRelayStatusList[chnn_index], sizeof(RelayStatusSt));
		g_stRelayStatusList[chnn_index].opt = EZAO_DEFAULT;
#if 0
		//emberEventControlSetDelayMS(kRelayOnOffEventControl, 10);
		kRelayUserCallback();
#else
		if (kTimerIsGoing(KTIMER3) != true){
			kTimerStart(KTIMER3, true, (g_stRelayStatusListBkp[chnn_index].opt ==  EZAO_OFF)?g_u32RelayDelayOffTimeUS:g_u32RelayDelayOnTimeUS, kRelayUserCallback, NULL);
		}
#endif
	}
}
#if 0
void kRelayOnOffEventHandler(void)
{
	emberEventControlSetInactive(kRelayOnOffEventControl);
#if 0
	//GPIOINT_Init();
	for (uint8_t i = 0; i < g_u8RelayChannelNum; i++){
		if (g_stRelayStatusListBkp[i].opt == EZAO_ON || g_stRelayStatusListBkp[i].opt == EZAO_OFF){
			RelayOptEnum opt = g_stRelayStatusListBkp[i].opt;
			kRelayChannelControl(g_stRelayStatusListBkp[i].u8RelayChannelId, opt ==  EZAO_OFF?false:true);
			//operate led status
			if (g_pRelayOptLedCallback){
				g_pRelayOptLedCallback(g_stRelayStatusListBkp[i]);
			}
			uint32_t u32TimeMS = halCommonGetInt32uMillisecondTick();
			iKonkeAfSelfPrint("\r\n####Relay-OPT 33333: channel(%d), operate(%d) time(%d) count(%d)\r\n",  g_stRelayStatusListBkp[i].u8RelayChannelId, opt, u32TimeMS, g_u32CheckedZeroCount);
			g_stRelayStatusListBkp[i].opt = EZAO_DEFAULT;
		}
	}
	//__enable_irq();   //��ȫ���ж�
	//CORE_ATOMIC_IRQ_ENABLE();
#endif
}
#endif
void kRelayUserCallback( void )
{
//	(void) user;
//	kRelayOnOffEventHandler();
//	extern bool g_bIsTimer2Going ;
//	g_bIsTimer2Going = false;
	for (uint8_t i = 0; i < g_u8RelayChannelNum; i++){
		if (g_stRelayStatusListBkp[i].opt == EZAO_ON || g_stRelayStatusListBkp[i].opt == EZAO_OFF){
			uint32_t u32TimeMS = halCommonGetInt32uMillisecondTick();
//			iKonkeAfSelfPrint("\r\n####Relay-OPT 44444: channel(%d), operate(%d) time(%d) count(%d)\r\n" \
//					, g_stRelayStatusListBkp[i].u8RelayChannelId, g_stRelayStatusListBkp[i].opt, u32TimeMS, g_u32CheckedZeroCount);
			RelayOptEnum opt = g_stRelayStatusListBkp[i].opt;
			kRelayChannelControl(g_stRelayStatusListBkp[i].u8RelayChannelId, opt ==  EZAO_OFF?false:true);
			//operate led status
			if (g_pRelayOptLedCallback){
				g_pRelayOptLedCallback(g_stRelayStatusListBkp[i]);
			}

			g_stRelayStatusListBkp[i].opt = EZAO_DEFAULT;
		}
	}
}

static void kRelayIRQCallback(uint8_t irq_no )
{
	if (g_u32CheckedZeroCount++ > 0xFFFFFFF){
		g_u32CheckedZeroCount = 0;
	}
	//GPIO_PinOutToggle(gpioPortA, 3);

#if 1
	for (uint8_t i = 0; i < g_u8RelayChannelNum; i++)
	{
		if (g_stRelayStatusList[i].opt == EZAO_ON || g_stRelayStatusList[i].opt == EZAO_OFF){
			//CORE_irqState_t state = CORE_EnterCritical();
			memcpy(&g_stRelayStatusListBkp, &g_stRelayStatusList, sizeof(g_stRelayStatusListBkp));
//			memset(&g_stRelayStatusList, 0xFF, sizeof(g_stRelayStatusList));

//			RelayOptEnum opt = g_stRelayStatusListBkp[i].opt;
//
			RelayOptEnum opt = g_stRelayStatusListBkp[i].opt;
			g_stRelayStatusList[i].opt = EZAO_DEFAULT;
//			NVIC_DisableIRQ(GPIO_EVEN_IRQn);//�����ж�
//			NVIC_DisableIRQ(GPIO_ODD_IRQn);
//			CORE_ATOMIC_IRQ_DISABLE();
			//__disable_irq();   //�ر�ȫ���ж�
#if 0
			if (emberEventControlGetActive(kRelayOnOffEventControl) != true){
				//uint32_t u32TimeMS = halCommonGetInt32uMillisecondTick();
				//iKonkeAfSelfPrint("\r\n####Relay-OPT 22222: channel(%d), operate(%d) time(%d) count(%d)\r\n",  g_stRelayStatusList[i].u8RelayChannelId, opt, u32TimeMS, g_u32CheckedZeroCount);
				emberEventControlSetDelayMS(kRelayOnOffEventControl, \
							opt ==  EZAO_OFF?g_u32RelayDelayOffTimeUS/1000:g_u32RelayDelayOnTimeUS/1000);
			}
# else
			if (kTimerIsGoing(KTIMER3) != true){
				kTimerStart(KTIMER3, true, (opt ==  EZAO_OFF)?g_u32RelayDelayOffTimeUS:g_u32RelayDelayOnTimeUS, kRelayUserCallback, NULL);
			}
#endif
			//CORE_ExitCritical(state);
			break;
		}
	}
#endif
}
/*	DESP: zero detect gpio init interface.
 * 	Auth: maozj.20191108.
 * */
static void kRelayGpioIRQInit(GPIO_Port_TypeDef port, uint8_t pin, RelayPolarityEnum ePolarity)
{
	if (port == PORT_UNKNOW || pin == PIN_UNKNOW){
//		iKonkeAfSelfPrint("###Erro:kRelayGpioIRQInit Port or Pin\r\n");
		return;
	}

	GPIO_PinModeSet(port, pin, gpioModeInput, (ePolarity == EZAP_HIGH)?0:1);
	/* Register callbacks before setting up and enabling pin interrupt.  ef32mg21 max pin is 7*/

	uint8_t irqIndex = kBtnGetAssignUsefullIndex();
	uint8_t irqNum = 0;
	if (irqIndex == 0xFF){
//		iKonkeAfSelfPrint("###Erro:Assin irqIndex\r\n");
	}else {
//		iKonkeAfSelfPrint("###Assin irqIndex is %d\r\n", irqIndex);
		irqNum = kBtnAssignIRQNO(irqIndex, pin);
		if (irqNum == 0xFF){
//				iKonkeAfSelfPrint("###Erro:Assin Irq Number\r\n");
		}
	}

	GPIOINT_CallbackRegister(irqNum, kRelayIRQCallback);

	if(ePolarity == EZAP_HIGH ) {
		/* Set rising and falling edge interrupts */
		GPIO_ExtIntConfig(port, pin, irqNum, true, false, true);
	}else  if (ePolarity == EZAP_LOW) {
		/* Set rising and falling edge interrupts */
		GPIO_ExtIntConfig(port, pin, irqNum, false, true, true);
	}else {
		GPIO_ExtIntConfig(port, pin, irqNum, true, true, true);
	}
//	uint32_t  u32Priority = NVIC_GetPriority(irqNum);
//	iKonkeAfSelfPrint("#####1111RelayIrqNum = %d u32Priority()%d\r\n", irqNum, u32Priority);
//	NVIC_SetPriority(irqNum, 0);
//	u32Priority = NVIC_GetPriority(irqNum);
//	iKonkeAfSelfPrint("#####2222RelayIrqNum = %d u32Priority()%d\r\n", irqNum, u32Priority);
//	NVIC_SetPriority(SysTick_IRQn, 0);
}

static void kRelayGpioInit(GPIO_Port_TypeDef port, uint8_t pin, bool out)
{
	GPIO_PinModeSet(port, pin, gpioModePushPull, out);
}

static void kRelayGpioControl(GPIO_Port_TypeDef port, uint8_t pin, bool status)
{
	if (status == true){
		GPIO_PinOutSet(port, pin);
	}else {
		GPIO_PinOutClear(port, pin);
	}
}

static void kRelayChannelControl(uint8_t channel, bool status)
{
	uint8_t channelIndex = kRelayGetChannelIndexByID(channel);
	if (channelIndex == 0xFF){
//		iKonkeAfSelfPrint("Erro:Get Channel Index\r\n");
		return;
	}

	RelayPolarityEnum ePolarity = g_stRelayChannelConfList[channelIndex].eActionPolarity;
	if (ePolarity == EZAP_LOW){
		status = !status;
	}

	for (uint8_t i = 0; i < g_stRelayChannelConfList[channelIndex].u8GpioMappNum; i++){
		if (g_stRelayChannelConfList[channelIndex].gpios[i].port != PORT_UNKNOW \
				&& g_stRelayChannelConfList[channelIndex].gpios[i].pin != PIN_UNKNOW){
			kRelayGpioControl(g_stRelayChannelConfList[channelIndex].gpios[i].port, g_stRelayChannelConfList[channelIndex].gpios[i].pin, status);
		}
	}
}
/*	DESP: Relay control module Initialize configuration interface.alse zero gpio is not necessary
 * 	PARAM[port]: the GPIO port for Zero-Access detection.
 * 	PARAM[pin]: the GPIO pin for Zero-Access detection.
 *  PARAM[zero_on_time]: relay delay on time when get zero interrupt
 *  PARAM[zero_off_time]: relay delay off time when get zero interrupt
 *  PARAM[ePolarity]: the Zero GPIO pin interrupt edge
 * 	PARAM[list]: the config list of all relay dection
 * 	PARAM[channel_num]: total group number of relay.
 * 	PARAM[callback]: callback function after relay operate done
 * 	Auth: maozj.20191115.
 * */
kk_err_t kRelayModuleInit(GPIO_Port_TypeDef port, uint8_t pin, uint32_t zero_on_time, uint32_t zero_off_time \
		, RelayPolarityEnum ePolarity,  RelayChannelConfSt *list, uint8_t channel_num,  pRelayOptLedCallback callback)
{
	kk_err_t err = KET_OK;

	memset(g_stRelayChannelConfList, RELAY_CHNN_UNUSED_ID, sizeof(g_stRelayChannelConfList));

	if( list && (channel_num > 0)) {
		g_u8RelayChannelNum = (channel_num < RELAY_OBJS_SUPPORT_MAXN)?channel_num:RELAY_OBJS_SUPPORT_MAXN;

		memcpy(g_stRelayChannelConfList, list, sizeof(RelayChannelConfSt) * g_u8RelayChannelNum);
	}else {
		err = KET_FAILED;
//		iKonkeAfSelfPrint("Err: Relay Parameter\r\n");
	}
	//interrupt init zero detect gpio
	kRelayGpioIRQInit(port, pin, ePolarity);

	for (uint8_t i = 0; i < g_u8RelayChannelNum; i++)
	{
		for (uint8_t j = 0; j < g_stRelayChannelConfList[i].u8GpioMappNum; j++){
			kRelayGpioInit(g_stRelayChannelConfList[i].gpios[j].port, g_stRelayChannelConfList[i].gpios[j].pin \
					, g_stRelayChannelConfList[i].eActionPolarity == EZAP_LOW?true:false);
		}
	}
	g_pRelayOptLedCallback = callback;
	g_u32RelayDelayOnTimeUS = zero_on_time;
	g_u32RelayDelayOffTimeUS = zero_off_time;
	return err;
}


/*	DESP: Relay module action detection task callback interface.
 * 	Auth: maozj.20191107.
 * */
void kRelayModuleActionDetectCallback(uint8_t poll_time)
{
	judgeCheckZeroIsUsefullPoll( poll_time, ZERO_TIMEOUT_MS);
}

static void judgeCheckZeroIsUsefullPoll(uint16_t poll_time_ms, uint16_t time_out_ms)
{
	static uint32_t u32LastCheckZeroCount = 0;
	static uint8_t u8GetCheckZeroCounter = 0;
	static uint16_t u32CheckZeroTimeoutCounter = 0;
	static bool bStartRecordCountFlg = true;

	if (poll_time_ms == 0){
		return;
	}

	if (time_out_ms/poll_time_ms < 5){
		return;
	}
#if 1
	//5�ּ�һ�ι��������,
	if (bStartRecordCountFlg == true && u8GetCheckZeroCounter++ > 5){
		u8GetCheckZeroCounter = 0;
		if (u32LastCheckZeroCount != g_u32CheckedZeroCount){
			u32CheckZeroTimeoutCounter = 0;
			g_bIsCheckedZeroFlg = true;
			u32LastCheckZeroCount = g_u32CheckedZeroCount;
		}
		bStartRecordCountFlg = false;
	}
	//����200MS�����ж��Ƿ��й���,�ٽ��ż�¼���������
	if (bStartRecordCountFlg == false  && u32CheckZeroTimeoutCounter++ > time_out_ms/poll_time_ms){
		u32CheckZeroTimeoutCounter = 0;
		u8GetCheckZeroCounter = 0;

//		uint8_t buffer[64];
		uint32_t u32TimeMS = halCommonGetInt32uMillisecondTick();
		if (u32LastCheckZeroCount == g_u32CheckedZeroCount && g_bIsCheckedZeroFlg == true){
			g_bIsCheckedZeroFlg = false;

//			sprintf(buffer, "\r\nErr: judgeCheckZeroIsUsefullPoll not checked zero time(%d) count(%d)\r\n \r\n", g_u32CheckedZeroCount, u32TimeMS);
//			iKonkeAfSelfPrint("\r\n****************Err: judgeCheckZeroIsUsefullPoll not checked zero time(%d) count(%d)*****\r\n \r\n", g_u32CheckedZeroCount, u32TimeMS);
//			emberSerialWriteData((uint8_t)APP_SERIAL, buffer, strlen((char *)buffer));
			//�ڹ����ⳬʱ��ֱ�ӻ�ȡ�������ݶ���
			memcpy(&g_stRelayStatusListBkp, &g_stRelayStatusList, sizeof(g_stRelayStatusListBkp));
			memset(&g_stRelayStatusList, 0xFF, sizeof(g_stRelayStatusList));
			//emberEventControlSetDelayMS(kRelayOnOffEventControl, 10);//ֱ�ӿ��ش���
//			kRelayUserCallback();
			if (kTimerIsGoing(KTIMER3) != true){
				kTimerStart(KTIMER3, true, g_u32RelayDelayOnTimeUS, kRelayUserCallback, NULL);
			}
		}else if (u32LastCheckZeroCount != g_u32CheckedZeroCount && g_bIsCheckedZeroFlg == false){
//			iKonkeAfSelfPrint("\r\n****************Err: judgeCheckZeroIsUsefullPoll checked zero time(%d) count(%d)*****\r\n \r\n", g_u32CheckedZeroCount, u32TimeMS);
//			sprintf(buffer, "\r\n****************judgeCheckZeroIsUsefullPoll checked zero time(%d) count(%d)*****\r\n \r\n", g_u32CheckedZeroCount, u32TimeMS);
//			emberSerialWriteData((uint8_t)APP_SERIAL, buffer, strlen((char *)buffer));
			g_bIsCheckedZeroFlg = true;
		}
		bStartRecordCountFlg = true;
		u32LastCheckZeroCount = g_u32CheckedZeroCount;
	}
#else
	g_bIsCheckedZeroFlg = true;
#endif

	if (g_u32RelayZeroCheckCountdownCounter > 0) {
		g_u32RelayZeroCheckCountdownCounter--;
	}
}

bool kRelayIsCheckedZero(void)
{
	return g_bIsCheckedZeroFlg;
}
bool kRelayCheckIsGoing(void)
{
	if (g_u32RelayZeroCheckCountdownCounter > 0) {
		return true;
	}
	return false;
}

