#ifndef __IKONKE_USER_H______________________________________
#define __IKONKE_USER_H______________________________________


#include <00ikonke-app/driver/ikk-adc.h>
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "ctype.h"

#include "driver/ikk-led.h"
#include "driver/ikk-uart.h"
#include "driver/ikk-button.h"
#include "driver/ikk-relay.h"
#include "general/ikk-common-utils.h"
#include "general/ikk-module-def.h"
#include "general/ikk-network.h"
#include "general/ikk-cluster.h"
#include "general/ikk-opt-tunnel.h"
#include "general/ikk-debug.h"
#include "./general/ikk-command.h"
#include "driver/ikk-pwm.h"
#include "tools/ikk-rtt.h"
#if Z30_DEVICE_OTA_ENABLE
#include "general/ikk-ota.h"
#endif
#if Z30_DEVICE_FACTORY_TEST_ENABLE
#include "general/ikk-factory-test.h"
#endif


// ENDPOINT DEFINEs
#define USER_ENDPOINT 				(1)

// DEVICE HEARTBEAT PERIOD
#define HEARTBEAT_PERIOD_S			14400UL	// UNIT: S

// iHORN device type
#define iHORN_DEV_NATURAL_GAS 		0X41	// ��ȼ��
#define iHORN_DEV_SMOKE 			0X51	// ����
#define iHORN_DEV_WATER	 			0X73	// ˮ��

// HEADERs
#define iHORN_MSGHEAD_DOWN			0XF5	// �������ݣ�ģ�鵽MCU��֡ͷ
#define iHORN_MSGHEAD_UP			0XFA	// �������ݣ�MCU��ģ�飩֡ͷ

// OPCODEs
#define CMD_OFFSET					2
#define CMD_RETRY_SENT				0X00	// �ط�������Ϣ
#define CMD_REPORT_DATA				0X01	// �����ϱ�
#define CMD_UP_JOIN_RESUME			0X02	// ���� & �ָ�����
#define CMD_DOWN_LED				0X03	// LED����

typedef enum {
	RUN_STATUS_SINGLE_BOARD_TEST = 0x01,
	RUN_STATUS_FULL_DEVICE_TEST,
	RUN_STATUS_NORMAL,
	RUN_STATUS_REBOOT1,
	RUN_STATUS_REBOOT2,
	RUN_STATUS_REBOOT3,
	RUN_STATUS_INIT = 0xFF,
}RUN_SATUS_E;

typedef enum{
	SCS_INIT = 0,
	SCS_LOCK,
	SCS_CLEAR_INT,
	SCS_DELAY_CHECK_BODY_EXIST,
	SCS_REPORT_BODY_EXIST,
	SCS_EXIT
}SENSOR_CHECK_STATUS_E;


typedef enum { EJSP_UNKNOW = 0, EJSP_DEVSNAP, EJSP_CMEI_CODE, EJSP_ISN_CODE, EJSP_ENDED }JoinedSPEnum;

typedef enum {
	ERM_LEAVE_NETWORK = 0,	// Leave Network
	ERM_RESUME_FACTORY,		// Resume to Factory
}ResumeMethodEm;

// iHORN CMD data Format
#pragma pack(1)
typedef struct tag_ihorn_cmd_data {
	unsigned char preamble;
	unsigned char length;
	unsigned char opcode;
	unsigned int  seq_devid;
	unsigned char vendor_id;
	unsigned char dev_type;
	unsigned char data1;
	unsigned char data2;
	unsigned char extend[16];
}iHORNCmdFormatSt;


typedef struct
{
  uint8_t work_state;  //0:no body; 1:somebody
  uint8_t trigger_state; //1:无人 2活动 3休闲 4睡眠
  uint8_t detection_area; //感应距离0~100%
  uint16_t light;
  uint8_t light_difference;
  uint16_t hold_time; //延时时间 30~1800s
  bool led_switch;   //LED指示开关
  bool exist_switch; //存在使能开关
  bool final_switch; //微动使能开关
  uint8_t connect_pair;     //连接配对
}HUMANBODY_DATA;

// iHORN  sensor staus buffer
#pragma pack(1)
typedef struct tag_ihorn_sensor_status_data {
	unsigned char mcu_version;
	unsigned char status;
}SensorStatusSt;


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


//__no_init uint8_t ramAddrTest1 @ 0x2000FFFC;
//__no_init uint8_t ramAddrTest2 @ 0x2000FFFD;
//uint8_t ramAddrTest3;

extern uint8_t runStatus;
extern EmberEventControl kkSystemSetUpEventControl;
extern EmberEventControl kkFindingAndBindingEvent;
extern EmberEventControl kUserJoinSucceedProcedureEventControl;
extern EmberEventControl kkHeartBeatLoopEvent;
extern KKDeviceObjectSt g_stDeviceObject;

/*	DESP: User Module init interface.
 * 	Auth: dingmz_frc.20190623.
 * */
void UserModuleInit(void );

void kkMfglibRunningStart(uint8_t channel);

static void kkMfg_Send(uint16_t opcode);


#endif
