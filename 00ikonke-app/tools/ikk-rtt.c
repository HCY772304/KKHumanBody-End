/*
 * ikk-comm-rtt.c
 *
 *  Created on: 2020��12��8��
 *      Author: K
 */
#include "ikk-rtt.h"
#include "SEGGER_RTT.h"
#include "stdint.h"
#include "stdbool.h"
#include "stack/include/ember-types.h"
#include "app/framework/include/af.h"
#include "../general/ikk-debug.h"
#include "../general/ikk-module-def.h"
#include "../general/ikk-opt-tunnel.h"

// RTT Data Receiving Time Cycle Period
#define RTT_RECVLOOP_INTV				10	// unit:ms
#define RTT_SENDLOOP_INTV				10	// unit:ms
#define ZIGBEE_SENDLOOP_INTV			10	// unit:ms
//message  queue
MsgQueueSt g_RttMsgRecvQueue = { 0 }, g_RttZigbeeSentQueue = { 0 };

// uart manager instance
RttManagerSt g_RttManagerInsc;

EmberEventControl ikkRttSentLoopCheckEventControl;
EmberEventControl ikkRttRecvLoopCheckEventControl;
EmberEventControl ikkRttMsgDispatchEventControl;

/*	DESP: segger rtt moudle init.
 * 	Auth: kim.20201208.
 * */
uint8_t rttUpBuffer[SEGGER_RTT_CMD_BUFFER_SIZE] = { 0 };
uint8_t rttDownBuffer[SEGGER_RTT_CMD_BUFFER_SIZE] = { 0 };
void kRttModuleInit(pRttMsgIncomingCallback pfunc_incoming)
{

	SEGGER_RTT_Init();

	SEGGER_RTT_ConfigUpBuffer(1, "CH_COMMAND", rttUpBuffer, SEGGER_RTT_CMD_BUFFER_SIZE, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
	SEGGER_RTT_ConfigDownBuffer(1, "CH_COMMAND", rttDownBuffer, SEGGER_RTT_CMD_BUFFER_SIZE, SEGGER_RTT_MODE_NO_BLOCK_SKIP);

	g_RttManagerInsc.pfunc_incoming = pfunc_incoming;
	emberEventControlSetDelayMS(ikkRttRecvLoopCheckEventControl, RTT_RECVLOOP_INTV);

}


/*  DESP: pop msg from the head of buffer queue chunk.
    Auth: dingmz_frc.20190626.
*/
static UMsgNodeSt *PopMsg(MsgQueueEm eQ )
{
	MsgQueueSt *pQ;
	if (eQ == EQU_RTT_RECV) {
		pQ = (MsgQueueSt *)&g_RttMsgRecvQueue;
	} else if (eQ == EQU_RTT_SEND ) {
		return NULL;
	} else if (eQ == EQU_ZIGBEE_SEND) {
		pQ = (MsgQueueSt *)&g_RttZigbeeSentQueue;
	}

	return (pQ->counter?(&(pQ->node[0])):NULL);
}


/*  DESP: push msg to buffer queue chunk .
    Auth: dingmz_frc.20180626.
*/
static kk_err_t PushMsg(MsgQueueEm eQ, UMsgNodeSt *pNode )
{
	if( NULL == pNode ) {
		return KET_ERR_INVALID_PARAM;
	}

	kk_err_t err = KET_OK;

	MsgQueueSt *pQ;
	if (eQ == EQU_RTT_RECV) {
		pQ = (MsgQueueSt *)&g_RttMsgRecvQueue;
	} else if (eQ == EQU_RTT_SEND ) {
		return NULL;
	} else if (eQ == EQU_ZIGBEE_SEND) {
		pQ = (MsgQueueSt *)&g_RttZigbeeSentQueue;
	}

	if( pQ->counter < MSG_NODE_MAX ) {
		memcpy(&pQ->node[pQ->counter], pNode, sizeof(UMsgNodeSt));
		pQ->counter++;

		if( eQ == EQU_RTT_RECV ) {
			// If received message, we need to ensure that msg dispatch polling events work!!!
			if( !emberEventControlGetActive(ikkRttMsgDispatchEventControl)) {
				emberEventControlSetActive(ikkRttMsgDispatchEventControl);
			}
		}else {
			// If sending queues, we need to ensure that sending polling events work!!!
			if( !emberEventControlGetActive(ikkRttSentLoopCheckEventControl)) {
				iKonkeAfSelfPrint("\r\nikkRttSentLoopCheckEventControl\r\n");
				emberEventControlSetDelayMS(ikkRttSentLoopCheckEventControl, RTT_SENDLOOP_INTV);
			}
		}
	}else {
		err = KET_ERR_NO_PERMISSIONS;
	}

	return err;
}


/*	DESP: Remove Msg from Specified position[index base 0].
	Auth: dingmz_frc.20180430.
*/
static kk_err_t RemoveMsg(MsgQueueEm eQ, uint8_t index_pos )
{
	kk_err_t err = KET_OK;

	MsgQueueSt *pQ;

	if (eQ == EQU_RTT_RECV) {
		pQ = (MsgQueueSt *)&g_RttMsgRecvQueue;
	} else if (eQ == EQU_RTT_SEND ) {
		return NULL;
	} else if (eQ == EQU_ZIGBEE_SEND) {
		pQ = (MsgQueueSt *)&g_RttZigbeeSentQueue;
	}

	if( index_pos < pQ->counter ) {
		for(int index = index_pos; index < (pQ->counter - 1); ++index ) {
			memcpy(&(pQ->node[index]), &(pQ->node[index + 1]), sizeof(UMsgNodeSt));
		}

		pQ->counter--;
	}else {
		err = KET_ERR_NON_EXIST;
	}

	return err;
}


/*	DESP: segger rtt msg receiving loop check handler.
 * 	Auth: kim.20201208.
 * */
void ikkRttRecvLoopCheck(void *pNode)
{
	UMsgNodeSt MsgNode = {0};
	uint8_t rxLen = 0;

	while(SEGGER_RTT_HasData(0) != 0) { //Data in buffer
		iKonkeAfSelfPrint("\r\nSEGGER_RTT_HasData 000\r\n");
		rxLen = SEGGER_RTT_Read(0, MsgNode.buffer, 255);
		MsgNode.length = rxLen;
		PushMsg(EQU_RTT_RECV, &MsgNode);
	}

	while(SEGGER_RTT_HasData(1) != 0) { //Data in buffer
		iKonkeAfSelfPrint("\r\nSEGGER_RTT_HasData 111\r\n");
		rxLen = SEGGER_RTT_Read(1, MsgNode.buffer, 255);
		MsgNode.length = rxLen;
		PushMsg(EQU_RTT_RECV, &MsgNode);
	}
}


void ikkRttSentLoopCheckEventHandler(void)
{
	emberEventControlSetInactive(ikkRttSentLoopCheckEventControl);
	iKonkeAfSelfPrint("\r\nikkRttSentLoopCheckEventHandler\r\n");
	UMsgNodeSt *pMsgNode = NULL;

	pMsgNode = PopMsg(EQU_RTT_SEND);
	if(pMsgNode) {
		RemoveMsg(EQU_RTT_SEND, 0);
		emberEventControlSetDelayMS(ikkRttSentLoopCheckEventControl, RTT_SENDLOOP_INTV);
	}
}


void ikkRttRecvLoopCheckEventHandler(void)
{
	emberEventControlSetInactive(ikkRttRecvLoopCheckEventControl);
	ikkRttRecvLoopCheck(NULL);
	emberEventControlSetDelayMS(ikkRttRecvLoopCheckEventControl, 20);
}


/*  DESP: serial usart message data sent interface.
 *  Note: Actively sending data requiring detection and retransmitting needs to join the sending queue,
 *  	But the ACK packet is sent only once.
    Auth: dingmz_frc.20190627.
*/
kk_err_t kRttMsgSent(UMsgNodeSt *pMsgNode ,int waitto_ms)
{
	kk_err_t err = KET_ERR_UNKNOW;

#if 1 // Just for debug
	iKonkeAfSelfPrint("\r\n-- RTT MSG SEND(%d):\r\n", pMsgNode->length);

	for (uint8_t i = 0; i < pMsgNode->length; i++) {
		iKonkeAfSelfPrint("%02x ", pMsgNode->buffer[i]);
	}
	iKonkeAfSelfPrint("\r\n--------------------\r\n");
#endif

	SEGGER_RTT_Write(1, pMsgNode->buffer, pMsgNode->length);

	return err;
}


kk_err_t kQueueMsgSent(UMsgNodeSt *pMsgNode ,int waitto_ms, int port) //����jlink send ����,
{
	kk_err_t err = KET_ERR_UNKNOW;

	if (port == EQU_RTT_SEND) {
		err = PushMsg(EQU_RTT_SEND, pMsgNode);
	}

	return err;
}


/*	DESP:
 * 	Auth:
 * */
void ikkRttMsgDispatchEventHandler(void)
{
	emberEventControlSetInactive(ikkRttMsgDispatchEventControl);
	iKonkeAfSelfPrint("\r\nikkRttMsgDispatchEventHandler\r\n");
	UMsgNodeSt *pMsgNode = PopMsg(EQU_RTT_RECV);

	if (pMsgNode) {
		bool bDispatchEnable = true;
		// reset the recving timeout counter.
		//g_uManagerInsc.u16SleepCounter = UART_RECV_TIMEOUT / UART_RECVLOOP_INTV;

		if (g_RttManagerInsc.pfunc_incoming) {
			g_RttManagerInsc.pfunc_incoming(pMsgNode);
		}

		// Remove the Msg form head;
		RemoveMsg(EQU_RTT_RECV, 0);

		emberEventControlSetDelayMS(ikkRttMsgDispatchEventControl, RTT_RECVLOOP_INTV);
	}
}


/*	DESP:
 * 	Auth:
 * */
void ikkRttSleepMs(uint16_t ms)
{
	iKonkeAfSelfPrint("ikkRttSleepMs(%d)\r\n", ms);
	emberEventControlSetInactive(ikkRttRecvLoopCheckEventControl);
	emberEventControlSetDelayMS(ikkRttRecvLoopCheckEventControl, ms);
}
