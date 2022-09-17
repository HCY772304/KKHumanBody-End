#include "af.h"

#include "ikk-network.h"
#include "ikk-debug.h"
#include "ikk-tick-handler.h"
#include "ikk-module-def.h"
#include "ikk-cluster.h"
#include "../driver/ikk-button.h"
#include "../driver/ikk-led.h"
#include "../driver/ikk-adc.h"
#include "ikk-common-utils.h"
#include "app/framework/plugin/network-steering/network-steering-internal.h"

#include "ikk-opt-tunnel.h"

#if Z30_DEVICE_OTA_ENABLE
#include "ikk-ota.h"
#endif
#if Z30_DEVICE_FACTORY_TEST_ENABLE
#include "ikk-factory-test.h"
#endif


// Add by dingmz_frc.20191107.need to modify by script!!!
#include "KKTemplateCodeProject.h"

// the maximum number of scheduled tasks supported!
// Note: The modification number here is invalid. It needs to be modified in the source code!!
#define SCHEDULE_TASK_MAXN					4

// Support for rejoin�������쳣���������ʱ���Է��������������ߣ���Ӱ��rejoin����������ģʽHBM_DO_NORMAL��RJM_DO_POLL����Ч��
#define KK_HEARTBEAT_STEP_ATTEMPTS_MAX		4

#if REJOIN_TIME_DEBUG// Just for debug
#define KK_REJOIN_DO_FAST_PERIOD			(1  * 60   * 1000)	// 15 min for FAST mode
#define KK_REJOIN_DO_SLOW_PERIOD			(5  * 60   * 1000)	// 2 hour for SLOW mode
#define KK_REJOIN_DO_POLL_PERIOD			(10  * 60   * 1000)	// 12hour for POLL mode
#else
#if SLEEPY_DEVICE_BATTERY_TYPE == BATTERY_LARGE_CAPCITY
#define KK_REJOIN_DO_FAST_PERIOD			(15 * 60   * 1000)	// 15 min for FAST mode
#define KK_REJOIN_DO_SLOW_PERIOD			(30 * 1 * 60 * 1000)	// 30min for SLOW mode
#define KK_REJOIN_DO_POLL_PERIOD			(45 * 1 * 60 * 1000)	// 45min for POLL mode
#elif (SLEEPY_DEVICE_BATTERY_TYPE == BATTERY_SMALL_CAPCITY)
#define KK_REJOIN_DO_FAST_PERIOD			(15 * 60   * 1000)	// 15 min for FAST mode
#define KK_REJOIN_DO_SLOW_PERIOD			(2 * 60 * 60 * 1000)	// 2 hour for SLOW mode
#define KK_REJOIN_DO_POLL_PERIOD			(12 * 60 * 60 * 1000)	// 12hour for POLL mode
#endif
#endif

// Enroll  Scheduled task Duratoin
#define ENROLL_SCHD_DURATION				(12 * 1000)// MS
// Rejoin  Scheduled task Duratoin
#define REJOIN_SCHD_DURATION				(6 * 1000)// MS
// Binding Scheduled task Duratoin
#define BINDING_SCHD_DURATION				6000// MS


#define ENROLL_CHECK_INTERVAL_TIME_MS		(3 * 1000)
#define ENROLL_CHECK_MAX_TIMES				3

#define SHORT_POLL_CHECK_INTERVAL_TIME_MS	(2 * 1000)


#define DELAY_RESTORE_NWK_STEERING_TIME_MS	(1.5 * 1000)

// �豸�����쳣����ʱ����ֵ����������ֵ�豸��Ҫ���������Իָ���
#define KK_NWK_ANOMALY_UPLIMIT				86400UL // unit:s
// �豸��Ϣ��������ERROR��������
#define MESSAGE_SENT_ERROR_MAXN				3

//delay close joining After network joined,alse delay bind cluster.
#define JOINED_DELAY_TIME_MS				(5 * 1000)



//join nwk timeout
#define NWK_STEERING_DEFAULT_TIMEOUT_MS 	(60 * 1000)

#define LED_SLOW_BLINK_ON_TIME_MS			800
#define LED_SLOW_BLINK_OFF_TIME_MS			800
#define LED_FAST_BLINK_ON_TIME_MS			200
#define LED_FAST_BLINK_OFF_TIME_MS			200

#define LED_SLOW_BLINK_CONTINUE_TIME_MS		(60 * 1000)
#define LED_FAST_BLINK_CONTINUE_TIME_MS		(5 * 1000)

// Automatic execution procedure after joined successed.
#define JOINED_PROCEDURE_NONE			(0)		// �����κδ���
#define JOINED_PROCEDURE_REP_SNAPSHOT	(1) 	// �ϱ��豸����
#define JOINED_PROCEDURE_REP_CMEI		(2) 	// �ϱ�CMEI
#define JOINED_PROCEDURE_REP_ISN		(3) 	// �ϱ�ISN
#define JOINED_PROCEDURE_END			(0XFF) 	// ���̽���

#define RETRY_BANDING_CLUSTER_MAX_TIMES	2 // �����ɹ������԰󶨴˴�

// Add by dingmz_frc.20190626: define for device instance object.
typedef struct {
	// �豸join�����JoinSucceedEvent�¼�����������������������ɹ������ֲ�����
	uint8_t u8ActionIndexAfterJoined;
}KKDeviceObjectSt;
// Add by dingmz_frc.20190509: device instance object.
KKDeviceObjectSt g_stDeviceObject = {
	.u8ActionIndexAfterJoined = JOINED_PROCEDURE_NONE,
};

// Network led indicate state.
typedef enum { 	ELS_INDI_OFFLINE = 0, 	// LED indicate for device offline, fast blink ntimes;
				ELS_INDI_ONLINE, 		// LED indicate for device  online, fast blink 1times;
				ELS_INDI_JOINING, 		// LED indicate for joining procedure, slow blinking;
				ELS_INDI_JOINED, 		// LED indicate for joined network, ON or OFF;
				ELS_INDI_JOIN_FAILED,	// LED indicate for join failed, fast blink 6times;
				ELS_INDI_LEAVED,		// LED indicate for leaved network, fast blink 10times;
				ELS_INDI_CLOSE
}NwkLedStEnum;

// Rejoin procedure state machine.
typedef enum { RJM_DO_STOP = 0, RJM_DO_FAST, RJM_DO_SLOW, RJM_DO_POLL }KKRejoinModeEnum;

// Add by dingmz_frc.20190509: define for device instance object.
typedef struct {
	// Rejoin operate mode, Rejoin Procedure is correlated with the u8RejoinFailedCount attr.
	KKRejoinModeEnum eRejoinMode;
	// heartbeat attempts count, Use in case of abnormal rejoin.
	uint8_t u8RejoinModeAttemptsCount;
}KKRejoinCtrlSt;

// Gateway Eui64 address cache.
EmberEUI64 g_Eui64GatewayAddr = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// The device supports cluster collections that need to be bound automatically.
BindObjSt g_lstBindingClusterConf[BINDING_CLUSTER_MAXN] = { 0 };

// Restart the counter after the detection device has been offline for a long time, and the system will be restarted if the timing timeout occurs!!
uint64_t g_u64NetworkAnomalyBaseTimeStamp = 0;

static uint32_t g_u32EnrollResultCheckCounter = 0;
static CommonOptCallbackSt g_stEnrolledTaskObj = { 0 };
static uint32_t g_u32RejoinResultCheckCounter = 0;
static CommonOptCallbackSt g_stRejoinedTaskObj = { 0 };
static uint32_t g_u32BindingResultCheckCounter = 0;
static CommonOptCallbackSt g_stBindedTaskObj = { 0 };

static uint8_t g_u8NwkLedId = LED_UNKNOW_ID;
static uint8_t g_u8NwkBtnId = BTN_UNKNOW_ID;

static uint32_t g_u32NwkJoiningDurationCounter   = 0;
static uint32_t g_u32NwkJoiningCountdownCounter  = 0;

static uint32_t g_u32JoinNwkTimeMs = 0;

static uint32_t g_u32NwkExitCountDownCounter = 0;


// joining complete result callback define.
pJoinCompleteCallback g_pJoinCompleteCallback = NULL;
// network status notify callback define.
pNetworkStatusNotifyCallback g_pNetworkStatusNotifyCallback = NULL;
//add bby maozj 20200511
//network exit complete callback define
pNetworkExitCompleteCallback g_pNetworkExitCompleteCallback = NULL;
pNetworkStopTaskCallback g_pNetworkStopTaskCallback = NULL;
// copy current network status.
static NwkStatusEnum g_eCurrentNetworkStatus = ENS_UNKNOW;
//add by maozj 20200225
static NwkStatusEnum g_eLastNetworkStatus = ENS_UNKNOW;

static bool g_bIsNwkJoiningFlg = false;

uint8_t g_schedule_id[SCHEDULE_TASK_MAXN] = { 0 };
EmberEventControl kNwkScheduleTaskEventControl[SCHEDULE_TASK_MAXN];
pScheduleTaskHandlerCallback g_pfScheduleTaskCallback[SCHEDULE_TASK_MAXN] = NULL;
uint32_t g_u32ScheduleTaskPeroidIntervalMS[SCHEDULE_TASK_MAXN] = {0, 0, 0, 0};

// Support for rejoin procedure
EmberEventControl kNwkRejoinProcedureEventControl;
EmberEventControl kUserJoinSucceedProcedureEventControl;

KKRejoinCtrlSt g_stRejoinCtrller;

// ������Ϣʧ�ܼ�����������һ�������������⴦��
static uint8_t g_u8MsgSentErrorCounter;

//joining scan channel 1 round failed flag
static bool g_bNwkSteeringCompleteFlg = false;
//channel mac response timeout ,may be channel dirty
static bool g_bNwkSteeringMacFailureFlg = false;
//add by maozj 0200708 is or not rejoined network
static bool g_bIsRejoinedNwkFlg = false;


extern EmberAfPluginNetworkSteeringJoiningState emAfPluginNetworkSteeringState;
extern bool kBtnaIRQCallbackRegister(pBtnActionCallback callback );
extern bool emberAfIasZoneClusterAmIEnrolled(uint8_t endpoint);

uint8_t emberAfGetBindingTableSize(void );
kk_err_t kNwkClusterBindingInspect(void );
void kNwkSetRejoinProcedureMode(KKRejoinModeEnum eRejoinMode );
void kNwkSetGateWayEui64(EmberEUI64 eui64);
void kNwkGetGateWayEui64(EmberEUI64 eui64);

void kNwksaveNodeIdToFlash(EmberNodeId node_id);
EmberNodeId kNwkgetNodeIdFromFlash(void);

void kNwkCheckRejoinProcedureMode(void );
void kNwkCheckConnectonAndTriggerRejoin(void);
kk_err_t kkIasEnrollRequestPlagiarizeOriginal(EmberEUI64 CIE_Address );

void kNwkSteeringStart(void);
kk_err_t kNwkClusterBindingTrigger(pFUNC_VOID pBindingCallback );
static void kNwkNetworkStatusNotifyCall(NwkStatusEnum nwkst );
static void kNwkNetworkStatusIndicator(NwkLedStEnum nwkst );

void kNwkjoinSucceedProcedureTrigger(uint32_t u32StartDelayNms );

/*	DESP: Network module tick loop handler interface.
 * 	Auth: dingmz_frc.20191106.
 * */
void kNwkModuleActionDetectCallback(void )
{
	//static EmberNetworkStatus s_old_network_st = EMBER_NO_NETWORK;
	static uint8_t ucRestoreSteeringCount = 0;
	EmberNetworkStatus new_network_status = emberAfNetworkState();

	// joining procedure
	if( g_u32NwkJoiningCountdownCounter > 0 ) {
		--g_u32NwkJoiningCountdownCounter;

		if((g_u32NwkJoiningCountdownCounter > 0) && (new_network_status == EMBER_NO_NETWORK)) {
			if( g_bNwkSteeringCompleteFlg == true && emAfProIsCurrentNetwork() \
					&& (emAfPluginNetworkSteeringState == EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_NONE)) {
				iKonkeAfSelfPrint("$$$$$$$$$$ Network Steering Start!\r\n");


				/*begin modify by maozj 20191127 repair issue that Difficult to network when gateway network permission joining is enabled with delay*/
				ucRestoreSteeringCount = 0;
				g_bNwkSteeringCompleteFlg = false;
				kNwkSteeringStart();
				//emberAfPluginNetworkSteeringStart();
				/*end modify by maozj 20191127*/

				// network status notify.
				kNwkNetworkStatusNotifyCall(ENS_JOINING);
				//add by maozj 20200321 for get joining network status
				g_bIsNwkJoiningFlg = true;
			}else {
				//other situation,must restart steering
				if (g_bNwkSteeringMacFailureFlg == true && ucRestoreSteeringCount++ > MS2COUNT(DELAY_RESTORE_NWK_STEERING_TIME_MS)){
					g_bNwkSteeringMacFailureFlg = false;
					ucRestoreSteeringCount = 0;
					g_bNwkSteeringCompleteFlg = true;
					emAfPluginNetworkSteeringState = EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_NONE;
					iKonkeAfSelfPrint("#########Steering Again...\r\n");
				}
			}

			// for joining...
			if((g_u8NwkLedId != LED_UNKNOW_ID) && (kLedIsBlinking(g_u8NwkLedId) == false)) {
				//delete by maozj 20200703 avoid always blink when use network button
				//kNwkNetworkStatusIndicator(ELS_INDI_JOINING);
			}
		}
		// timeout over!!!
		else if( g_u32NwkJoiningCountdownCounter == 0 ) {
			iKonkeAfSelfPrint("$$$$$$$$$$ Network Steering Stop!\r\n");
			emberAfPluginNetworkSteeringStop();

			g_bIsNwkJoiningFlg = false;//add by maozj 20200327

			// Operate the indicator light according to the network status after the countdown!
			if((new_network_status == EMBER_JOINED_NETWORK) || (new_network_status == EMBER_JOINED_NETWORK_NO_PARENT)) {
				kNwkNetworkStatusIndicator(ELS_INDI_JOINED);

				//start get mac and self bind cluster
				kNwkClusterBindingTrigger(NULL);

				if( new_network_status == EMBER_JOINED_NETWORK ) {
					kNwkNetworkStatusNotifyCall(ENS_ONLINE );
				}else {
					kNwkNetworkStatusNotifyCall(ENS_OFFLINE);
				}

				if( g_pJoinCompleteCallback ) {
					iKonkeAfSelfPrint("777777g_pJoinCompleteCallback succeed\r\n");
					g_pJoinCompleteCallback(EJC_JOIN_SUCCEED);
				}
			}else {
				kNwkNetworkStatusIndicator(ELS_INDI_JOIN_FAILED);

				if( g_pJoinCompleteCallback && g_bIsRejoinedNwkFlg != true) {
					iKonkeAfSelfPrint("888888g_pJoinCompleteCallback succeed\r\n");
					g_pJoinCompleteCallback(EJC_JOIN_FAILED);
				}
			}
		}
	}

	// check the result of rejoining procedure.
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
	if( g_u32RejoinResultCheckCounter > 0 ) {
		g_u32RejoinResultCheckCounter--;

		// check rejoin state
		if( emberAfNetworkState() == EMBER_JOINED_NETWORK ) {
			iKonkeAfSelfPrint("\r\n########Rejoin SUCC!  Rejoin SUCC!#############\r\n");

			if( g_stRejoinedTaskObj.pfunc ) {
				g_stRejoinedTaskObj.pfunc(g_stRejoinedTaskObj.param);
			}

			g_u32RejoinResultCheckCounter = 0;
		}

		if( g_u32RejoinResultCheckCounter == 0 ) {
			g_stRejoinedTaskObj.pfunc = NULL;
			g_stRejoinedTaskObj.param = NULL;
		}
	}
#endif

	// check enroll state
#ifdef ZCL_USING_IAS_ZONE_CLUSTER_SERVER
	static uint8_t count = 0;
	static uint8_t times = 0;
	if( g_u32EnrollResultCheckCounter > 0  ) {

		g_u32EnrollResultCheckCounter--;

		if( emberAfIasZoneClusterAmIEnrolled(1)) {
			iKonkeAfSelfPrint("Re-Enroll SUCC!");

			if( g_stEnrolledTaskObj.pfunc ) {
				g_stEnrolledTaskObj.pfunc(g_stEnrolledTaskObj.param);
			}

			g_u32EnrollResultCheckCounter = 0;
			times = 0;
			count = 0;
		}else {
			//add by maozj 20191206
			if(emberAfNetworkState() == EMBER_JOINED_NETWORK && count++ > MS2COUNT(ENROLL_CHECK_INTERVAL_TIME_MS)) {
				count = 0;
				if (times++ < ENROLL_CHECK_MAX_TIMES){
					if( kUtilsIsValidEui64Addr(g_Eui64GatewayAddr)) {
						//enroll
						iKonkeAfSelfPrint("###POLL ENROLL REQUEST CHECK %d\r\n", times);
						kkIasEnrollRequestPlagiarizeOriginal(g_Eui64GatewayAddr);
					}
				}else{
					times = 0;
					g_u32EnrollResultCheckCounter = 0;
				}
			}
		}

		if( g_u32EnrollResultCheckCounter == 0 ) {
			g_stEnrolledTaskObj.pfunc = NULL;
			g_stEnrolledTaskObj.param = NULL;
		}
	}else {
		count = 0;
	}
#endif

	// find and bind procedure
	if( g_u32BindingResultCheckCounter > 0 ) {
		if( --g_u32BindingResultCheckCounter == 0 ) {
			g_stBindedTaskObj.pfunc = NULL;
			g_stBindedTaskObj.param = NULL;
		}
	}

	if (g_u32NwkExitCountDownCounter > 0){
		if (--g_u32NwkExitCountDownCounter == 0){
			//keep gpio restored after reboot
			//kStoreGpioStatus();
			//halReboot();
			if (g_pNetworkExitCompleteCallback){
				g_pNetworkExitCompleteCallback();
			}
		}
	}

	// make sure heartbeat control event active 20200601
	if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
		kNwkScheduleTaskStart(SCHEDULE_ALLOPT_ID);
	}else{
		kNwkScheduleTaskStop(SCHEDULE_ALLOPT_ID);
	}
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
	//add by maozj 20200708 short poll control check, data request
	extern uint32_t kkPollCheckCountdown(void);
	static uint8_t pollCount = 0;
	if (kkPollCheckCountdown() > 0) {
		if(emberAfNetworkState() == EMBER_JOINED_NETWORK && pollCount++ > MS2COUNT(SHORT_POLL_CHECK_INTERVAL_TIME_MS)) {
			pollCount = 0;
			kkPollProceduleCompletedCallback(EMBER_SUCCESS);
		}
	}
#endif
}

/*	DESP: Associated network light status indicator.
 * 	Auth: dingmz_frc.20191109.
 * */
static void kNwkNetworkStatusIndicator(NwkLedStEnum nwkst )
{
	iKonkeAfSelfPrint("Nwl Led Status Indicator status(%d)r\n", nwkst);
	if( g_u8NwkLedId != LED_UNKNOW_ID ) {
		switch(nwkst) {
			case (ELS_INDI_OFFLINE):
			{
				//kLedOptTrigger(g_u8NwkLedId, LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_OFF_TIME_MS, 3, LED_ON, LED_OFF);
				break;
			}
			case (ELS_INDI_ONLINE ):
			{
				//kLedOptTrigger(g_u8NwkLedId,  LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_OFF_TIME_MS, 1, LED_ON, LED_OFF);
				break;
			}
			case (ELS_INDI_JOINING):
			{
				//kLedOptTrigger(g_u8NwkLedId, LED_SLOW_BLINK_ON_TIME_MS, LED_SLOW_BLINK_OFF_TIME_MS, g_u32JoinNwkTimeMs, LED_ON, LED_OFF);
				break;
			}
			case (ELS_INDI_JOINED):
			{
				//here is unused ,can operate it in ikk_user.c

				#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
				//LED_OPT_OFF(g_u8NwkLedId);
				#else
				//LED_OPT_ON(g_u8NwkLedId);
				#endif
				break;
			}
			case (ELS_INDI_JOIN_FAILED):
			{

				//kLedOptTrigger(g_u8NwkLedId, 100, 100,  6, LED_ON, LED_OFF);
				//LED_OPT_OFF(g_u8NwkLedId);
				if (g_pNetworkExitCompleteCallback){
					g_pNetworkExitCompleteCallback();
				}
				break;
			}
			case (ELS_INDI_LEAVED):
			{
				//kLedOptTrigger(g_u8NwkLedId, LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_OFF_TIME_MS, \
				//		LED_FAST_BLINK_CONTINUE_TIME_MS/(LED_FAST_BLINK_ON_TIME_MS+LED_FAST_BLINK_OFF_TIME_MS), LED_ON, LED_OFF);
				break;
			}
			default:
			{
				//LED_OPT_OFF(g_u8NwkLedId);
				break;
			}
		}
	}
}

/*	DESP: If the network status notification callback has been associated, the network status needs to be synchronized.
 * 	Auth: dingmz_frc.20191109.
 * */
static void kNwkNetworkStatusNotifyCall(NwkStatusEnum nwkst )
{
	//static NwkStatusEnum s_nwk_st = ENS_UNKNOW;

	g_eCurrentNetworkStatus = nwkst;

	if( g_eCurrentNetworkStatus != g_eLastNetworkStatus ) {
		g_eLastNetworkStatus = g_eCurrentNetworkStatus;

		if( g_pNetworkStatusNotifyCallback ) {
			kBatteryGetVoltageWithMsgSent();
			if (ENS_JOINING == g_eLastNetworkStatus){
			    iKonkeAfSelfPrint("\r\n-------NetWork ENS_JOING-------\r\n");
			}else{
			    iKonkeAfSelfPrint("\r\n-------NetWork ENS_UNJOING-------\r\n");
			}
			g_pNetworkStatusNotifyCallback(g_eCurrentNetworkStatus);
		}
	}

	if( g_eCurrentNetworkStatus == ENS_ONLINE ) {
		iKonkeAfSelfPrint("######network ok, start the registered scheduled tasks.\r\n");
		kNwkScheduleTaskStart(SCHEDULE_ALLOPT_ID);
		//add by maozj 2020068
		//report snapshot,cmei and isn when gateway is made by konke
//		if (kIsKonkeRemoteGateway() == true){
//			uint64_t u64CurrentTimeMS = halCommonGetInt64uMillisecondTick();
//			uint32_t randTimeMS = NWK_MIN_DELAY_TIME_MS_AFTER_POWER_ON * 2;
//#if (Z30_DEVICE_DTYPE != Z30_DEVICE_ZED_SLEEPY &&  Z30_DEVICE_DTYPE != Z30_DEVICE_ZED)
//			if (g_bIsRejoinedNwkFlg == true) {
//
//
//				//power on
//				iKonkeAfSelfPrint("####1111Nwk Power on rand delay report snapshot\r\n");
//				randTimeMS = kUtilsGetRandNum(RAND_MIN_NUM, RAND_MAX_NUM);
//			}
//#endif
//			iKonkeAfSelfPrint("####2222Nwk Power on rand delay report snapshot delaytime(%dms)\r\n", randTimeMS);
//			kNwkjoinSucceedProcedureTrigger(randTimeMS);
//		}
	}else {
		iKonkeAfSelfPrint("network anomaly, stop the registered scheduled tasks.\r\n");
		kNwkScheduleTaskStop(SCHEDULE_ALLOPT_ID);
	}
}

/*	DESP: get the current network status.
 * 	Auth: dingmz_frc.20191109.
 * */
NwkStatusEnum kNwkGetCurrentStatus(void )
{
	//add by maozj 20191205
	EmberNetworkStatus status = emberAfNetworkState();
//	status = emberAfNetworkState();
//	status = emberAfNetworkState();
	if (status == EMBER_JOINED_NETWORK_NO_PARENT){
		g_bIsNwkJoiningFlg = false;
		g_eCurrentNetworkStatus = ENS_OFFLINE;
	}else if (status == EMBER_JOINED_NETWORK){
		g_bIsNwkJoiningFlg = false;
		g_eCurrentNetworkStatus = ENS_ONLINE;
	}else if (status == EMBER_JOINING_NETWORK || g_bIsNwkJoiningFlg == true){
		g_eCurrentNetworkStatus = ENS_JOINING;
	}else {
		g_bIsNwkJoiningFlg = false;
		g_eCurrentNetworkStatus = ENS_LEAVED;
	}

	return g_eCurrentNetworkStatus;
}
/*	DESP: Network button action notification callback interface.
 * 	Auth: dingmz_frc.20191109.
 * */
void kNwkNetworkButtonAcitonCallback(unsigned char button_id, BtnActionEnum action )
{
	iKonkeAfSelfPrint("kNwkButton: Index(%d), Action(%d), Nwk(%d)\r\n", button_id, action, kNwkGetCurrentStatus());

/*begin add by maozj 20200317 do not deal system key long press or short press when device is aging*/
#if Z30_DEVICE_FACTORY_TEST_ENABLE
	if (kGetFactoryTestStatus() != FTS_NORMAL && kGetFactoryTestStatus() != FTS_FULL_DEVICE_TEST){
		return;
	}
#endif
/*begin add by maozj 20200317 */

	if( g_u8NwkBtnId == button_id ) {
		switch(action) {
			case (EBA_CLICK):
			{
				if (kNwkIsExiting() == true){
					iKonkeAfSelfPrint("#####NWK is exiting\r\n");
					return;
				}

				if( kNwkGetCurrentStatus() == ENS_ONLINE ) { // report connected!
					iKonkeAfSelfPrint("#####Report Connected\r\n");
#if Z30_DEVICE_OTA_ENABLE
					if (kGetOTAStatus() == OTA_NORMAL)
#endif
					{
						kNwkNetworkStatusIndicator(ELS_INDI_ONLINE);
					}
					if (kIsKonkeRemoteGateway() == true){
						// report connected
						kOptTunnelOODReport(0, 0XCD, NULL, 0);
					}
				}else if( kNwkGetCurrentStatus() == ENS_OFFLINE ) {
					iKonkeAfSelfPrint("#####LED Blink 3 Times\r\n");
					kNwkNetworkStatusIndicator(ELS_INDI_OFFLINE);
					//button rejoin network
					//kNwkCheckConnectonAndTriggerRejoin();
					//add by maozj 20200706
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
					kNwkRejoinMethodTrigger(NULL, NULL);
#endif

				}else if (kNwkGetCurrentStatus() == ENS_LEAVED){
					//avoid IEEE token cannot cleared
					kNwkFactoryReset(false);
					//steering timeout can be set
					iKonkeAfSelfPrint("#####Start Steering\r\n");
					//kNwkJoiningStart(g_u32JoinNwkTimeMs, g_pJoinCompleteCallback);
					//kNwkNetworkStatusIndicator(ELS_INDI_JOINING);
					//kkPollProceduleTrigger(15, true);
				}

				break;
			}
			case (EBA_LONGPRESS):
			{
				bool status = false;
				if (kNwkGetCurrentStatus() == ENS_ONLINE || kNwkGetCurrentStatus() == ENS_OFFLINE){
					status = true;
				}
				if (kNwkGetCurrentStatus() != ENS_JOINING){
					kNwkFactoryReset(status);
				}
				break;
			}
			default: break;
		}
	}
}

/*	DESP: The interface of resource release and reset after the device is restored to the factory, which is executed when the network is removed.
 * 	Auth: dingmz_frc.20191106.
 * */
void kNwkFactoryReset(bool is_nwk_indicator)
{
	kNwkJoiningStart(0, NULL);
	emberLeaveNetwork();
	emberClearBindingTable();
	emberAfClearReportTableCallback();//add by maozj 20200521

	for(uint8_t endpointIndex = 0; endpointIndex < emberAfEndpointCount(); ++endpointIndex ) {
		uint8_t endpoint = emberAfEndpointFromIndex(endpointIndex);
		//emberAfResetAttributes(endpoint);
		emberAfGroupsClusterClearGroupTableCallback(endpoint);
		emberAfScenesClusterClearSceneTableCallback(endpoint);
	}

	memset(g_Eui64GatewayAddr, 0xFF, sizeof(g_Eui64GatewayAddr));
	kNwkSetGateWayEui64(g_Eui64GatewayAddr);
	//add by maozj 20200320
#if Z30_DEVICE_OTA_ENABLE
	kSetOTAStatus(OTA_NORMAL);
#endif
	EmberNodeId node = 0xFFFF;
	kNwksaveNodeIdToFlash(node);

	g_u32NwkJoiningCountdownCounter = 0;
	g_u32EnrollResultCheckCounter   = 0;
	g_u32RejoinResultCheckCounter   = 0;
	g_u32BindingResultCheckCounter  = 0;

#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
	kNwkSetRejoinProcedureMode(RJM_DO_STOP);
	kBatterySetToken();
#endif

	//led can  blink when network leaved
	if (is_nwk_indicator == true){
		kNwkNetworkStatusIndicator(ELS_INDI_LEAVED);
		kNwkNetworkStatusNotifyCall(ENS_LEAVED);
		//add by maozj 20200407 for reset count down after network leaved
		g_u32NwkExitCountDownCounter = MS2COUNT(LED_FAST_BLINK_CONTINUE_TIME_MS);
	}
	kNwkScheduleTaskStop(SCHEDULE_ALLOPT_ID);

	//add by maozj 20191213 need identify whether gateway is made by konke
	uint8_t isKonkeGateway = 0;
	halCommonSetToken(TOKEN_IS_KONKE_GATEWAY, &isKonkeGateway);
}

/*	DESP: Check whether the specified cluster is binded.
 * 	RETURN[KET_OK]: is binded!
 * 	RETURN[KET_ERR_NON_EXIST]: is unbinded!
 * 	RETURN[KET_ERR_NO_PERMISSIONS]: this binding item is not configured!
 * 	Auth: dingmz_frc.20191112.
 * */
kk_err_t kNwkClusterBindingObjIsOK(uint8_t endpoint, uint16_t cluster )
{
	EmberBindingTableEntry result;
	kNwkGetGateWayEui64(g_Eui64GatewayAddr);

	for(int index = 0; index < emberAfGetBindingTableSize(); ++index ) {
		EmberStatus status = emberGetBinding(index, &result);

		if( status == EMBER_SUCCESS && result.type == EMBER_UNICAST_BINDING ) {
			if( result.local == endpoint && result.clusterId == cluster && !MEMCOMPARE(result.identifier, g_Eui64GatewayAddr, EUI64_SIZE)) {
				return KET_OK;
			}
		}
	}

	// If binding table is not set, query whether it has been configured.
	for(int index = 0; index < BINDING_CLUSTER_MAXN; ++index ) {
		if( g_lstBindingClusterConf[index].endpoint == endpoint &&
			g_lstBindingClusterConf[index].cluster == cluster ) {
			return KET_ERR_NON_EXIST;
		}
	}

	return KET_ERR_NO_PERMISSIONS;
}

// on binding-table set <index> <cluster> <local ep> <remote ep> <EUI>
static kk_err_t kNwkClusterBindingObjSet(uint8_t endpoint, uint16_t cluster_id, EmberEUI64 eui64 )
{
	kk_err_t err = KET_OK;

	EmberStatus status = emberAfPushEndpointNetworkIndex(1);

	if( status == EMBER_SUCCESS ) {
		uint8_t binding_table_size = emberAfGetBindingTableSize();
		EmberBindingTableEntry entry;

		int valid_index = binding_table_size;

		for(int index = 0; index < binding_table_size; ++index ) {
			status = emberGetBinding(index, &entry);

			if( EMBER_SUCCESS == status ) {
				if( entry.type == EMBER_UNICAST_BINDING ) {
					if( entry.local == endpoint && entry.clusterId == cluster_id && !MEMCOMPARE(entry.identifier, eui64, EUI64_SIZE)) {
						valid_index = index;
						break;
					}
				}else if((entry.type == EMBER_UNUSED_BINDING) && (valid_index == binding_table_size)) {
					valid_index = index;
				}
			}
		}

		if( valid_index < binding_table_size ) {
			entry.type = EMBER_UNICAST_BINDING;
			entry.clusterId = cluster_id;
			entry.local 	= endpoint;
			entry.remote 	= BINDING_EP_DEF;
			MEMMOVE(entry.identifier, eui64, EUI64_SIZE);
			entry.networkIndex = emberGetCurrentNetwork();

			if( emberSetBinding(valid_index, &entry) != EMBER_SUCCESS ) {
				err = KET_ERR_UNKNOW;
			}
		}else {
			err = KET_ERR_INSUFFICIENT_SPACE;
		}

		emberAfPopNetworkIndex();
	}else {
		err = KET_ERR_OPRATE_ILLEGAL;
	}

	iKonkeAfSelfPrint("----set bind[%d]: endpoint(%d), cluster(%2x)\r\n", err, endpoint, cluster_id);

	return err;
}

/*	DESP: Check if there is a cluster requiring active bind.
 * 	Auth: dingmz_frc.20191106.
 * */
kk_err_t kNwkClusterBindingInspect(void )
{
	kNwkGetGateWayEui64(g_Eui64GatewayAddr);

	if( kUtilsIsValidEui64Addr(g_Eui64GatewayAddr) == false ) {
		return KET_ERR_NO_PERMISSIONS;
	}

	//EmberBindingTableEntry result;

	for(int ix = 0; ix < BINDING_CLUSTER_MAXN; ++ix ) {
		if( g_lstBindingClusterConf[ix].cluster != CLUSTER_UNKNOW ) {
			kk_err_t err = kNwkClusterBindingObjIsOK(g_lstBindingClusterConf[ix].endpoint, g_lstBindingClusterConf[ix].cluster);
			if( err == KET_ERR_NON_EXIST ) {
				kNwkClusterBindingObjSet(g_lstBindingClusterConf[ix].endpoint, g_lstBindingClusterConf[ix].cluster, g_Eui64GatewayAddr);
			}
		}else {
			break;
		}
	}

	return KET_OK;
}

/*	DESP: Callback processing function after successfully obtaining the gateway EUI64 address.
 * 	Auth: dingmz_frc.20191106.
 * */
static void kNwkGetGatewayEui64AddrSucc_Callback(const EmberAfServiceDiscoveryResult* result )
{
	//iKonkeAfSelfPrint("######Get Gateway Ieee Request Response\r\n");
	if( !emberAfHaveDiscoveryResponseStatus(result->status)) {
		iKonkeAfSelfPrint("######Get Gateway Ieee Request Response(%d)\r\n", result->status);
		// Do nothing
	}else if( result->zdoRequestClusterId == IEEE_ADDRESS_REQUEST ) {
		uint8_t *eui64ptr = (uint8_t *)(result->responseData);

		memcpy(g_Eui64GatewayAddr, eui64ptr, EUI64_SIZE);
		kNwkSetGateWayEui64(g_Eui64GatewayAddr);
		// If the gateway address is currently obtained, submit the enroll request directly!
		//kNwkGetGateWayEui64(g_Eui64GatewayAddr);

#ifdef ZCL_USING_IAS_ZONE_CLUSTER_SERVER
		if((emberAfNetworkState() == EMBER_JOINED_NETWORK) && (!emberAfIasZoneClusterAmIEnrolled(1))) {
			if( kUtilsIsValidEui64Addr(g_Eui64GatewayAddr)) {
				iKonkeAfSelfPrint("Enroll Request\r\n");
				kkIasEnrollRequestPlagiarizeOriginal(g_Eui64GatewayAddr);
			}
		}
#endif

		iKonkeAfSelfPrint("########Get EUI64 From Gateway: %X%X%X%X%X%X%X%X\r\n", g_Eui64GatewayAddr[7], g_Eui64GatewayAddr[6]
			, g_Eui64GatewayAddr[5], g_Eui64GatewayAddr[4], g_Eui64GatewayAddr[3], g_Eui64GatewayAddr[2]
			, g_Eui64GatewayAddr[1], g_Eui64GatewayAddr[0]);

		kNwkClusterBindingInspect();

		// If self binding is currently being performed, the joined procedure needs to be performed automatically!!
		if( g_stBindedTaskObj.pfunc != NULL ) {
			g_stBindedTaskObj.pfunc(g_stBindedTaskObj.param);

			g_stBindedTaskObj.pfunc  = NULL;
			g_stBindedTaskObj.param  = NULL;
		}

		if( g_pJoinCompleteCallback ) {
			iKonkeAfSelfPrint("##############JoinCompleteCallback!!!\r\n");
			g_pJoinCompleteCallback(EJC_CONNLINK_OK);
			g_pJoinCompleteCallback = NULL;
		}
	}
}

/*	DESP: device cluster binding trigger interface.
 * 	Auth: dingmz_frc.20191112.
 * */
kk_err_t kNwkClusterBindingTrigger(pFUNC_VOID pBindingCallback )
{
	if( g_u32BindingResultCheckCounter > 0 ) {
		return KET_ERR_OPRATE_IN_PROGRESS;
	}
	kk_err_t err = KET_ERR_UNKNOW;

#ifdef ZCL_USING_IAS_ZONE_CLUSTER_SERVER
	g_u32EnrollResultCheckCounter = MS2COUNT(ENROLL_SCHD_DURATION);
	kTickRunnningTrigger();
#endif

	EmberEUI64 tmpEui64 = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

	kNwkGetGateWayEui64(g_Eui64GatewayAddr);

	iKonkeAfSelfPrint("##########GetEui64FromFlash %x%x%x%x%x%x%x%x --\r\n",g_Eui64GatewayAddr[7], g_Eui64GatewayAddr[6], \
			g_Eui64GatewayAddr[5], g_Eui64GatewayAddr[4], g_Eui64GatewayAddr[3], g_Eui64GatewayAddr[2], g_Eui64GatewayAddr[1],g_Eui64GatewayAddr[0]);

	if(MEMCOMPARE(g_Eui64GatewayAddr, tmpEui64, EUI64_SIZE)){	 //�Ѿ����ڹ����صĳ���ַ�������ٴ����������󣬽�ʡ����
		iKonkeAfSelfPrint("Gateway IEEE have existed\r\n");

		kNwkClusterBindingInspect();

#ifdef ZCL_USING_IAS_ZONE_CLUSTER_SERVER
		if((emberAfNetworkState() == EMBER_JOINED_NETWORK) && (!emberAfIasZoneClusterAmIEnrolled(1))) {
			if( kUtilsIsValidEui64Addr(g_Eui64GatewayAddr)) {
				iKonkeAfSelfPrint("Enroll Check.....\r\n");
				kkIasEnrollRequestPlagiarizeOriginal(g_Eui64GatewayAddr);
			}
		}
#endif
		err = KET_OK;
	}else {
		if(emberAfFindIeeeAddress(0x0000, kNwkGetGatewayEui64AddrSucc_Callback) == EMBER_SUCCESS) {
			iKonkeAfSelfPrint("###########Gateway IEEE Request\r\n");
			g_stBindedTaskObj.pfunc = pBindingCallback;
			g_u32BindingResultCheckCounter = MS2COUNT(BINDING_SCHD_DURATION);
			kTickRunnningTrigger();
			err = KET_OK;
		}
	}

	//make sure network status is joined, reboot device if nwk status is no parents
	if (emberAfNetworkState() == EMBER_JOINED_NETWORK_NO_PARENT){
		iKonkeAfSelfPrint("\r\n###########No Parents Reboot .... \r\n");
		//halReboot();
	}
	return err;
}

/*	DESP: Traverse print the cluster binding table content.
 * 	Auth: dingmz_frc.20191106.
 * */
void kNwkClusterBindingTableTraverse(void )
{
	EmberBindingTableEntry result;

	iKonkeAfSelfPrint("-- BINDED LSIT ------\r\n");
	for(int index = 0; index < emberAfGetBindingTableSize(); ++index ) {
		EmberStatus status = emberGetBinding(index, &result);

		if (status == EMBER_SUCCESS && result.type != EMBER_UNUSED_BINDING ) {
			iKonkeAfSelfPrint("BindT[%2d] type(%d), endpoint(%d:%d), cluster(%2x), id(%x%x%x%x%x%x%x%x)\r\n"
				, index, result.type, result.local, result.remote, result.clusterId
				, result.identifier[7], result.identifier[6]
				, result.identifier[5], result.identifier[4]
				, result.identifier[3], result.identifier[2]
				, result.identifier[1], result.identifier[0]
				);
		}
	}

	iKonkeAfSelfPrint("-------------------------\r\n");
}

/*	DESP: network rejoin event loop handler.
 * 	Auth: dingmz_frc.20190708.
 * */
void kNwkRejoinProcedureEventHandler(void )
{
	iKonkeAfSelfPrint("Rejoin Event Handler heare(NetStatus: %d)!\r\n", emberAfNetworkState());
	emberEventControlSetInactive(kNwkRejoinProcedureEventControl);

#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
	// �������״̬�Ƿ���Ҫ����
	kNwkCheckConnectonAndTriggerRejoin();
	// ���ݵ�ǰ����״̬����rejoin����ģʽ
	kNwkCheckRejoinProcedureMode();
#endif
}

#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
/*	DESP: load the heartbeat mode from token.
 * 	Auth: dingmz_frc.20190511.
 * */
uint8_t kNwkLoadRejoinMode(void )
{
	uint8_t hbmode = 0;

	halCommonGetToken(&hbmode, TOKEN_REJOIN_MODE);
	//iKonkeAfSelfPrint("~~~~~~~~~~ Load Heartbeat Mode[%d] ~~~~~~~~~~\r\n", hbmode);

	return hbmode;
}

/*	DESP: save the heartbeat mode to token.
 * 	Auth: dingmz_frc.20190511.
 * */
void kNwkSaveRejoinMode(uint8_t hbmode )
{
	uint8_t token_tmp = kNwkLoadRejoinMode();

	if( token_tmp != hbmode ) {
		iKonkeAfSelfPrint("~~~~~~~~~~ Save Rejoin Mode[%d] ~~~~~~~~~~\r\n", hbmode);
		halCommonSetToken(TOKEN_REJOIN_MODE, &hbmode);
	}
}

/*	DESP: save the heartbeat mode to token.
 * 	Auth: dingmz_frc.20190511.
 * */
void kNwkCheckConnectonAndTriggerRejoin(void)
{
	emberAfCorePrintln("mmmmm check rejoin status.......");

#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
	//emberAfPluginEndDeviceSupportStackStatusCallback(0);//add by maozj 20190510�ж�����״̬���Ƿ�����
	if (emberAfNetworkState() == EMBER_JOINED_NETWORK_NO_PARENT){
		if (!emberStackIsPerformingRejoin()){
			emberAfStartMoveCallback();
		}
	}
#endif
}

/*	DESP: get the current rejoin procedure mode.
 * 	Auth: dingmz_frc.20190511.
 * */
KKRejoinModeEnum kNwkGetRejoinProcedureMode(void )
{
	return g_stRejoinCtrller.eRejoinMode;
}

/*	DESP: Check whether the rejoin procedure mode is running.
 * 	Auth: dingmz_frc.20190517.
 * */
bool kNwkRejoinProcedureRunning(void )
{
	return emberEventControlGetActive(kNwkRejoinProcedureEventControl);
}

/*	DESP: get the current rejoin procedure mode.
 * 	Auth: dingmz_frc.20190511.
 * */
void kNwkSetRejoinProcedureMode(KKRejoinModeEnum eRejoinMode )
{
	// reset the rejoin procedure attempts count
	g_stRejoinCtrller.u8RejoinModeAttemptsCount = 0;

	if( g_stRejoinCtrller.eRejoinMode == eRejoinMode ) {
		return ;
	}

	g_stRejoinCtrller.eRejoinMode = eRejoinMode;

	// if eRejoinMode is RJM_DO_STOP or RJM_DO_POLL, must save it to flash
	if( g_stRejoinCtrller.eRejoinMode == RJM_DO_STOP || g_stRejoinCtrller.eRejoinMode == RJM_DO_POLL ) {
		kNwkSaveRejoinMode(g_stRejoinCtrller.eRejoinMode);
	}

	// Setting up reporting rules according to different modes.
	switch(g_stRejoinCtrller.eRejoinMode) {
		case (RJM_DO_FAST):
		{
			iKonkeAfSelfPrint("Rejoin Mode Set: FAST.\r\n");
			emberEventControlSetDelayMS(kNwkRejoinProcedureEventControl, KK_REJOIN_DO_FAST_PERIOD);
			break;
		}
		case (RJM_DO_SLOW):
		{
			iKonkeAfSelfPrint("Rejoin Mode Set: SLOW.\r\n");
			emberEventControlSetDelayMS(kNwkRejoinProcedureEventControl, KK_REJOIN_DO_SLOW_PERIOD);
			break;
		}
		case (RJM_DO_POLL):
		{
			iKonkeAfSelfPrint("Rejoin Mode Set: POLL.\r\n");
			emberEventControlSetDelayMS(kNwkRejoinProcedureEventControl, KK_REJOIN_DO_POLL_PERIOD);
			break;
		}
		default:
		{
			iKonkeAfSelfPrint("Rejoin Mode Set: STOP.\r\n");
			emberEventControlSetInactive(kNwkRejoinProcedureEventControl);
			break;
		}
	}
}

/*	DESP: check and adjust the Rejoin mode according to current network state.
 * 	Auth: dingmz_frc.20190511.
 * */
void kNwkCheckRejoinProcedureMode(void )
{
	uint8_t networkStatus = emberAfNetworkState();

	iKonkeAfSelfPrint("RejoinMode: network(%d), Mode(%d), Attempts(%d).\r\n", networkStatus
		, g_stRejoinCtrller.eRejoinMode
		, g_stRejoinCtrller.u8RejoinModeAttemptsCount);


	if( networkStatus == EMBER_JOINED_NETWORK_NO_PARENT ) {
		switch(g_stRejoinCtrller.eRejoinMode) {
			case (RJM_DO_FAST):
			{
				if( ++g_stRejoinCtrller.u8RejoinModeAttemptsCount < KK_HEARTBEAT_STEP_ATTEMPTS_MAX ) {
					emberEventControlSetDelayMS(kNwkRejoinProcedureEventControl, KK_REJOIN_DO_FAST_PERIOD);
				}else {
					kNwkSetRejoinProcedureMode(RJM_DO_SLOW);
				}
				break;
			}
			case (RJM_DO_SLOW):
			{
				if( ++g_stRejoinCtrller.u8RejoinModeAttemptsCount < KK_HEARTBEAT_STEP_ATTEMPTS_MAX ) {
					emberEventControlSetDelayMS(kNwkRejoinProcedureEventControl, KK_REJOIN_DO_SLOW_PERIOD);
				}else {
					kNwkSetRejoinProcedureMode(RJM_DO_POLL);
				}
				break;
			}
			case (RJM_DO_POLL):
			{
				emberEventControlSetDelayMS(kNwkRejoinProcedureEventControl, KK_REJOIN_DO_POLL_PERIOD);
				break;
			}
			default:
			{
				kNwkSetRejoinProcedureMode(RJM_DO_STOP);
				break;
			}
		}
	}else {
		kNwkSetRejoinProcedureMode(RJM_DO_STOP);
	}
}

/*	DESP: initialize the rejoin procedure to specified mode.
 * 	Auth: dingmz_frc.20190511.
 * */
void kNwkRejoinProcedureModeInit(KKRejoinModeEnum eRejoinMode )
{
	// initialize the heartbeat mode to NORMAL.
	g_stRejoinCtrller.eRejoinMode = eRejoinMode;

	switch(g_stRejoinCtrller.eRejoinMode) {
		case ( RJM_DO_FAST ):
		{
			emberEventControlSetDelayMS(kNwkRejoinProcedureEventControl, KK_REJOIN_DO_FAST_PERIOD);
			break;
		}
		case ( RJM_DO_SLOW ):
		{
			emberEventControlSetDelayMS(kNwkRejoinProcedureEventControl, KK_REJOIN_DO_SLOW_PERIOD);
			break;
		}
		case ( RJM_DO_POLL ):
		{
			emberEventControlSetDelayMS(kNwkRejoinProcedureEventControl, KK_REJOIN_DO_POLL_PERIOD);
			break;
		}
		default:
		{
			g_stRejoinCtrller.eRejoinMode = RJM_DO_STOP;
			emberEventControlSetInactive(kNwkRejoinProcedureEventControl);
			break;
		}
	}

	iKonkeAfSelfPrint("Initialize Rejoin Mode to (%d).\r\n", g_stRejoinCtrller.eRejoinMode);
}

/*	DESP: Rejoin procedure method trigger interface for ZED device.
 * 	Auth: dingmz_frc.20190701.modify by maozj 20200706
 * */
kk_err_t kNwkRejoinMethodTrigger(pFUNC_VOID pRejoinedCallback, void *param)
{
	typedef struct  {
	_IO uint8_t  src_endpoint;
	_IO uint16_t group_id;
	_IO uint8_t scene_id;
	_IO uint16_t time;
	}ZclSceneRecallRepSt;

	kNwkCheckConnectonAndTriggerRejoin();

	g_stRejoinedTaskObj.pfunc = pRejoinedCallback;
	g_stRejoinedTaskObj.param = param;
	g_u32RejoinResultCheckCounter = MS2COUNT(REJOIN_SCHD_DURATION);
	kTickRunnningTrigger();

//	ZclSceneRecallRepSt *pZclSceneRecallRept = (ZclSceneRecallRepSt *)param;
//	ZclSceneRecallRepSt *pZclSceneRecallRept2 = (ZclSceneRecallRepSt *)(g_stRejoinedTaskObj.param);
	//memcpy(&zclSceneRecallRept, (ZclSceneRecallRepSt *)param, sizeof(ZclSceneRecallRepSt));

//	iKonkeAfSelfPrint("kNwkRejoinMethodTrigger111 srcEp(%d) groupId(0x%x) sceneId(0x%x)\r\n", \
//			pZclSceneRecallRept->src_endpoint, pZclSceneRecallRept->group_id, pZclSceneRecallRept->scene_id);
//	iKonkeAfSelfPrint("kNwkRejoinMethodTrigger222 srcEp(%d) groupId(0x%x) sceneId(0x%x)\r\n", \
//			pZclSceneRecallRept2->src_endpoint, pZclSceneRecallRept2->group_id, pZclSceneRecallRept2->scene_id);

	return KET_OK;
}
#endif

#ifdef ZCL_USING_IAS_ZONE_CLUSTER_SERVER
/*	DESP: Modified re-enroll interface, reference from SDK callback[emberAfIasZoneClusterServerPreAttributeChangedCallback].
 * 	Auth: dingmz_frc.20190705.
 * */
kk_err_t kkIasEnrollRequestPlagiarizeOriginal(EmberEUI64 CIE_Address )
{
	EmberBindingTableEntry bindingEntry;
	EmberBindingTableEntry currentBind;
	uint8_t endpoint = 1;

	// This code assumes that the endpoint and device that is setting the CIE
	// address is the CIE device itself, and as such the remote endpoint to bind
	// to is the endpoint that generated the attribute change.  This
	// assumption is made based on analysis of the behavior of CIE devices
	// currently existing in the field.
	bindingEntry.type = EMBER_UNICAST_BINDING;
	bindingEntry.local = endpoint;
	bindingEntry.clusterId = ZCL_IAS_ZONE_CLUSTER_ID;
	bindingEntry.remote = endpoint;
	MEMCOPY(bindingEntry.identifier, CIE_Address, EUI64_SIZE);

	// Cycle through the binding table until we find a valid entry that is not
	// being used, then use the created entry to make the bind.
	for (int index = 0; index < EMBER_BINDING_TABLE_SIZE; ++index ) {
		if( emberGetBinding(index, &currentBind) != EMBER_SUCCESS ) {
			//break out of the loop to ensure that an error message still prints
			break;
		}

		if( currentBind.type != EMBER_UNUSED_BINDING ) {
			// If the binding table entry created based on the response already exists
			// do nothing.
			if ((currentBind.local == bindingEntry.local) && (currentBind.clusterId == bindingEntry.clusterId)
				&& (currentBind.remote == bindingEntry.remote) && (currentBind.type == bindingEntry.type)) {
				break;
			}

			// If this spot in the binding table already exists, move on to the next
			continue;
		}else {
			emberSetBinding(index, &bindingEntry);
			break;
		}
	}

	// Only send the enrollment request if the mode is AUTO-ENROLL-REQUEST.
	// We need to delay to get around a bug where we can't send a command
	// at this point because then the Write Attributes response will not
	// be sent.  But we also delay to give the client time to configure us.

	/* Add by dingmz_frc.20190705:
	 * In a normal enroll process, the delay here should not be less than 1000 milliseconds,
	 * Because the write CIE address response sent by the device to gateway may not have returned correctly before that.
	 * so we must delay enough times to wait write CIE address response sent successfully before sending this
	 * enroll request.
	 * but here, wo just want to re-enroll(not normal enroll process), the CIE address(gateway`s EUI64 address) has been written successfully!
	 * so, we dont need delay more time to wait, just send this enroll request immediately!!!
	 */
	iKonkeAfSelfPrint("Sending enrollment after %d ms\r\n", 50);
	emberAfScheduleServerTickExtended(endpoint,
									  ZCL_IAS_ZONE_CLUSTER_ID,
									  50, // delay ms to sending enroll request.
									  EMBER_AF_SHORT_POLL,
									  EMBER_AF_STAY_AWAKE);

	return KET_OK;
}

/*	DESP: Enroll procedure method trigger interface for ZED, ZR device.
 * 	Auth: dingmz_frc.20190701.
 * */
kk_err_t kNwkEnrollMethodTrigger(Z3DevTypeEm dev_type, pFUNC_VOID pEnrolledCallback )
{
	kk_err_t err = KET_OK;

	switch(dev_type) {
		// Note: Since the router device has no rejoin process, we can only start enroll by writing CIE address.
		case (Z3D_ROUTER):
		{
			// If the gateway address is currently obtained, submit the enroll request directly!
			kNwkGetGateWayEui64(g_Eui64GatewayAddr);

			if( kUtilsIsValidEui64Addr(g_Eui64GatewayAddr)) {
				iKonkeAfSelfPrint("ROUTER ENROLL REQUEST\r\n");
				kkIasEnrollRequestPlagiarizeOriginal(g_Eui64GatewayAddr);
			}
			// Otherwise, we need to obtain the gateway  EUI64 address first!!
			else {
				if( emberAfFindIeeeAddress(0x00, kNwkGetGatewayEui64AddrSucc_Callback) != EMBER_SUCCESS ) {
					err = KET_ERR_OPRATE_FAILED;
				}
			}
			break;
		}
		// Note: End devices can re-enroll directly by forcing rejoin.
		case (Z3D_ENDDEVICE):
		{
			if (!emberStackIsPerformingRejoin()) {
				if((emberAfNetworkState() == EMBER_JOINED_NETWORK) || (emberAfNetworkState() == EMBER_JOINED_NETWORK_NO_PARENT)) {
					emberAfStartMoveCallback();
				}
			}
			break;
		}
		default:
		{
			err = KET_ERR_INVALID_PARAM;
			break;
		}
	}

	// check enroll result...
	if( err == KET_OK ) {
		g_stEnrolledTaskObj.pfunc = pEnrolledCallback;
		g_u32EnrollResultCheckCounter = MS2COUNT(ENROLL_SCHD_DURATION);
		kTickRunnningTrigger();
	}

	return err;
}
#endif
/** @brief Complete
 *
 * This callback is fired when the Network Steering plugin is complete.
 *
 * @param status On success this will be set to EMBER_SUCCESS to indicate a
 * network was joined successfully. On failure this will be the status code of
 * the last join or scan attempt. Ver.: always
 * @param totalBeacons The total number of 802.15.4 beacons that were heard,
 * including beacons from different devices with the same PAN ID. Ver.: always
 * @param joinAttempts The number of join attempts that were made to get onto
 * an open Zigbee network. Ver.: always
 * @param finalState The finishing state of the network steering process. From
 * this, one is able to tell on which channel mask and with which key the
 * process was complete. Ver.: always
 */
void emberAfPluginNetworkSteeringCompleteCallback(EmberStatus status,
                                                  uint8_t totalBeacons,
                                                  uint8_t joinAttempts,
                                                  uint8_t finalState)
{
	//add by maozj 20191205
	iKonkeAfSelfPrint("############NetworkSteeringComplete status(0x%x)\r\n", status);
	if (status == EMBER_JOIN_FAILED || status ==  EMBER_NO_BEACONS \
			|| status == EMBER_ERR_FATAL || status == EMBER_NETWORK_DOWN){
		//channel scan again
		//g_bNwkSteeringCompleteFlg = true;
		if (g_u32NwkJoiningCountdownCounter > 0){
			//emberAfPluginNetworkSteeringStart();
			g_bNwkSteeringMacFailureFlg = true;
		}
	}
}
/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be notified
 * of changes to the stack status and take appropriate action.  The return code
 * from this callback is ignored by the framework.  The framework will always
 * process the stack status after the callback returns.
 *
 * @param status   Ver.: always
 */
boolean emberAfStackStatusCallback(EmberStatus status )
{
	if( status == EMBER_NETWORK_UP ) { // joined and is not 'NO_PARENT'
		// NETWORK_UP in the process of joining, it means device joined successful!!
		if( g_u32NwkJoiningCountdownCounter > 0 ) {
			iKonkeAfSelfPrint("######join succeed, start the succeed procedure1111...\r\n");
			//add by maozj 20200708
			g_bIsRejoinedNwkFlg = false;
		}else { // rejoined succeed! may be power on afer joined
			iKonkeAfSelfPrint("######join succeed, start the succeed procedure2222...\r\n");
			//add by maozj 20200708
			g_bIsRejoinedNwkFlg = true;
		}

		//delay 5 seconds after  nwk joined
		uint64_t u64CurrentTimeMS = halCommonGetInt64uMillisecondTick();
		if (g_bIsRejoinedNwkFlg != true) {
			//is not power on
			g_u32NwkJoiningCountdownCounter = MS2COUNT(JOINED_DELAY_TIME_MS);
		}else {
			//fast get current network status after power on
			g_u32NwkJoiningCountdownCounter = MS2COUNT(NWK_MIN_DELAY_TIME_MS_AFTER_POWER_ON);
		}

		//start nwk detect,make sure tick event is active
		kTickRunnningTrigger();

		g_u64NetworkAnomalyBaseTimeStamp = 0;
		// Add by dingmz_frc.20190511: reset rejoin procedure mode to STOP.
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
		kNwkSetRejoinProcedureMode(RJM_DO_STOP);
#endif
	}else {
		if( g_u32NwkJoiningCountdownCounter == 0 ) {
			if((status == EMBER_NETWORK_DOWN) && (emberAfNetworkState() == EMBER_JOINED_NETWORK_NO_PARENT)) {
				kNwkNetworkStatusNotifyCall(ENS_OFFLINE);

#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
				if( g_u64NetworkAnomalyBaseTimeStamp == 0 ) {
					g_u64NetworkAnomalyBaseTimeStamp = halCommonGetInt64uMillisecondTick();
				}

				//emberAfPluginNetworkSteeringStop();
				iKonkeAfSelfPrint("################ network anomaly time: %lu ################\r\n"
					, halCommonGetInt64uMillisecondTick() - g_u64NetworkAnomalyBaseTimeStamp);

				// ������߳���Rejoin�����������ϣ���ɳ��������豸!
				if((halCommonGetInt64uMillisecondTick() - g_u64NetworkAnomalyBaseTimeStamp) > (KK_NWK_ANOMALY_UPLIMIT * 1000)) {
					iKonkeAfSelfPrint("~~ network anomaly reboot! ~~\r\n");
					halReboot();
				}else {
					// Add by dingmz_frc.20190511: rejoinʧ�ܺ����ѱ��������ģʽ�Ƿ�ΪPOLL��
					if( kNwkLoadRejoinMode() == RJM_DO_POLL ) {
						kNwkSetRejoinProcedureMode(RJM_DO_POLL);
					}
					else if( kNwkGetRejoinProcedureMode() == RJM_DO_STOP ) {
						kNwkSetRejoinProcedureMode(RJM_DO_FAST);
					}

					// �����������Rejoin���̲�δ�������У�����Ҫ���³�ʼ��Rejoin���ģ��
					if( !kNwkRejoinProcedureRunning() ) {
						iKonkeAfSelfPrint("$$$$$$$$$$$$$ Rejoin Procedure running  err!! $$$$$$$$$$$$$$\r\n");
						kNwkRejoinProcedureModeInit(RJM_DO_POLL);
					}
				}
#endif
			}
			// network leaved
			else if((status == EMBER_NETWORK_DOWN) && (emberAfNetworkState() == EMBER_NO_NETWORK)) {
				if( g_u32NwkJoiningCountdownCounter == 0 ) {
					iKonkeAfSelfPrint("Warnning: device leaved network!!\r\n");
					#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
					kNwkFactoryReset(false);
					#else
					kNwkFactoryReset(true);
					#endif
				}
			}
		}

		/*report if NodeID has change 20200601*/
		if (status == EMBER_NODE_ID_CHANGED){
			iKonkeAfSelfPrint("##EMBER_NODE_ID_CHANGED \r\n");
			kOptTunneCheckNodeIdIsChangedReport();
		}

		/* avoid changing the NodeID to stop the heartbeat  20200601*/
		if (emberAfNetworkState() != EMBER_JOINED_NETWORK){
			iKonkeAfSelfPrint("##rejoin network failed, stop the registered scheduled tasks status(0x%x).\r\n", status);
			kNwkScheduleTaskStop(SCHEDULE_ALLOPT_ID);
		}
	}

	return true;
}

/*	DESP: Whether there are still some tasks to be performed.
 * 	Auth: dingmz_frc.20191107.
 * */
bool kNwkModuleActionIsGoing(void )
{
	if(	(g_u32NwkJoiningCountdownCounter > 0) || (g_u32EnrollResultCheckCounter  > 0) ||
		(g_u32RejoinResultCheckCounter > 0)   || (g_u32BindingResultCheckCounter > 0) || (g_u32NwkExitCountDownCounter > 0)) {
		return true;
	}

	return false;
}

/*	DESP: start network steering
 * 	Auth: maozj.20191126.
 * */
void kNwkSteeringStart(void)
{
	//extern uint8_t emAfPluginNetworkSteeringCurrentChannel;
	//emAfPluginNetworkSteeringCurrentChannel = 0;
	emAfPluginNetworkSteeringCleanup(EMBER_NOT_JOINED);
	emberAfPluginNetworkSteeringStart();

}

/*	DESP: steering action trigger interface: eg. start or stop.
 * 	Note: The joining procedure will be triggered only when the network is not connected.
 * 	Auth: dingmz_frc.20190627.
 * */
void kNwkJoiningStart(uint32_t u32JoiningDuration/* MS */, pJoinCompleteCallback callback )
{
  if( u32JoiningDuration > 0 ) {

    /*begin add by maozj 20200317 not permit join network when device is aging*/
    #if Z30_DEVICE_FACTORY_TEST_ENABLE
   // if (kGetFactoryTestStatus() != FTS_NORMAL && kGetFactoryTestStatus() != FTS_FULL_DEVICE_TEST){
   //   return; //临时修改 产测判断
   // }
    #endif
    /*begin add by maozj 20200317 */

    iKonkeAfSelfPrint("\r\n777777777777777");
    uint8_t ss = 0;
    ss = (uint8_t)(emberAfNetworkState());
    iKonkeAfSelfPrint(":%d....",ss);

    #if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
    if((emberAfNetworkState() != EMBER_JOINED_NETWORK) && (emberAfNetworkState() != EMBER_JOINED_NETWORK_NO_PARENT)) {
    #else
    if(emberAfNetworkState() != EMBER_JOINED_NETWORK) {
    #endif
      //firstly close led blink whatever led status, add by maozj 20191205
      //kNwkNetworkStatusIndicator(ELS_INDI_CLOSE);
      iKonkeAfSelfPrint("#####Set Steering Counter\r\n");
      g_u32NwkJoiningDurationCounter = MS2COUNT(u32JoiningDuration);
      g_u32NwkJoiningCountdownCounter= MS2COUNT(u32JoiningDuration);
      g_pJoinCompleteCallback = callback;
      kTickRunnningTrigger();

      kNwkNetworkStatusIndicator(ELS_INDI_JOINING);//add by maozj 20200703

      //add by maozj 20191206
      g_bNwkSteeringCompleteFlg = true;
      //add by maozj 20200225
      g_eLastNetworkStatus = ENS_UNKNOW;
      //add by maozj 20200708
      g_bIsRejoinedNwkFlg = false;
      //kkPollProceduleTrigger(15, true);
      //kkPollProceduleCompletedCallback(true);
    }
  }else {
    g_u32NwkJoiningDurationCounter = MS2COUNT(0);
    g_u32NwkJoiningCountdownCounter= MS2COUNT(0);
    g_pJoinCompleteCallback = NULL;
    emberAfPluginNetworkSteeringStop();
    iKonkeAfSelfPrint("\r\n-------NetworkSteeringStop------\r\n");

    //add by maozj 20200915
    g_bIsNwkJoiningFlg = false;

    EmberNetworkStatus nwn_status = emberAfNetworkState();

    if((g_u8NwkLedId != LED_UNKNOW_ID) && (kLedIsBlinking(g_u8NwkLedId))) {
      if((nwn_status == EMBER_JOINED_NETWORK) || (nwn_status == EMBER_JOINED_NETWORK_NO_PARENT)) {
        kNwkNetworkStatusIndicator(ELS_INDI_JOINED);
      }else {
        //kNwkNetworkStatusIndicator(ELS_INDI_LEAVED);
      }
    }
  }
}

/*	DESP: device online planning task processing interface.
 * 	Auth: dingmz_frc.20191111.modify by maozj 20200708
 * */
void kNwkScheduleTaskEventHandler(EmberEventControl *control )
{
	emberEventControlSetInactive(*control);

	iKonkeAfSelfPrint("=====================================status: %d\r\n", kNwkGetCurrentStatus());

	if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
		for(int index = 0; index < SCHEDULE_TASK_MAXN; ++index ) {
			if( g_schedule_id[index] != SCHEDULE_UNKNOW_ID ) {
				if( &kNwkScheduleTaskEventControl[index] == control ) {
					g_pfScheduleTaskCallback[index](g_schedule_id[index]);
					// reset next poll period!!!
					emberEventControlSetDelayMS(kNwkScheduleTaskEventControl[index]
						, g_u32ScheduleTaskPeroidIntervalMS[index]);
				}
			}else {
				break;
			}
		}
	}
}

/*	DESP: start the scheduled task with specified ID.
 * 	Auth: dingmz_frc.20191111.
 * */
void kNwkScheduleTaskStart(uint8_t schedule_id )
{
	for(int index = 0; index < SCHEDULE_TASK_MAXN; ++index ) {
		if (g_schedule_id[index] != SCHEDULE_UNKNOW_ID){
			if((g_schedule_id[index] == schedule_id) || (schedule_id == SCHEDULE_ALLOPT_ID)) {
				if( !emberEventControlGetActive(kNwkScheduleTaskEventControl[index])) {
					// reset next poll period!!!
					emberEventControlSetDelayMS(kNwkScheduleTaskEventControl[index]
						, g_u32ScheduleTaskPeroidIntervalMS[index]);
				}

				if( schedule_id != SCHEDULE_ALLOPT_ID ) {
					break;
				}
			}
		}
	}
}

/*	DESP: stop the scheduled task with specified ID.
 * 	Auth: dingmz_frc.20191111.
 * */
void kNwkScheduleTaskStop(uint8_t schedule_id )
{
	for(int index = 0; index < SCHEDULE_TASK_MAXN; ++index ) {
		if (g_schedule_id[index] != SCHEDULE_UNKNOW_ID) {
			if((g_schedule_id[index] == schedule_id) || (schedule_id == SCHEDULE_ALLOPT_ID)) {
				if (emberEventControlGetActive(kNwkScheduleTaskEventControl[index]) == true){
					//disable binded cluster report when remote gateway config report interval time
					if (g_pNetworkStopTaskCallback){
						g_pNetworkStopTaskCallback(schedule_id);
					}
					emberEventControlSetInactive(kNwkScheduleTaskEventControl[index]);
				}

				if( schedule_id != SCHEDULE_ALLOPT_ID ) {
					break;
				}
			}
		}
	}



}

/*	DESP: device online planning task processing interface.
 * 	Note: automatically start running after registering the scheduled task by default!
 * 	Auth: dingmz_frc.20191111.
 * */
kk_err_t kNwkScheduleTaskRegister(uint8_t schid, uint32_t u32PeroidIntervalMS, pScheduleTaskHandlerCallback pfScheduleCallback )
{
	if( SCHEDULE_UNKNOW_ID == schid || SCHEDULE_ALLOPT_ID == schid || u32PeroidIntervalMS == 0 ||
		NULL == pfScheduleCallback ) {
		return KET_ERR_INVALID_PARAM;
	}

	iKonkeAfSelfPrint("Schedule Register: id(%d), PeriodInterval(%d)\r\n", schid, u32PeroidIntervalMS);

	int valid_index = SCHEDULE_TASK_MAXN;

	for(int index = 0; index < SCHEDULE_TASK_MAXN; ++index ) {
		if( g_schedule_id[index] == schid ) {
			valid_index = index;
			break;
		}else if((g_schedule_id[index] == SCHEDULE_UNKNOW_ID) && (valid_index == SCHEDULE_TASK_MAXN)) {
			valid_index = index;
		}
	}

	if( valid_index < SCHEDULE_TASK_MAXN ) {
		g_schedule_id[valid_index] = schid;
		g_u32ScheduleTaskPeroidIntervalMS[valid_index] = u32PeroidIntervalMS;
		g_pfScheduleTaskCallback[valid_index] = pfScheduleCallback;

		// Automatic startup and operation of the device when it is online!
		if( kNwkGetCurrentStatus() == ENS_ONLINE ) {
			emberEventControlSetDelayMS(kNwkScheduleTaskEventControl[valid_index], u32PeroidIntervalMS);
		}

		return KET_OK;
	}

	return KET_ERR_INSUFFICIENT_SPACE;
}

/*	DESP: network module initialize interface.
 * 	PARAM[item_list] the cluster objects requiring auto binding.
 * 	PARAM[nwk_led_id] Associate a light to indicate network status:
 * 		S1[offline] led off;
 * 		S2[joining]	led blink;
 * 		S3[online] led on for route device ,and led off for sleepy device.
 * 		Note: If this parameter is LED_UNKNOW_ID, it means that the network indicator is not associated!!
 * 	PARAM[nwk_button_id] Associate a button for network operation.
 * 	Auth: dingmz_frc.20191106.
 * */
void kNwkModuleInit(BindObjSt cluster_obj_list[], uint8_t cluster_obj_num, pNetworkStatusNotifyCallback pNSCallback
		, pNetworkExitCompleteCallback pNECallback, pNetworkStopTaskCallback pTaskStopCallback
		, uint32_t join_nwk_time, uint8_t nwk_led_id, uint8_t nwk_button_id )
{
	iKonkeAfSelfPrint("--kNwk Init------\r\n");
	for(int index = 0; index < cluster_obj_num; ++index ) {
		iKonkeAfSelfPrint("Cluster[%02d]: endpoint(%d), cluster_id(%2x)\r\n", index
			, cluster_obj_list[index].endpoint, cluster_obj_list[index].cluster);
	}
	iKonkeAfSelfPrint("NWK_LED_ID: %d\r\n", nwk_led_id);
	iKonkeAfSelfPrint("NWK_BTN_ID: %d\r\n", nwk_button_id);
	iKonkeAfSelfPrint("-----------------\r\n");

	memset(g_lstBindingClusterConf, 0XFF, sizeof(g_lstBindingClusterConf));
	memset(g_schedule_id, SCHEDULE_UNKNOW_ID, sizeof(g_schedule_id));

	if( cluster_obj_list ) {
		uint8_t objs_num = (cluster_obj_num < BINDING_CLUSTER_MAXN)?cluster_obj_num:BINDING_CLUSTER_MAXN;
		memcpy(g_lstBindingClusterConf, cluster_obj_list, sizeof(BindObjSt) * objs_num);
	}
	//add by maozj 20200511
	g_pNetworkExitCompleteCallback = pNECallback;

	g_pNetworkStatusNotifyCallback = pNSCallback;
	g_u8NwkLedId = nwk_led_id;
	g_u8NwkBtnId = nwk_button_id;
	//add by maozj20200325
	g_u32JoinNwkTimeMs = (join_nwk_time < NWK_STEERING_DEFAULT_TIMEOUT_MS)?NWK_STEERING_DEFAULT_TIMEOUT_MS : join_nwk_time;

	kBtnaIRQCallbackRegister(kNwkNetworkButtonAcitonCallback);

	iKonkeAfSelfPrint("kNwk Init: emberAfNetworkState() = %d\r\n", emberAfNetworkState());

	if( emberAfNetworkState() == EMBER_JOINED_NETWORK ) {
		kNwkNetworkStatusNotifyCall(ENS_ONLINE);
	}else if( emberAfNetworkState() == EMBER_JOINED_NETWORK_NO_PARENT ) {
		kNwkNetworkStatusNotifyCall(ENS_OFFLINE);
	}else if( emberAfNetworkState() == EMBER_NO_NETWORK ) {
		//kNwkNetworkStatusNotifyCall(ENS_LEAVED);
	}

#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
	kNwkRejoinProcedureModeInit(RJM_DO_STOP);
#endif

	//add by maozj 202008708
	g_pNetworkStopTaskCallback = pTaskStopCallback;
}
#if Z30_DEVICE_OTA_ENABLE
/** @brief Message Sent
 *
 * This function is called by the application framework from the message sent
 * handler, when it is informed by the stack regarding the message sent status.
 * All of the values passed to the emberMessageSentHandler are passed on to this
 * callback. This provides an opportunity for the application to verify that its
 * message has been sent successfully and take the appropriate action. This
 * callback should return a bool value of true or false. A value of true
 * indicates that the message sent notification has been handled and should not
 * be handled by the application framework.
 *
 * @param type   Ver.: always
 * @param indexOrDestination   Ver.: always
 * @param apsFrame   Ver.: always
 * @param msgLen   Ver.: always
 * @param message   Ver.: always
 * @param status   Ver.: always
 */
boolean emberAfMessageSentCallback(EmberOutgoingMessageType type,
                                int16u indexOrDestination,
                                EmberApsFrame* apsFrame,
                                int16u msgLen,
                                int8u* message,
                                EmberStatus status)
{
//	iKonkeAfSelfPrint("<><><><><><>(: emberAfMessageSentCallback here, msg(%2x), status(%d), err-count(%d)\r\n"
//			, apsFrame->clusterId
//			, status
//			, g_u8MsgSentErrorCounter);
	
	//kGetBatteryTriggerSet(0, DEFAULT_GETBATTERY_DELAY_S);
	//emberAfPrint(0xffff, "INTO kGetBatteryTriggerSet 777\r\n");
	//kBatteryGetVoltageWithMsgSent();  //导致复位重启
	if( status == EMBER_SUCCESS ) {
		g_u8MsgSentErrorCounter = 0;
	}else {
//		if( ++g_u8MsgSentErrorCounter > MESSAGE_SENT_ERROR_MAXN ){
//			g_u8MsgSentErrorCounter = 0;
//			//halReboot();
//		}

		if (  apsFrame->clusterId == ZCL_OTA_BOOTLOAD_CLUSTER_ID && (g_u8MsgSentErrorCounter++ >=  3)
						&& emberAfNetworkState() == EMBER_JOINED_NETWORK){
			g_u8MsgSentErrorCounter = 0;
			kOTAMoudleCallback(OTA_FAILED);
		}
	}

	return false;
}
#endif
/*	DESP: Set the gateway`s address tp flash token.
 * 	Auth: maozj.20191111.
 * */
void kNwkSetGateWayEui64(EmberEUI64 eui64)
{
	halCommonSetToken(TOKEN_GATEWAY_EUI64_VALUE, eui64);
}

/*	DESP: Get the gateway`s address from flash token.
 * 	Auth: maozj.20191111.
 * */
void kNwkGetGateWayEui64(EmberEUI64 eui64)
{
	halCommonGetToken(eui64, TOKEN_GATEWAY_EUI64_VALUE);
}

void kNwksaveNodeIdToFlash(EmberNodeId node_id)
{
	halCommonSetToken(TOKEN_NODE_ID, &node_id);
}

EmberNodeId kNwkgetNodeIdFromFlash(void)
{
	EmberNodeId node_id = 0xFFFF;
	halCommonGetToken(&node_id, TOKEN_NODE_ID);

	return node_id;
}
/*	DESP: Get whether  network is exiting
 * 	Auth: maozj.20200407.
 * */
bool kNwkIsExiting(void)
{
	if (g_u32NwkExitCountDownCounter > 0){
		return true;
	}
	return false;
}


//�����ɹ���Ļص�����,Ŀǰ���ڿ������ա�CMEI��ISN�ϱ�,�������ǿؿ�����ʱ�ϱ���׼cluster
void kNwkjoinSucceedProcedureTrigger(uint32_t u32StartDelayNms )
{
  iKonkeAfSelfPrint("\r\n!!! INTO kNwkjoinSucceedProcedureTrigger u32StartDelayNms(%dms) 111 !!!\r\n", u32StartDelayNms);
  g_stDeviceObject.u8ActionIndexAfterJoined = JOINED_PROCEDURE_REP_SNAPSHOT;
	emberEventControlSetDelayMS(kUserJoinSucceedProcedureEventControl, u32StartDelayNms);
}
//�����ɹ���������ϱ��¼����ϱ����ա�cmei��isn
/*	DESP: Joined Succeed Event Handler, delayed execution after steering or rejoin.
 * 	Auth: dingmz_frc.20190513.
 * */
void kUserJoinSucceedProcedureEventHandler(void )
{
	iKonkeAfSelfPrint("\r\n6666Join Succeed procedure here(step: %d)\r\n", g_stDeviceObject.u8ActionIndexAfterJoined);

	emberEventControlSetInactive(kUserJoinSucceedProcedureEventControl);

	//bool bIsKonkeGateway = kIsKonkeRemoteGateway();
	//kkPollProceduleCompletedCallback(false);
	// joined execute procedure
	switch(g_stDeviceObject.u8ActionIndexAfterJoined) {
		case (JOINED_PROCEDURE_REP_SNAPSHOT):
		{
			// report the device snap info.
			iKonkeAfSelfPrint("++++++++++++== report devsnap\r\n");
			kOptTunnelCommonReport(ECA_OPTDATA);
			g_stDeviceObject.u8ActionIndexAfterJoined = JOINED_PROCEDURE_REP_CMEI;
			emberEventControlSetDelayMS(kUserJoinSucceedProcedureEventControl, 500);
			break;
		}
		case (JOINED_PROCEDURE_REP_CMEI):
		{
			// report the device snap info.
			iKonkeAfSelfPrint("++++++++++++== report CMEI\r\n");
			kOptTunnelCommonReport(ECA_CMEI);
			g_stDeviceObject.u8ActionIndexAfterJoined = JOINED_PROCEDURE_REP_ISN;
			emberEventControlSetDelayMS(kUserJoinSucceedProcedureEventControl, 500);
			break;
		}
		case (JOINED_PROCEDURE_REP_ISN):
		{
			// report the device snap info.
			iKonkeAfSelfPrint("++++++++++++== report ISN\r\n");
			kOptTunnelCommonReport(ECA_ISN);
			g_stDeviceObject.u8ActionIndexAfterJoined = JOINED_PROCEDURE_END;
			emberEventControlSetDelayMS(kUserJoinSucceedProcedureEventControl, 200);
			break;
		}
		case (JOINED_PROCEDURE_END):
		{
			// report the device snap info.
			iKonkeAfSelfPrint("++++++++++++== JSP ended!\r\n");
			g_stDeviceObject.u8ActionIndexAfterJoined = JOINED_PROCEDURE_NONE;
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
			//direct sleep t
			//kkPollProceduleTrigger(0, false);
			//kkPollProceduleCompletedCallback(false);
#endif
			break;
		}
		default: break;
	}
}
