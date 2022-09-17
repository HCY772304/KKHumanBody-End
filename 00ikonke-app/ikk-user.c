#include "em_gpio.h"
#include "ikk-user.h"
#include "ikk-p824m.h"

/*********
Release Notes:
SV0.2 20200917
1.�޸�������ʱʱ��Ϊ30S
2.�޸������Ӧʱ����ʱ��Ϊ15S֮���и�Ӧ����Ҫ��ʱ�ϱ�
3.�������¶ȱ仯����������������
SV0.1 20200721
1.��ʼ�汾


 * ******/

#define HEART_DEBUG	true

uint8_t runStatus = RUN_STATUS_NORMAL ;

#define OPCODE_READ_FUN_DATA           0x00
#define OPCODE_WORK_STATE              0X01
#define OPCODE_DETECTION_AREA          0X02
#define OPCODE_LIGHT                   0X03
#define OPCODE_LIGHT_DIFFERENCE        0X04
#define OPCODE_HOLD_TIME               0X05
#define OPCODE_LED_SWITCH              0X06
#define OPCODE_EXIST_SWITCH            0X07
#define OPCODE_BLOCK_SENSOR            0X08
#define OPCODE_FINAL_SWITCH            0X09
#define OPCODE_LIGHT_COEFF             0X0B
#define OPCODE_TRIGGER_STATE           0X0C
#define OPCODE_CONNECT_PAIR            0X0F

#define HCY_TOKEN_INSTALLATION_CODE    0x1172

#define SINGLE_BOARD_TEST_DONE_VALUE          0xF1   //单板测试通过
#define FULL_DEVICE_TEST_DONE_VALUE           0xF2   //整机测试通过
#define FORCE_FACTORY_TEST_VALUE        0xF8  //强制退出

#define AGING_TEST_15MIN_VALUE      0xA1
#define AGING_TEST_8HOU_VALUE     0xA2
#define AGINE_TEST_DONE_VALUE     0xFA


#define   COM_PORT(x)           COM_USART##x
#define     RATE                9600
#define     PARITY_NONE         USART_FRAME_PARITY_NONE
#define     StopBits          1

#define SYS_BUTTON_PORT							gpioPortD
#define SYS_BUTTON_PIN							14
#define IR_SENSOR_PORT							gpioPortD
#define IR_SENSOR_PIN							15
#define TAMPER_SWITCH_PORT						gpioPortB
#define TAMPER_SWITCH_PIN						11

#define LED_YELLOW_PORT							gpioPortC
#define LED_YELLOW_PIN								10
//#define	LED_WHITE_PORT							gpioPortC
//#define LED_WHITE_PIN							11
#define	LED_SYS_PORT							gpioPortC
#define LED_SYS_PIN								11
//delay to report cmei and isn after network joined
#define DELAY_REPORT_DATA_AFTER_NWK_JOINED_MS	(3 * 1000)

#define GET_TAMPER_SWITCH_STATUS  				GPIO_PinInGet(TAMPER_SWITCH_PORT, TAMPER_SWITCH_PIN)
#define GET_IR_SENSOR_STATUS  					GPIO_PinInGet(IR_SENSOR_PORT, IR_SENSOR_PIN)

#define SYS_BUTTON_LONG_PRESS_TIME_MS			(4 * 1000)
#define SENSOR_LONG_PRESS_TIME_MS				200

//#define IR_SENSOR_LOCK_TIME_MS					(13 * 1000)  //�ܵķ���ʱ�����������Ӳ����һ������ʱ�䣬�ܹ���Լ15S
#define IR_SENSOR_LOCK_TIME_MS					(7 * 1000)
//#define IR_SENSOR_LOCK_TIME_MS					(1 * 1000)
#define IR_SENSOR_DELAY_TIME_MS					(2 * 1000)  //Blind Time ����0.5S

#define MIN_JOINED_NWK_DELAY_TIME_MS 			(7 * 1000) //�����ϵ�����ʱ�䣬�����ж��Ƿ������ʱ10~40S�ϱ�������CMEI��ISN
#define BOOT_DONE_TIME_MS						(30 *  1000 )
#if HEART_DEBUG
#define HEARTBEAT_TIME_MS						(5 * 60 * 1000)
#else
#define HEARTBEAT_TIME_MS						(20 * 60 * 1000)
#endif

#define NO_DISTURB_MODE_LED_ON_TIME_MS			(10 * 1000)
#define	OFF_DISTURB_MODE_LED_OFF_LEVEL			700

#define MAX_ON_LINE_PRESS_TIME_MS				(1 * 1000)

#define LED_SLOW_BLINK_ON_TIME_MS				800
#define LED_SLOW_BLINK_OFF_TIME_MS				800
#define LED_FAST_BLINK_ON_TIME_MS				200
#define LED_FAST_BLINK_OFF_TIME_MS				200

#if Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY
#if (SLEEPY_DEVICE_BATTERY_TYPE == BATTERY_LARGE_CAPCITY)
#define LED_SLOW_BLINK_CONTINUE_TIME_MS			(60 * 1000)
#else
#define LED_SLOW_BLINK_CONTINUE_TIME_MS			(10 * 1000)
#endif
#else
#define LED_SLOW_BLINK_CONTINUE_TIME_MS			(60 * 1000)
#endif

#define LED_FAST_BLINK_CONTINUE_TIME_MS			(5 * 1000)

//join nwk timeout
#define NWK_STEERING_TIMEOUT_MS 				(120 * 1000)

#define CHECKED_ZERO_DELAY_ON_TIME_MS			(5570)//��̵���������ʱ����⵽���󣬰������ػ������·�������ʱȥ����ʱ�䣬
#define CHECKED_ZERO_DELAY_OFF_TIME_MS			(5800)//��̵���������ʱ����⵽���󣬰������ػ������·�������ʱȥ�ص�ʱ�䣬

#define REPORT_ATTR_MAX_RAND_NUM				(7 * 1000) //�����·����ƺ���������ʱ�ϱ�״̬,״̬�ı��SDK�������ϱ�
#define REPORT_ATTR_MIN_RAND_NUM				(2.5 * 1000) //�����·����ƺ������С��ʱ�ϱ�״̬

#define BATTERY_MAX_VOLTAGE_MV					(3000)
#define BATTERY_MIN_VOLTAGE_MV  				(2700)
#define BATTERY_LOWPOWER_PERCENTAGE			(10)

#define BATTERY_LOWPOWER_LIMIT_VAL_MV			(2450)

#define CUSTOM_OTA_UPGRADE_CONTINUE_MAX_TIME_MS   (50*60*1000)

#define MAX_SAMPLE_TEMPERATURE_INTERVAL_TIMES	(2)
#define SENSOR_LOCKED_MIN_TIME_MS				(2000)

typedef enum {SCENES1=0xF1,SCENES2=0xF2,SCENES3=0xF3,SCENES4=0xF4}SCENES_STATUS;		//modify by zbw@200511

typedef enum {KEY_SYS_ID=1,/*KEY1_ID, KEY2_ID, KEY3_ID, KEY2A_ID, */IR_SENSOR_ID, TAMPER_ID}KEY_ID_ENUM;
typedef enum {LED_SYS_ID=1,LED1_ID, LED2_ID, LED3_ID, LED2A_ID, LED123_ID, LED_RED_ID, LED_YELLOW_ID}LED_ID_ENUM;
typedef enum {RELAY_CHANNEL1_ID=1,RELAY_CHANNEL2_ID, RELAY_CHANNEL3_ID, RELAY_CHANNEL4_ID}RELAY_ID_ENUM;
typedef enum {PWM1_ID=1,PWM2_ID, PWM3_ID, PWM4_ID, PWM5_ID, PWM6_ID, PWM7_ID, PWM8_ID, PWM_ALL=0xFF}PWM_ID_ENUM;
typedef enum {ADC_1_ID=1,ADC_2_ID, ADC_3_ID, ADC_4_ID, ADC_BATTERY_ID, ADC_TEMP_ID}ADC_ID_ENUM;




#if 0
typedef enum{
	NO_DISTURB_RELAY_OFF_LED_OFF = 0,
	NO_DISTURB_RELAY_ON_LED_10S_DELAY,
	NO_DISTURB_RELAY_ON_LED_OFF,
}NO_DISTURB_MODE_STATUS_E;

typedef struct{
	uint8_t endpoint;
	NO_DISTURB_MODE_STATUS_E status;
}NoDisturbMode_ST;

NoDisturbMode_ST g_stNoDisturbModeStatus[ENDPOINT_MAX_NUM] = {
		{1,NO_DISTURB_RELAY_OFF_LED_OFF},
		{2,NO_DISTURB_RELAY_OFF_LED_OFF},
		{3,NO_DISTURB_RELAY_OFF_LED_OFF},
		{4,NO_DISTURB_RELAY_OFF_LED_OFF},
};
#endif
typedef enum{
	SETUP_STATUS_INIT = 0,
	SETUP_SYN1,
	SETUP_SYN2,
	SETUP_SYN3,
	SETUP_SYN4,
	SETUP_SYN5,
	SETUP_STATUS_EXIT,
}SYSTEM_SETUP_STATUS_E;

//typedef enum{
//	SCS_INIT = 0,
//	SCS_LOCK,
//	SCS_CLEAR_INT,
//	SCS_DELAY_CHECK_BODY_EXIST,
//	SCS_REPORT_BODY_EXIST,
//	SCS_EXIT
//}SENSOR_CHECK_STATUS_E;



EmberEventControl kkSystemSetUpEventControl;
EmberEventControl kkUartMsgDelaySentEventControl;
EmberEventControl kEndpoint1DelayReportAttrEventControl;
EmberEventControl kEndpoint2DelayReportAttrEventControl;
EmberEventControl kEndpoint3DelayReportAttrEventControl;
EmberEventControl kEndpoint4DelayReportAttrEventControl;
EmberEventControl kDelaySetLedStatusEventControl;
EmberEventControl kConnectDelayReportEventControl;

EmberEventControl kkRepeatSeqenceClearEventControl;

EmberEventControl kSetFirstJionFlgEventControl;

extern EmberEventControl kPwmGradientChangeEventControl;


EmberEventControl kkMfglibStartEventControl;

EmberEventControl kkMfglibWaitReceiveEventControl;

EmberEventControl kkMfglibStopEventControl;

EmberEventControl kkDelaySendMfgMsgEventControl;

EmberEventControl  ikkNetJoinFaild1EventControl;

EmberEventControl  ikkNetJoinFaild2EventControl;


SensorStatusSt g_stSensorStatusBuffer = {
	.mcu_version = 0,
	.status = 0x0,
};

typedef struct  {
_IO uint8_t  src_endpoint;
_IO uint16_t group_id;
_IO uint8_t scene_id;
_IO uint16_t time;
}ZclSceneRecallRepSt;


// BUTTONs CONFIG
BtnConfSt g_btnConfList[] = {
	{KEY_SYS_ID, SYS_BUTTON_PORT, SYS_BUTTON_PIN, EBP_LOW, false, true, false, SYS_BUTTON_LONG_PRESS_TIME_MS},	// System Button
	{IR_SENSOR_ID, IR_SENSOR_PORT, IR_SENSOR_PIN, EBP_HIGH, false, false, false, SENSOR_LONG_PRESS_TIME_MS},	// ir sensor
//	{TAMPER_ID, TAMPER_SWITCH_PORT, TAMPER_SWITCH_PIN, EBP_BOTH, false, false,  SENSOR_LONG_PRESS_TIME_MS},	// tamper switch
	//{4, gpioPortD, 15, EBP_HIGH, false, 3000},	// Channel3
};

// LEDs CONFIG
LedConfSt g_ledConfList[] = {
	{LED_SYS_ID, 1, {LED_SYS_PORT, LED_SYS_PIN, 0, PORT_UNKNOW, PIN_UNKNOW, 0, PORT_UNKNOW, PIN_UNKNOW, 0, PORT_UNKNOW,  PIN_UNKNOW, 0}, ELP_HIGH},	// System LED
//	{LED_RED_ID, 1, {LED_WHITE_PORT, LED_WHITE_PIN, 0, PORT_UNKNOW, PIN_UNKNOW, 0, PORT_UNKNOW, PIN_UNKNOW, PORT_UNKNOW, PIN_UNKNOW, 0}, ELP_HIGH},	// sensor
//NOW, PORT_UNKNOW, PIN_UNKNOW, PORT_UNKNOW, PIN_UNKNOW}, ELP_HIGH },	// Channel2
	//{ 4, 1, {gpioPortB,11, PORT_UNKNOW, PIN_UNKNOW, PORT_UNKNOW, PIN_UNKNOW, PORT_UNKNOW, PIN_UNKNOW}, ELP_HIGH },	// Channel3

};
//{ 5, 1, {gpioPortB,12, PORT_UNKNOW, PIN_UNKNOW, PORT_UNKNOW, PIN_UNKNOW, PORT_UNKNOW, PIN_UNKNOW}, ELP_HIGH },	// Channel3 - A

// ZERO-ACCESS CONFIG
//ZaChannelConfSt g_zaConfList[] = {
	//{ 1, 1, {gpioPortF, 2, PORT_UNKNOW, PIN_UNKNOW, PORT_UNKNOW, PIN_UNKNOW, PORT_UNKNOW, PIN_UNKNOW}, ELP_HIGH },	// Channel1
	//{ 1, 1, {gpioPortF,13, PORT_UNKNOW, PIN_UNKNOW, PORT_UNKNOW, PIN_UNKNOW, PORT_UNKNOW, PIN_UNKNOW}, ELP_HIGH },	// Channel2
	//{ 1, 1, {gpioPortF,11, PORT_UNKNOW, PIN_UNKNOW, PORT_UNKNOW, PIN_UNKNOW, PORT_UNKNOW, PIN_UNKNOW}, ELP_HIGH },	// Channel3
//};

//factory  test config
#if Z30_DEVICE_FACTORY_TEST_ENABLE
FactoryTestConfSt g_factoryTestConfList[] = {
		{KEY_SYS_ID, LONG_PRESS_1_TIMES, MECHANICAL_KEY_LONG_PRESS_TIME_MS},
};
#endif
//first cluster as main cluster heartbeat to report,
BindObjSt cluster_obj_list[] = {
		//{ZCL_POWER_CONFIG_CLUSTER_ID, ZCL_BATTERY_VOLTAGE_ATTRIBUTE_ID,  1},
		{ZCL_IAS_ZONE_CLUSTER_ID, ZCL_ZONE_STATUS_ATTRIBUTE_ID, 1},
		{ZCL_DIAGNOSTICS_CLUSTER_ID, ZCL_LAST_MESSAGE_RSSI_ATTRIBUTE_ID,  1},
		{ZCL_ILLUM_MEASUREMENT_CLUSTER_ID, ZCL_ILLUM_MEASURED_VALUE_ATTRIBUTE_ID,  1},
		{ZCL_BASIC_CLUSTER_ID, ZCL_RESET_REASON_ATTRIBUTE_ID,  1},
		{ZCL_BASIC_CLUSTER_ID, ZCL_IndicateMode_ATTRIBUTE_ID,  1},
		{ZCL_BASIC_CLUSTER_ID, ZCL_Sensitivity_ATTRIBUTE_ID,  1},
		{ZCL_BASIC_CLUSTER_ID, ZCL_DelayConfiguration_ATTRIBUTE_ID,  1},
		{ZCL_BASIC_CLUSTER_ID, ZCL_MCUSoftVersion_ATTRIBUTE_ID,  1},
		{ZCL_BASIC_CLUSTER_ID, ZCL_MCUHardVersion_ATTRIBUTE_ID,  1},
		{ZCL_BASIC_CLUSTER_ID, ZCL_SlightMoveSwitch_ATTRIBUTE_ID,  1},
		{ZCL_BASIC_CLUSTER_ID, ZCL_ExistenceSwitch_ATTRIBUTE_ID,  1},
};

//Adc Sample GPIO/vdd config
//adc scan max number, can not more than 4
//AdcConfSt g_stAdcConfList[] = {
//		{ADC_BATTERY_ID, false,  PORT_UNKNOW, PIN_UNKNOW, adcPosSelAVDD, 1, BATTERY_MAX_VOLTAGE_MV, adcRef5V},
//		{ADC_TEMP_ID, false,  PORT_UNKNOW, PIN_UNKNOW, adcPosSelTEMP, 1, 1250, adcRef1V25},
//};

//�ǿؿ������״������Ƿ��ϱ���־
static bool g_bIsNotKonkeGatewayFirstReportedFlg = false;
static uint8_t g_u8CurrentHeartIndex = 0;

//#define GET_LED_ID_BY_PWM_ID(x) (x==PWM1_ID?LED1_ID:(x==PWM2_ID?LED2_ID:(x==PWM3_ID?LED3_ID:0xFF)))

static uint8_t g_u8CurrentHeartEndpoint = 1;
static uint8_t g_u8SystemSetupStatus = SETUP_STATUS_INIT;
static bool g_bIsIdentifyBlinkTesting = false;
static bool g_bWakeupFlg = false;  //��ص�ѹ����һ��ֵ�����͵紫�������ܻḴλ�󱨣�������
SENSOR_CHECK_STATUS_E g_eSensorCheckStatus = SCS_INIT;
bool g_bStartBodySensorSingleBoardTestFlg = false;
uint16_t g_u16BodySensorSequenceRecord = 0;
uint16_t g_u16BodySensorStopReportSequenceRecord = 0;
static float g_fLastGetTemperatureValue = 25.0;
static uint64_t u64LastReportMsgTime = 0;
bool g_bBatteryLowPower = false; 
bool g_bHumanBodyStopReportFlg = false;
bool g_bBodyLensTestFlg = false;
//bool g_bBanOnLightFlg = false;
//bool g_bClickSysButtonBlinkFlg = false;
static uint8_t g_u8MessageSeqRecord = 0XFF;
HUMANBODY_DATA humanbody = {0,1,50,200,10,30,1,1,0},humanbody_tocken;
static uint8_t MCUSoftVersion,MCUHardVersion;
static uint8_t grep_2 = 1,grep_3 =1,grep_5 =1,grep_6 =1,grep_7 =1,grep_9 = 2,grep_c =0,grep_f = 0;
static uint8_t setup_report_ias = 1;
FactoryTestStatusEnum g_eFactoryTestStatus = FTS_NORMAL;
static uint8_t fat_test_flag;
static bool fac_ed00 = false,fac_000a = false,fac_0000 = false,fac_0002 = false,fac_ed04 = false,fac_000b = false;
static bool setup_send_msg = false;
static uint8_t netjoinfaild = 0;
static uint8_t uart_recv_flag = 1;

bool syn_area = false,syn_hold= false,syn_led = false,syn_ex = false,syn_fin = false;


uint8_t g_u8MfgChannel = 20;
uint8_t g_u8MfgBindChannel;
bool bind_channel_flag[16] = {0};
EmberEUI64 default_eui64 = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint16_t lastPanId = 0;
static EmberEUI64 lastEui64 = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static EmberEUI64 lastEui64_ed00 = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static EmberEUI64 lastEui64_ed04 = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static EmberEUI64 lastEui64_0000 = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static EmberEUI64 lastEui64_0002 = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static EmberEUI64 lastEui64_000a = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static EmberEUI64 lastEui64_000b = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};



static int8_t lastRssi = 0;
bool bindOkFlg = false;
uint32_t magicId = 0;
uint8_t rebootFlg = 0;
static bool firstClickAfterStopFlg = false;

uint8_t g_u8OnlinePressConut = 0; //������set������

bool g_bFirstJionFlg = false;

void JoinCompleteCallback(NwkJoinCompltEnum nwkcomplt );
void kUserScheduleTaskHandlerCallback(uint8_t schedule_id );
void kUserButtonAcitonCallback(unsigned char button_id, BtnActionEnum action );
void kUserNetworkStatusNotify(NwkStatusEnum nwkst );
void kUserRttMsgInComingPaser(UMsgNodeSt* pMsgNode);
void kZclClusterRecallSceneCallback(uint8_t endpoint,  uint16_t cluster, uint8_t command_id
		, bool reportEnable, bool sceneRecallFlg, bool reportGatewayEnable, bool reportChildDeviceEnable);
void kUserLedActionDoneCallback(unsigned char id);
void kUserLedBlinkHandelCallback(unsigned char id);
void UartMsgProcessHandle(UMsgNodeSt *pMsgNode );
void UartSendHandle(uint8_t opcode,uint16_t value);
uint8_t Get_CheckSum(uint8_t *str, int str_length);
void kUserRttMsgInComingPaser(UMsgNodeSt* pMsgNode); // Jlink Rtt

#if Z30_DEVICE_FACTORY_TEST_ENABLE
void kUserFactorTestPollCallback(FactoryTestStatusEnum status);
#endif
#if Z30_DEVICE_OTA_ENABLE
void kOTAMoudleUserCallback(OTAStatusEnum status);
#endif
#if 0
void kUserPwmActionDoneCallback(unsigned char id, PwmGradientDirectionEnum opt);
#endif
void kUserNetworkExitCompleteCallback(void);
void kUserUartMsgInComingPaser(UMsgNodeSt *pMsgNode );
kk_err_t kUserOODMessageIncoming(uint8_t channel, uint8_t opcode, uint8_t args_in_out[], uint8_t *length_in_out );
void kSetIndicatorNotDisturbModeFlg(bool status);
bool kGetIndicatorNotDisturbModeFlg(void);
static void kSetLedStatus(unsigned char id);

#if 1
void kIasZoneSensorChangedStatusReport(bool ir_sensor_status, uint8_t trigger_state);
#endif


static void kBodySensorSingleBoardTestSent(void);
static void kBodySensorStopReportSent(void);
#if 0
static void kZclSceneRecall(uint8_t srcEndpoint,uint16_t groupId,uint8_t sceneId,uint16_t time);
#endif
static bool kClustePeriodReportCheckIsSet(uint8_t endpoint, uint16_t clusterID, uint16_t attributeID);
#if 0
static void kPwmClearCountAndCloseLedByPwmID(PWM_ID_ENUM pwmId, bool bIsClearGradientCounter);
#endif

static void kZclClusterReportRecordBeforeRejoin(uint8_t srcEndpoint, uint16_t groupId, uint8_t sceneId, uint16_t time);
static void kZclSceneRecallReportCallback(void *param);
static void  kUserNetworkStopTaskCallback(uint8_t schedule_id);
static void kClearBatteryVoltageAndPercentage(void);

static void kSystemSetupStatusSet(uint8_t systemStartedStatus, uint32_t delayTimeMS);
#if 0
static bool SceneActionEnableflag[4] = {1,1,1,1};
static uint64_t SceneActionOTATime[4] = {0, 0, 0, 0};
#endif
/** @brief Main Init
 *
 * This function is called from the application's main function. It gives the
 * application a chance to do any initialization required at system startup. Any
 * code that you would normally put into the top of the application's main()
 * routine should be put into this function. This is called before the clusters,
 * plugins, and the network are initialized so some functionality is not yet
 * available.
        Note: No callback in the Application Framework is
 * associated with resource cleanup. If you are implementing your application on
 * a Unix host where resource cleanup is a consideration, we expect that you
 * will use the standard Posix system calls, including the use of atexit() and
 * handlers for signals such as SIGTERM, SIGINT, SIGCHLD, SIGPIPE and so on. If
 * you use the signal() function to register your signal handler, please mind
 * the returned value which may be an Application Framework function. If the
 * return value is non-null, please make sure that you call the returned
 * function from your handler to avoid negating the resource cleanup of the
 * Application Framework itself.
 *
 */
void emberAfMainInitCallback(void)
{
	UserModuleInit();
}
#if 0
/** @brief Called whenever the GPIO sensor detects a change in state.
 *
 * @param newSensorState The new state of the sensor based alarm
 * (EMBER_AF_PLUGIN_GPIO_SENSOR_ACTIVE or
 * EMBER_AF_PLUGIN_GPIO_SENSOR_NOT_ACTIVE)  Ver.: always
 */
void emberAfPluginGpioSensorStateChangedCallback(uint8_t newSensorState)
{
	iKonkeAfSelfPrint("Sensor Status(%d)", newSensorState);
	//���𿪹ش���
	//kIasZoneSensorChangedStatusReport(false,newSensorState?true:false);
}
#endif

/*  DESP: User Module init interface.
 *  Auth: dingmz_frc.20190623.
 * */
void UserModuleInit(void )
{
  kk_err_t err = KET_OK;
  kRttModuleInit(kUserRttMsgInComingPaser);

  iKonkeAfSelfPrint("\r\n************************************************\r\n");
  iKonkeAfSelfPrint("xxx Reboot Reason: R-INFO(0x%x: %p), R-EXTEND(0x%x: %p) xxx\r\n"
    , halGetResetInfo(), halGetResetString()
    , halGetExtendedResetInfo(), halGetExtendedResetString());
  iKonkeAfSelfPrint("xxx Network Status: %d, systime: %d xxx\r\n", emberAfNetworkState()
    , halCommonGetInt32uMillisecondTick());
  iKonkeAfSelfPrint("\r\n************************************************\r\n");

  //kAdcModuleInit(g_stAdcConfList, sizeof(g_stAdcConfList) / sizeof(AdcConfSt), 5000);

#if 1



  kNwkModuleInit(cluster_obj_list, sizeof(cluster_obj_list) / sizeof(BindObjSt), kUserNetworkStatusNotify \
            , kUserNetworkExitCompleteCallback, kUserNetworkStopTaskCallback, NWK_STEERING_TIMEOUT_MS, LED_UNKNOW_ID, BTN_UNKNOW_ID);

  kZclClusterPermitReportTableInit(cluster_obj_list, sizeof(cluster_obj_list) / sizeof(BindObjSt));//add by maozj 20200514


  // ZCL
  //kZclOnOffClusterServerInit(kUserOnOffClusterOnOffStatusCallback);
  //kZclOTAClusterClientInit(1);

  // init the private cluster protocol module.
  kOptTunnelModuleInit(kUserOODMessageIncoming);

  #if 1
	uint8_t uart_recv_header[2] = { 0x00,0x00 };

	if (kGetFactoryTestStatus() == FTS_NORMAL){
	   kUartModuleInit(COM_USART0, RATE, PARITY_NONE,StopBits,uart_recv_header, 2, CheckSum,kUserUartMsgInComingPaser, false );
	   iKonkeAfSelfPrint("\r\n***********UART0_9600_NIT SUCCESS*********\r\n");
	}

#endif

  //kkPollProceduleTrigger(5, true);

#if 1
  kNwkScheduleTaskRegister(1,  HEARTBEAT_TIME_MS, kUserScheduleTaskHandlerCallback);
  //kNwkScheduleTaskRegister(2, 3600000UL, kUserScheduleTaskHandlerCallback);
  //kNwkScheduleTaskRegister(3,  5000, kUserScheduleTaskHandlerCallback);
  //kNwkScheduleTaskRegister(4, 10000, kUserScheduleTaskHandlerCallback);
#endif
  if (kGetFactoryTestStatus() == FTS_NORMAL){
      kSystemSetupStatusSet(SETUP_STATUS_INIT, BOOT_DONE_TIME_MS);
  }
  //emberEventControlSetDelayMS(kkSystemSetUpEventControl, 50);
  //emberEventControlSetInactive(kkUartMsgDelaySentEventControl);

  GPIO_DbgSWOEnable(false);

  //kCmdCommandConsoleModuleInit();
#if Z30_DEVICE_OTA_ENABLE
  emberAfOtaStorageClearTempDataCallback();
  kOTAMoudleInit(kOTAMoudleUserCallback, LED_SYS_ID);
#endif
#if Z30_DEVICE_FACTORY_TEST_ENABLE
  //kFactoryTestInit(kUserFactorTestPollCallback, g_factoryTestConfList, sizeof(g_factoryTestConfList)/ sizeof(FactoryTestConfSt), LED_SYS_ID);
#endif

 
#endif


  kOptTunnelTrytoWriteInstallCode();

  
  if (kNwkGetCurrentStatus() == ENS_LEAVED && halGetResetInfo() == 0x04 && kGetFactoryTestStatus() == FTS_NORMAL) {
	  kNwkJoiningStart(LED_SLOW_BLINK_CONTINUE_TIME_MS, JoinCompleteCallback);
  }



  if(kNwkGetCurrentStatus() == ENS_LEAVED && kGetFactoryTestStatus() == FTS_FULL_DEVICE_TEST){
      kkMfglibRunningStart(20);
  }

}


void kUserNetworkStatusNotify(NwkStatusEnum nwkst )
{
	iKonkeAfSelfPrint("###########################kUserNetworkStatusNotify = %d\r\n", nwkst);
	//static NwkStatusEnum lastStatus = ENS_UNKNOW;
	switch(nwkst) {
		case (ENS_OFFLINE):
		{
			//LED_OPT_OFF(LED_WHITE_ID);
			break;
		}
		case (ENS_ONLINE ):
		{
			uint32_t randTimeMS = DELAY_REPORT_DATA_AFTER_NWK_JOINED_MS;
			//power on after joined network
			uint64_t currentTimeMS = halCommonGetInt64uMillisecondTick();
			if ( currentTimeMS < MIN_JOINED_NWK_DELAY_TIME_MS){

				//randTimeMS = kUtilsGetRandNum(RAND_MIN_NUM, RAND_MAX_NUM);
			}else {
				//���������ϵ��λ����ɫ��˸��ر�
				//LED_OPT_OFF(LED123_ID);
				//�����ɹ�����5SϨ��
				//kLedOptTrigger(LED_SYS_ID, LED_FAST_BLINK_CONTINUE_TIME_MS, LED_SLOW_BLINK_OFF_TIME_MS, 1, LED_ON, LED_OFF);
			}
			//�ָ����а������ϱ�ʹ��
			//kSystemSetupStatusSet(SETUP_STATUS_EXIT, 10);
			kZclClusterSetPermitReportInfo(1, ZCL_DIAGNOSTICS_CLUSTER_ID,  true, false, true, false);
			//kZclClusterSetPermitReportInfo(1, ZCL_POWER_CONFIG_CLUSTER_ID,  true, false, false, false);
			kZclClusterSetPermitReportInfo(1, ZCL_BASIC_CLUSTER_ID,  true, false, true, false);
			uint16_t temp_reset = halGetExtendedResetInfo();
			emberAfWriteServerAttribute(1,
			                       ZCL_BASIC_CLUSTER_ID,
			                       ZCL_RESET_REASON_ATTRIBUTE_ID,
			                       (uint8_t *)&temp_reset,
			                       ZCL_INT16U_ATTRIBUTE_TYPE);
//			iKonkeAfSelfPrint("Joined Network RandTime(%dms) CurrentTime(%ld)\r\n", randTimeMS, currentTimeMS);
			iKonkeAfSelfPrint("!!!!!!!!!!!INTO ENS_ONLINE temp_reset = (%02x) 111 !!!!!!!!!!!\r\n",temp_reset);
			iKonkeAfSelfPrint("!!!!!!!!!!!INTO ENS_ONLINE!!!!!!!!!!!\r\n");
//			kNwkjoinSucceedProcedureTrigger(randTimeMS);
			//add by maozj 20200608
			g_u8CurrentHeartIndex = 0;
			//if (kIsKonkeRemoteGateway() != true){
				g_bIsNotKonkeGatewayFirstReportedFlg = false;
				//report heartbeat data
				kNwkScheduleTaskRegister(1, randTimeMS, kUserScheduleTaskHandlerCallback);
			//}

			break;
		}
		case (ENS_JOINING):
		{
			//kPwmClearCountAndCloseLedByPwmID(PWM_ALL, true);
		//	LED_OPT_OFF(LED123_ID);
		//	kLedOptTrigger(LED_SYS_ID, LED_SLOW_BLINK_ON_TIME_MS, LED_SLOW_BLINK_OFF_TIME_MS, \
		//		LED_SLOW_BLINK_CONTINUE_TIME_MS/(LED_SLOW_BLINK_ON_TIME_MS+LED_SLOW_BLINK_OFF_TIME_MS), LED_ON, LED_OFF);
//			iKonkeAfSelfPrint("!!!!!!!!!!!INTO LED_SYS_ID BLINK 111!!!!!!!!!!!\r\n");
			break;
		}
		case (ENS_LEAVED):
		{
			iKonkeAfSelfPrint("!!!!!!!!!!!INTO kUserNetworkStatusNotify ENS_LEAVED 1111!!!!!!!!!!!\r\n");
			//������رջ�ɫLED
			//LED_OPT_OFF(LED_SYS_ID);
			//kPwmClearCountAndCloseLedByPwmID(PWM_ALL, true);
			//LED_OPT_OFF(LED_YELLOW_ID);
			bool isFullDeviceTestDoneFlg = false;
			if (kGetFactoryTestStatus() == FTS_FULL_DEVICE_TEST_DONE){
				isFullDeviceTestDoneFlg = true;
			}
		//	kLedOptTrigger(LED_SYS_ID, LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_OFF_TIME_MS, \
		//			isFullDeviceTestDoneFlg != true?LED_FAST_BLINK_CONTINUE_TIME_MS/(LED_FAST_BLINK_ON_TIME_MS+LED_FAST_BLINK_OFF_TIME_MS):0xFFFFFF, LED_ON, LED_OFF);
//			iKonkeAfSelfPrint("!!!!!!!!!!!INTO LED_SYS_ID BLINK 222!!!!!!!!!!!\r\n");
			//network leave open led blink when humanbody exist
			//�򿪴���ģʽ������Ӧ���˺����2��
			kSetIndicatorNotDisturbModeFlg(false);
			//kZclOnOffClusterServerOnOffControl(1, kZclOnOffClusterServerOnOffGet(1));
			//kZclOnOffClusterServerOnOffControl(2, kZclOnOffClusterServerOnOffGet(2));
			//kZclOnOffClusterServerOnOffControl(3, kZclOnOffClusterServerOnOffGet(3));
			break;
		}
		default:
		{
	//		LED_OPT_OFF(LED_SYS_ID);
//			iKonkeAfSelfPrint("!!!!!!!!!!!INTO LED_SYS_ID BLINK 333!!!!!!!!!!!\r\n");
			break;
		}
	}
	//lastStatus = nwkst;
}

#if 1
void JoinCompleteCallback(NwkJoinCompltEnum nwkcomplt )
{
	iKonkeAfSelfPrint("#########JoinCompleteCallback = %d\r\n", nwkcomplt);
	switch(nwkcomplt) {
		case (EJC_JOIN_FAILED ):
		{
			//LED_OPT_OFF(LED_SYS_ID);
//			iKonkeAfSelfPrint("!!!!!!!!!!!INTO LED_SYS_ID BLINK 444!!!!!!!!!!!\r\n");
//			kBatterySetToken();
//			kUserNetworkExitCompleteCallback();
			kNwkFactoryReset(false);
			iKonkeAfSelfPrint("\r\n-------[ZB state:EJC_JOIN_FAILED]-------\r\n");
			//grep_f ++;
			//UartSendHandle(0x0f,0);   //入网失败，LED灭10s
			uart_recv_flag = 0;
			UartSendHandle(0x0f,1);  //入网成功，LED常亮10s
            netjoinfaild = 1;
			emberEventControlSetDelayMS(ikkNetJoinFaild1EventControl,800);
			break;
		}
		case (EJC_JOIN_SUCCEED):  //入网成功
		{
		  iKonkeAfSelfPrint("\r\n-------[ZB state:EJC_JOIN_SUCCESS]-------\r\n");
		  UartSendHandle(0x0f,1);  //入网成功，LED常亮10s

			if (kGetFactoryTestStatus() == FTS_NORMAL){
				//kkPollProceduleTrigger(SHORT_POLL_TIME_AFTER_JOIN_NWK_S, true);
				g_bFirstJionFlg = true;
				emberEventControlSetDelayMS(kSetFirstJionFlgEventControl, 300);
				iKonkeAfSelfPrint("!!!!!!!!!!!!!!INTO SHORT_POLL !!!!!!!!!!!\r\n");
			}else {
				//kkPollProceduleTrigger(0xFF, 1);
			}


			break;
		}
		case (EJC_CONNLINK_OK ):
		{
			//kNwkjoinSucceedProcedureTrigger(3000);
			break;
		}
		default: break;
	}
}
#endif
void kUserButtonAcitonCallback(unsigned char button_id, BtnActionEnum action )
{

}


//�������̵���������ɣ�ͬ��LED״̬�����ǹ�������̵���Ҳ�ǵ��������LED
void kUserRelayLedCallback(RelayStatusSt status)
{
	iKonkeAfSelfPrint("#########kUserZaLedCallback channel(%d) opt(%d)\r\n", status.u8RelayChannelId, status.opt);

}

void kSetIndicatorNotDisturbModeFlg(bool status) //�˹���������ʾ�Ƿ�ر������Ӧָʾ�ƣ�trueΪ�أ�falseΪ��
{
	halCommonSetToken(TOKEN_INDICATOR_NOT_DISTURB_MODE_FLG, &status);
}

bool kGetIndicatorNotDisturbModeFlg(void)
{
	bool status = false;
	halCommonGetToken(&status, TOKEN_INDICATOR_NOT_DISTURB_MODE_FLG);
	return status;
}



//固定事件，用于心跳上报
void kUserScheduleTaskHandlerCallback(uint8_t schedule_id )
{
	iKonkeAfSelfPrint("###### SCH_ID(%d).\r\n", schedule_id);
	iKonkeAfSelfPrint("!!!!!!!!!INTO SCH_ID time111(%d) !!!!!!!!!!!\r\n", halCommonGetInt64uMillisecondTick());
	uint32_t randTimeMS = NWK_MIN_DELAY_TIME_MS_AFTER_POWER_ON * 2;
	switch(schedule_id) {
		case (1):
		{
           if(kIsKonkeRemoteGateway() == true){
		   	 kOptTunnelCommonReport(ECA_OPTDATA);
           }else{
              if(kClustePeriodReportCheckIsSet(1, ZCL_IAS_ZONE_CLUSTER_ID, ZCL_ZONE_STATUS_ATTRIBUTE_ID) == true){
			  	  kZclClusterHeartbeatControl(1, ZCL_IAS_ZONE_CLUSTER_ID, ZCL_ZONE_STATUS_ATTRIBUTE_ID);
              	}
           }
		   kNwkScheduleTaskRegister(1,	HEARTBEAT_TIME_MS, kUserScheduleTaskHandlerCallback);
		   break;

	}
		default: break;
		}

}


//���س��������·����������ʱ�ϱ��������Ը��ݲ�ͬ����cluster�޸�
/*	DESP: recall scene rand delay report Attribute set
 * 	Auth: maozj.20191224.
 * */
void kZclClusterRecallSceneCallback(uint8_t endpoint,  uint16_t cluster, uint8_t command_id
		, bool reportEnable, bool sceneRecallFlg, bool reportGatewayEnable, bool reportChildDeviceEnable)
{

}
//��������ʱ������report,��ʱ�ϱ����ԣ�һ�㲻��Ҫ�޸�
bool kZclClusterReportChangeCallback(uint8_t endpoint, EmberAfClusterId clusterId, EmberAfAttributeId attributeId, EmberAfDifferenceType *difference, uint32_t reportableChange)
{
	iKonkeAfSelfPrint("kZclClusterReportChangeCallback: endpoint(%d), clusterId(0x%2x), attributeId(0x%2x).", endpoint, clusterId, attributeId);


	if (clusterId == ZCL_ILLUM_MEASUREMENT_CLUSTER_ID ||  clusterId == ZCL_BASIC_CLUSTER_ID){

	return true;
	}else if(clusterId == ZCL_IAS_ZONE_CLUSTER_ID ){
	    if(setup_report_ias == 1){
	        return true;
	    }else if(setup_report_ias == 0){
	        return false;
	    }
	}
//	return false;
}
//������ʱ�ϱ��¼����ɸ��ݲ�ͬ��������ʱ�ϱ��Զ����޸�
void kEndpoint1DelayReportAttrEventHandler(void)
{
	emberEventControlSetInactive( kEndpoint1DelayReportAttrEventControl);
	kZclClusterSetPermitReportInfo(1, ZCL_ON_OFF_CLUSTER_ID, true, false, true, false);
	uint8_t newValue = 1;
	emberAfWriteAttribute(1,
						   ZCL_ON_OFF_CLUSTER_ID,
						   ZCL_ON_OFF_ATTRIBUTE_ID,
						   CLUSTER_MASK_SERVER,
						   &newValue,
						   ZCL_BOOLEAN_ATTRIBUTE_TYPE);
}
void kEndpoint2DelayReportAttrEventHandler(void)
{
	emberEventControlSetInactive( kEndpoint2DelayReportAttrEventControl);
	kZclClusterSetPermitReportInfo(2, ZCL_ON_OFF_CLUSTER_ID, true, false, true, false);
	uint8_t newValue = 1;
	emberAfWriteAttribute(2,
						   ZCL_ON_OFF_CLUSTER_ID,
						   ZCL_ON_OFF_ATTRIBUTE_ID,
						   CLUSTER_MASK_SERVER,
						   &newValue,
						   ZCL_BOOLEAN_ATTRIBUTE_TYPE);
}

void kEndpoint3DelayReportAttrEventHandler(void)
{
	emberEventControlSetInactive( kEndpoint3DelayReportAttrEventControl);
	kZclClusterSetPermitReportInfo(3, ZCL_ON_OFF_CLUSTER_ID, true, false, true, false);
	uint8_t newValue = 1;
	emberAfWriteAttribute(3,
						   ZCL_ON_OFF_CLUSTER_ID,
						   ZCL_ON_OFF_ATTRIBUTE_ID,
						   CLUSTER_MASK_SERVER,
						   &newValue,
						   ZCL_BOOLEAN_ATTRIBUTE_TYPE);
}

void kEndpoint4DelayReportAttrEventHandler(void)
{
	emberEventControlSetInactive( kEndpoint4DelayReportAttrEventControl);
	kZclClusterSetPermitReportInfo(4, ZCL_ON_OFF_CLUSTER_ID, true, false, true, false);
	uint8_t newValue = 1;
	emberAfWriteAttribute(4,
						   ZCL_ON_OFF_CLUSTER_ID,
						   ZCL_ON_OFF_ATTRIBUTE_ID,
						   CLUSTER_MASK_SERVER,
						   &newValue,
						   ZCL_BOOLEAN_ATTRIBUTE_TYPE);
}


void kSetFirstJionFlgEventHandler(void)
{
	emberEventControlSetInactive( kSetFirstJionFlgEventControl);
	g_bFirstJionFlg = false;
	
  iKonkeAfSelfPrint("\r\n-------SetFirstJionFlgEventHandler--------\r\n");

  kOptTunnelCommonReport(ECA_OPTDATA);

  
}



void ikkNetJoinFaild1EventHandler(void)
{
	emberEventControlSetInactive( ikkNetJoinFaild1EventControl);
	
  iKonkeAfSelfPrint("\r\n-------ikkNetJoinFaildEventHandler 111--------\r\n");

	UartSendHandle(OPCODE_LED_SWITCH,0); //关闭LED
  	emberEventControlSetDelayMS(ikkNetJoinFaild2EventControl,10000);

  
}

void ikkNetJoinFaild2EventHandler(void)
{
  emberEventControlSetInactive( ikkNetJoinFaild2EventControl);
	
  iKonkeAfSelfPrint("\r\n-------ikkNetJoinFaildEventHandler  222--------\r\n");

  UartSendHandle(OPCODE_LED_SWITCH,1); //打开LED

  uart_recv_flag = 1;
  
}


void kConnectDelayReportEventHandler(void)
{
	emberEventControlSetInactive( kConnectDelayReportEventControl);
	if (kIsKonkeRemoteGateway() == true) {
		// report connected...
		kOptTunnelOODReport(0, 0XCD, NULL, 0);
	}
}



//LED��˸��ɣ����������ָ��̵�����LED״̬
void kUserLedActionDoneCallback(unsigned char id)
{
	iKonkeAfSelfPrint("##############kUserLedActionDoneCallback id(%d)##################\r\n", id);
	kSetLedStatus(id);
}

void kUserLedBlinkHandelCallback(unsigned char id)
{

}

#if Z30_DEVICE_FACTORY_TEST_ENABLE
//�ϻ�����ѯ�ص�������һ������ʵ���ϻ��ڼ��LED��˸��̵�����ת��Ҳ����ֻLED��˸
void kUserFactorTestPollCallback(FactoryTestStatusEnum status)  //产测流程
{
	iKonkeAfSelfPrint("##############kUserFactorTestPollCallback status(%d)##################\r\n", status);
//	static bool FactoryStatus = 0;
	switch(status)
	{
		case FTS_NORMAL:
			break;
		case FTS_START:
			//û���ϻ������ȶ��������
//			kNwkFactoryReset(false);
//			//�رջ�ɫLED
//			LED_OPT_OFF(LED_SYS_ID);
			break;
		case FTS_AGING_15MIN:
			break;
		case FTS_AGING_8HOU_START:
			//�ر����е���ɫLED,�п���֮ǰ������˸��
	//		LED_OPT_OFF(LED123_ID);
			//ͬ�����е�LED�ͼ̵���
			break;
		case  FTS_AGING_8HOU:
			//��ת�̵�����LEDҲ��ͬ��
//			LED_OPT_TOGGLE(LED123_ID);
//			kLedOptTrigger(LED123_ID, 500, 500, 1, LED_ON, LED_OFF);
			break;
		case  FTS_AGING_DONE:
			//�ϻ�������,�����е�LED�ͼ̵���
			LED_OPT_ON(LED123_ID);
			//����е���ϵͳLEDҲ��Ҫ������
//			LED_OPT_ON(LED_SYS_ID);
			break;
		case FTS_FULL_DEVICE_TEST_DONE:
			break;
		case FTS_FORCE_EXIT:
			iKonkeAfSelfPrint("############## FactoryTestSatus is FTS_AGING_FORCE_EXIT\r\n");
			//����ǿ�����ϻ�/����/����������
			kLedOptTrigger(LED_SYS_ID, LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_OFF_TIME_MS, BLINK_DEAD, LED_ON, LED_IGNORE);
//			iKonkeAfSelfPrint("!!!!!!!!!!!INTO LED_SYS_ID BLINK 111111!!!!!!!!!!!\r\n");
			break;
		case FTS_WAITING_RELEASED:
			break;
		case FTS_FORCE_REBOOT:
		  iKonkeAfSelfPrint("!!!!!!!!!!!INTO FactoryTestSatus is FTS_FORCE_REBOOT 2222!!!!!!!!!!!\r\n");
			halReboot();
			break;
		default:
			iKonkeAfSelfPrint("##############Erro: FactoryTestStatus is not exist\r\n");
			break;
	}
}
#endif

#if Z30_DEVICE_OTA_ENABLE
//�����ʼ��OTA����callback���ﴦ������ʹ��otaģ���Ĭ��callback,�Ͳ�������
void kOTAMoudleUserCallback(OTAStatusEnum status)
{
#if 1
	static bool flg = false; //��ֹ���ضϵ��,otaʧ�ܺ��������
	iKonkeAfSelfPrint("##############kOTAMoudleUserCallback status(%d)##################\r\n", status);
	switch(status)
	{
		case OTA_NORMAL:
			break;
		case OTA_START:
			//�ر�������ɫLED
			//LED_OPT_OFF(LED_SYS_ID);
			kLedOptTrigger(LED_SYS_ID, 2 * LED_FAST_BLINK_ON_TIME_MS, 14 * LED_FAST_BLINK_ON_TIME_MS,\
					CUSTOM_OTA_UPGRADE_CONTINUE_MAX_TIME_MS /(2 * LED_FAST_BLINK_ON_TIME_MS + 14 * LED_FAST_BLINK_ON_TIME_MS), LED_ON, LED_OFF);
//			iKonkeAfSelfPrint("!!!!!!!!!!!INTO LED_SYS_ID BLINK 121212!!!!!!!!!!!\r\n");
			kSetOTAStatus(OTA_START);
			flg = false;
			emberAfSetShortPollIntervalMsCallback(500);
			break;
		case OTA_DOWNLOAD_DONE:
		{
			uint8_t buffer[] = {"Konke download disable watchdog.....\r\n"};
		 	  //����ע�͵��� ��Ȼ�޷����ÿ��Ź������µڶ�������������������Ϊ�������൱�ڼ�����ʱ
		  	emberSerialWriteData((uint8_t)APP_SERIAL, buffer, strlen((char *)buffer));
		  	halInternalDisableWatchDog(MICRO_DISABLE_WATCH_DOG_KEY);//add by maozj 20190308�ļ������غú���ÿ��Ź�
		  	kSetOTAStatus(OTA_DOWNLOAD_DONE);
		  	break;
		}
		case OTA_VERITY_SUCCEED:
		{
			emberAfSetShortPollIntervalMsCallback(1000);
			//����5S
			kLedOptTrigger(LED_SYS_ID, 25 * LED_FAST_BLINK_ON_TIME_MS,  LED_FAST_BLINK_ON_TIME_MS, 1, LED_ON, LED_OFF);
//			iKonkeAfSelfPrint("!!!!!!!!!!!INTO LED_SYS_ID BLINK 131313!!!!!!!!!!!\r\n");
			kBatterySetToken();
			kSetOTAStatus(OTA_NORMAL);
			break;
		}
		case OTA_FAILED:
		{
			if (flg != true){
				emberAfSetShortPollIntervalMsCallback(1000);
				halInternalWatchDogEnabled();  //ʹ�ܿ��Ź�
				emberAfOtaStorageClearTempDataCallback();
				kLedOptTrigger(LED_SYS_ID, LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_ON_TIME_MS,\
									LED_FAST_BLINK_CONTINUE_TIME_MS /(LED_FAST_BLINK_ON_TIME_MS + LED_FAST_BLINK_ON_TIME_MS), LED_ON, LED_OFF);
//				iKonkeAfSelfPrint("!!!!!!!!!!!INTO LED_SYS_ID BLINK 141414!!!!!!!!!!!\r\n");
				kSetOTAStatus(OTA_NORMAL);
			}
			flg = true;
			break;
		}
		default:
			iKonkeAfSelfPrint("##############Err: OTA status is not exist\r\n");
			break;
	}

#endif
}
#endif

//����5S����������Ļص�������һ��������塢�����ʹ������������ɺ���ȫ���ر�ʱ��λ
void kUserNetworkExitCompleteCallback(void)
{
	if (kGetFactoryTestStatus() != FTS_NORMAL){
		return;
	}
	//halReboot();
   //iKonkeAfSelfPrint("Start Steering 2222\r\n");
   //kNwkJoiningStart(NWK_STEERING_TIMEOUT_MS, JoinCompleteCallback);
}


uint16_t g_u16FactoryTestSeq = 0;


void kkMfglibRunningStart(uint8_t channel)
{
  g_u8MfgChannel = channel;
  bindOkFlg = false;
  emberEventControlSetActive(kkMfglibStartEventControl);
}


/*  DESP: rtt command message incoming and process interface.
 *  Auth: kkk.20210222.
 * */
void kUserRttMsgInComingPaser(UMsgNodeSt* pMsgNode)
{
    uint16_t opcode;
    uint16_t frameLenth = 0;
    uint16_t crc16DataIn = 0;
    uint16_t crc16Value = 0;
    uint8_t* args_in;
    uint8_t reply_control_field = Z_TO_H_NO_ACK;
    DataField_st send_buf;

    if (NULL == pMsgNode) {
        return;
    }

#if 1 // Just for debug
    iKonkeAfSelfPrint("\r\n#### RTT MSG INCOMING[%d]:\r\n", pMsgNode->length);
    iKonkeAfSelfPrintBuffer(pMsgNode->buffer, pMsgNode->length, 0);
    iKonkeAfSelfPrint("\r\n--------------------\r\n");
#endif

    // check SOF
    if ((pMsgNode->buffer[0] != 0xAA) || (pMsgNode->buffer[1] != 0x55)) {
        return;
    }

    frameLenth = UINT16_HL(pMsgNode->buffer[2], pMsgNode->buffer[3]);
    // check CRC16
    crc16Value = kCmdGetMsgCrc16Value(&pMsgNode->buffer[4], frameLenth);
    crc16DataIn = UINT16_HL(pMsgNode->buffer[pMsgNode->length - 2], pMsgNode->buffer[pMsgNode->length - 1]);

    if (crc16Value != crc16DataIn) {
        iKonkeAfSelfPrint("Err: CRC(%d)(%d)\r\n", crc16Value, crc16DataIn);
        return;
    }

    // check for parsable packets
    if (pMsgNode->length < 10 /* Minimum Packet Length */) {
        iKonkeAfSelfPrint("Err: Unparsable Packets!!\r\n");
        return;
    }

    opcode = ((uint16_t)pMsgNode->buffer[9] << 8) | (uint16_t)(pMsgNode->buffer[10]);
    args_in = &pMsgNode->buffer[8];
    send_buf.seq = g_u16FactoryTestSeq++;
    send_buf.u8ChannelID = args_in[0];
    send_buf.u16Opcode = opcode;
    send_buf.u8Datalen = 3;
    iKonkeAfSelfPrint("Rtt-opcode(%d)\r\n", opcode);

    switch (opcode) {

       case UART_MSG_QUERY_DEVICE_VERSION_OPCODE:{
         send_buf.u8ARG[0] = ERR_OK;
         uint8_t hardversion = 0;
         uint8_t softversion = 0;
         emberAfReadServerAttribute(1, ZCL_BASIC_CLUSTER_ID, ZCL_HW_VERSION_ATTRIBUTE_ID,
                                     (uint8_t *)&hardversion,sizeof(uint8_t));
         emberAfReadServerAttribute(1, ZCL_BASIC_CLUSTER_ID, ZCL_APPLICATION_VERSION_ATTRIBUTE_ID,
                                     (uint8_t *)&softversion,sizeof(uint8_t));
         send_buf.u8ARG[1] = hardversion;
         send_buf.u8ARG[2] = softversion;
         send_buf.u8ARG[3] = 0XFF;
         send_buf.u8ARG[4] = 0XFF;
         send_buf.u8ARG[0] = ERR_OK;
         send_buf.u8Datalen = 5;
         reply_control_field = Z_TO_H_NO_ACK;
         break;
       }
       case UART_MSG_QUERY_DEVICE_INFO_OPCODE:{       //0xED01
         send_buf.u8ARG[0] = ERR_OK;
          EmberEUI64 deviceEui;
          MEMMOVE(deviceEui, emberGetEui64(), EUI64_SIZE);
          send_buf.u8ARG[1] = deviceEui[7];
          send_buf.u8ARG[2] = deviceEui[6];
          send_buf.u8ARG[3] = deviceEui[5];
          send_buf.u8ARG[4] = deviceEui[4];
          send_buf.u8ARG[5] = deviceEui[3];
          send_buf.u8ARG[6] = deviceEui[2];
          send_buf.u8ARG[7] = deviceEui[1];
          send_buf.u8ARG[8] = deviceEui[0];
          send_buf.u8Datalen = 9;      //ARG长度
          reply_control_field = Z_TO_H_NO_ACK;
          break;
       }

       case PRI_MSG_READ_KEY_TEST_OPCODE:
           send_buf.u8ARG[0] = 0x00;
           for (uint8_t i = 1; i < 7; i++) {
               send_buf.u8ARG[i] = 1;
           }
           send_buf.u8Datalen = 7;
           break;

          case(UART_MSG_EXIT_FACTORY_TEST_OPCODE):{  //ED04
                uint8_t token_data = 0;
                DataField_st data;

                switch(pMsgNode->buffer[11]){
                  case 0x01:
                    token_data = SINGLE_BOARD_TEST_DONE_VALUE;
                    halCommonSetToken(TOKEN_SINGLE_BOARD_TEST_FLG,&token_data);
                    halCommonGetToken(&token_data,TOKEN_SINGLE_BOARD_TEST_FLG);
                    if (token_data == SINGLE_BOARD_TEST_DONE_VALUE){
                      send_buf.u8ARG[0] = ERR_OK;
                      g_eFactoryTestStatus = FTS_SINGLE_BOARD_TEST_DONE;
                      //单板结束，一直快闪
                      //kLedOptTrigger(g_u8FactoryTestLedId, LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_OFF_TIME_MS, BLINK_DEAD, LED_ON, LED_ON);
                      iKonkeAfSelfPrint("!!INTO SINGLE_BOARD_TEST_DONE_VALUE = (%d) 111 !!!\r\n",FTS_SINGLE_BOARD_TEST_DONE);
                    }else{
                      send_buf.u8ARG[0] = ERR_EXIST;
                    }
                    kkMfglibRunningStart(20);    //开MFG 20信道
                    break;
                  case 0x02:
                    token_data = FULL_DEVICE_TEST_DONE_VALUE;
                    halCommonSetToken(TOKEN_FULL_DEVICE_TEST_FLG,&token_data);
                    halCommonGetToken(&token_data,TOKEN_FULL_DEVICE_TEST_FLG);
                    if (token_data == FULL_DEVICE_TEST_DONE_VALUE){
                      send_buf.u8ARG[0] = ERR_OK;
                      g_eFactoryTestStatus = FTS_FULL_DEVICE_TEST_DONE;
                                  //整机结束，一直快闪
                      iKonkeAfSelfPrint("!!INTO FTS_FULL_DEVICE_TEST_DONE = (%d) 222 !!!\r\n",FTS_FULL_DEVICE_TEST_DONE);
                    }else{
                      send_buf.u8ARG[0] = ERR_EXIST;
                    }
                    break;
                  case 0x03:
                    token_data = AGINE_TEST_DONE_VALUE;
                    halCommonSetToken(TOKEN_AGING_TEST_FLG,&token_data);
                    halCommonGetToken(&token_data,TOKEN_AGING_TEST_FLG);
                    if (token_data == AGINE_TEST_DONE_VALUE){
                      send_buf.u8ARG[0] = ERR_OK;
                      //g_eFactoryTestStatus = FTS_AGING_DONE;
                    }else{
                      send_buf.u8ARG[0] = ERR_EXIST;
                    }
                    break;
                  default:
                    send_buf.u8ARG[0] = ERR_FORMAT;
                    break;
                }
                send_buf.u8ARG[1] = pMsgNode->buffer[11];
                send_buf.u8Datalen = 2;

      break;
    }

          case (UART_MSG_WRITE_INSTALL_CODE_OPCODE): // write install code
          {
            reply_control_field = Z_TO_H_NO_ACK;
             //kCmdGeneralMsgPaser(pMsgNode, ECOM_PORT_RTT, &send_buf);
            DataField_st data;
            memcpy(data.u8ARG,&pMsgNode->buffer[11],data.u8Datalen - 3);

            tokTypeMfgInstallationCode custom_install_code;
            uint16_t crc = 0xFFFF;
            reply_control_field = Z_TO_H_NO_ACK;
            halCommonGetToken(&custom_install_code, TOKEN_MFG_INSTALLATION_CODE);

            //check install code length 20200725
            uint8_t length = data.u8ARG[0];
            if (length != 6 && length != 8 && length != 12 && length != 16) {
              iKonkeAfSelfPrint("4444444444444444 flg = %2x\r\n", custom_install_code.flags);
              send_buf.u8ARG[0] = ERR_FORMAT;
              send_buf.u8ARG[1] = data.u8ARG[0];
              memcpy(&send_buf.u8ARG[2], &data.u8ARG[1], data.u8ARG[0]);
              //send_buf.u8Datalen = data.u8ARG[0] + 5;
              send_buf.u8Datalen = data.u8ARG[0] + 1 + 1;
              break;
            }

            custom_install_code.flags = 0;

            if (length == 6) {
              custom_install_code.flags = custom_install_code.flags;
            } else if (length == 8) {
              custom_install_code.flags = custom_install_code.flags | 0x02;
            } else if (length == 12) {
              custom_install_code.flags = custom_install_code.flags | 0x04;
            } else if (length == 16) {
              custom_install_code.flags = custom_install_code.flags | 0x06;
            }

            iKonkeAfSelfPrint("Custom mfg token flg(%2x)\r\n", custom_install_code.flags);

            memcpy(custom_install_code.value, &data.u8ARG[1], data.u8ARG[0]);
            //uint8_t dd[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
            //memcpy(custom_install_code.value, &dd[0], data.u8ARG[0]);

            for(uint8_t i = 0; i<8; i++){
                iKonkeAfSelfPrint("[ins[%d]= %x]",i,custom_install_code.value[i]);
            }

            for (uint8_t index = 0; index < data.u8ARG[0]; index++) {
              crc = halCommonCrc16(reverse(custom_install_code.value[index]), crc);
            }

            crc = ~HIGH_LOW_TO_INT(reverse(LOW_BYTE(crc)), reverse(HIGH_BYTE(crc)));
            custom_install_code.crc = crc;

            //halCommonSetToken(TOKEN_MFG_INSTALLATION_CODE,&custom_install_code);
            kSetMfgTokenUserData(4466, &custom_install_code, sizeof(tokTypeMfgInstallationCode));
            //write installationcode
            //kOptTunnelTrytoWriteInstallCode();

            send_buf.u8ARG[0] = ERR_OK;
            send_buf.u8ARG[1] = data.u8ARG[0];
            memcpy(&send_buf.u8ARG[2], &data.u8ARG[1], data.u8ARG[0]);
            send_buf.u8Datalen = data.u8ARG[0] + 1 + 1;
            //send_buf.u8Datalen = data.u8ARG[0] + 5;

            break;
          }
          case (UART_MSG_READ_INSTALL_CODE_OPCODE): //read install code
          {
            uint8_t install_code_len = 0;
            tokTypeMfgInstallationCode install_code;

            //halCommonGetToken(&install_code, TOKEN_MFG_INSTALLATION_CODE);
            kGetMfgTokenUserData(4466, &install_code, sizeof(tokTypeMfgInstallationCode));

            send_buf.u8ARG[0] = ERR_OK;
            if (install_code.flags & 0x0001) {
            iKonkeAfSelfPrint("read install code failed, not exist \r\n");
              install_code_len = 0;
              send_buf.u8ARG[0] = ERR_NOT_EXIST;
            } else {
              uint8_t data = (install_code.flags >> 1);
              switch (data) {
              case 0:
                install_code_len = 6;
                memcpy(&send_buf.u8ARG[2], install_code.value, install_code_len);
                break;
              case 1:
                install_code_len = 8;
                memcpy(&send_buf.u8ARG[2], install_code.value, install_code_len);
                break;
              case 2:
                install_code_len = 12;
                memcpy(&send_buf.u8ARG[2], install_code.value, install_code_len);
                break;
              case 3:
                install_code_len = 16;
                memcpy(&send_buf.u8ARG[2], install_code.value, install_code_len);
                break;
              default:
                break;
              }
            }
            iKonkeAfSelfPrint("flg = %2x code length = %d\r\n", install_code.flags, install_code_len);
            send_buf.u8ARG[1] = install_code_len;
            send_buf.u8ARG[install_code_len + 2] = (uint8_t) (install_code.crc >> 8);
            send_buf.u8ARG[install_code_len + 3] = (uint8_t) (install_code.crc);
            send_buf.u8Datalen = install_code_len + 2 + 2; //  err(1) + codelen(1) + crc(2)
            //send_buf.u8Datalen = install_code_len + 7;
            reply_control_field = Z_TO_H_NO_ACK;
            break;
          }
          case (UART_MSG_READ_MOUDLE_ID_OPCODE):
          {
            uint8_t length = 0;
            uint8_t tmp_buffer[33];

            reply_control_field = Z_TO_H_NO_ACK;
            memset(tmp_buffer, 0x00, 33);
            emberAfReadAttribute(1, ZCL_BASIC_CLUSTER_ID, ZCL_MODEL_IDENTIFIER_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                                tmp_buffer, sizeof(tmp_buffer), NULL); // data type
            length = tmp_buffer[0];
            iKonkeAfSelfPrintBuffer(tmp_buffer, 32, 0);
            if (length > 32) {
              send_buf.u8ARG[0] = ERR_NOT_EXPECTED;
              send_buf.u8ARG[1] = 0x00;
              send_buf.u8Datalen = 2;
              break;
            }

            send_buf.u8ARG[0] = ERR_OK;
            memcpy(&send_buf.u8ARG[1], tmp_buffer, length + 1);

            //send_buf.u8Datalen = length + 1 + 1+ 3;
            send_buf.u8Datalen = length + 1 + 1;
            break;
          }


       default:
           break;
       }


    kCmdMsgDataSentByPort(reply_control_field, send_buf, false, 1);
}





void kkMfglibSendBindMsg(uint16_t panid,EmberEUI64 deviceEui,uint16_t opcode,uint8_t *msg)
{
//  uint8_t tmpbuff[47] = { 0x2E, 0x01, 0xCC, 0xA2, 0xE8, 0xCA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0xCA, 0x23, 0x16,
//      0x34, 0xFE, 0xFF, 0x57, 0xB4, 0x14, 0x0B, 0x00, 0x0B, 0x01, 0x00, 0x00, 0x00, 0xAA, 0x55, 0x00, 0x08,
//      0x20, 0x00, 0x08, 0x04, 0x00, 0xED, 0xCC, 0x01, 0x6F, 0x6A, 0xFF, 0xFF};

  uint8_t buffer[80];
  static uint16_t sendSeq = 0;
  EmberEUI64 source = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
  bool Send_falg = false;
  char gInterpanMsgRssi;


  MEMMOVE(source, emberGetEui64(), EUI64_SIZE);


  sendSeq++;
  uint8_t length = 0;
  buffer[length++] = 0x00;
  buffer[length++] = 0x01;

  buffer[length++] = 0xCC;
  buffer[length++] = sendSeq;

  buffer[length++] = panid & 0xFF;
  buffer[length++] = panid >> 8;
  for (int index = 0; index < 8; index++) {
      buffer[length++] = deviceEui[7-index];
  }
  buffer[length++] = 0xFF;
  buffer[length++] = 0xFF;

  for (int index = 0; index < 8; index++) {
      buffer[length++] = source[index];
  }

  buffer[length++] = 0x0B;
  buffer[length++] = 0x00;

  buffer[length++] = 0x0B;

  buffer[length++] = 0x01;
  buffer[length++] = 0x00;
  buffer[length++] = 0x00;
  buffer[length++] = 0x00;

  buffer[length++] = 0xAA; //msg header
  buffer[length++] = 0x55; //msg header

  buffer[length++] = 0x00; //数据长度
  buffer[length++] = 0x00;  //数据长度
  buffer[length++] = 0x20;
  //  seq++;
  buffer[length++] = (uint8_t) (sendSeq >> 8);
  buffer[length++] = (uint8_t) sendSeq;


   if(opcode == 0xED00){
      uint8_t hardversion = 0;
      uint8_t softversion = 0;

      buffer[length++] = 5 +3;
      buffer[length++] = 0x00;    //channel
      uint8_t Opcode_H = opcode >> 8;
      uint8_t Opcode_L = opcode & 0xFF;
      buffer[length++] = Opcode_H;
      buffer[length++] = Opcode_L;

      buffer[length++] = ERR_OK;
      emberAfReadServerAttribute(1, ZCL_BASIC_CLUSTER_ID, ZCL_HW_VERSION_ATTRIBUTE_ID,
                                  (uint8_t *)&hardversion,sizeof(uint8_t));
      emberAfReadServerAttribute(1, ZCL_BASIC_CLUSTER_ID, ZCL_APPLICATION_VERSION_ATTRIBUTE_ID,
                                  (uint8_t *)&softversion,sizeof(uint8_t));
      buffer[length++] = hardversion;
      buffer[length++] = softversion;
      buffer[length++] = 0xff;
      buffer[length++] = 0xff;
      emberAfCorePrintln("!!! kkMfglibSendBindMsg  opcode 0x%02X 222",opcode);
  }

  else if(opcode == 0xED01){
      EmberEUI64 deviceEui;
      MEMMOVE(deviceEui, emberGetEui64(), EUI64_SIZE);

      buffer[length++] = 9 +3;
      buffer[length++] = 0x00;    //channel
      uint8_t Opcode_H = opcode >> 8;
      uint8_t Opcode_L = opcode & 0xFF;
      buffer[length++] = Opcode_H;
      buffer[length++] = Opcode_L;

      buffer[length++] = ERR_OK;

      buffer[length++] = deviceEui[7];
      buffer[length++] = deviceEui[6];
      buffer[length++] = deviceEui[5];
      buffer[length++] = deviceEui[4];
      buffer[length++] = deviceEui[3];
      buffer[length++] = deviceEui[2];
      buffer[length++] = deviceEui[1];
      buffer[length++] = deviceEui[0];
      emberAfCorePrintln("!!! kkMfglibSendBindMsg opcode 0x%02X 222",opcode);
  }

  else if(opcode == 0xED04){
      buffer[length++] = 2 +3;
      buffer[length++] = 0x00;    //channel
      uint8_t Opcode_H = opcode >> 8;
      uint8_t Opcode_L = opcode & 0xFF;
      buffer[length++] = Opcode_H;
      buffer[length++] = Opcode_L;

	  emberAfCorePrintln("!!! fat_test_flag = %d !!",fat_test_flag);

            switch(fat_test_flag){

                  case 0x02:
                    uint8_t token_data = 0;

                    token_data = FULL_DEVICE_TEST_DONE_VALUE;
                    halCommonSetToken(TOKEN_FULL_DEVICE_TEST_FLG,&token_data);
                    halCommonGetToken(&token_data,TOKEN_FULL_DEVICE_TEST_FLG);
                    if (token_data == FULL_DEVICE_TEST_DONE_VALUE){
                        buffer[length++] =  ERR_OK;
                      g_eFactoryTestStatus = FTS_FULL_DEVICE_TEST_DONE;
                                  //整机结束，一直快闪
                      //kLedOptTrigger(g_u8FactoryTestLedId, LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_OFF_TIME_MS, BLINK_DEAD, LED_ON, LED_ON);
                      iKonkeAfSelfPrint("!!INTO FTS_FULL_DEVICE_TEST_DONE = (%d) 222 !!!\r\n",FTS_FULL_DEVICE_TEST_DONE);
                    }else{
                        buffer[length++] =  ERR_EXIST;
                    }

                    emberEventControlSetActive(kkMfglibStopEventControl);
                    break;

				  case 0x04:

                    buffer[length++] =  ERR_OK;

                    break;

                  default:
                    buffer[length++] = ERR_EXIST;
                    break;
                }

      buffer[length++] = fat_test_flag;

      emberAfCorePrintln("!!! kkMfglibSendBindMsg opcode 0x%02X 222",opcode);
  }

  else if(opcode == 0x0002){
      uint8_t install_code_len = 0;
      tokTypeMfgInstallationCode install_code;
      uint8_t u8ARG[] = {0};

      buffer[length++] = 0x0f;
      buffer[length++] = 0x00;    //channel
      uint8_t Opcode_H = opcode >> 8;
      uint8_t Opcode_L = opcode & 0xFF;
      buffer[length++] = Opcode_H;
      buffer[length++] = Opcode_L;


      //halCommonGetToken(&install_code, TOKEN_MFG_INSTALLATION_CODE);
      kGetMfgTokenUserData(4466, &install_code, sizeof(tokTypeMfgInstallationCode));

      if (install_code.flags & 0x0001) {
      iKonkeAfSelfPrint("read install code failed, not exist \r\n");
        install_code_len = 0;
        buffer[length++] = ERR_NOT_EXIST;
      } else {
        uint8_t data = (install_code.flags >> 1);
		
        buffer[length++] = ERR_OK;
        switch (data) {
        case 0:
			install_code_len = 6;
			buffer[length++] = install_code_len;
			//memcpy(&u8ARG[2], install_code.value, install_code_len);
			buffer[length++] = install_code.value[0];
			buffer[length++] = install_code.value[1];
			buffer[length++] = install_code.value[2];
			buffer[length++] = install_code.value[3];
			buffer[length++] = install_code.value[4];
			buffer[length++] = install_code.value[5];


          break;
        case 1:
          install_code_len = 8;
		  buffer[length++] = install_code_len;
          //memcpy(&u8ARG[2], install_code.value, install_code_len);
		  buffer[length++] = install_code.value[0];
		  buffer[length++] = install_code.value[1];
		  buffer[length++] = install_code.value[2];
		  buffer[length++] = install_code.value[3];
		  buffer[length++] = install_code.value[4];
		  buffer[length++] = install_code.value[5];
		  buffer[length++] = install_code.value[6];
		  buffer[length++] = install_code.value[7];
          
          break;
        case 2:
			install_code_len = 12;
			buffer[length++] = install_code_len;
			//memcpy(&u8ARG[2], install_code.value, install_code_len);
			buffer[length++] = install_code.value[0];
			buffer[length++] = install_code.value[1];
			buffer[length++] = install_code.value[2];
			buffer[length++] = install_code.value[3];
			buffer[length++] = install_code.value[4];
			buffer[length++] = install_code.value[5];
			buffer[length++] = install_code.value[6];
			buffer[length++] = install_code.value[7];
			buffer[length++] = install_code.value[8];
			buffer[length++] = install_code.value[9];
			buffer[length++] = install_code.value[10];
			buffer[length++] = install_code.value[11];


          break;
        case 3:
			install_code_len = 16;
			buffer[length++] = install_code_len;
			//memcpy(&u8ARG[2], install_code.value, install_code_len);
			buffer[length++] = install_code.value[0];
			buffer[length++] = install_code.value[1];
			buffer[length++] = install_code.value[2];
			buffer[length++] = install_code.value[3];
			buffer[length++] = install_code.value[4];
			buffer[length++] = install_code.value[5];
			buffer[length++] = install_code.value[6];
			buffer[length++] = install_code.value[7];
			buffer[length++] = install_code.value[8];
			buffer[length++] = install_code.value[9];
			buffer[length++] = install_code.value[10];
			buffer[length++] = install_code.value[11];
			buffer[length++] = install_code.value[12];
			buffer[length++] = install_code.value[13];
			buffer[length++] = install_code.value[14];
			buffer[length++] = install_code.value[15];

          break;
        default:
          break;
        }
      }

      iKonkeAfSelfPrint("flg = %2x code length = %d\r\n", install_code.flags, install_code_len);

      buffer[length++] = (uint8_t) (install_code.crc >> 8);
      buffer[length++] = (uint8_t) (install_code.crc);

      emberAfCorePrintln("!!! kkMfglibSendBindMsg opcode 0x%02X 222",opcode);
  }

  else if(opcode == 0x0000){

      buffer[length++] = 1 +3;
      buffer[length++] = 0x00;    //channel
      uint8_t Opcode_H = opcode >> 8;
      uint8_t Opcode_L = opcode & 0xFF;
      buffer[length++] = Opcode_H;
      buffer[length++] = Opcode_L;

      buffer[length++] = ERR_OK;

      emberAfCorePrintln("!!! kkMfglibSendBindMsg opcode 0x%02X 222",opcode);
  }
  
  else if(opcode == 0x000b){

      buffer[length++] = 1 +3;
      buffer[length++] = 0x00;    //channel
      uint8_t Opcode_H = opcode >> 8;
      uint8_t Opcode_L = opcode & 0xFF;
      buffer[length++] = Opcode_H;
      buffer[length++] = Opcode_L;

      buffer[length++] = ERR_OK;
	  emberGetLastHopRssi(&gInterpanMsgRssi);
	  buffer[length++] = gInterpanMsgRssi;

      emberAfCorePrintln("!!! kkMfglibSendBindMsg opcode 0x%02X 222",opcode);
  }

  else if(opcode == 0x000a){
      uint8_t len = 0;
      uint8_t tmp_buffer[33];

      buffer[length++] = 21;
      buffer[length++] = 0x00;    //channel
      uint8_t Opcode_H = opcode >> 8;
      uint8_t Opcode_L = opcode & 0xFF;
      buffer[length++] = Opcode_H;
      buffer[length++] = Opcode_L;

      memset(tmp_buffer, 0x00, 33);
            emberAfReadAttribute(1, ZCL_BASIC_CLUSTER_ID, ZCL_MODEL_IDENTIFIER_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                                tmp_buffer, sizeof(tmp_buffer), NULL); // data type
      len = tmp_buffer[0];

      iKonkeAfSelfPrintBuffer(tmp_buffer, 32, 0);
      if (len > 32) {
          buffer[length++] = ERR_NOT_EXPECTED;
          buffer[length++] = 0x00;

      }else{
          buffer[length++] = ERR_OK;
		  buffer[length++] = tmp_buffer[0];
		  buffer[length++] = tmp_buffer[1];
		  buffer[length++] = tmp_buffer[2];
		  buffer[length++] = tmp_buffer[3];
		  buffer[length++] = tmp_buffer[4];
		  buffer[length++] = tmp_buffer[5];
		  buffer[length++] = tmp_buffer[6];
		  buffer[length++] = tmp_buffer[7];
		  buffer[length++] = tmp_buffer[8];
		  buffer[length++] = tmp_buffer[9];
		  buffer[length++] = tmp_buffer[10];
		  buffer[length++] = tmp_buffer[11];
		  buffer[length++] = tmp_buffer[12];
		  buffer[length++] = tmp_buffer[13];
		  buffer[length++] = tmp_buffer[14];
		  buffer[length++] = tmp_buffer[15];
		  buffer[length++] = tmp_buffer[16];

          //memcpy(&buffer[length++], tmp_buffer, len + 1);

      emberAfCorePrintln("!!! kkMfglibSendBindMsg opcode 0x%02X 222",opcode);
      }
  }


  buffer[33] = (uint8_t) ((length - 35) >> 8);
  buffer[34] = (uint8_t) (length - 35);
  uint16_t crc = kCmdGetMsgCrc16Value(&buffer[35], length - 35);

  buffer[length++] = (uint8_t) (crc >> 8);
  buffer[length++] = (uint8_t) (crc);
  buffer[length++] = 0xFF;
  buffer[0] = length;

  EmberStatus status = mfglibSendPacket(buffer,2);
  emberAfCorePrintln("!!! kkMfglibSendBindMsg <mfglib> send packet length(%d), status 0x%X",buffer[0], status);
  emberAfPrint(0xFFFF, "!!! kkMfglibSendBindMsg <mfglib> send packet length(%d), opcode(0x%02X) status 0x%X\r\n",buffer[0], opcode, status);
  emberAfCorePrintln("!!! kkMfglibSendBindMsg <mfglib> send packet des mac=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", \
                          deviceEui[0],deviceEui[1],deviceEui[2],deviceEui[3],deviceEui[4],deviceEui[5],deviceEui[6],deviceEui[7]);
}

static void kkMfg_Send(uint16_t opcode){
  emberAfPrint(0xFFFF, "!!!! INTO kkDelaySendMfgMsgEventHandler 8888\r\n");
  uint8_t msg[12] = {0};
  EmberEUI64  deviceEui;
  MEMMOVE(deviceEui, emberGetEui64(), EUI64_SIZE);
  msg[0] = deviceEui[7];
  msg[1] = deviceEui[6];
  msg[2] = deviceEui[5];
  msg[3] = deviceEui[4];
  msg[4] = deviceEui[3];
  msg[5] = deviceEui[2];
  msg[6] = deviceEui[1];
  msg[7] = deviceEui[0];
  msg[8] = (uint8_t)(((magicId >> 8) >> 8) >> 8);
  msg[9] = (uint8_t)((magicId >> 8) >> 8) & 0xFF;
  msg[10] = (uint8_t)(magicId >> 8) & 0xFF;
  msg[11] = (uint8_t)magicId & 0xFF;
  msg[12] = 0;
  msg[13] = 0;

  kkMfglibSendBindMsg(lastPanId,deviceEui,opcode,msg);
}


static void kkMfglibRxHandler(uint8_t *packet, uint8_t linkQuality, int8_t rssi )
{
  // This increments the total packets for the whole mfglib session
  // this starts when mfglibStart is called and stops when mfglibEnd
  // is called.
  uint16_t MacHeader_FrameControl = ((uint16_t)packet[2] << 8) | packet[1];
  uint8_t NwkHeader_FrameType = packet[24] & 0x03;
  uint8_t ApsHeader_FrameType = packet[26] & 0x03;
  uint8_t ApsHeader_SecurityEnabled = packet[26] & 0x20;
  uint8_t ApsHeader_ApsExtendedHeaderPresent = packet[26] & 0x80;
  uint16_t MsgHeader = ((uint16_t)packet[31] << 8) | packet[32];
  uint16_t Opcode = ((uint16_t)packet[40] << 8) | packet[41];
  EmberEUI64  deviceEui = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  EmberEUI64  my_deviceEui = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  uint8_t checkEui[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  uint8_t  mac_src[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
  bool Send_falg = false;


  MEMMOVE(my_deviceEui, emberGetEui64(), EUI64_SIZE);
  checkEui[0] = my_deviceEui[7];
  checkEui[1] = my_deviceEui[6];
  checkEui[2] = my_deviceEui[5];
  checkEui[3] = my_deviceEui[4];
  checkEui[4] = my_deviceEui[3];
  checkEui[5] = my_deviceEui[2];
  checkEui[6] = my_deviceEui[1];
  checkEui[7] = my_deviceEui[0];


  for(uint8_t k =0; k<8; k++){
      mac_src[k] = packet[13-k];
  }

  //strcmp(mac_src,checkEui) == 0
  
  if((checkEui[0] ==  mac_src[0])&&(checkEui[1] ==  mac_src[1])&&(checkEui[2] ==  mac_src[2])&&(checkEui[3] ==  mac_src[3])&&\
  	(checkEui[4] ==  mac_src[4])&&(checkEui[5] ==  mac_src[5])&&(checkEui[6] ==  mac_src[6])&&(checkEui[7] ==  mac_src[7])){
  	Send_falg = true;
	emberAfCorePrintln("\r\n----Send_falg TRUE------\r\n");
  }
  
 // emberAfCorePrintln("\r\n-------Mfg-send_mac = %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x-----\r\n", \
 // 	                      mac_src[0],mac_src[1],mac_src[2],mac_src[3],mac_src[4],mac_src[5],mac_src[6],mac_src[7]);

  static bool firstReceive = false;
  static uint16_t panId = 0;

  uint16_t crc = kCmdGetMsgCrc16Value(&packet[35], packet[34]);
  uint16_t EDCE_CRC = ((uint16_t)packet[52] << 8) | packet[53];
  uint16_t EDCF_CRC = ((uint16_t)packet[43] << 8) | packet[44];

  uint8_t chanel = 0;
  static uint8_t lastChanel = 0;

  for(uint8_t i = 0; i<8; i++){
    deviceEui[i] = packet[23 - i];
  }

   panId = ((uint16_t)packet[15] << 8) | packet[14];
   lastPanId = panId;


if(Send_falg){

//  emberAfCorePrintln("!!! kkMfglibRxHandler opcode 0x%02X 000",Opcode);
  if(MacHeader_FrameControl == 0xCC01 || MacHeader_FrameControl == 0xCC21) {
      if((NwkHeader_FrameType == 0x03) && (ApsHeader_FrameType == 0x03) && (ApsHeader_SecurityEnabled == 0x00) && (ApsHeader_ApsExtendedHeaderPresent == 0x00)){
          if((MsgHeader == 0xAA55)){


              if((Opcode == 0xED00) ){
                 chanel = mfglibGetChannel();
                 fac_ed00 = true;

				  for(uint8_t i = 0; i<8 ;i++){
				  	lastEui64_ed00[i] = deviceEui[i];
				  }

				 
                // emberAfCorePrintln("!!!! kkMfglibRxHandler MAC:0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X",deviceEui[0],deviceEui[1],deviceEui[2],
                //                    deviceEui[3],deviceEui[4],deviceEui[5],deviceEui[6],deviceEui[7]);
                 if(chanel == 20){
                     emberEventControlSetDelayMS(kkDelaySendMfgMsgEventControl,10);
                  }
                 }


              else if((Opcode == 0xED04)){
                 chanel = mfglibGetChannel();
                 fac_ed04 = true;

				  for(uint8_t i = 0; i<8 ;i++){
				  	lastEui64_ed04[i] = deviceEui[i];
				  }

				  fat_test_flag = packet[42];
			  
			  if(chanel == 20){
                     emberEventControlSetDelayMS(kkDelaySendMfgMsgEventControl,10);
                  }
                 }


              else if((Opcode == 0x000a)){
                 chanel = mfglibGetChannel();
                 fac_000a = true;	  
			  	 for(uint8_t i = 0; i<8 ;i++){
				  	lastEui64_000a[i] = deviceEui[i];
				  }


                 if(chanel == 20){
                     emberEventControlSetDelayMS(kkDelaySendMfgMsgEventControl,10);
                  }
                 }

              else if((Opcode == 0x0000) ){
                 chanel = mfglibGetChannel();
                 fac_0000 = true;
			  	 for(uint8_t i = 0; i<8 ;i++){
				  	lastEui64_0000[i] = deviceEui[i];
				  }
	  

                 if(chanel == 20){
                     emberEventControlSetDelayMS(kkDelaySendMfgMsgEventControl,10);

                  }
                 }

              else if((Opcode == 0x0002) ){
                 chanel = mfglibGetChannel();
                 fac_0002 = true;
			  	  for(uint8_t i = 0; i<8 ;i++){
				  	lastEui64_0002[i] = deviceEui[i];
				  }

                 if(chanel == 20){
                     emberEventControlSetDelayMS(kkDelaySendMfgMsgEventControl,10);
                  }
                 }
			  
			  else if((Opcode == 0x000b) ){
                 chanel = mfglibGetChannel();
                 fac_000b = true;
			  	  for(uint8_t i = 0; i<8 ;i++){
				  	lastEui64_000b[i] = deviceEui[i];
				  }

                 if(chanel == 20){
                     emberEventControlSetDelayMS(kkDelaySendMfgMsgEventControl,10);
                  }
                 }

          }
      }
  }
}
}



void kkMfglibStartEventHandler()
{
  emberEventControlSetInactive(kkMfglibStartEventControl);

  uint8_t msg[8] = {0};
  if(bindOkFlg == false){
      EmberStatus status = mfglibStart(kkMfglibRxHandler);

      emberAfCorePrintln("<mfglib> enable status %d", status);

      status = mfglibSetChannel(20);
      emberAfCorePrintln("<mfglib> set channel,channel(%d) status 0x%X 111",g_u8MfgChannel,status);

//      g_u8MfgChannel++;
//      emberAfCorePrintln("<mfglib> set channel,channel(%d) status 0x%X 222",g_u8MfgChannel,status);
      status = mfglibSetPower(0,0);
      emberAfCorePrintln("<mfglib> set power and mode, status 0x%X", status);


      emberEventControlSetDelayMS(kkMfglibWaitReceiveEventControl,300);
  }

}



void kkMfglibWaitReceiveEventHandler()
{
  emberEventControlSetInactive(kkMfglibWaitReceiveEventControl);
  static uint32_t lastTimeMS = 0;
  static bool firstScanFlg = false;
  uint32_t currentTimeMS = halCommonGetInt64uMillisecondTick();

  if(g_u8MfgChannel == 15){
    if(firstScanFlg == false){
       emberEventControlSetActive(kkMfglibStartEventControl);
       firstScanFlg = true;
       lastTimeMS = currentTimeMS;
       emberAfCorePrintln("!!! INTO kkMfglibWaitReceiveEventHandler lastTimeMS = (%d)",lastTimeMS);
    }else{
       g_u8MfgChannel = 20;
       emberEventControlSetActive(kkMfglibStartEventControl);
    }
  }else if(g_u8MfgChannel == 20){
    g_u8MfgChannel = 20;
    emberEventControlSetActive(kkMfglibStartEventControl);
  }else{
      if((bindOkFlg == false) && (currentTimeMS - lastTimeMS < 10000)){
        g_u8MfgChannel = 20;
        emberEventControlSetActive(kkMfglibStartEventControl);
        emberAfCorePrintln("!!! INTO kkMfglibWaitReceiveEventHandler 111");
      }else{
        firstScanFlg = false;
        emberAfCorePrintln("!!! INTO kkMfglibWaitReceiveEventHandler 222");
        EmberStatus status = mfglibSetChannel(0x14);
//        status = mfglibSetPower(0,20);
        emberAfCorePrintln("<mfglib> set power and mode status %d", status);
        status = mfglibEnd();
        emberAfCorePrintln("<mfglib> stop status %d", status);
//        emberEventControlSetDelayMS(kkDelayRebootEventControl,500);
      }
  }
}


void kkMfglibStopEventHandler()
{
  emberEventControlSetInactive(kkMfglibStopEventControl);

  //halCommonSetToken(TOKEN_BIND_CHANNEL_FLG, bind_channel_flag);
  EmberStatus status = mfglibSetChannel(0x0B);
//  status = mfglibSetPower(0,20);
  emberAfCorePrintln("<mfglib> set power and mode status %d", status);
  status = mfglibEnd();
  emberAfCorePrintln("<mfglib> stop status %d", status);
  emberAfPrint(0xFFFF, "<mfglib> stop status %d\r\n", status);
//  emberEventControlSetDelayMS(kkDelayRebootEventControl,500);

}



void kkDelaySendMfgMsgEventHandler()
{
  emberEventControlSetInactive(kkDelaySendMfgMsgEventControl);
  emberAfPrint(0xFFFF, "!!!! INTO kkDelaySendMfgMsgEventHandler 9999\r\n");
  uint8_t msg[12] = {0};
  EmberEUI64  deviceEui;
  MEMMOVE(deviceEui, emberGetEui64(), EUI64_SIZE);
  

  msg[0] = deviceEui[7];
  msg[1] = deviceEui[6];
  msg[2] = deviceEui[5];
  msg[3] = deviceEui[4];
  msg[4] = deviceEui[3];
  msg[5] = deviceEui[2];
  msg[6] = deviceEui[1];
  msg[7] = deviceEui[0];
  msg[8] = (uint8_t)(((magicId >> 8) >> 8) >> 8);
  msg[9] = (uint8_t)((magicId >> 8) >> 8) & 0xFF;
  msg[10] = (uint8_t)(magicId >> 8) & 0xFF;
  msg[11] = (uint8_t)magicId & 0xFF;
  msg[12] = 0;
  msg[13] = 0;


  //emberAfCorePrintln("\r\n------Mfg-mac = %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x ----\r\n", msg[0],msg[1],msg[2],msg[3],msg[4],msg[5],msg[6],msg[7]);

  if(fac_ed00){
  	 emberAfCorePrintln("\r\n-----Mfg-send_opcode = ED00------\r\n");
     kkMfglibSendBindMsg(lastPanId,lastEui64_ed00,0xed00,msg);
     fac_ed00 = false;
  }
  if(fac_ed04){
  	 emberAfCorePrintln("\r\n-----Mfg-send_opcode = ED04------\r\n");
     kkMfglibSendBindMsg(lastPanId,lastEui64_ed04,0xed04,msg);
     fac_ed04 = false;
  }
  if(fac_0000){
  	 emberAfCorePrintln("\r\n-----Mfg-send_opcode = 0000------\r\n");
     kkMfglibSendBindMsg(lastPanId,lastEui64_0000,0x0000,msg);
     fac_0000 = false;
  }
  if(fac_0002){
  	 emberAfCorePrintln("\r\n-----Mfg-send_opcode = 0002------\r\n");
     kkMfglibSendBindMsg(lastPanId,lastEui64_0002,0x0002,msg);
     fac_0002 = false;
  }
  if(fac_000a){
  	 emberAfCorePrintln("\r\n-----Mfg-send_opcode = 000a------\r\n");
     kkMfglibSendBindMsg(lastPanId,lastEui64_000a,0x000a,msg);
     fac_000a = false;
  }
    if(fac_000b){
  	 emberAfCorePrintln("\r\n-----Mfg-send_opcode = 000a------\r\n");
     kkMfglibSendBindMsg(lastPanId,lastEui64_000b,0x000b,msg);
     fac_000b = false;
  }


}




/*	DESP: uart command message incoming and process interface.
 * 	Auth: maozj.20200725.
 * */
void kUserUartMsgInComingPaser(UMsgNodeSt *pMsgNode )
{
	if( NULL == pMsgNode ) {
		return ;
	}

#if 1 // Just for debug
	iKonkeAfSelfPrint("\r\n-- MSG INCOMING[%d]:\r\n", pMsgNode->length);
	iKonkeAfSelfPrintBuffer(pMsgNode->buffer, pMsgNode->length, true);
	iKonkeAfSelfPrint("\r\n--------------------\r\n");
#endif

	// check for parsable packets
	if( pMsgNode->length < 10 /* Minimum Packet Length */ ) {
		iKonkeAfSelfPrint("Err: Unparsable Packets!!\r\n");
		return ;
	}
#if 0
	MsgFrameworkFormat_st *pfmt = (MsgFrameworkFormat_st *)pMsgNode->buffer;

	// retransmit message filtering


	iKonkeAfSelfPrint("SNAP: %X:%X:%X:%4X:%X:%X:%X\r\n", pfmt->preamble, pfmt->length
		, pfmt->opcode, NT32(pfmt->seq_devid), pfmt->vendor_id, pfmt->dev_type, pfmt->data1);
#endif
	uint16_t length;
		uint8_t control_field;
	//	uint16_t seq;
		DataField_st data;
		DataField_st send_buf;

	length = ((uint16_t)pMsgNode->buffer[2] << 8) |  (uint16_t)(pMsgNode->buffer[3]);
	control_field = pMsgNode->buffer[4];

	data.seq = ((uint16_t)pMsgNode->buffer[5] << 8) |  (uint16_t)(pMsgNode->buffer[6]);

	data.u8Datalen = pMsgNode->buffer[7];
	data.u8ChannelID = pMsgNode->buffer[8];
	data.u16Opcode = ((uint16_t)pMsgNode->buffer[9] << 8) |  (uint16_t)(pMsgNode->buffer[10]);
	memcpy(data.u8ARG, &pMsgNode->buffer[11], data.u8Datalen - 3);

	send_buf.seq = data.seq;
	send_buf.u8ChannelID = data.u8ChannelID;
	send_buf.u16Opcode = data.u16Opcode;
	switch(data.u16Opcode){
		case UART_MSG_READ_DEVICE_SNAP_OPCODE:
		case UART_MSG_JOIN_NWK_REQUEST_OPCODE:
		case UART_MSG_LEAVE_NWK_REQUEST_OPCODE:
		case UART_MSG_QUERY_NWK_STATUS_REQUEST_OPCODE:
		case UART_MSG_NWK_STATUS_NOTIFY_OPCODE:
		case UART_MSG_READ_ATTRIBUTE_REQUEST_OPCODE:
		case UART_MSG_WRITE_ATTRIBUTE_REQUEST_OPCODE:
		case UART_MSG_ZCL_COMMAND_REQUEST_OPCODE:
		case UART_MSG_BIND_REQUEST_OPCODE:
//			if (data.u16Opcode == UART_MSG_JOIN_NWK_REQUEST_OPCODE){
//				extern int8_t g_i8GatewayRssiValue;
//				g_i8GatewayRssiValue = -127;
//			}
			//kCmdGeneralMsgHanderCallback(pMsgNode);
			break;
		case UART_MSG_WRITE_INSTALL_CODE_OPCODE:
		case UART_MSG_READ_INSTALL_CODE_OPCODE:
		case UART_MSG_QUERY_INFO_OPCODE:
		case UART_MSG_QUERY_DEVICE_INFO_OPCODE:
		case UART_MSG_CONFIG_DEVICE_SLEEP_OPCODE:
		case UART_MSG_EXIT_FACTORY_TEST_OPCODE:
		case UART_MSG_QUERY_FACTORY_INFO_OPCODE:
			kkFactoryMsgInComingCallBack(pMsgNode);
			break;
		case UART_MSG_BODY_TRIGGER_START_REPORT_OPCODE:
			//��ʼ�����Ӧ����
			//kP824mReset();
			if(kGetFactoryTestStatus() != FTS_SINGLE_BOARD_TEST){
				g_bBodyLensTestFlg = true;
				g_bHumanBodyStopReportFlg = false;
				iKonkeAfSelfPrint("!!!!INTO UART_MSG_BODY_TRIGGER_START_REPORT_OPCODE 111111!!!!!\r\n");
			}
			g_bStartBodySensorSingleBoardTestFlg = true;
			g_u16BodySensorSequenceRecord = data.seq;
			g_eSensorCheckStatus = SCS_EXIT;
			emberEventControlSetDelayMS(kDelaySetLedStatusEventControl, 1 * 1000);
			break;
		case UART_MSG_BODY_TRIGGER_STOP_REPORT_OPCODE:
			g_bHumanBodyStopReportFlg = true;
			g_bBodyLensTestFlg = false;
			g_u16BodySensorStopReportSequenceRecord = data.seq;
			iKonkeAfSelfPrint("!!!!INTO UART_MSG_BODY_TRIGGER_STOP_REPORT_OPCODE  g_bHumanBodyStopReportFlg = (%d) 222222!!!!!\r\n", g_bHumanBodyStopReportFlg);\
			kBodySensorStopReportSent();
			break;
		default:
			break;
	}
}
/*	DESP: OptData message incoming process callback..
 * 	PARAM[pframe][in/out] input command frame content, output command process result ack content.
 * 	Auth: dingmz_frc.20190701.
 * */
kk_err_t kUserOODMessageIncoming(uint8_t channel, uint8_t opcode, uint8_t args_in_out[], uint8_t *length_in_out )
{
	if( NULL == args_in_out || NULL == length_in_out ) {
		return KET_ERR_INVALID_POINTER;
	}

	iKonkeAfSelfPrint("OOD-INCOMING: channel(%d), opcode(%X)--\r\n", channel, opcode);
	for(int index = 0; index < *length_in_out; ++index ) {
		iKonkeAfSelfPrint("%X ", args_in_out[index]);
	}
	iKonkeAfSelfPrint("--------------------------------------\r\n");

	switch(opcode) {
		case (0x00): // get the device snap.  FCC0快照
		{

		uint8_t trigger,led,dect,fina,exist,endpoint = 1;
		uint16_t light,hold;

        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_IndicateMode_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&led, sizeof(led), NULL); // data type

        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_Sensitivity_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&dect, sizeof(dect), NULL); // data type

        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_DelayConfiguration_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&hold, sizeof(hold), NULL); // data type

        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_SlightMoveSwitch_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&fina, sizeof(fina), NULL); // data type

        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_ExistenceSwitch_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&exist, sizeof(exist), NULL); // data type

			args_in_out[0] = 0x00;
			args_in_out[1] = humanbody.trigger_state - 1;
			args_in_out[2] = (uint8_t)(humanbody.light >> 8);
			args_in_out[3] = (uint8_t)(humanbody.light);
			args_in_out[4] = led;
			args_in_out[5] = dect;
			args_in_out[6] = (uint8_t)(hold >> 8);
			args_in_out[7] = (uint8_t)hold;
			args_in_out[8] = fina;
			args_in_out[9] = exist;
			args_in_out[10] = 0xff;
			args_in_out[11] = 0xff;
			args_in_out[12] = 0xff;
			args_in_out[13] = 0xff;
			args_in_out[14] = 0xff;
			args_in_out[15] = 0xff;

			iKonkeAfSelfPrint("OOOOOOPCODE: 1(%d), 2(%d), 3(%d), 4(%d), 5(%d), 6(%d), 7(%d), 8(%d), 9(%d)\r\n", \
			                  args_in_out[1], args_in_out[2], args_in_out[3], args_in_out[4], args_in_out[5], args_in_out[6], args_in_out[7], args_in_out[8], args_in_out[9]);
			*length_in_out = 16;

			break;
		}
#if 0
		case (0xFE):
		{
			//set disturb mode
			uint8_t mode = args_in_out[0];
			if (mode != 0x00 && mode != 0x01){
				args_in_out[0] = ERR_NO_COMMAND_FORMAT;
			}else {
				kSetIndicatorNotDisturbModeFlg(mode);
				args_in_out[0] = ERR_NO_NONE;
			}
			iKonkeAfSelfPrint("Disturb Mode(%d)\r\n", mode);
			args_in_out[1] = mode;
			*length_in_out = 2;
			break;
		}
#endif
#if 0
		case (0xFF):
		{
			int8_t permitJoinTimeS = args_in_out[0]; //unit:s
			iKonkeAfSelfPrint("###Opcode is channel(%d) JoinPermitTime(0x%X)!!\r\n",channel,  permitJoinTimeS);
			//·��Эͬ����,�Ƶ���˸(OTA����)ʱ��Эͬ��������
			if (kLedIsBlinking(LED123_ID) != true
					/*&& kGetOTAStatus() == OTA_NORMAL*/
					&& permitJoinTimeS != 0
					&& kNwkGetCurrentStatus() != ENS_JOINING){
				iKonkeAfSelfPrint("####LED Blink\r\n");
				kLedOptTrigger(LED_YELLOW_ID,  LED_SLOW_BLINK_ON_TIME_MS, LED_SLOW_BLINK_OFF_TIME_MS,
						permitJoinTimeS * 1000/(LED_SLOW_BLINK_ON_TIME_MS+LED_SLOW_BLINK_OFF_TIME_MS), LED_ON, LED_OFF);
			}else if (kLedIsBlinking(LED_YELLOW_ID) == true
					/*&& kGetOTAStatus() == OTA_NORMAL*/
					&& permitJoinTimeS == 0
					&& kNwkGetCurrentStatus() != ENS_JOINING){
				iKonkeAfSelfPrint("####LED Blink over\r\n");
				LED_OPT_OFF(LED_YELLOW_ID);
			}
			return KET_NO_RESPONSE;
		}
#endif
		default: {
			iKonkeAfSelfPrint("Err: opcode is not exist(%d:%X)!!\r\n", channel, opcode);
			args_in_out[0] = ERR_NO_OPCODE;
			*length_in_out = 1;
		}
	}

	return KET_OK;
}

/*	DESP: system setup delay callback handler, called once.
 * 	Auth: dingmz_frc.20190628.
 * */
void kkSystemSetUpEventHandler(void )  //上电
{
	emberEventControlSetInactive(kkSystemSetUpEventControl);
	iKonkeAfSelfPrint("\r\n--------!!!!!! kkSystemSetUpEventHandler !!!!!-------\r\n");
	setup_send_msg = true;


	switch (g_u8SystemSetupStatus)
	{
		case SETUP_STATUS_INIT:


      //kRelayChannelOpt(RELAY_CHANNEL1_ID, EOOS_ON);

			iKonkeAfSelfPrint("\r\n---------INTO System setup event. 111------- \r\n");
			//emberAfClearResponseData();		//??

			if(kGetFactoryTestStatus() == FTS_FULL_DEVICE_TEST){

				//kkEnterSleep(false, false);

				extern EmberEventControl ikkUartRecvLoopCheckEventControl;
				emberEventControlSetActive(ikkUartRecvLoopCheckEventControl);
				iKonkeAfSelfPrint("!!!!!!INTO kkSystemSetUpEventHandler FTS_SINGLE_BOARD_TEST ikkUartRecvLoopCheckEventControl 111111!!!!!!!\r\n");
			}

			//kOptTunnelTrytoWriteModelIdenfierMfgToken();

            if(humanbody.led_switch != humanbody_tocken.led_switch){
				syn_led = true;
            }

			if(humanbody.detection_area != humanbody_tocken.detection_area){
				syn_area = true;
			}

            if(humanbody.hold_time != humanbody_tocken.hold_time){
				syn_hold = true;
            }

            if(humanbody.exist_switch != humanbody_tocken.exist_switch){
				syn_ex = true;
            }

			if(humanbody.final_switch != humanbody_tocken.final_switch){
				syn_fin = true;
			}

			kSystemSetupStatusSet(SETUP_SYN1, 500);
			break;
			
		case SETUP_SYN1:
            if(syn_led){
				iKonkeAfSelfPrint("\r\n---- SYN LED_SWITCH (%d) -> (%d) ----\r\n",humanbody.led_switch,humanbody_tocken.led_switch);
				//grep_6++;
				//UartSendHandle(OPCODE_LED_SWITCH,humanbody_tocken.led_switch);
				humanbody.led_switch = humanbody_tocken.led_switch;
            }else{
				iKonkeAfSelfPrint("\r\n---- SYN LED_SWITCH OK  -----\r\n");
            }

		      kZclClusterSetPermitReportInfo(1, ZCL_BASIC_CLUSTER_ID, true, false, true, false); //ledswitch上报
		      emberAfWriteServerAttribute(1, ZCL_BASIC_CLUSTER_ID,
		          ZCL_IndicateMode_ATTRIBUTE_ID, (uint8_t*)&humanbody.led_switch,
		          ZCL_INT8U_ATTRIBUTE_TYPE);			
		
			kSystemSetupStatusSet(SETUP_SYN2, 500);
			break;
		
		case SETUP_SYN2:
			if(syn_area == true){
				iKonkeAfSelfPrint("\r\n---- SYN DETECTION_AREA (%d) -> (%d) ----\r\n",humanbody.detection_area,humanbody_tocken.detection_area);
				//grep_2++;
				//UartSendHandle(OPCODE_DETECTION_AREA,humanbody_tocken.detection_area);
				humanbody.detection_area = humanbody_tocken.detection_area;
            }else{
				iKonkeAfSelfPrint("\r\n---- SYN DETECTION_AREA OK -----\r\n");
            }

			
		      kZclClusterSetPermitReportInfo(1, ZCL_BASIC_CLUSTER_ID, true, false, true, false); //灵敏度上报
		      emberAfWriteServerAttribute(1, ZCL_BASIC_CLUSTER_ID,
		          ZCL_Sensitivity_ATTRIBUTE_ID, (uint8_t*)&humanbody.detection_area,
		          ZCL_INT8U_ATTRIBUTE_TYPE);

		    kSystemSetupStatusSet(SETUP_SYN3, 500);
		    break;
		
		case SETUP_SYN3:
			if(syn_hold){
				iKonkeAfSelfPrint("\r\n---- SYN HOLD_TIME (%d) -> (%d) -----\r\n",humanbody.hold_time,humanbody_tocken.hold_time);
				//grep_5++;
				//UartSendHandle(OPCODE_HOLD_TIME,humanbody_tocken.hold_time);
				humanbody.hold_time = humanbody_tocken.hold_time;
            }else{
				iKonkeAfSelfPrint("\r\n---- SYN HOLD_TIME OK  -----\r\n");
            }

	        kZclClusterSetPermitReportInfo(1, ZCL_BASIC_CLUSTER_ID, true, false, true, false); //延时上报
		      emberAfWriteServerAttribute(1, ZCL_BASIC_CLUSTER_ID,
		          ZCL_DelayConfiguration_ATTRIBUTE_ID, (uint8_t*)&humanbody.hold_time,
		          ZCL_INT16U_ATTRIBUTE_TYPE);

			kSystemSetupStatusSet(SETUP_SYN4, 500);
			break;

		case SETUP_SYN4:
			if(syn_ex){
				iKonkeAfSelfPrint("\r\n---- SYN EXIST_SWITCH (%d) -> (%d) ----\r\n",humanbody.exist_switch,humanbody_tocken.exist_switch);
				//grep_7++;
				//UartSendHandle(OPCODE_EXIST_SWITCH,humanbody_tocken.exist_switch);
				humanbody.exist_switch = humanbody_tocken.exist_switch;
            }else{
				iKonkeAfSelfPrint("\r\n---- SYN EXIST_SWITCH OK  -----\r\n");
            }	

			 kZclClusterSetPermitReportInfo(1, ZCL_BASIC_CLUSTER_ID, true, false, true, false); //存在使能上报
		      emberAfWriteServerAttribute(1, ZCL_BASIC_CLUSTER_ID,
		          ZCL_ExistenceSwitch_ATTRIBUTE_ID, (uint8_t*)&humanbody.exist_switch,
		          ZCL_INT8U_ATTRIBUTE_TYPE);

			kSystemSetupStatusSet(SETUP_SYN5, 500);
			break;

		case SETUP_SYN5:
			if(syn_fin){
				iKonkeAfSelfPrint("\r\n---- SYN FINAL_SWITCH (%d) -> (%d) ----\r\n",humanbody.final_switch,humanbody_tocken.final_switch);
				//grep_9++;
				//UartSendHandle(OPCODE_FINAL_SWITCH,humanbody_tocken.final_switch);
				humanbody.final_switch = humanbody_tocken.final_switch;
            }else{
				iKonkeAfSelfPrint("\r\n---- SYN FINAL_SWITCH OK  -----\r\n");
            }	

		    kZclClusterSetPermitReportInfo(1, ZCL_BASIC_CLUSTER_ID, true, false, true, false); //微动使能上报
		      emberAfWriteServerAttribute(1, ZCL_BASIC_CLUSTER_ID,
		          ZCL_SlightMoveSwitch_ATTRIBUTE_ID, (uint8_t*)&humanbody.final_switch,
		          ZCL_INT8U_ATTRIBUTE_TYPE);

			kSystemSetupStatusSet(SETUP_STATUS_EXIT, 500);
			break;

			
		case SETUP_STATUS_EXIT:
			g_u8SystemSetupStatus = SETUP_STATUS_INIT;


			  kZclClusterSetPermitReportInfo(1, ZCL_IAS_ZONE_CLUSTER_ID,  true, false, true, false);
			  emberAfWriteServerAttribute(1, ZCL_IAS_ZONE_CLUSTER_ID,
					                            ZCL_ZONE_STATUS_ATTRIBUTE_ID, (uint8_t*)humanbody.trigger_state,
					                            ZCL_ENUM16_ATTRIBUTE_TYPE);                      //环境状态上报

		      kZclClusterSetPermitReportInfo(1, ZCL_ILLUM_MEASUREMENT_CLUSTER_ID, true, false, true, false);
		      emberAfWriteServerAttribute(1, ZCL_ILLUM_MEASUREMENT_CLUSTER_ID,
		          ZCL_ILLUM_MEASURED_VALUE_ATTRIBUTE_ID, (uint8_t*)&humanbody.light,
		          ZCL_INT16U_ATTRIBUTE_TYPE);                                             //光照上报

 

      setup_report_ias = 0;
			break;

			
		default:
			break;
	}
}


void kDelaySetLedStatusEventHandler(void)
{
	emberEventControlSetInactive(kDelaySetLedStatusEventControl);
	//ota����ʧ�ܺ�������ɹ���ʧ�ܣ� led״̬����
	//��������������͵紫�������ж��ź�
	iKonkeAfSelfPrint("\r\n####kDelaySetLedStatusEventHandler status(%d)\r\n", g_eSensorCheckStatus);
	//static uint8_t count = 0;

	switch (g_eSensorCheckStatus){
	case SCS_INIT:
		break;
	case SCS_LOCK:
	case SCS_CLEAR_INT:
	{
		kP824mReset();
	}

#if 0
		g_eSensorCheckStatus = SCS_DELAY_CHECK_BODY_EXIST;
		count = 0;

		emberEventControlSetDelayMS(kDelaySetLedStatusEventControl, IR_SENSOR_DELAY_TIME_MS);
		iKonkeAfSelfPrint("\r\nSensor Clear\r\n");
		break;
	case SCS_DELAY_CHECK_BODY_EXIST:
		if (GET_IR_SENSOR_STATUS == true){
			if (count++ >= 2){
				g_eSensorCheckStatus = SCS_REPORT_BODY_EXIST;
				count = 0;
				iKonkeAfSelfPrint("#### report IasZoneStatus After 15S\r\n");
				emberEventControlSetDelayMS(kDelaySetLedStatusEventControl, IR_SENSOR_LOCK_TIME_MS + IR_SENSOR_DELAY_TIME_MS);
			}else {
				emberEventControlSetDelayMS(kDelaySetLedStatusEventControl, IR_SENSOR_DELAY_TIME_MS / 10);
			}
		}else
#endif
		{
			//count = 0;
			iKonkeAfSelfPrint("#### return sensor init\r\n");
			g_eSensorCheckStatus = SCS_INIT;
		}
		break;
#if 0
	case SCS_REPORT_BODY_EXIST:
		//���͵紫��������
		kIasZoneSensorChangedStatusReport(true, false);
		g_eSensorCheckStatus = SCS_CLEAR_INT;
		emberEventControlSetDelayMS(kDelaySetLedStatusEventControl, IR_SENSOR_LOCK_TIME_MS);
		break;
	case SCS_EXIT:
		kP824mReset();
		g_eSensorCheckStatus = SCS_INIT;
		break;
#endif
	default:
		kP824mReset();
		g_eSensorCheckStatus = SCS_INIT;
		break;
	}
}


//ota����ʧ�ܻ������ɹ���ʧ�ܺ�LED��˸��ɺ�����Ҫ���ݲ�ͬ��Ʒ���޸�
static void kSetLedStatus(unsigned char id)
{
	//�ָ���ɫLED����״̬
//	if (kNwkGetCurrentStatus() == ENS_ONLINE){
//		if (kGetIndicatorNotDisturbModeFlg() == true){
//			LED_OPT_OFF(LED_SYS_ID);
//		}else {
//			LED_OPT_ON(LED_SYS_ID);
//		}
//	}else {
//		LED_OPT_OFF(LED_SYS_ID);
//	}
#if 0
	if (id == LED123_ID){
		//close  short poll check (data request),
		kkPollProceduleTrigger(0, true);
		kkEnterSleep(true, true);
		LED_OPT_OFF(LED_SYS_ID);
	}

	if (kPwmGradientChangeIsGoing() == true){
		//�Ƶ���һ�»������º�ָ��ƵƳ���
		if (kNwkGetCurrentStatus() == ENS_ONLINE || kNwkGetCurrentStatus() == ENS_OFFLINE){
			LED_OPT_ON(LED_SYS_ID);
		}
		if(!emberEventControlGetActive(kPwmGradientChangeEventControl)) {
			emberEventControlSetDelayMS(kPwmGradientChangeEventControl, 50);
		}
	}
#endif
	//when ota update timeout, restore ota status
	if (kGetOTAStatus() != OTA_NORMAL) {
		kOTAMoudleUserCallback(OTA_FAILED);
	}
}
static bool kClustePeriodReportCheckIsSet(uint8_t endpoint, uint16_t clusterID, uint16_t attributeID)
{
	uint16_t u16MinTimeMS = 0;
	uint16_t u16MaxTimeMS = 0;
//	if (endpoint <= g_ucRelayTotalNum){
		bool status = kkClusterGetReportingPeriod(endpoint, clusterID, attributeID, &u16MinTimeMS, &u16MaxTimeMS);
		if (status == true){
			if (u16MaxTimeMS > 0){
				return true;
			}
		}
	//}
	return false;
}





static void kZclClusterReportRecordBeforeRejoin(uint8_t srcEndpoint, uint16_t groupId, uint8_t sceneId, uint16_t time)
{
#if 1
	static ZclSceneRecallRepSt zclSceneRecallRept = {.src_endpoint = 0, .group_id = 0, .scene_id = 0};
	zclSceneRecallRept.src_endpoint = srcEndpoint;
	zclSceneRecallRept.group_id = groupId;
	zclSceneRecallRept.scene_id = sceneId;
	zclSceneRecallRept.time = time;
	if( kNwkGetCurrentStatus() == ENS_OFFLINE ) {
		iKonkeAfSelfPrint("kZclClusterReportRecordBeforeRejoin srcEp(%d) groupId(0x%x) sceneId(0x%x)\r\n", \
				zclSceneRecallRept.src_endpoint, zclSceneRecallRept.group_id, zclSceneRecallRept.scene_id);
		kNwkRejoinMethodTrigger(kZclSceneRecallReportCallback, &zclSceneRecallRept);
	}
#endif
}
/*	DESP: report scene recall after network rejoined when key triggered
 * 	Auth: maozj.20200706.
 * */
static void kZclSceneRecallReportCallback(void *param)
{
	ZclSceneRecallRepSt *pZclSceneRecallRept = (ZclSceneRecallRepSt *)param;

	//memcpy(&zclSceneRecallRept, (ZclSceneRecallRepSt *)param, sizeof(ZclSceneRecallRepSt));
	if (pZclSceneRecallRept->src_endpoint != 0){
		iKonkeAfSelfPrint("kZclSceneRecallReportCallback srcEp(%d) groupId(0x%x) sceneId(0x%x)\r\n", \
				pZclSceneRecallRept->src_endpoint, pZclSceneRecallRept->group_id, pZclSceneRecallRept->scene_id);

		kZclSceneRecall(pZclSceneRecallRept->src_endpoint, pZclSceneRecallRept->group_id \
				, pZclSceneRecallRept->scene_id, pZclSceneRecallRept->time);
	}
}
static void kSystemSetupStatusSet(uint8_t systemStartedStatus, uint32_t delayTimeMS)
{
	g_u8SystemSetupStatus = systemStartedStatus;
	emberEventControlSetDelayMS(kkSystemSetUpEventControl, delayTimeMS);
}

//�����豸���������Ӳ��ϣ��������а����Ե��ϱ�������Ӱ�칦��
static void  kUserNetworkStopTaskCallback(uint8_t schedule_id)
{
	if (kNwkGetCurrentStatus() == ENS_OFFLINE){
		//kZclClusterSetPermitReportInfo(1, ZCL_IAS_ZONE_CLUSTER_ID,  true, false, true, false);

		if (kClustePeriodReportCheckIsSet(1, ZCL_DIAGNOSTICS_CLUSTER_ID, ZCL_LAST_MESSAGE_RSSI_ATTRIBUTE_ID)== true \
			|| kClustePeriodReportCheckIsSet(1, ZCL_DIAGNOSTICS_CLUSTER_ID, ZCL_LAST_MESSAGE_LQI_ATTRIBUTE_ID) == true ){
			kZclClusterSetPermitReportInfo(1, ZCL_DIAGNOSTICS_CLUSTER_ID,  true, false, false, false);
		}

		if (kClustePeriodReportCheckIsSet(1, ZCL_POWER_CONFIG_CLUSTER_ID, ZCL_BATTERY_VOLTAGE_ATTRIBUTE_ID)== true \
			|| kClustePeriodReportCheckIsSet(1, ZCL_POWER_CONFIG_CLUSTER_ID, ZCL_BATTERY_PERCENTAGE_REMAINING_ATTRIBUTE_ID)== true){
			kZclClusterSetPermitReportInfo(1, ZCL_POWER_CONFIG_CLUSTER_ID,  true, false, false, false);
		}
	}
}

static void kClearBatteryVoltageAndPercentage(void)
{
#if 1
	uint8_t value = 0;
	kZclClusterSetPermitReportInfo(1, ZCL_POWER_CONFIG_CLUSTER_ID,  true, false, false, false);
	emberAfWriteServerAttribute(1, ZCL_POWER_CONFIG_CLUSTER_ID, ZCL_BATTERY_VOLTAGE_ATTRIBUTE_ID, &value, ZCL_INT8U_ATTRIBUTE_TYPE);
	emberAfWriteServerAttribute(1, ZCL_POWER_CONFIG_CLUSTER_ID, ZCL_BATTERY_PERCENTAGE_REMAINING_ATTRIBUTE_ID, &value, ZCL_INT8U_ATTRIBUTE_TYPE);
#endif
}


#if 1  //环境上报 人体上报
void kIasZoneSensorChangedStatusReport(bool ir_sensor_status, uint8_t trigger_state)
{
	static uint64_t lastTimeMS = 0;
	static uint64_t lastPreventTimeMS = 0;
	uint64_t currentTimeMS = halCommonGetInt64uMillisecondTick();
	uint64_t timeDiffMS = currentTimeMS - lastTimeMS;
	uint64_t preventTimeDiffMS =  currentTimeMS - lastPreventTimeMS;
	uint8_t status;
	uint8_t alarm;


    switch(trigger_state){
      case 1:  //无人
        alarm = 0;
      break;

      case 2: //有人活动
        alarm = 1;
      break;

      case 3:  //有人休闲
        alarm = 2;
      break;

      case 4:  //有人睡眠
        alarm = 3;
      break;
    }

    status = alarm ;

    if (ir_sensor_status == true){

      kZclClusterSetPermitReportInfo(1, ZCL_IAS_ZONE_CLUSTER_ID,  true, false, true, false);

        kZclIASZoneStatusUpdate(1, status , false);

        iKonkeAfSelfPrint("!!!!INTO kIasZoneSensorChangedStatusReport ir_sensor_status = (%d)  !!!!!!\r\n", status);

      lastTimeMS = currentTimeMS;
      lastPreventTimeMS = currentTimeMS;
    }else {
      kZclIASZoneStatusUpdate(1, status , false);
    }
    //avoid to private cluster  response payload combined
    emberAfClearResponseData();

}
#else
void kIasZoneSensorChangedStatusReport(void)
{
	uint8_t iasSosValue = 0x02;
	kZclIASZoneStatusUpdate(1, iasSosValue, false);

	//check short poll(data request)
	kkPollProceduleTrigger(60, true);
}
#endif

static void kBodySensorSingleBoardTestSent(void)
{
	DataField_st send_buf;
	uint16_t batteryVoltage = 0;
	batteryVoltage = 50;
	send_buf.u8ARG[0] = 0x00;

	send_buf.u8ARG[1] = (uint8_t)(batteryVoltage >> 8);
	send_buf.u8ARG[2] = (uint8_t)(batteryVoltage);
	send_buf.u8Datalen = 6;
	send_buf.seq = g_u16BodySensorSequenceRecord;
	send_buf.u8ChannelID = 0x00;
	send_buf.u16Opcode = UART_MSG_BODY_TRIGGER_START_REPORT_OPCODE;
	kCmdMsgDataSent(Z_TO_H_NO_ACK, send_buf, false);
}


static void kBodySensorStopReportSent(void)
{
	iKonkeAfSelfPrint("!!!!INTO kBodySensorStopReportSent 111111!!!!!\r\n");
	DataField_st send_buf;
	send_buf.u8ARG[0] = 0x00;
	send_buf.u8Datalen = 4;
	send_buf.seq = g_u16BodySensorStopReportSequenceRecord;
	send_buf.u8ChannelID = 0x00;
	send_buf.u16Opcode = UART_MSG_BODY_TRIGGER_STOP_REPORT_OPCODE;
	kCmdMsgDataSent(Z_TO_H_NO_ACK, send_buf, false);
}
/** @brief Pre Message Send
 *
 * This function is called by the framework when it is about to pass a message
 * to the stack primitives for sending.   This message may or may not be ZCL,
 * ZDO, or some other protocol.  This is called prior to
        any ZigBee
 * fragmentation that may be done.  If the function returns true it is assumed
 * the callback has consumed and processed the message.  The callback must also
 * set the EmberStatus status code to be passed back to the caller.  The
 * framework will do no further processing on the message.
        If the
 * function returns false then it is assumed that the callback has not processed
 * the mesasge and the framework will continue to process accordingly.
 *
 * @param messageStruct The structure containing the parameters of the APS
 * message to be sent.  Ver.: always
 * @param status A pointer to the status code value that will be returned to the
 * caller.  Ver.: always
 */
boolean emberAfPreMessageSendCallback(EmberAfMessageStruct* messageStruct,
                                   EmberStatus* status)
{
	iKonkeAfSelfPrint("###emberAfPreMessageSendCallback status(0x%x)\r\n", status);
#if 0
	if (g_eSensorCheckStatus == SCS_INIT) {
		g_eSensorCheckStatus = SCS_LOCK;
		iKonkeAfSelfPrint("###Sensor Locked After Message Sent\r\n");

		emberEventControlSetDelayMS(kDelaySetLedStatusEventControl, 3.2 * 1000);
	}
#else
	u64LastReportMsgTime = halCommonGetInt64uMillisecondTick();
#endif
#if 1
	//change sensitivity max interval 120 minutes
	static uint8_t u8MsgSendCounter = 0;
	if (u8MsgSendCounter++ >= MAX_SAMPLE_TEMPERATURE_INTERVAL_TIMES) {
		u8MsgSendCounter = 0;

	}

#endif
//	float temperature = kGetTemperatureByAdcId(ADC_TEMP_ID);
//	kP824SensitivitySetByTemp(temperature);
	return false;
}

/** @brief Power Configuration Cluster Server Attribute Changed
 *
 * Server Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 */
void emberAfPowerConfigClusterServerAttributeChangedCallback(uint8_t endpoint,     //这里
                                                             EmberAfAttributeId attributeId)
{
  if (ZCL_BATTERY_PERCENTAGE_REMAINING_ATTRIBUTE_ID == attributeId){
    uint8_t temp_percentage = 0;
    emberAfReadServerAttribute( endpoint,
                        ZCL_POWER_CONFIG_CLUSTER_ID,
                        ZCL_BATTERY_PERCENTAGE_REMAINING_ATTRIBUTE_ID,
                        &temp_percentage,
                        sizeof(temp_percentage));
    temp_percentage = temp_percentage / 2;
    emberAfPrint(0xFFFF, "emberAfPowerConfigClusterServerAttributeChangedCallback, battery temp_percentage(%d)\r\n", temp_percentage);
    if(temp_percentage < BATTERY_LOWPOWER_PERCENTAGE){
      g_bBatteryLowPower = true;
    }else{
      g_bBatteryLowPower = false;
    }
  }
}






/*  DESP: uart command message incoming and process interface.
 *  Auth: maozj.20200725.
 * */
void UartMsgProcessHandle(UMsgNodeSt *pMsgNode )
{
  uint8_t recvdata[] = {0};
  uint8_t i = 0,len = 0;
  static uint16_t last_light = 0,light;
  static uint8_t last_alarm;
  static uint8_t poweron_flag = 0;
  uint8_t l_f,f_f,e_f;

  len = pMsgNode->length -1;

  for(i=0; i<len; i++){
      recvdata[i] = pMsgNode->buffer[i+1];
  }

  if( (NULL == pMsgNode) || (uart_recv_flag == 0 )) {
     return ;
  }

  if((recvdata[0] != 0)|| (recvdata[1] != 0) ){
      if((recvdata[1] == 0x06) &&(recvdata[2] == 0x03) &&(poweron_flag == 0)){
          if(((recvdata[3]<<8)|recvdata[4])<=1200) {
             humanbody.light = (recvdata[3]<<8)|recvdata[4];
             iKonkeAfSelfPrint("opcode=%d,light=%d\n",OPCODE_LIGHT,humanbody.light);
             light = humanbody.light;
             poweron_flag =1;
          }

      }else{
          return;
      }

  }

#if 1 // Just for debug
  iKonkeAfSelfPrint("\r\n-- MSG INCOMING[%d]:\r\n", len);
  iKonkeAfSelfPrintBuffer(recvdata, len, true);
  iKonkeAfSelfPrint("\r\n--------------------\r\n");
#endif


  uint8_t opcode = recvdata[3];



 switch(opcode){
     case OPCODE_WORK_STATE:        //0x01
        if((recvdata[4] == 0x01) || (recvdata[4] == 0x02)){
            humanbody.work_state = recvdata[4];
            //iKonkeAfSelfPrint("opcode=%d,work_state=%d\n",OPCODE_WORK_STATE,humanbody.work_state);
        }else{
            //iKonkeAfSelfPrint("opcode=%d,work_state=err\n",OPCODE_WORK_STATE);
        }
     break;

     case OPCODE_DETECTION_AREA:    //0x02灵敏度

       if((recvdata[4] >0) && (recvdata[4] <=100) && (grep_2 == 0)){
           humanbody.detection_area = recvdata[4];
           iKonkeAfSelfPrint("opcode=%d,detection_area=%d\n",OPCODE_DETECTION_AREA,humanbody.detection_area);

		   uint8_t detect;

		  emberAfReadAttribute(1, ZCL_BASIC_CLUSTER_ID, ZCL_Sensitivity_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
              (uint8_t*)&detect, sizeof(detect), NULL); // data type

		  if(detect != humanbody.detection_area){
		  	humanbody.detection_area = detect;
			iKonkeAfSelfPrint("SYN detection_area =%d\n",humanbody.detection_area);
		  }

           kZclClusterSetPermitReportInfo(1, ZCL_BASIC_CLUSTER_ID, true, false, true, false); //灵敏度上报
           emberAfWriteServerAttribute(1, ZCL_BASIC_CLUSTER_ID,
               ZCL_Sensitivity_ATTRIBUTE_ID, (uint8_t*)&humanbody.detection_area,
               ZCL_INT8U_ATTRIBUTE_TYPE);

       }else if((recvdata[4] >0) && (recvdata[4] <=100) && (grep_2 != 0)){
           grep_2--;
           humanbody.detection_area = recvdata[4];
           iKonkeAfSelfPrint("save-opcode=%d,detection_area=%d\n",OPCODE_DETECTION_AREA,humanbody.detection_area);
       }else{
           iKonkeAfSelfPrint("opcode=%d,detection_area=err\n",OPCODE_DETECTION_AREA);
       }
     break;

     case OPCODE_LIGHT:             //0x03
       if( ((recvdata[4]<<8)|recvdata[5])>=0 && (((recvdata[4]<<8)|recvdata[5])<=1200) && (grep_3 == 0) ){

           humanbody.light = (recvdata[4]<<8)|recvdata[5];
           iKonkeAfSelfPrint("opcode=%d,light=%d\n",OPCODE_LIGHT,humanbody.light);
           light = humanbody.light;

           if(abs(humanbody.light - last_light) >=30){

               kZclClusterSetPermitReportInfo(1, ZCL_ILLUM_MEASUREMENT_CLUSTER_ID, true, false, true, false); //光照值上报

               emberAfWriteServerAttribute(1, ZCL_ILLUM_MEASUREMENT_CLUSTER_ID,
                   ZCL_ILLUM_MEASURED_VALUE_ATTRIBUTE_ID, (uint8_t*)&light,
                   ZCL_INT16U_ATTRIBUTE_TYPE);

               last_light = light;
            }

       }else if(((recvdata[4]<<8)|recvdata[5])>=0 && (((recvdata[4]<<8)|recvdata[5])<=1200) && (grep_3 != 0)){
           grep_3--;
           humanbody.light = (recvdata[4]<<8)|recvdata[5];
           iKonkeAfSelfPrint("save-opcode=%d,light=%d\n",OPCODE_LIGHT,humanbody.light);
       } else{
           iKonkeAfSelfPrint("opcode=%d,light=err",OPCODE_LIGHT);
       }


     break;

     case OPCODE_HOLD_TIME :        //0x05
       if( (((recvdata[4]<<8)|recvdata[5]) >=30) && (((recvdata[4]<<8)|recvdata[5])<=1800) && (grep_5 == 0)){
           humanbody.hold_time = (recvdata[4]<<8)|recvdata[5];
           iKonkeAfSelfPrint("opcode=%d,hold_time=%d\n",OPCODE_HOLD_TIME,humanbody.hold_time);

		   uint16_t hold;
		   
		   emberAfReadAttribute(1, ZCL_BASIC_CLUSTER_ID, ZCL_DelayConfiguration_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
             (uint8_t*)&hold, sizeof(hold), NULL); // data type

		   if(hold != humanbody.hold_time){
		   	 humanbody.hold_time = hold;
			 iKonkeAfSelfPrint("SYN hold_time =%d\n",humanbody.hold_time);
		   }

           kZclClusterSetPermitReportInfo(1, ZCL_BASIC_CLUSTER_ID, true, false, true, false); //延时上报

           emberAfWriteServerAttribute(1, ZCL_BASIC_CLUSTER_ID,
               ZCL_DelayConfiguration_ATTRIBUTE_ID, (uint8_t*)&humanbody.hold_time,
               ZCL_INT16U_ATTRIBUTE_TYPE);
       }else if((((recvdata[4]<<8)|recvdata[5]) >=30) && (((recvdata[4]<<8)|recvdata[5])<=1800) && (grep_5 != 0)){
           grep_5 --;
           humanbody.hold_time = (recvdata[4]<<8)|recvdata[5];
           iKonkeAfSelfPrint("save-opcode=%d,hold_time=%d\n",OPCODE_HOLD_TIME,humanbody.hold_time);
       }else{
           iKonkeAfSelfPrint("opcode=%d,hold_time=err\n",OPCODE_HOLD_TIME);
       }
     break;

     case OPCODE_LED_SWITCH:        //0x06
        if((recvdata[4] == 0x00) && (grep_6 == 0)){
            humanbody.led_switch = false;
            l_f = 1;
            iKonkeAfSelfPrint("opcode=%d,led_switch=off\n",OPCODE_LED_SWITCH,humanbody.led_switch);
        }else if((recvdata[4] == 0x01) && (grep_6 == 0)){
            humanbody.led_switch = true;
            l_f = 1;
            iKonkeAfSelfPrint("opcode=%d,led_switch=on\n",OPCODE_LED_SWITCH,humanbody.led_switch);
        }else if(((recvdata[4] == 0x00)||(recvdata[4] == 0x01)) && (grep_6 != 0)){
            grep_6--;
            humanbody.led_switch = recvdata[4];
            iKonkeAfSelfPrint("save-opcode=%d,led_switch=%d\n",OPCODE_LED_SWITCH,humanbody.led_switch);
        }else{
            iKonkeAfSelfPrint("opcode=%d,led_switch=err\n",OPCODE_LED_SWITCH);
            l_f = 0;
        }


        if(l_f == 1){
            bool led = true;

		    emberAfReadAttribute(1, ZCL_BASIC_CLUSTER_ID, ZCL_IndicateMode_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
               (uint8_t*)&led, sizeof(led), NULL); // data type    

			if(led != humanbody.led_switch){
				humanbody.led_switch = led;
				iKonkeAfSelfPrint("SYN led_switch = %d",humanbody.led_switch);
			}
		
            kZclClusterSetPermitReportInfo(1, ZCL_BASIC_CLUSTER_ID, true, false, true, false); //ledswitch上报

            emberAfWriteServerAttribute(1, ZCL_BASIC_CLUSTER_ID,
                ZCL_IndicateMode_ATTRIBUTE_ID, (uint8_t*)&humanbody.led_switch,
                ZCL_INT8U_ATTRIBUTE_TYPE);
        }


     break;

     case OPCODE_EXIST_SWITCH:      //0x07
        if((recvdata[4] == 0x00) && (grep_7 == 0)){
            humanbody.exist_switch = false;
            e_f = 1;
            iKonkeAfSelfPrint("opcode=%d,exist_switch=off\n",OPCODE_EXIST_SWITCH,humanbody.exist_switch);
        }else if((recvdata[4] == 0x01) && (grep_7 == 0)){
            humanbody.exist_switch = true;
            e_f = 1;
            iKonkeAfSelfPrint("opcode=%d,exist_switch=on\n",OPCODE_EXIST_SWITCH,humanbody.exist_switch);
        }else if(((recvdata[4] == 0x00)||(recvdata[4] == 0x01)) &&(grep_7 != 0)){
            humanbody.exist_switch = recvdata[4];
            iKonkeAfSelfPrint("save-opcode=%d,exist_switch=%d\n",OPCODE_EXIST_SWITCH,humanbody.exist_switch);
        }else{
            iKonkeAfSelfPrint("opcode=%d,exist_switch=err\n",OPCODE_EXIST_SWITCH);
            e_f = 0;
        }

        if(e_f == 1){
		    bool exist = true;

		    emberAfReadAttribute(1, ZCL_BASIC_CLUSTER_ID, ZCL_ExistenceSwitch_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
               (uint8_t*)&exist, sizeof(exist), NULL); // data type

			if(exist != humanbody.exist_switch){
				humanbody.exist_switch = exist;
				iKonkeAfSelfPrint("SYN exist_switch = %d",humanbody.exist_switch);
			}
               
            kZclClusterSetPermitReportInfo(1, ZCL_BASIC_CLUSTER_ID, true, false, true, false); //存在使能上报

            emberAfWriteServerAttribute(1, ZCL_BASIC_CLUSTER_ID,
                ZCL_ExistenceSwitch_ATTRIBUTE_ID, (uint8_t*)&humanbody.exist_switch,
                ZCL_INT8U_ATTRIBUTE_TYPE);
        }
     break;

     case OPCODE_FINAL_SWITCH:      //0x09
        if((recvdata[4] == 0x00) && (grep_9 == 0)){
            humanbody.final_switch = false;
            f_f = 1;
            iKonkeAfSelfPrint("opcode=%d,final_switch=off\n",OPCODE_FINAL_SWITCH,humanbody.final_switch);
        }else if((recvdata[4] == 0x01) && (grep_9 == 0)){
            humanbody.final_switch = true;
            f_f = 1;
            iKonkeAfSelfPrint("opcode=%d,final_switch=on\n",OPCODE_FINAL_SWITCH,humanbody.final_switch);
        }else if(((recvdata[4] == 0x00)||(recvdata[4] == 0x01)) &&(grep_9 != 0)){
            grep_9 --;
            humanbody.final_switch = recvdata[4];
            iKonkeAfSelfPrint("save-opcode=%d,final_switch=%d\n",OPCODE_FINAL_SWITCH,humanbody.final_switch);
        }else{
            iKonkeAfSelfPrint("opcode=%d,final_switch=err\n",OPCODE_FINAL_SWITCH);
            f_f = 0;
        }

        if(f_f == 1){
			bool final = true;

		    emberAfReadAttribute(1, ZCL_BASIC_CLUSTER_ID, ZCL_SlightMoveSwitch_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
               (uint8_t*)&final, sizeof(final), NULL); // data type

			if(final != humanbody.final_switch){
				humanbody.final_switch = final;
				iKonkeAfSelfPrint("SYN final_switch = %d",humanbody.final_switch);
			}	
			
            kZclClusterSetPermitReportInfo(1, ZCL_BASIC_CLUSTER_ID, true, false, true, false); //微动使能上报

            emberAfWriteServerAttribute(1, ZCL_BASIC_CLUSTER_ID,
                ZCL_SlightMoveSwitch_ATTRIBUTE_ID, (uint8_t*)&humanbody.final_switch,
                ZCL_INT8U_ATTRIBUTE_TYPE);
        }
     break;

     case OPCODE_TRIGGER_STATE:     //0x0c
        if((recvdata[4] >=1) && (recvdata[4] <=4) &&(grep_c ==0)){
            humanbody.trigger_state = recvdata[4];
            iKonkeAfSelfPrint("opcode=%d,trigger_state=%d\n",OPCODE_TRIGGER_STATE,humanbody.trigger_state);
        }else if((recvdata[4] >=1) && (recvdata[4] <=4) &&(grep_c !=0)){
            grep_c --;
            humanbody.trigger_state = recvdata[4];
            iKonkeAfSelfPrint("save-opcode=%d,trigger_state=%d\n",OPCODE_TRIGGER_STATE,humanbody.trigger_state);
        }else{
            iKonkeAfSelfPrint("opcode=%d,trigger_state=err\n",OPCODE_TRIGGER_STATE);
        }

        if((last_alarm != humanbody.trigger_state) && (grep_c == 0)){
            kIasZoneSensorChangedStatusReport(true,humanbody.trigger_state);    //状态commamd上报
            last_alarm = humanbody.trigger_state;


        }
     break;

     case OPCODE_CONNECT_PAIR:      //0x0f
        if((recvdata[4]>=0) && (recvdata[4]<=2)){
            humanbody.connect_pair = recvdata[4];
            iKonkeAfSelfPrint("opcode=%d,connect_pair=%d\n",OPCODE_CONNECT_PAIR,humanbody.connect_pair);
        }else{
            iKonkeAfSelfPrint("opcode=%d,connect_pair=err\n",OPCODE_CONNECT_PAIR);
        }

        if((humanbody.connect_pair == 2) &&  (grep_f == 0)){

            bool status = false;
            if (kNwkGetCurrentStatus() == ENS_ONLINE || kNwkGetCurrentStatus() == ENS_OFFLINE ) {
                if(kNwkGetCurrentStatus() != ENS_JOINING){
                  status = true;
                  iKonkeAfSelfPrint("Factory Reset  111\r\n");
                  kNwkFactoryReset(status);  //先退网
                }
            }else if (kNwkGetCurrentStatus() == ENS_LEAVED){
                iKonkeAfSelfPrint("Start Steering  111\r\n");
                kNwkJoiningStart(NWK_STEERING_TIMEOUT_MS, JoinCompleteCallback);  //NWK_STEERING_TIMEOUT_MS
            }

        }else if(grep_f != 0){
            grep_f--;
        }
     break;
 }


}


void UartSendHandle(uint8_t opcode,uint16_t value){
    uint8_t value_8,len;
    uint16_t value_16;
    uint8_t buffer[] = {0x00,0x00};

    iKonkeAfSelfPrint("\r\n----UartSendHandle----\r\n");

    switch(opcode){
      case OPCODE_DETECTION_AREA:    //02
        grep_2 ++;
        buffer[2] = 0x05;
        buffer[3] = opcode;
        buffer[4] = (uint8_t)value;
        buffer[5] = Get_CheckSum(buffer,5);
        MsgSent(&buffer,6);
        iKonkeAfSelfPrint("\r\n----Set DETECTION_AREA  value=%d % -----\r\n",value);
      break;

      case OPCODE_HOLD_TIME :       //05
        grep_5++;
        buffer[2] = 0x06;
        buffer[3] = opcode;
        buffer[4] = (value>>8) &0xff;
        buffer[5] = value & 0xff;
        buffer[6] = Get_CheckSum(buffer,6);
        iKonkeAfSelfPrint("\r\n----Set HOLD_TIME  value=%d s -----\r\n",value);
        MsgSent(&buffer,7);
      break;

      case OPCODE_LED_SWITCH:       //06
        grep_6++;
        buffer[2] = 0x05;
        buffer[3] = opcode;
        buffer[4] = (uint8_t)value;    //1Enable  0Dis
        buffer[5] = Get_CheckSum(buffer,5);
        iKonkeAfSelfPrint("\r\n----Set LED_SWITCH  value=%d  -----\r\n",value);
        MsgSent(&buffer,6);
      break;

      case OPCODE_EXIST_SWITCH:    //07
        grep_7++;
        buffer[2] = 0x05;
        buffer[3] = opcode;
        buffer[4] = (uint8_t)value;    //1Enable  0Dis
        buffer[5] = Get_CheckSum(buffer,5);
        iKonkeAfSelfPrint("\r\n----Set EXIST_SWITCH  value=%d  -----\r\n",value);
        MsgSent(&buffer,6);
      break;

      case OPCODE_FINAL_SWITCH:
        grep_9 ++;
        buffer[2] = 0x05;
        buffer[3] = opcode;
        buffer[4] = (uint8_t)value;    //1Enable  0Dis
        buffer[5] = Get_CheckSum(buffer,5);
        iKonkeAfSelfPrint("\r\n----Set FINAL_SWITCH  value=%d  -----\r\n",value);
        MsgSent(&buffer,6);
      break;

      case OPCODE_CONNECT_PAIR:
        buffer[2] = 0x05;
        buffer[3] = opcode;
        buffer[4] = (uint8_t)value;
        buffer[5] = Get_CheckSum(buffer,5);
        iKonkeAfSelfPrint("\r\n----Set CONNECT_PAIR  value=%d  -----\r\n",value);
        MsgSent(&buffer,6);
        MsgSent(&buffer,6);
      break;

      default:
        iKonkeAfSelfPrint("\r\n----UartSend_err_parameter----\r\n");
      break;
    }


}

void emberAfBasicClusterServerAttributeChangedCallback(int8u endpoint,
                                                       EmberAfAttributeId attributeId)
{
  iKonkeAfSelfPrint("\r\n-------!!!BasicClusterServerAttributeChangedCallback!!!------\r\n");

#if 1
  //(kGetFactoryTestStatus() == FTS_NORMAL)
  
if((endpoint == 1)){
 if ( (setup_send_msg == true)){

    if(attributeId == ZCL_IndicateMode_ATTRIBUTE_ID){   //触发指示灯
        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_IndicateMode_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&humanbody.led_switch, sizeof(humanbody.led_switch), NULL); // data type
        iKonkeAfSelfPrint("\r\n-------!!!INTO BasicChangedCallback  = (%d) (%d)--------\r\n", OPCODE_LED_SWITCH,humanbody.led_switch);
        UartSendHandle(OPCODE_LED_SWITCH,humanbody.led_switch);
    }
    if(attributeId ==  ZCL_Sensitivity_ATTRIBUTE_ID){  //灵敏度
        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_Sensitivity_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&humanbody.detection_area, sizeof(humanbody.detection_area), NULL); // data type
        iKonkeAfSelfPrint("\r\n-------!!!INTO BasicChangedCallback  = (%d) (%d)--------\r\n", OPCODE_DETECTION_AREA,humanbody.detection_area);
        UartSendHandle(OPCODE_DETECTION_AREA,humanbody.detection_area);
    }
    if(attributeId ==  ZCL_DelayConfiguration_ATTRIBUTE_ID){  //延时时间
        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_DelayConfiguration_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&humanbody.hold_time, sizeof(humanbody.hold_time), NULL); // data type
        iKonkeAfSelfPrint("\r\n-------!!!INTO BasicChangedCallback  = (%d) (%d)--------\r\n", OPCODE_HOLD_TIME,humanbody.hold_time);
        UartSendHandle(OPCODE_HOLD_TIME,humanbody.hold_time);
    }
    if(attributeId ==  ZCL_SlightMoveSwitch_ATTRIBUTE_ID){  //微动开关
        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_SlightMoveSwitch_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&humanbody.final_switch, sizeof(humanbody.final_switch), NULL); // data type
        iKonkeAfSelfPrint("\r\n-------!!!INTO BasicChangedCallback  = (%d) (%d)--------\r\n",OPCODE_FINAL_SWITCH, humanbody.final_switch);
        UartSendHandle(OPCODE_FINAL_SWITCH,humanbody.final_switch);
    }
    if(attributeId ==  ZCL_ExistenceSwitch_ATTRIBUTE_ID){  //存在开关
        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_ExistenceSwitch_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&humanbody.exist_switch, sizeof(humanbody.exist_switch), NULL); // data type
        iKonkeAfSelfPrint("\r\n-------!!!INTO BasicChangedCallback  = (%d) (%d)--------\r\n", OPCODE_EXIST_SWITCH,humanbody.exist_switch);
        UartSendHandle(OPCODE_EXIST_SWITCH,humanbody.exist_switch);
    }
  	}

	else if(setup_send_msg == false){

	
  	if(attributeId == ZCL_IndicateMode_ATTRIBUTE_ID){   //触发指示灯
        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_IndicateMode_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&humanbody_tocken.led_switch, sizeof(humanbody_tocken.led_switch), NULL); // data type
        iKonkeAfSelfPrint("\r\n-------!!!GET basic tocken  = (%d) (%d)--------\r\n", OPCODE_LED_SWITCH,humanbody_tocken.led_switch);
        //UartSendHandle(OPCODE_LED_SWITCH,humanbody.led_switch);
    }
    if(attributeId ==  ZCL_Sensitivity_ATTRIBUTE_ID){  //灵敏度
        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_Sensitivity_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&humanbody_tocken.detection_area, sizeof(humanbody_tocken.detection_area), NULL); // data type
        iKonkeAfSelfPrint("\r\n-------!!!GET basic tocken  = (%d) (%d)--------\r\n", OPCODE_DETECTION_AREA,humanbody_tocken.detection_area);
        //UartSendHandle(OPCODE_DETECTION_AREA,humanbody.detection_area);
    }
    if(attributeId ==  ZCL_DelayConfiguration_ATTRIBUTE_ID){  //延时时间
        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_DelayConfiguration_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&humanbody_tocken.hold_time, sizeof(humanbody_tocken.hold_time), NULL); // data type
        iKonkeAfSelfPrint("\r\n-------!!!GET basic tocken  = (%d) (%d)--------\r\n", OPCODE_HOLD_TIME,humanbody_tocken.hold_time);
        //UartSendHandle(OPCODE_HOLD_TIME,humanbody.hold_time);
    }
    if(attributeId ==  ZCL_SlightMoveSwitch_ATTRIBUTE_ID){  //微动开关
        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_SlightMoveSwitch_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&humanbody_tocken.final_switch, sizeof(humanbody_tocken.final_switch), NULL); // data type
        iKonkeAfSelfPrint("\r\n-------!!!GET basic tocken  = (%d) (%d)--------\r\n",OPCODE_FINAL_SWITCH, humanbody_tocken.final_switch);
        //UartSendHandle(OPCODE_FINAL_SWITCH,humanbody.final_switch);
    }
    if(attributeId ==  ZCL_ExistenceSwitch_ATTRIBUTE_ID){  //存在开关
        emberAfReadAttribute(endpoint, ZCL_BASIC_CLUSTER_ID, ZCL_ExistenceSwitch_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
        (uint8_t*)&humanbody_tocken.exist_switch, sizeof(humanbody_tocken.exist_switch), NULL); // data type
        iKonkeAfSelfPrint("\r\n-------!!!GET basic tocken  = (%d) (%d)--------\r\n", OPCODE_EXIST_SWITCH,humanbody_tocken.exist_switch);
        //UartSendHandle(OPCODE_EXIST_SWITCH,humanbody.exist_switch);
    }
  	}

  }
#endif
}

uint8_t Get_CheckSum(uint8_t *str, int str_length)
{
    uint8_t CheckSum_Value = 0;
    int i = 0;
    for(i = 0; i < str_length; i++)
    {
       CheckSum_Value += str[i];
    }
    CheckSum_Value = CheckSum_Value & 0xff;
    //return CheckSum_Value;
    return CheckSum_Value;
}


