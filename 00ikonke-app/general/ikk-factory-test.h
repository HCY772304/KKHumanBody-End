#ifndef __IKONKE_MODULE_FACTORY_TEST_H_______________________
#define __IKONKE_MODULE_FACTORY_TEST_H_______________________

#include "app/framework/include/af.h"
#include "ikk-module-def.h"
#include "ikk-debug.h"
#include "../driver/ikk-button.h"
#include "ikk-command.h"
#include "../driver/ikk-uart.h"
//ʹ��debug���ٲ����ϻ�
#define AGING_DEBUG                 			false
//�������������ť�����˳��ϻ�
#define AGING_EXIT_MAX_BTN_NUM					2


//����ǿ�����ϻ�ʱ���������������粬��10A/16A�����Ĵ�����������5S�������ϻ����ᴥ��е����ֻҪ1��20S
#define LONG_PRESS_1_TIMES						1
#define LONG_PRESS_2_TIMES						2
//ǿ�����ϻ�ʱ��
#define TOUCH_KEY_LONG_PRESS_TIME_MS			(5 * 1000)
#define MECHANICAL_KEY_LONG_PRESS_TIME_MS		(20 * 1000)

#define FIRST_BTN_LONG_PRESSED_VALIED_TIME_MS	(10 * 1000)

#define FACTORY_TEST_POLL_TTME_1S				(1 * 1000)
#define FACTORY_TEST_POLL_TTME_10S				(10 * 1000)
#define FACTORY_TEST_POLL_TIMES					(FACTORY_TEST_POLL_TTME_10S / FACTORY_TEST_POLL_TTME_1S)
#if AGING_DEBUG
#define FACTORY_TEST_15MIN_MS					(1 * 60 * 1000)
#define FACTORY_TEST_4HOU_MS					(4 * 1 * 60 * 1000)
#else
#define FACTORY_TEST_15MIN_MS					(15 * 60 * 1000)
#define FACTORY_TEST_4HOU_MS					(4 * 60 * 60 * 1000)
#endif
#define FACTORY_TEST_1S							(1000)


typedef enum{
  FTS_NORMAL=0,
  FTS_START,
  FTS_AGING_15MIN,
  FTS_AGING_8HOU_START,
  FTS_AGING_8HOU,
  FTS_AGING_DONE,
  FTS_FORCE_EXIT,
  FTS_WAITING_RELEASED,//waiting key released
  FTS_FORCE_REBOOT,
  FTS_FIRST_LONG_PRESSED,
  FTS_WAITING_SECOND_PRESS,
  FTS_SINGLE_BOARD_TEST,
  FTS_FULL_DEVICE_TEST,
  FTS_SINGLE_BOARD_TEST_DONE,
  FTS_FULL_DEVICE_TEST_DONE,     //14
}FactoryTestStatusEnum;

#pragma pack(1)
typedef struct{
	uint8_t u8BtnId;
	uint8_t u8BtnLongPressTimes; //touch keys must be long pressed 2 times, mechanical keys is pressed 1 time
	uint32_t u32AgingForceExitTimeMs;  //key long pressed time for  exit aging test
}FactoryTestConfSt;
#pragma pack()

//10S��ѯ�����������ɸ��ݲ�ͬ�������Ʒ�Զ���ʵ��
typedef void (*pFactoryTestPollCallback)(FactoryTestStatusEnum status);

//��ȡ��ǰ����״̬
FactoryTestStatusEnum kGetFactoryTestStatus(void);
//void  kGetFactoryTestBtnAction(uint8_t btn_id, BtnActionEnum action);
void kFactoryTestInit(pFactoryTestPollCallback callback, FactoryTestConfSt conf_list[], uint8_t btn_num, uint8_t led_id);

void kkFactoryMsgInComingCallBack(UMsgNodeSt *pMsgNode);

void kClusterExitFullDeviceTestCallback(EmberAfClusterCommand* cmd);

kk_err_t kkFactoryMsgInComingPaser(UMsgNodeSt *pMsgNode, ComPortEm port, DataField_st *pDataOut);

#endif

