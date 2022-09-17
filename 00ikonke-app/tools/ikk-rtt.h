/*
 * ikk-comm-rtt.h
 *
 *  Created on: 2020Äê12ÔÂ8ÈÕ
 *      Author: K
 */

#ifndef __IKONKE_MODULE_RTT_H_______________________________
#define __IKONKE_MODULE_RTT_H_______________________________

#include "stdint.h"
#include "ctype.h"
#include "app/framework/include/af.h"
#include "../driver/ikk-uart.h"

// MSG BUFFER OF NODE MAXLENGTH
#define RTT_MSG_NODE_SIZE					128
// Maximum number of message nodes in the queue
#define MSG_NODE_MAX					2


typedef struct tag_rtt_message_queue {
	UMsgNodeSt node[MSG_NODE_MAX];
	uint8_t counter;
	uint8_t send_port;
}MsgQueueSt;

typedef enum {EQU_RTT_RECV = 0, EQU_RTT_SEND, EQU_ZIGBEE_SEND }MsgQueueEm;
//pLedActionDoneCallback
typedef void (*pRttMsgIncomingCallback)(UMsgNodeSt *pMsgNode );
typedef struct tag_rtt_manager_st {

	pRttMsgIncomingCallback pfunc_incoming;
	// support for sleepy device. when waking up, the serial port receives automatically,
	// and after receiving, it sleeps actively.
	bool bSleepSupport;
	// support for sleepy device. the countdown ends and goes to sleep!
	uint16_t u16SleepCounter;
}RttManagerSt;


void kRttModuleInit(pRttMsgIncomingCallback pfunc_incoming);
kk_err_t kRttMsgSent(UMsgNodeSt *pMsgNode ,int waitto_ms);
kk_err_t kQueueMsgSent(UMsgNodeSt *pMsgNode ,int waitto_ms, int port);
#endif /* 00IKONKE_APP_TOOLS_IKK_COMM_RTT_H_ */
