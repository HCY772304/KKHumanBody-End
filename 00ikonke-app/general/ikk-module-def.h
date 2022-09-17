#ifndef __IKONKE_MODULE_DEFINE_H_____________________________
#define __IKONKE_MODULE_DEFINE_H_____________________________

#include "stdint.h"

// choose the type of equipment, ZC(Coordinator), ZR(Router), ZED(End Device).
#define Z30_DEVICE_ZC							(0)
#define Z30_DEVICE_ZR							(1)
#define Z30_DEVICE_ZED							(2)
#define Z30_DEVICE_ZED_SLEEPY					(3)

#define Z30_DEVICE_DTYPE						(Z30_DEVICE_ZED)  //设备类型修改
//�Ƿ���ҪOTA
#define Z30_DEVICE_OTA_ENABLE					true

//�����ܿ��أ� �Ƿ����ϻ��򵥰����������
#define Z30_DEVICE_FACTORY_TEST_ENABLE			true
// �����Ƿ���Ҫ�ϻ���Ŀǰһ��·���豸��Ҫ
#define Z30_DEVICE_AGING_ENABLE					false
//������ԣ�Ŀǰһ���������豸��Ҫ
#define Z30_DEVICE_SINGLE_BOARD_ENABLE			true
//�������ԣ� Ŀǰһ���������豸��Ҫ
#define Z30_DEVICE_FULL_DEVICE_ENBALE			true

//sleepy device battery define, used to implement rejoin mechanism, different battery different rejoin time
#define BATTERY_LARGE_CAPCITY					1
#define BATTERY_SMALL_CAPCITY					2
//���������豸�������
#define SLEEPY_DEVICE_BATTERY_TYPE         	 	BATTERY_LARGE_CAPCITY

#if SLEEPY_DEVICE_BATTERY_TYPE == BATTERY_LARGE_CAPCITY
#define SHORT_POLL_TIME_AFTER_JOIN_NWK_S		(30)
#elif
#define SHORT_POLL_TIME_AFTER_JOIN_NWK_S		(15)
#endif

//�����豸�Ƿ��ǿ����豸�� ��������,��ʱδʹ��
#define SLEEPY_DEVICE_CONTROL_ENABEL			true

#if SLEEPY_DEVICE_CONTROL_ENABEL
#define SHORT_POLL_WAKE_TIME_S	(30)
#else
#define SHORT_POLL_WAKE_TIME_S	(6)
#endif

// Real time IO operation, no optimization!!!
#define _IO										volatile

#define CLUSTER_BASIC_ID							0X0000	// Basic Cluster���ڻ�ȡ���豸������Ϣ
#define CLUSTER_POWER_ID							0X0001	// ����
#define CLUSTER_IDENTIFY_ID							0X0003	// Identify
#define CLUSTER_GROUPS_ID							0X0004	// Groups
#define CLUSTER_SCENES_ID							0X0005	// Scenes
#define CLUSTER_ONOFF_ID							0X0006	// ����
#define CLUSTER_LEVEL_CONTROL_ID					0X0008	// Level Control
#define CLUSTER_IAS_ZONE_ID							0X0500	// IAS Zone(����̽���豸)
#define CLUSTER_IAS_WD_ID							0X0502	// IAS WD(���������豸)
#define CLUSTER_TEMP_ID								0X0402	// �¶�
#define CLUSTER_HUMI_ID								0X0405	// ʪ��
#define CLUSTER_WINDOW_COVERING_ID					0X0102	// ����
#define CLUSTER_ELECTRICAL_MEASUREMENT_ID			0X0B04	// ��������
#define CLUSTER_OPTTUNNEL_ID						0XFCC0	// �Զ���Cluster-0XFC01������ʵ��˽��Э��
#define CLUSTER_MULTI_CONTROL_ON_OFF_ID				0XFCC6	// �Զ��壬����ʵ��ON-OFF���,ֻ���ڰ󶨱��Ƿ����ж�
#define CLUSTER_MULTI_CONTROL_WINDOW_COVERING_OPTTUNNEL_ID	0XFCC7	// �Զ���Cluster�� ����ʵ��window-covering���
#define CLUSTER_UNKNOW								0XFFFF

#define ATTRIBUTE_UNKNOW							0XFFFF

//δ֪��GPIO���Ŷ���
#define	PORT_UNKNOW									0xFF
#define PIN_UNKNOW									0XFF

//����ͨ��Э��OPCODE
#define UART_MSG_QUERY_INFO_OPCODE					0x0000
#define UART_MSG_WRITE_INSTALL_CODE_OPCODE			0x0001
#define UART_MSG_READ_INSTALL_CODE_OPCODE			0x0002
#define UART_MSG_WRITE_CMEI_CODE_OPCODE       0x0003
#define UART_MSG_READ_CMEI_CODE_OPCODE        0x0004
#define UART_MSG_WRITE_ISN_CODE_OPCODE        0x0005
#define UART_MSG_READ_ISN_CODE_OPCODE       0x0006
#define UART_MSG_WRITE_MAC_CODE_OPCODE        0x0007
#define UART_MSG_READ_MAC_CODE_OPCODE       0x0008
#define UART_MSG_WRITE_MOUDLE_ID_OPCODE       0x0009
#define UART_MSG_READ_MOUDLE_ID_OPCODE        0x000A
#define UART_MSG_READ_DEV_RSSI_OPCODE       0x000B
#define UART_MSG_WRITE_AGING_TIME_OPCODE      0x000C
#define UART_MSG_READ_AGING_TIME_OPCODE       0x000D
#define UART_MSG_WRITE_INTERPAN_PARA_OPCODE     0x000E
#define UART_MSG_READ_INTERPAN_PARA_OPCODE      0x000F
#define UART_MSG_READ_DEVICE_SNAP_OPCODE			0x0010
#define UART_MSG_JOIN_NWK_REQUEST_OPCODE			0x0100
#define UART_MSG_LEAVE_NWK_REQUEST_OPCODE			0x0101
#define UART_MSG_QUERY_NWK_STATUS_REQUEST_OPCODE	0x0102
#define UART_MSG_NWK_STATUS_NOTIFY_OPCODE			0x0103
#define UART_MSG_READ_ATTRIBUTE_REQUEST_OPCODE		0x0104
#define UART_MSG_WRITE_ATTRIBUTE_REQUEST_OPCODE		0x0105
#define UART_MSG_ZCL_COMMAND_REQUEST_OPCODE			0x0106
#define UART_MSG_BIND_REQUEST_OPCODE				0x0110

#define UART_MSG_GATEWAY_NWK_CHANNEL_ONOFF_OPCODE 0x00FC
#define UART_MSG_PRIVATE_PROTOCOL_CONTROL_CHILD_DEVICE_START_OPCODE 0xF000
#define UART_MSG_PRIVATE_PROTOCOL_CONTROL_CHILD_DEVICE_END_OPCODE 0xF0FF
#define UART_MSG_TTS_OPCODE

//factory test message opcode
#define UART_MSG_QUERY_DEVICE_VERSION_OPCODE        0xED00
#define UART_MSG_QUERY_DEVICE_INFO_OPCODE			0xED01
#define UART_MSG_CONFIG_DEVICE_SLEEP_OPCODE			0xED02
#define UART_MSG_EXIT_FACTORY_TEST_OPCODE			0xED04
#define	UART_MSG_QUERY_FACTORY_INFO_OPCODE			0xED05
#define PRI_MSG_READ_KEY_TEST_OPCODE            0xED06
#define UART_MSG_ENTER_FACTORY_TEST_OPCODE      0xED14
#define UART_MSG_ONOFF_SWITCH_OPCODE        0xED15
#define UART_MSG_QUERY_FACTORY_STATE_OPCODE     0xED16
#define UART_MSG_LED_GROUP_CONTROL_OPCODE     0xED17

#define UART_MSG_READ_MAC_REQUEST_OPCODE			0x0120
#define UART_MSG_GPIO_TEST_OPCODE					0xED10
#define UART_MSG_GET_GPIO_TEST_STATUS_OPCODE		0xED11
#define UART_MSG_READ_RSSI_REQUEST_OPCODE			0xED12

//offline voice panel
#define UART_MSG_HOST_SNAP_REQUEST_OPCODE			0x1000
#define UART_MSG_CTRL_HOST_ONOFF_OPCODE				0x1001  //zigbee module control panel
#define UART_MSG_HOST_ONOFF_NOTIFY_OPCODE     0x1002 //panel onoff status changed notify zigbee
#define UART_MSG_HOST_TRIGGER_SCENE_NOTIFY_OPCODE 0x1003  //pannel recall scene notify zigbees
#define UART_MSG_KEY_LIGHT_SCENE_TRANSFORM_OPCODE 0x1004
#define UART_MSG_TIME_SET_OPCODE          0x1005
#define UART_MSG_TIME_GET_OPCODE          0x1006
#define UART_MSG_DEVICE_OPERATOR_OPCODE       0x1007
#define UART_MSG_FILE_UPDATE_NOTIFY_OPCODE      0x1008
#define UART_MSG_FILE_DATA_REQUEST_OPCODE     0x1009
#define UART_MSG_FILE_UPDATE_STATUS_NOTIFY      0x100A

 //bangde human body sensor
#define UART_MSG_BODY_TRIGGER_START_REPORT_OPCODE	0xED03
#define UART_MSG_BODY_TRIGGER_STOP_REPORT_OPCODE	0xED06
//bangde sos button
#define UART_MSG_SOS_REPORT_OPCODE					0xED03
//bangde scene button
#define UART_MSG_BUTTON_SCENE_REPORT_OPCODE			0xED03
//bangde environment sensor
#define UART_MSG_QUERY_TEMP_HUMI_INFO_OPCODE		0xED03
//bangde door sensor
#define UART_MSG_BUTTON_ONOFF_REPORT_OPCODE			0xED03



typedef enum {
	ERR_OK=0x00,
	ERR_EXIST=0x01,
	ERR_FORMAT=0x02,
	ERR_NOT_EXPECTED=0x03,
	ERR_WRITE_FAILED=0x04,
	ERR_READ_WRITE_TIMEOUT=0x05,
	ERR_NOT_EXIST=0x06,
	ERR_EXEC_FAILED=0xe0,
	ERR_UNKNOW=0xFF,
}ERR_RET_E;



typedef enum { ECOM_PORT_UART = 0, ECOM_PORT_RTT, ECOM_PORT_ZIGBEE, ECOM_PORT_INTERPAN, ECOM_PORT_FCC0}ComPortEm;


typedef enum kk_error_type {
	KET_OK = 0,							// �������ɹ���

	KET_FAILED = -1,					// ʧ��
	KET_ERR_TIMEOUT = -2,				// ������ʱ
	KET_ERR_MALLOC_FAILED = -3,			// mallocʧ��
	KET_NO_RESPONSE = -4,				//����Ҫack

	KET_ERR_INVALID_FD = -100,			// ��Ч��FD
	KET_ERR_OPEN_FAIL = -101,			// �ļ�/socket�ȴ�ʧ��
	KET_ERR_CLOSE_FAILED = -102,		// �ļ�/socket�ȹر�ʧ��
	KET_ERR_READ_FAILED = -103,			// �ļ�/socket�ȶ�ȡʧ��
	KET_ERR_WRITE_FAILED = -104,		// �ļ�/socket��д��ʧ��

	KET_ERR_INVALID_POINTER = -200,		// ��Чָ��
	KET_ERR_INVALID_PARAM = -201,		// ��Ч����

	KET_ERR_CMD_FORMAT = -301,			// ָ���ʽ����
	KET_ERR_CMD_PARSE_FAILED = -302,	// ָ���������
	KET_ERR_CMD_INVALID = -303,			// ָ����Ч
	KET_ERR_CMD_FAILED = -304,			// ָ�����ʧ��

	KET_ERR_NON_EXIST = -900,			// ������
	KET_ERR_NO_PERMISSIONS = -901,		// û��Ȩ��
	KET_ERR_PROCESS_ABORT = -902,		// �쳣�ж�
	KET_ERR_NOT_READY = -903,			// not ready
	KET_ERR_OPRATE_ILLEGAL = -904,		// �Ƿ�����
	KET_ERR_CRC_WRONG = -905,			// У�����
	KET_ERR_CONTENT_MISMATCH = -906,	// ���ݲ�ƥ��
	KET_ERR_INSUFFICIENT_SPACE = -907,	// �ռ䲻��
	KET_ERR_OPRATE_FAILED = -908,		// ����ʧ��
	KET_ERR_OPRATE_IN_PROGRESS = -908,	// ����������
	KET_ERR_UNKNOW = -999,				// λ�ô���
}kk_err_t;

typedef enum { Z3D_COORDINATOR = 0, Z3D_ROUTER, Z3D_ENDDEVICE }Z3DevTypeEm;

typedef void (*pFUNC_VOID)(void *);

typedef struct tag_common_opt_callback_st {
	pFUNC_VOID pfunc;
	void *param;
}CommonOptCallbackSt;

#endif
