//#include "app/framework/plugin/network-steering/network-steering.h"
//#include "app/framework/plugin/network-steering/network-steering-internal.h"
#include "ikk-factory-test.h"
#include "ikk-debug.h"
#include "../driver/ikk-button.h"
#include "../driver/ikk-led.h"
#include "ikk-cluster.h"
#include "ikk-command.h"


#define AGING_TEST_15MIN_VALUE					0xA1
#define AGING_TEST_8HOU_VALUE					0xA2
#define AGINE_TEST_DONE_VALUE					0xFA

#define SINGLE_BOARD_TEST_DONE_VALUE       	 	0xF1   //����������
#define FULL_DEVICE_TEST_DONE_VALUE          	0xF2   //�����������
#define FORCE_FACTORY_TEST_VALUE				0xF8	//���������ǿ���˳�

//��ʼֵ
#define TEST_INIT_VALUE							0xFF


//#define FACTORY_TEST_ENABLE 					true



#define LED_SLOW_BLINK_ON_TIME_MS				800
#define LED_SLOW_BLINK_OFF_TIME_MS				800
#define LED_FAST_BLINK_ON_TIME_MS				200
#define LED_FAST_BLINK_OFF_TIME_MS				200

#define LED_SLOW_BLINK_CONTINUE_TIME_MS			(60 * 1000)
#define LED_FAST_BLINK_CONTINUE_TIME_MS			(5 * 1000)


EmberEventControl kFactoryTestEventControl;


typedef struct{
	uint8_t u8BtnId;
	uint8_t u8BtnLongPressTimes; //touch keys must be long pressed 2 times, mechanical keys is pressed 1 time
	uint32_t u32AgingForceExitTimeMs;  //key long pressed time for  exit aging test
	uint32_t u32AgingForceExitTimeCounter;  //key long pressed time for  exit aging test
	BtnActionEnum elastBtnAction;
	BtnActionEnum eCurrentBtnAction;
	bool bIsFirstBtnLongPressed ;
	uint32_t u32FirstBtnLongPressedCountDown;
}FactoryTestControllerSt;

typedef enum{FT_SINGLE_BOARD_TEST=0x01, FT_FULL_DEVICE_TEST=0x02, FT_NORMAL_DONE=0x04, FT_FORCE_EXIT=0x08}FT_STATUS;

FactoryTestControllerSt g_stFactoryTestControllerList[AGING_EXIT_MAX_BTN_NUM] \
	= {{BTN_UNKNOW_ID, 0, 0, 0, EBA_IDLE, EBA_IDLE, 0, 0}, {BTN_UNKNOW_ID, 0, 0, 0, EBA_IDLE, EBA_IDLE, 0, 0}};

static FactoryTestStatusEnum g_eFactoryTestStatus = FTS_NORMAL;
static FactoryTestStatusEnum g_eLastFactoryTestStatus = FTS_NORMAL;
static uint32_t g_u32AgingTotalTimeMS = 0;
static pFactoryTestPollCallback g_pFactoryTestPollCallback = NULL;
static 	uint8_t g_u8LongPressedKeyIndex = 0;
static uint32_t g_u32Count = 0;
static uint8_t g_u8FactoryTestLedId = LED_UNKNOW_ID;  //just for single board test or full device test

/*	DESP: get the test index by id.
 * 	Auth: mazj.20200227.
 * */
static uint8_t kGetFactoryTestIndexByID(uint8_t id )
{
	for(int index = 0; index < AGING_EXIT_MAX_BTN_NUM; ++index ) {
		if( g_stFactoryTestControllerList[index].u8BtnId != BTN_UNKNOW_ID ) {
			if( g_stFactoryTestControllerList[index].u8BtnId == id ) {
				return index;
			}
		}else break;
	}

	return 0XFF;
}

FactoryTestStatusEnum kGetFactoryTestStatus(void)
{
	return g_eFactoryTestStatus;
}
//
//void kSetFactoryTestValue(uint8_t value)
//{
//	halCommonSetToken(TOKEN_FACTORY_TEST_FLG, &value);
//}

//uint8_t kGetFactoryTestValue(void)
//{
//	uint8_t value = TEST_INIT_VALUE;
//	halCommonGetToken(&value, TOKEN_FACTORY_TEST_FLG);
//	return value;
//}

void kSetAgingTestValue(uint8_t value)
{
	halCommonSetToken(TOKEN_AGING_TEST_FLG, &value);
}

uint8_t kGetAgingTestValue(void)
{
	uint8_t value = TEST_INIT_VALUE;
	halCommonGetToken(&value, TOKEN_AGING_TEST_FLG);
	return value;
}

void kSetSingleBoardValue(uint8_t value)
{
	halCommonSetToken(TOKEN_SINGLE_BOARD_TEST_FLG, &value);
}

uint8_t kGetSingleBoardValue(void)
{
	uint8_t value = TEST_INIT_VALUE;
	halCommonGetToken(&value, TOKEN_SINGLE_BOARD_TEST_FLG);
	return value;
}

void kSetFullDeviceValue(uint8_t value)  //����������־λ  FULL_DEVICE
{
	halCommonSetToken(TOKEN_FULL_DEVICE_TEST_FLG, &value);
}

uint8_t kGetFullDeviceValue(void)   //��ȡ�����־λ  FULL_DEVICE
{
	uint8_t value = TEST_INIT_VALUE;
	halCommonGetToken(&value, TOKEN_FULL_DEVICE_TEST_FLG);
	return value;
}
//�����ϻ�
uint8_t kGetFactoryTestValueStatus(void)
{
    uint8_t status = 0;
#if Z30_DEVICE_FACTORY_TEST_ENABLE

#if Z30_DEVICE_SINGLE_BOARD_ENABLE
    if (kGetSingleBoardValue() == FORCE_FACTORY_TEST_VALUE) {
        status = status | FT_FORCE_EXIT;
     }else if (kGetSingleBoardValue() != SINGLE_BOARD_TEST_DONE_VALUE) {
    	status = status | FT_SINGLE_BOARD_TEST;
    }else {
#if !Z30_DEVICE_FULL_DEVICE_ENBALE
    	status = status | FT_NORMAL_DONE;
#endif
    }
#endif

#if Z30_DEVICE_FULL_DEVICE_ENBALE
    if (kGetFullDeviceValue() == FORCE_FACTORY_TEST_VALUE) {
        status = status | FT_FORCE_EXIT;
     }else if (kGetFullDeviceValue() != FULL_DEVICE_TEST_DONE_VALUE){
    	status = status | FT_FULL_DEVICE_TEST;
    }else {
    	status = status | FT_NORMAL_DONE;
    }
#endif
#endif
    return status;
}

void  kFactoryTestBtnActionCallback(unsigned char button_id, BtnActionEnum action)
{
	if (kGetFactoryTestStatus() != FTS_NORMAL)
	{
		uint8_t u8FactoryTestIndex = kGetFactoryTestIndexByID(button_id);
		if (u8FactoryTestIndex != 0xFF){
			//add bby maozj 20200828
			if (emberEventControlGetActive(kFactoryTestEventControl) != true) {
				emberEventControlSetActive(kFactoryTestEventControl);
			}

			iKonkeAfSelfPrint("######kGetFactoryTestBtnAction action(%d)\r\n", action);
			g_stFactoryTestControllerList[u8FactoryTestIndex].eCurrentBtnAction = action;

			//���ָ�λ,����������ʱ�䰴��Ҳ���Զ�����
			if ((g_eFactoryTestStatus == FTS_WAITING_RELEASED || g_eFactoryTestStatus == FTS_FORCE_EXIT) \
			          && g_stFactoryTestControllerList[g_u8LongPressedKeyIndex].eCurrentBtnAction == EBA_RELEASED){
				g_eFactoryTestStatus = FTS_FORCE_REBOOT;
				g_u32Count = FACTORY_TEST_POLL_TIMES; //���ִ���Զ��庯��
				iKonkeAfSelfPrint("######33333333333333333 FactoryTest Released\r\n");
			}
		}
	}
}

static uint16_t opcodeReampTable[][2] =
{
  //FCC0, AA55
  {0xF0, 0xED14},//打开产测
  {0xF1, 0xED04},//退出产测
  {0xF2, 0xED05},//查询产测（按bit位）
    {0xF8, 0xED16},//查询产测（按字节）
};

static uint16_t kCmdOpcodeRemap(uint16_t Opcode, uint8_t direction)
{
  uint16_t remapOpcode = 0xFFFF;
  for (uint8_t i = 0; i < (sizeof(opcodeReampTable)/(2*sizeof(uint16_t))); i++ ) {
    if (opcodeReampTable[i][0] == Opcode) {
      remapOpcode = opcodeReampTable[i][1];
      break;
    }

  }

  iKonkeAfSelfPrint("kCmdOpcodeRemap(%d)\r\n", remapOpcode);
  return remapOpcode;
}

void kFactoryTestInit(pFactoryTestPollCallback callback, FactoryTestConfSt conf_list[], uint8_t btn_num, uint8_t led_id)
{
#if Z30_DEVICE_FACTORY_TEST_ENABLE

	if(NULL == callback || NULL == conf_list || btn_num > AGING_EXIT_MAX_BTN_NUM) {
		return;
	}
	for (uint8_t i = 0; i < AGING_EXIT_MAX_BTN_NUM; i++){
		g_stFactoryTestControllerList[i].u8BtnId = conf_list[i].u8BtnId;
		g_stFactoryTestControllerList[i].u8BtnLongPressTimes = conf_list[i].u8BtnLongPressTimes;
		g_stFactoryTestControllerList[i].u32AgingForceExitTimeMs = conf_list[i].u32AgingForceExitTimeMs;
		//g_stFactoryTestControllerList[i].u32AgingForceExitTimeCounter = conf_list[i].u32AgingForceExitTimeCounter;
	}


	iKonkeAfSelfPrint("######g_stFactoryTestControllerList[0].u8BtnId= %d u8BtnLongPressTimes = %d\r\n", g_stFactoryTestControllerList[0].u8BtnId,  g_stFactoryTestControllerList[0].u8BtnLongPressTimes);
	iKonkeAfSelfPrint("######g_stFactoryTestControllerList[1].u8BtnId= %d u8BtnLongPressTimes = %d\r\n", g_stFactoryTestControllerList[1].u8BtnId,  g_stFactoryTestControllerList[1].u8BtnLongPressTimes);
#if Z30_DEVICE_AGING_ENABLE
	if (kGetAgingTestValue() != AGINE_TEST_DONE_VALUE){
		kSetAgingTestValue(AGING_TEST_15MIN_VALUE);
		g_eFactoryTestStatus = FTS_START;
		//ע�ᰴť�ص�����
		kBtnaIRQCallbackRegister(kFactoryTestBtnActionCallback);
		//���������¼�
		emberEventControlSetDelayMS(kFactoryTestEventControl, 10);  //kFactoryTestEventHandler
		g_eFactoryTestStatus = FTS_START;
		g_pFactoryTestPollCallback = callback;
		g_u32AgingTotalTimeMS = 0;
	}else {
		g_eFactoryTestStatus = FTS_NORMAL;
	}
#endif

#if Z30_DEVICE_SINGLE_BOARD_ENABLE
	if (g_eFactoryTestStatus == FTS_NORMAL && kGetSingleBoardValue() != SINGLE_BOARD_TEST_DONE_VALUE \
			&& kGetSingleBoardValue() != FORCE_FACTORY_TEST_VALUE){
		//ע�ᰴť�ص�����
		kBtnaIRQCallbackRegister(kFactoryTestBtnActionCallback);
		//���������¼�
		emberEventControlSetDelayMS(kFactoryTestEventControl, 10); // kFactoryTestEventHandler
		g_eFactoryTestStatus = FTS_SINGLE_BOARD_TEST;
		g_pFactoryTestPollCallback = callback;
		g_u8FactoryTestLedId = led_id;
		//kkEnterSleep(false, false);

		//�����ϵ���˸һ�κ���
		kLedOptTrigger(g_u8FactoryTestLedId, LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_OFF_TIME_MS, 1, LED_ON, LED_ON);
	}
#endif

#if Z30_DEVICE_FULL_DEVICE_ENBALE
	if (g_eFactoryTestStatus == FTS_NORMAL && kGetFullDeviceValue() != FULL_DEVICE_TEST_DONE_VALUE \
			&& kGetFullDeviceValue() != FORCE_FACTORY_TEST_VALUE){
		//ע�ᰴť�ص�����
		kBtnaIRQCallbackRegister(kFactoryTestBtnActionCallback);
		//���������¼�
		emberEventControlSetDelayMS(kFactoryTestEventControl, 10);
		g_eFactoryTestStatus = FTS_FULL_DEVICE_TEST;
		g_pFactoryTestPollCallback = callback;
		g_u8FactoryTestLedId = led_id;
		//��������һֱpoll
//		kkPollProceduleTrigger(0xFF, true);
		//kkEnterSleep(false, false);
		//���������ϵ���˸���κ���
		iKonkeAfSelfPrint("!!!!!INTO kFactoryTestInit Z30_DEVICE_FULL_DEVICE_ENBALE 1111!!!!!!!!\r\n");
		kLedOptTrigger(g_u8FactoryTestLedId, LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_OFF_TIME_MS, 2, LED_ON, LED_ON);
	}
#endif

//	g_u8AgingForceExitBtnId = btn_id;
#else
	uint8_t value = TEST_INIT_VALUE;
	kSetAgingTestValue(value);
	kSetSingleBoardValue(value);
	kSetFullDeviceValue(value);
	g_eFactoryTestStatus = FTS_NORMAL;
#endif


	iKonkeAfSelfPrint("\r\n######kFactoryTestInit status(%d)\r\n", g_eFactoryTestStatus);
}


//********************

kk_err_t kkFactoryMsgInComingPaser(UMsgNodeSt *pMsgNode, ComPortEm port, DataField_st *pDataOut)
 {
  //UMsgNodeSt uMsgNode = {0};
  uint16_t length = 0;
  uint8_t control_field = 0;
  DataField_st data;
  DataField_st send_buf;
  uint8_t reply_control_field = 0;

  if( NULL == pMsgNode ) {
    return KET_ERR_INVALID_PARAM;
  }

#if 1 // Just for debug
  iKonkeAfSelfPrint("\r\n-- MSG INCOMING[%d]:\r\n", pMsgNode->length);
  //iKonkeAfSelfPrintBuffer(pMsgNode->buffer, pMsgNode->length, true);
  iKonkeAfSelfPrint("\r\n--------------------\r\n");
#endif

  // check for parsable packets
  if( pMsgNode->length < 5 /* Minimum Packet Length */ ) {
    iKonkeAfSelfPrint("Err: Unparsable Packets!!\r\n");
    return KET_ERR_INVALID_PARAM;
  }
  //length(1) pid(4) ch(1) opcode(1) arg
  if (port == ECOM_PORT_FCC0) {
    length = pMsgNode->length;
    //control_field = pMsgNode->buffer[4];
    data.seq = 0x00;
    data.u8Datalen = pMsgNode->buffer[0];
    data.u8Datalen = data.u8Datalen - 6; // fcc0 - id (4) - ch(1) - opcode(1)
    data.u8ChannelID = pMsgNode->buffer[5];
    data.u16Opcode = kCmdOpcodeRemap((uint16_t)(pMsgNode->buffer[6]), 0);

    if(data.u16Opcode == 0xFFFF) {
      return KET_ERR_INVALID_PARAM;
    }

    memcpy(data.u8ARG,&pMsgNode->buffer[7], data.u8Datalen); //6 =  pid(4) + ch(1) + opcode(1)

    send_buf.seq = data.seq;
    send_buf.u8ChannelID = data.u8ChannelID;
    send_buf.u16Opcode = data.u16Opcode;
  } else {
    length = ((uint16_t)pMsgNode->buffer[2] << 8) | (uint16_t)(pMsgNode->buffer[3]);
    control_field = pMsgNode->buffer[4];

    data.seq = ((uint16_t)pMsgNode->buffer[5] << 8) | (uint16_t)(pMsgNode->buffer[6]);

    data.u8Datalen = pMsgNode->buffer[7];
    data.u8Datalen = data.u8Datalen - 3; // aa55 - ch(1) - opcode(2)
    data.u8ChannelID = pMsgNode->buffer[8];
    data.u16Opcode = ((uint16_t)pMsgNode->buffer[9] << 8) |  (uint16_t)(pMsgNode->buffer[10]);
    memcpy(data.u8ARG,&pMsgNode->buffer[11],data.u8Datalen - 3);

    send_buf.seq = data.seq;
    send_buf.u8ChannelID = data.u8ChannelID;
    send_buf.u16Opcode = data.u16Opcode;
  }

  send_buf.u8Datalen = 0;

  switch (data.u16Opcode) {
  case (0x0000):
  {
    send_buf.u8ARG[0] = ERR_OK;
    send_buf.u8Datalen = 1;
    reply_control_field = Z_TO_H_NO_ACK;
    break;
  }
    case (UART_MSG_QUERY_DEVICE_VERSION_OPCODE):
  {
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
  case (UART_MSG_QUERY_DEVICE_INFO_OPCODE):
  {
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
    send_buf.u8Datalen = 9;
    reply_control_field = Z_TO_H_NO_ACK;
    break;
  }
  case (UART_MSG_CONFIG_DEVICE_SLEEP_OPCODE):
  {
    send_buf.u8ARG[0] = ERR_OK;
    send_buf.u8Datalen = 1;
    reply_control_field = Z_TO_H_NO_ACK;

    LED_OPT_OFF(g_u8FactoryTestLedId);
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
    //设备休眠
    kkEnterSleep(true, true);
#endif
    extern EmberEventControl ikkUartRecvLoopCheckEventControl;
    emberEventControlSetInactive(ikkUartRecvLoopCheckEventControl);
    emberEventControlSetInactive(kFactoryTestEventControl);
    break;
  }
  case (UART_MSG_EXIT_FACTORY_TEST_OPCODE):
  {
    uint8_t token_data = 0;
    switch (data.u8ARG[0]) {
    case 0x01:
      token_data = SINGLE_BOARD_TEST_DONE_VALUE;
      //halCommonSetToken(TOKEN_SINGLE_BOARD_TEST_FLG, &token_data);
      //halCommonGetToken(&token_data, TOKEN_SINGLE_BOARD_TEST_FLG);
      iKonkeAfSelfPrint("######kGetSingleBoardValue(%02x)\r\n", token_data);
      if (token_data == SINGLE_BOARD_TEST_DONE_VALUE) {
        send_buf.u8ARG[0] = ERR_OK;
        g_eFactoryTestStatus = FTS_SINGLE_BOARD_TEST_DONE;
        //单板结束，一直快闪
        kLedOptTrigger(g_u8FactoryTestLedId, LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_OFF_TIME_MS, BLINK_DEAD, LED_ON, LED_ON);
      } else {
        send_buf.u8ARG[0] = ERR_EXIST;
      }
      break;
    case 0x02:
      token_data = FULL_DEVICE_TEST_DONE_VALUE;
      //halCommonSetToken(TOKEN_FULL_DEVICE_TEST_FLG, &token_data);
      //halCommonGetToken(&token_data, TOKEN_FULL_DEVICE_TEST_FLG);
      iKonkeAfSelfPrint("######kGetSingleBoardValue(%02x)\r\n", token_data);
      if (token_data == FULL_DEVICE_TEST_DONE_VALUE) {
        send_buf.u8ARG[0] = ERR_OK;
        g_eFactoryTestStatus = FTS_FULL_DEVICE_TEST_DONE;
                //整机结束，一直快闪
        kLedOptTrigger(g_u8FactoryTestLedId, LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_OFF_TIME_MS, BLINK_DEAD, LED_ON, LED_ON);
      } else {
        send_buf.u8ARG[0] = ERR_EXIST;
      }
      break;
    case 0x03:
    case 0x04:
      token_data = AGINE_TEST_DONE_VALUE;
      //halCommonSetToken(TOKEN_AGING_TEST_FLG, &token_data);
      //halCommonGetToken(&token_data, TOKEN_AGING_TEST_FLG);
      if (token_data == AGINE_TEST_DONE_VALUE) {
        send_buf.u8ARG[0] = ERR_OK;
        //g_eFactoryTestStatus = FTS_AGING_DONE;
      } else {
        send_buf.u8ARG[0] = ERR_EXIST;
      }
      break;
    default:
      send_buf.u8ARG[0] = ERR_FORMAT;
      break;
    }
    send_buf.u8ARG[1] = data.u8ARG[0];
    send_buf.u8Datalen = 2;
    reply_control_field = Z_TO_H_NO_ACK;
    break;
  }
  case (UART_MSG_QUERY_FACTORY_INFO_OPCODE):
  {
    break;
  }
  case (UART_MSG_ENTER_FACTORY_TEST_OPCODE):
  {

    break;
  }
  case UART_MSG_ONOFF_SWITCH_OPCODE: {

    break;
  }

  case UART_MSG_QUERY_FACTORY_STATE_OPCODE: {

    break;
  }
  case UART_MSG_LED_GROUP_CONTROL_OPCODE: {

    break;
  }
  default:
    break;
  }

  memcpy(pDataOut, (uint8_t *)&send_buf, sizeof(DataField_st));

  if (send_buf.u8Datalen == 0) { //if cmd exist, lengtg must > 0
    return KET_ERR_CMD_INVALID;
  } else {
    if (port == ECOM_PORT_FCC0) {
     // pDataOut->u8Datalen = pDataOut->u8ArgLen;
    } else {
      //pDataOut->u8Datalen = pDataOut->u8ArgLen + 3; //AA 55 (opcode(2), ch(1))
    }
    return KET_OK;
  }

}


/** @brief Basic Cluster Server Attribute Changed
 *
 * Server Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 */
/*
void emberAfBasicClusterServerAttributeChangedCallback(int8u endpoint,
                                                       EmberAfAttributeId attributeId)
{
#if 0
  uint8_t value[3] = {0};
  if (attributeId == ZCL_MANUFACTURER_NAME_ATTRIBUTE_ID){
    emberAfReadAttribute( endpoint,
                ZCL_BASIC_CLUSTER_ID,
                ZCL_MANUFACTURER_NAME_ATTRIBUTE_ID,
                CLUSTER_MASK_SERVER,
                (uint8_t *)&value,
                sizeof(value),
                NULL);
    iKonkeAfSelfPrint("emberAfBasicClusterServerAttributeChangedCallback:[0x%x],[0x%x],[0x%x] \r\n",
              value[0],value[1],value[2]);
    if (value[0] == 0xF1 && value[1] == 0xF1 && value[2] == 0x00){
      kSetFullDeviceValue(FULL_DEVICE_TEST_DONE_VALUE);
      g_eFactoryTestStatus = FTS_FULL_DEVICE_TEST_DONE;
      kNwkFactoryReset(true);
    }
  }
#endif
}
*/

void kFactoryTestEventHandler(void)
{
	emberEventControlSetInactive(kFactoryTestEventControl);

	//iKonkeAfSelfPrint("######kFactoryTestEventHandler status(%d)\r\n", g_eFactoryTestStatus);

	g_u32AgingTotalTimeMS += FACTORY_TEST_POLL_TTME_1S;

	if (g_eFactoryTestStatus != FTS_WAITING_RELEASED && g_eFactoryTestStatus != FTS_WAITING_SECOND_PRESS){
		switch (g_eFactoryTestStatus)
		{
			case (FTS_START):
				//g_eFactoryTestStatus = FTS_AGING_15MIN;
				g_u32Count = FACTORY_TEST_POLL_TIMES;
				break;
			case (FTS_AGING_15MIN):
				//�ϵ��15����ʱ�䵽��15�����ڿ��Զ̰������ͳ���������������
				if (g_u32AgingTotalTimeMS >= FACTORY_TEST_15MIN_MS){
					kSetAgingTestValue(AGING_TEST_8HOU_VALUE);
					g_eFactoryTestStatus = FTS_AGING_8HOU_START;
					//15���Ӻ����¿�ʼ��ʱ
					g_u32AgingTotalTimeMS = 0;
					g_u32Count = FACTORY_TEST_POLL_TIMES;//��㴦���Զ��庯��
				}
				break;
			case (FTS_AGING_8HOU_START):
					//�����������е�LED״̬һ�£��ڻص�����ʵ��
					g_eFactoryTestStatus = FTS_AGING_8HOU;
				break;
			case (FTS_AGING_8HOU):
				//�ϵ��4Сʱ�ϻ�ʱ�䵽��4Сʱ�ڲ����Զ̰������ͳ�������������
				if (g_u32AgingTotalTimeMS >= FACTORY_TEST_4HOU_MS){
					kSetAgingTestValue(AGINE_TEST_DONE_VALUE);
					g_eFactoryTestStatus = FTS_AGING_DONE;
				}
				break;
			case (FTS_AGING_DONE):
				//�ϻ����˳���ǰ�¼�
				//return;
				g_u32Count = FACTORY_TEST_POLL_TIMES;//��㴦���Զ��庯��
				break;
			case (FTS_FORCE_EXIT):
				g_eFactoryTestStatus = FTS_WAITING_RELEASED;
				break;
			case (FTS_WAITING_RELEASED):
				break;
			case (FTS_FIRST_LONG_PRESSED):
				g_eFactoryTestStatus = 	FTS_WAITING_SECOND_PRESS;
				break;
			case (FTS_WAITING_SECOND_PRESS):
				break;
			default:
				break;
		}
	}

//	//ǿ�����ϻ����˵�������������Դ���
//#if AGINE_TEST_FORCE_EXIT_TIMES == 1
	for (uint8_t index = 0; index < AGING_EXIT_MAX_BTN_NUM; index++){
		if (g_stFactoryTestControllerList[index].u8BtnId != BTN_UNKNOW_ID ){
			if (g_stFactoryTestControllerList[index].eCurrentBtnAction == EBA_PRESSED \
					&& g_stFactoryTestControllerList[index].elastBtnAction != EBA_PRESSED){

				g_stFactoryTestControllerList[index].elastBtnAction = EBA_PRESSED;
				//g_stFactoryTestControllerList[index].eCurrentBtnAction = EBA_PRESSING;
				//���ó�������ʱ����
				g_stFactoryTestControllerList[index].u32AgingForceExitTimeCounter = \
						g_stFactoryTestControllerList[index].u32AgingForceExitTimeMs / FACTORY_TEST_POLL_TTME_1S;
			}else if (g_stFactoryTestControllerList[index].eCurrentBtnAction == EBA_RELEASED \
					&& g_stFactoryTestControllerList[index].elastBtnAction != EBA_RELEASED){
				iKonkeAfSelfPrint("&&&&&&& EBA_RELEASED u32AgingForceExitTimeCounter == %d\r\n",\
						g_stFactoryTestControllerList[index].u32AgingForceExitTimeCounter);
				g_stFactoryTestControllerList[index].elastBtnAction = EBA_RELEASED;
				g_stFactoryTestControllerList[index].u32AgingForceExitTimeCounter = 0;
			}else {
				 g_stFactoryTestControllerList[index].elastBtnAction  = g_stFactoryTestControllerList[index].eCurrentBtnAction;
			}
			//����ʱ20S��5S
			if (g_stFactoryTestControllerList[index].u32AgingForceExitTimeCounter > 0 \
					&& g_stFactoryTestControllerList[index].eCurrentBtnAction == EBA_PRESSED){

				iKonkeAfSelfPrint("######u32AgingForceExitTimeCounter-- %d\r\n", \
						g_stFactoryTestControllerList[index].u32AgingForceExitTimeCounter);

				if (--g_stFactoryTestControllerList[index].u32AgingForceExitTimeCounter == 0){

					iKonkeAfSelfPrint("######u32AgingForceExitTimeCounter == 0\r\n");

					if ( g_stFactoryTestControllerList[index].u8BtnLongPressTimes == LONG_PRESS_1_TIMES \
							|| g_stFactoryTestControllerList[index].bIsFirstBtnLongPressed == true){
						//successfully exit aging test
						g_eFactoryTestStatus = FTS_FORCE_EXIT;
#if Z30_DEVICE_AGING_ENABLE
						kSetAgingTestValue(AGINE_TEST_DONE_VALUE);
#endif
#if Z30_DEVICE_SINGLE_BOARD_ENABLE
//						uint8_t value = 0xF8;
						kSetSingleBoardValue(FORCE_FACTORY_TEST_VALUE);
						iKonkeAfSelfPrint("!!!!INTO kFactoryTestEventHandler kSetSingleBoardValue = (%02x)!!!!\r\n",kGetSingleBoardValue());
//						halCommonSetToken(TOKEN_IS_KONKE_GATEWAY, &value);
//						halCommonGetToken(&value, TOKEN_IS_KONKE_GATEWAY);
//						iKonkeAfSelfPrint("!!!!INTO kFactoryTestEventHandler TOKEN_IS_KONKE_GATEWAY = (%02x)!!!!\r\n",value);
#endif
#if Z30_DEVICE_FULL_DEVICE_ENBALE
						kSetFullDeviceValue(FORCE_FACTORY_TEST_VALUE);
						iKonkeAfSelfPrint("!!!!INTO kFactoryTestEventHandler kSetFullDeviceValue = (%02x)!!!!\r\n",kGetFullDeviceValue());
#endif
						g_u32Count = FACTORY_TEST_POLL_TIMES; //���ִ���Զ��庯��
						//��ֹ�ϻ�״̬������
						g_stFactoryTestControllerList[index].u32FirstBtnLongPressedCountDown = 0;
						g_u8LongPressedKeyIndex = index;
						//g_u32AgingTotalTimeMS = FACTORY_TEST_12HOU_MS;
						iKonkeAfSelfPrint("######11111111111 == 0\r\n");
					}else if ( g_stFactoryTestControllerList[index].u8BtnLongPressTimes == LONG_PRESS_2_TIMES){
						//����������Ҫ�������β����˳��ϻ�
						g_stFactoryTestControllerList[index].bIsFirstBtnLongPressed = true;
						g_stFactoryTestControllerList[index].u32FirstBtnLongPressedCountDown = \
								(FIRST_BTN_LONG_PRESSED_VALIED_TIME_MS + FACTORY_TEST_POLL_TTME_1S)/ FACTORY_TEST_POLL_TTME_1S;
						g_u32Count = FACTORY_TEST_POLL_TIMES;//��㴦���Զ��庯��
						//��¼��ǰ���ϻ�״̬
						g_eLastFactoryTestStatus = g_eFactoryTestStatus;
						g_eFactoryTestStatus = FTS_FIRST_LONG_PRESSED;

						iKonkeAfSelfPrint("######2222222222222222222222 == 0\r\n");
					}
				}
			}else {
				//g_stFactoryTestControllerList[index].elastBtnAction = g_stFactoryTestControllerList[index].eCurrentBtnAction;
			}



			//�ڶ��γ�����Чʱ�䵹��ʱ
			if (g_stFactoryTestControllerList[index].u32FirstBtnLongPressedCountDown > 0 && g_stFactoryTestControllerList[index].u8BtnLongPressTimes == LONG_PRESS_2_TIMES){
				if (--g_stFactoryTestControllerList[index].u32FirstBtnLongPressedCountDown == 0){
					g_stFactoryTestControllerList[index].bIsFirstBtnLongPressed = false;
					//�ָ�֮ǰ�ϻ�״̬
					g_eFactoryTestStatus = g_eLastFactoryTestStatus;
				}
			}
		}
	}

	//10Sִ��һ���Զ����ϻ�,��ʱ����ݲ�ͬ״̬����¼���ʱ�䣬�ø���ִ��
	if (g_u32Count++ >= FACTORY_TEST_POLL_TIMES){
		g_u32Count = 0;
		//�û��Զ����ϻ����Է���
		if (g_pFactoryTestPollCallback){
			g_pFactoryTestPollCallback(g_eFactoryTestStatus);
		}

		if (g_eFactoryTestStatus == FTS_START){
			g_eFactoryTestStatus = FTS_AGING_15MIN;
		}else if (g_eFactoryTestStatus == FTS_AGING_DONE){
			//�ϻ������˳���ǰ�¼�
			return;
		}
	}

	emberEventControlSetDelayMS(kFactoryTestEventControl, FACTORY_TEST_POLL_TTME_1S);
}


// Reverse the bits in a byte
static uint8_t reverse(uint8_t b)
{
#if defined(EZSP_HOST) || defined(BOARD_SIMULATION)
  return ((b * 0x0802UL & 0x22110UL) | (b * 0x8020UL & 0x88440UL)) * 0x10101UL >> 16;
#else
  return (__RBIT((uint32_t)b) >> 24); // Cortex function __RBIT uses uint32_t
#endif // EZSP_HOST
}

void kkFactoryMsgInComingCallBack(UMsgNodeSt *pMsgNode)
{
	uint16_t length;
	uint8_t control_field;
//	uint16_t seq;
	DataField_st data;
	DataField_st send_buf;

	if( NULL == pMsgNode ) {
		return ;
	}

#if 1 // Just for debug
	iKonkeAfSelfPrint("\r\n-- MSG INCOMING[%d]:\r\n", pMsgNode->length);
//	iKonkeAfSelfPrintBuffer(pMsgNode->buffer, pMsgNode->length, true);
	iKonkeAfSelfPrint("\r\n--------------------\r\n");
#endif

	// check for parsable packets
	if( pMsgNode->length < 5 /* Minimum Packet Length */ ) {
		iKonkeAfSelfPrint("Err: Unparsable Packets!!\r\n");
		return ;
	}

	length = ((uint16_t)pMsgNode->buffer[2] << 8) |  (uint16_t)(pMsgNode->buffer[3]);
	control_field = pMsgNode->buffer[4];

	data.seq = ((uint16_t)pMsgNode->buffer[5] << 8) |  (uint16_t)(pMsgNode->buffer[6]);

	data.u8Datalen = pMsgNode->buffer[7];
	data.u8ChannelID = pMsgNode->buffer[8];
	data.u16Opcode = ((uint16_t)pMsgNode->buffer[9] << 8) |  (uint16_t)(pMsgNode->buffer[10]);
	memcpy(data.u8ARG,&pMsgNode->buffer[11], data.u8Datalen - 3);

	send_buf.seq = data.seq;
	send_buf.u8ChannelID = data.u8ChannelID;
	send_buf.u16Opcode = data.u16Opcode;
	switch(data.u16Opcode){
		case(0x0000):
			send_buf.u8ARG[0] = ERR_OK;
			send_buf.u8Datalen = 4;
			kCmdMsgDataSent(Z_TO_H_NO_ACK, send_buf, false);
			break;

		case(0x0001):
			tokTypeMfgInstallationCode custom_install_code;
			uint16_t crc = 0xFFFF;
			halCommonGetToken(&custom_install_code, TOKEN_MFG_CUSTOM_INSTALL_CODE);
			if (!(custom_install_code.flags & 0x0001)){
				iKonkeAfSelfPrint("333333333333333333333333 flg = %2x\r\n", custom_install_code.flags);
				send_buf.u8ARG[0] = ERR_EXIST;
				send_buf.u8ARG[1] = data.u8ARG[0];
				memcpy(&send_buf.u8ARG[2],&data.u8ARG[1],data.u8ARG[0]);
				send_buf.u8Datalen = data.u8ARG[0] + 5;
				kCmdMsgDataSent(Z_TO_H_NO_ACK, send_buf, false);
				break;
			}

			//check install code length 20200725
			uint8_t length = data.u8ARG[0];
			if (length != 6 && length != 8 && length != 12 && length != 16){
				iKonkeAfSelfPrint("4444444444444444 flg = %2x\r\n", custom_install_code.flags);
				send_buf.u8ARG[0] = ERR_FORMAT;
				send_buf.u8ARG[1] = data.u8ARG[0];
				memcpy(&send_buf.u8ARG[2],&data.u8ARG[1],data.u8ARG[0]);
				send_buf.u8Datalen = data.u8ARG[0] + 5;
				kCmdMsgDataSent(Z_TO_H_NO_ACK, send_buf, false);
				break;
			}

			custom_install_code.flags = 0x02;
			memcpy(custom_install_code.value,&data.u8ARG[1],data.u8ARG[0]);
			for (uint8_t index = 0; index < data.u8ARG[0]; index++) {
				crc = halCommonCrc16(reverse(custom_install_code.value[index]), crc);
			}
			halCommonSetToken(TOKEN_MFG_CUSTOM_INSTALL_CODE, &custom_install_code);
//			halCommonSetToken(TOKEN_MFG_INSTALLATION_CODE,&custom_install_code);
			//write installationcode
			kOptTunnelTrytoWriteInstallCode();

			iKonkeAfSelfPrint("11111111111111111111\r\n");
			send_buf.u8ARG[0] = ERR_OK;
			send_buf.u8ARG[1] = data.u8ARG[0];
			memcpy(&send_buf.u8ARG[2],&data.u8ARG[1],data.u8ARG[0]);
			iKonkeAfSelfPrint("22222222222222222222222\r\n");
			send_buf.u8Datalen = data.u8ARG[0] + 5;
			kCmdMsgDataSent(Z_TO_H_NO_ACK, send_buf, false);
			break;

		case(0x0002):
    	    uint8_t install_code_len = 0;
    	    tokTypeMfgInstallationCode install_code;

    	    kGetMfgTokenUserData(4466, &install_code, sizeof(tokTypeMfgInstallationCode));
    	    //halCommonGetToken(&install_code, TOKEN_MFG_INSTALLATION_CODE);
    	    send_buf.u8ARG[0] = ERR_OK;
    	    if (install_code.flags & 0x0001){
    	    	iKonkeAfSelfPrint("3456789876\r\n");
    	    	install_code_len = 0;
    	    	send_buf.u8ARG[0] = ERR_NOT_EXIST;
    	    }else{
    	    	uint8_t data = (install_code.flags >> 1);
    	    	switch (data)
    	    	{
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
	    	send_buf.u8ARG[install_code_len + 2] = (uint8_t)(install_code.crc >> 8);
	    	send_buf.u8ARG[install_code_len + 3] = (uint8_t)(install_code.crc);
	    	send_buf.u8Datalen = install_code_len + 7;
	    	kCmdMsgDataSent(Z_TO_H_NO_ACK, send_buf, false);
			break;
		case(0xED01):
			send_buf.u8ARG[0] = ERR_OK;
			EmberEUI64  deviceEui;
			MEMMOVE(deviceEui, emberGetEui64(), EUI64_SIZE);
			send_buf.u8ARG[1] = deviceEui[7];
			send_buf.u8ARG[2] = deviceEui[6];
			send_buf.u8ARG[3] = deviceEui[5];
			send_buf.u8ARG[4] = deviceEui[4];
			send_buf.u8ARG[5] = deviceEui[3];
			send_buf.u8ARG[6] = deviceEui[2];
			send_buf.u8ARG[7] = deviceEui[1];
			send_buf.u8ARG[8] = deviceEui[0];
			send_buf.u8Datalen = 12;
			kCmdMsgDataSent(Z_TO_H_NO_ACK, send_buf, false);
			break;

		case(0xED02):
			send_buf.u8ARG[0] = ERR_OK;
			send_buf.u8Datalen = 4;
			kCmdMsgDataSent(Z_TO_H_NO_ACK, send_buf, false);

			LED_OPT_OFF(g_u8FactoryTestLedId);
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
			//�豸����
			kkEnterSleep(true, true);
#endif
			extern EmberEventControl ikkUartRecvLoopCheckEventControl;
			emberEventControlSetInactive(ikkUartRecvLoopCheckEventControl);
			emberEventControlSetInactive(kFactoryTestEventControl);
			break;
#if 0
		case(0xED03):
			int16_t temperature = 0;
			uint16_t humidity = 0;
			uint16_t illuminance = 0;
			uint16_t batteryVoltage = 0;
			SHT3X_GetTempAndHumi(&temperature, &humidity, REPEATAB_HIGH, MODE_CLKSTRETCH);
			TSL2572_GetLight(0, &illuminance);
			batteryVoltage = kBatteryMonitorReadVotage(false, false, BATTERY_MAX_VOLTAGE_MV, 0);
			send_buf.u8ARG[0] = 0x00;
			send_buf.u8ARG[1] = (uint8_t)(temperature >> 8);
			send_buf.u8ARG[2] = (uint8_t)(temperature);
			send_buf.u8ARG[3] = (uint8_t)(humidity >> 8);
			send_buf.u8ARG[4] = (uint8_t)(humidity);
			send_buf.u8ARG[5] = (uint8_t)(illuminance >> 8);
			send_buf.u8ARG[6] = (uint8_t)(illuminance);
			send_buf.u8ARG[7] = (uint8_t)(batteryVoltage >> 8);
			send_buf.u8ARG[8] = (uint8_t)(batteryVoltage);
			send_buf.u8Datalen = 12;
			kkUartMsgSent(&send_buf);
			break;
#endif
		case(0xED04):
			uint8_t token_data = 0;
			switch(data.u8ARG[0]){
				case 0x01:
					token_data = SINGLE_BOARD_TEST_DONE_VALUE;
					halCommonSetToken(TOKEN_SINGLE_BOARD_TEST_FLG,&token_data);
					halCommonGetToken(&token_data,TOKEN_SINGLE_BOARD_TEST_FLG);
					if (token_data == SINGLE_BOARD_TEST_DONE_VALUE){
						send_buf.u8ARG[0] = ERR_OK;
						g_eFactoryTestStatus = FTS_SINGLE_BOARD_TEST_DONE;
						//���������һֱ����
						kLedOptTrigger(g_u8FactoryTestLedId, LED_FAST_BLINK_ON_TIME_MS, LED_FAST_BLINK_OFF_TIME_MS, BLINK_DEAD, LED_ON, LED_ON);
					}else{
						send_buf.u8ARG[0] = ERR_EXIST;
					}
					break;
				case 0x02:
					token_data = FULL_DEVICE_TEST_DONE_VALUE;
					halCommonSetToken(TOKEN_FULL_DEVICE_TEST_FLG,&token_data);
					halCommonGetToken(&token_data,TOKEN_FULL_DEVICE_TEST_FLG);
					if (token_data == FULL_DEVICE_TEST_DONE_VALUE){
						send_buf.u8ARG[0] = ERR_OK;
						g_eFactoryTestStatus = FTS_FULL_DEVICE_TEST_DONE;
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
						g_eFactoryTestStatus = FTS_AGING_DONE;
					}else{
						send_buf.u8ARG[0] = ERR_EXIST;
					}
					break;
				default:
					send_buf.u8ARG[0] = ERR_FORMAT;
					break;
			}
			send_buf.u8ARG[1] = data.u8ARG[0];
			send_buf.u8Datalen = 5;
			kCmdMsgDataSent(Z_TO_H_NO_ACK, send_buf, false);
			break;

		case(0xED05):
			send_buf.u8ARG[0] = ERR_OK;
			send_buf.u8ARG[1] = kGetFactoryTestValueStatus();
			send_buf.u8Datalen = 5;
			kCmdMsgDataSent(Z_TO_H_NO_ACK, send_buf, false);
			break;

		default:
			break;
	}
}


/** @brief Basic Cluster Server Attribute Changed
 *
 * Server Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 */
/*
void emberAfBasicClusterServerAttributeChangedCallback(int8u endpoint,
                                                       EmberAfAttributeId attributeId)
{
#if 0
  uint8_t value[3] = {0};
  if (attributeId == ZCL_MANUFACTURER_NAME_ATTRIBUTE_ID){
    emberAfReadAttribute( endpoint,
                ZCL_BASIC_CLUSTER_ID,
                ZCL_MANUFACTURER_NAME_ATTRIBUTE_ID,
                CLUSTER_MASK_SERVER,
                (uint8_t *)&value,
                sizeof(value),
                NULL);
    iKonkeAfSelfPrint("emberAfBasicClusterServerAttributeChangedCallback:[0x%x],[0x%x],[0x%x] \r\n",
              value[0],value[1],value[2]);
    if (value[0] == 0xF1 && value[1] == 0xF1 && value[2] == 0x00){
      kSetFullDeviceValue(FULL_DEVICE_TEST_DONE_VALUE);
      g_eFactoryTestStatus = FTS_FULL_DEVICE_TEST_DONE;
      kNwkFactoryReset(true);
    }
  }
#endif
}
*/

void kClusterExitFullDeviceTestCallback(EmberAfClusterCommand* cmd)
{
	uint8_t payload[32] = {0};
	memcpy(payload, cmd->buffer + cmd->payloadStartIndex + 3 + 1, 32);
	//if (attributeId == ZCL_MANUFACTURER_NAME_ATTRIBUTE_ID){
		iKonkeAfSelfPrint("emberAfBasicClusterServerAttributeChangedCallback:[0x%x],[0x%x],[0x%x] \r\n",
				payload[0],payload[1],payload[2]);
		if (payload[0] == 0xF1 && payload[1] == 0xF1 && payload[2] == 0x00){
			kSetFullDeviceValue(FULL_DEVICE_TEST_DONE_VALUE);
			g_eFactoryTestStatus = FTS_FULL_DEVICE_TEST_DONE;
			 emberAfSendDefaultResponse(cmd, EMBER_ZCL_STATUS_SUCCESS);
			 halCommonDelayMicroseconds(60000);
			 kNwkFactoryReset(true);
		}else {
			 emberAfSendDefaultResponse(cmd, EMBER_ZCL_STATUS_INVALID_VALUE);
		}
	//}
}


