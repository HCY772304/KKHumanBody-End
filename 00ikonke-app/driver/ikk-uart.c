#include "ikk-uart.h"

#include "plugin/serial/com_config.h"
#include "../general/ikk-debug.h"


// Serial Data Sending Time Cycle Period
#define UART_SENTLOOP_INTV				10	// unit:ms
// Serial Data retry Sending Time Cycle Period
#define UART_SENTRETRY_INTERVAL			500	// unit:ms
// Serial Data Receiving Time Cycle Period
#define UART_RECVLOOP_INTV				10	// unit:ms
// Waitting times for Serial Receiving Data
#define UART_RECV_TIMEOUT				3000// unit:ms
#define UART_MAX_RECV_INTERVAL_TIMEOUT	(UART_SENTLOOP_INTV * 2)//ms

// uart message recv queue
UMsgQueueSt g_uMsgRecvQueue = { 0 }, g_uMsgSentQueue = { 0 };
// uart message sent queue
//UMsgQueueSt g_uMsgSentQueue = { 0 };

// uart manager instance
UartManagerSt g_uManagerInsc  = {.port = COM_UNKNOW};

EmberEventControl ikkUartSentLoopCheckEventControl;
EmberEventControl ikkUartRecvLoopCheckEventControl;
EmberEventControl ikkUartMsgDispatchEventControl;

void ikkUartRecvLoopCheck(UMsgNodeSt *pNode);

void ikkUartRecvLoopCheckEventHandler(void );

/*	DESP: serial usart driver init interface.
 *	PARAM[recv_header]: Recognition Sequence of recv effective data frame header and detection of Data Frame start.
 *	PARAM[recv_header_n]: the length of recv data frame header.
 *	PARAM[pfunc_crc]: CRC function.
 *	PARAM[pfunc_incoming]: Callback function when valid data frame is detected, provided by youself.
 *	PARAM[bSleepSupport]: support for sleepy device function.
 * 	Auth: dingmz_frc.20190620.
 * */
kk_err_t kUartModuleInit(COM_Port_t port, uint32_t rate, USART_Parity_TypeDef parity, uint8_t stopBits
		, uint8_t recv_header[], uint8_t recv_header_n
		, PFUNC_CRC pfunc_crc, PFUNC_INCOMING_CALLBACK pfunc_incoming
		, bool bSleepSupport)
{
	if((NULL == pfunc_crc) || (NULL == pfunc_incoming)) {
		return KET_ERR_INVALID_PARAM;
	}

	if( recv_header_n > sizeof(g_uManagerInsc.recv_header)) {
		return KET_ERR_INVALID_PARAM;
	}

	kk_err_t err = KET_OK;

	g_uManagerInsc.port = port;
	if (recv_header && recv_header_n) {
		memcpy(g_uManagerInsc.recv_header, recv_header, recv_header_n);
	}
	g_uManagerInsc.recv_header_n = recv_header_n;
	g_uManagerInsc.pfunc_crc = pfunc_crc;
	g_uManagerInsc.pfunc_incoming = pfunc_incoming;
	g_uManagerInsc.bSleepSupport = bSleepSupport;
	g_uManagerInsc.u16SleepCounter = UART_RECV_TIMEOUT / UART_RECVLOOP_INTV;

	// setup recv loop check event.
	emberEventControlSetActive(ikkUartRecvLoopCheckEventControl);
	// setup recv loop check event.
	emberEventControlSetActive(ikkUartMsgDispatchEventControl);

#if 0
	COM_Init_t initdata;

	switch (port) {
	    case UCOM_USART0:
	      initdata = (COM_Init_t) COM_USART0_DEFAULT;
	      break;
	    case UCOM_USART1:
	      initdata = (COM_Init_t) COM_USART1_DEFAULT;
	      break;
#if 0
	    case UCOM_USART2:
	      initdata = (COM_Init_t) COM_USART2_DEFAULT;
	      break;
#endif
	    default:
	      return KET_ERR_NON_EXIST;
	}

	initdata.uartdrvinit.uartinit.baudRate = rate;
	initdata.uartdrvinit.uartinit.parity = (USART_Parity_TypeDef)parity;

	if( stopBits == 1 ) {
		initdata.uartdrvinit.uartinit.stopBits = usartStopbits1;
	} else if ( stopBits == 2 ) {
		initdata.uartdrvinit.uartinit.stopBits = usartStopbits2;
	}

	if( COM_Init((COM_Port_t) port, &initdata) != EMBER_SUCCESS ) {
		err = KET_ERR_UNKNOW;
	}
#endif

	return err;
}

/*  DESP: serial usart message data sent interface.
    Auth: dingmz_frc.20190627.
*/
kk_err_t MsgSent(uint8_t *buffer, uint8_t length )
{
#if 1 // Just for debug
	iKonkeAfSelfPrint("\r\n-- MSG SENT[%d]:\r\n", length);
	iKonkeAfSelfPrintBuffer(buffer, length, true);
	iKonkeAfSelfPrint("\r\n-----------------\r\n");
#endif

	emberSerialWriteData(g_uManagerInsc.port, buffer, length);

	EmberStatus enResult = emberSerialWaitSend(g_uManagerInsc.port);

	return ((enResult == EMBER_SUCCESS)?KET_OK:KET_ERR_UNKNOW);
}

/*  DESP: message header sniffer.
    Auth: dingmz_frc.20190601.
*/
static bool MsgHeaderSniffer(uint8_t org_data )
{
	if( g_uManagerInsc.header_chunk_index < g_uManagerInsc.recv_header_n ) {
		g_uManagerInsc.header_chunk[g_uManagerInsc.header_chunk_index++] = org_data;
	}else { // shift...
		int index = 0;
		for( ; index < (g_uManagerInsc.recv_header_n - 1); ++index ) {
			g_uManagerInsc.header_chunk[index] = g_uManagerInsc.header_chunk[index + 1];
		}

		g_uManagerInsc.header_chunk[index] = org_data;
	}

	bool _bfound = false;

	if( g_uManagerInsc.header_chunk_index == g_uManagerInsc.recv_header_n ) {
		if( memcmp(g_uManagerInsc.header_chunk, g_uManagerInsc.recv_header, g_uManagerInsc.recv_header_n) == 0 ) {
			_bfound = true;
		}
	}

	return _bfound;
}

/*  DESP: pop msg from the head of buffer queue chunk.
    Auth: dingmz_frc.20190626.
*/
static UMsgNodeSt *PopMsg(QueueEm eQ )
{
	UMsgQueueSt *pQ = (eQ == EQU_RECV)?(&g_uMsgRecvQueue):(&g_uMsgSentQueue);

	return (pQ->counter?(&(pQ->node[0])):NULL);
}

/*  DESP: push msg to buffer queue chunk .
    Auth: dingmz_frc.20180626.
*/
static kk_err_t PushMsg(QueueEm eQ, UMsgNodeSt *pNode )
{
	if( NULL == pNode ) {
		return KET_ERR_INVALID_PARAM;
	}

	kk_err_t err = KET_OK;

	UMsgQueueSt *pQ = (eQ == EQU_RECV)?(&g_uMsgRecvQueue):(&g_uMsgSentQueue);

	if( pQ->counter < UMSG_NODE_MAX ) {
		memcpy(&pQ->node[pQ->counter], pNode, sizeof(UMsgNodeSt));
		pQ->counter++;

		if( eQ == EQU_RECV ) {
			// If received message, we need to ensure that msg dispatch polling events work!!!
			if( !emberEventControlGetActive(ikkUartMsgDispatchEventControl)) {
				emberEventControlSetActive(ikkUartMsgDispatchEventControl);
			}
		}else {
			// If sending queues, we need to ensure that sending polling events work!!!
			if( !emberEventControlGetActive(ikkUartSentLoopCheckEventControl)) {
				emberEventControlSetActive(ikkUartSentLoopCheckEventControl);
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
static kk_err_t RemoveMsg(QueueEm eQ, uint8_t index_pos )
{
	kk_err_t err = KET_OK;

	UMsgQueueSt *pQ = (eQ == EQU_RECV)?(&g_uMsgRecvQueue):(&g_uMsgSentQueue);

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

/*	DESP: serial usart msg sending loop check handler.
 * 	Auth: dingmz_frc.20190702.
 * */
void ikkUartSentLoopCheckEventHandler(void )
{
	emberEventControlSetInactive(ikkUartSentLoopCheckEventControl);
	static uint8_t u8SendRetryCount = 0;
	static uint16_t u16RecvTimeoutCounter = 0;
	static bool bIsSentBusy = false;
	static bool bIsWaitingAck = false;
	static bool bIsReadySend = false;
	static bool bIsReceivedOrTimeout = false;

	UMsgNodeSt *pMsgNode = PopMsg(EQU_SENT);
	if( pMsgNode) {
		//sent message non-block,wait ack
		if (pMsgNode->ack) {
			if (bIsSentBusy != true) {
				bIsSentBusy = true;
				u8SendRetryCount = pMsgNode->sent_try;
				bIsReadySend = true;
			}

			if (bIsReadySend == true) {
				bIsReadySend = false;
				emberEventControlSetInactive(ikkUartRecvLoopCheckEventControl);
				u16RecvTimeoutCounter = pMsgNode->wait_timeout / UART_SENTLOOP_INTV;
				//sent message
				pMsgNode->ack_length = 0;
				MsgSent(pMsgNode->buffer, pMsgNode->length);
				bIsWaitingAck = true;
			}

			if (bIsWaitingAck == true) {
				if (u16RecvTimeoutCounter--) {
					ikkUartRecvLoopCheck(pMsgNode);
					//received ack
					if( pMsgNode->ack_length > 0 ) {
						bIsReceivedOrTimeout = true;
					}
					emberEventControlSetDelayMS(ikkUartSentLoopCheckEventControl, UART_SENTLOOP_INTV);
				}else {
					//check retry count
					if (u8SendRetryCount > 0) {
						u8SendRetryCount--;
						bIsReadySend = true;
						emberEventControlSetDelayMS(ikkUartSentLoopCheckEventControl, UART_SENTLOOP_INTV);
					}else {
						bIsReceivedOrTimeout = true;
					}
				}
			}

			if (bIsReceivedOrTimeout == true) {
				bIsReceivedOrTimeout = false;
				bIsReadySend = false;
				bIsSentBusy = false;
				bIsWaitingAck = false;

				RemoveMsg(EQU_SENT, 0);
				emberEventControlSetActive(ikkUartRecvLoopCheckEventControl);
				emberEventControlSetDelayMS(ikkUartSentLoopCheckEventControl, UART_SENTLOOP_INTV);
			}
		}else {
			// check if the message is valid.
			if( pMsgNode->sent_try > 0 ) {
				MsgSent(pMsgNode->buffer, pMsgNode->length);
				--pMsgNode->sent_try;

				emberEventControlSetDelayMS(ikkUartSentLoopCheckEventControl, UART_SENTRETRY_INTERVAL);
			}
			// delete msg form head when invaild!
			else {

				bIsReceivedOrTimeout = false;
				bIsReadySend = false;
				bIsSentBusy = false;
				bIsWaitingAck = false;

				RemoveMsg(EQU_SENT, 0);
				emberEventControlSetDelayMS(ikkUartSentLoopCheckEventControl, UART_SENTLOOP_INTV);
			}
		}
	}else {
		bIsReceivedOrTimeout = false;
		bIsReadySend = false;
		bIsSentBusy = false;
		bIsWaitingAck = false;

		iKonkeAfSelfPrint("EQU_SENT empty, shutdown to sleep!\r\n");
	}
}

/*	DESP: serial usart msg receiving loop check handler.
 * 	Auth: dingmz_frc.20190626.
 * */

void ikkUartRecvLoopCheck(UMsgNodeSt *pNode)
{
#define BLOCK_INTERVAL_TIME_MS  2
  uint8_t data = 0;
  static UMsgNodeSt MsgNode = { 0 };
  static bool recving = false;
//  uint16_t  uiMs_Count  =(uint16_t)(waitto_ms / BLOCK_INTERVAL_TIME_MS) ;
  static uint64_t lastTime = 0;
  uint64_t currentTime = 0;
  // Check for the existence of receivable serial data, Non-blocking!!!
//  if(uiMs_Count == 0){
    if( g_uManagerInsc.port != COM_UNKNOW && emberSerialReadAvailable(g_uManagerInsc.port)) {
      while(emberSerialReadByte(g_uManagerInsc.port, &data) == EMBER_SUCCESS ) {
        //iKonkeAfSelfPrint(">>>>>>1111Recv Data %x\r\n", data);
        currentTime = halCommonGetInt64uMillisecondTick();
        //add by maozj 20200602
        //When the data header is detected, fix the problem that the data is too long to verify successfully
        if (currentTime - lastTime >= UART_MAX_RECV_INTERVAL_TIMEOUT){
          //iKonkeAfSelfPrint(">>>>>>>>>>1111New data package\r\n");
          recving = false;
        }
        lastTime = currentTime;

      // find the message header
        if( recving == false ) {
          //modify by maozj 20200908 avoid data header is not fixed,recv header length is 0
          if (g_uManagerInsc.recv_header_n == 0) {
            recving = true;
            MsgNode.buffer[0] = data;
            MsgNode.length = 1;
          }else if(g_uManagerInsc.recv_header_n > 0 && MsgHeaderSniffer(data) ) {
            recving = true;
            memcpy(MsgNode.buffer, g_uManagerInsc.recv_header, g_uManagerInsc.recv_header_n);
            MsgNode.length = g_uManagerInsc.recv_header_n;
          }
        }else {
          if( MsgNode.length < UMSG_NODE_SIZE ) {
            MsgNode.buffer[MsgNode.length++] = data;

            // push to msg queue when crc succeed.or set ack length when get right ack.
            if( (MsgNode.length > 6) &&(g_uManagerInsc.pfunc_crc(MsgNode.buffer, MsgNode.length-1)) ) {
              recving = false;
              // process;
              if((pNode) && (memcmp(pNode->buffer + pNode->matcher_offset, pNode->matcher, pNode->matcher_n) == 0)) {
                if( pNode->ack ) {
                  memcpy(pNode->ack, MsgNode.buffer, MsgNode.length);
                  pNode->ack_length = MsgNode.length;

                  iKonkeAfSelfPrint(">>>>>>>>>>1Get Ack Data,length= %0x\r\n", MsgNode.length);
                }
              }
              UartMsgProcessHandle(&MsgNode);
              //PushMsg(EQU_RECV, &MsgNode);
            }
          }else {
            recving = false;
          }
        }
      }
    }
//  }
#if 0
  else{
    iKonkeAfSelfPrint("enter ...%d\r\n", uiMs_Count);
    while(uiMs_Count){
      uiMs_Count--;
      halCommonDelayMicroseconds(BLOCK_INTERVAL_TIME_MS * 1000);          //delay 2ms
      halInternalResetWatchDog();
      if(emberSerialReadAvailable(g_uManagerInsc.port)){
        while(emberSerialReadByte(g_uManagerInsc.port, &data) == EMBER_SUCCESS ) {
          iKonkeAfSelfPrint(">>>>>>2222Recv Data %x\r\n", data);
          currentTime = halCommonGetInt64uMillisecondTick();
          //add by maozj 20200602
          //When the data header is detected, fix the problem that the data is too long to verify successfully
          if (currentTime - lastTime >=  UART_MAX_RECV_INTERVAL_TIMEOUT){
            iKonkeAfSelfPrint(">>>>>>>>>>2222New data package\r\n");
            recving = false;
          }
          lastTime = currentTime;

        // find the message header
          if( recving == false ) {
            if( MsgHeaderSniffer(data) ) {
              recving = true;
              memcpy(MsgNode.buffer, g_uManagerInsc.recv_header, g_uManagerInsc.recv_header_n);
              MsgNode.length = g_uManagerInsc.recv_header_n;
            }
          }else {
            if( MsgNode.length < UMSG_NODE_SIZE ) {
              MsgNode.buffer[MsgNode.length++] = data;

              // push to msg queue when crc succeed.
              if( g_uManagerInsc.pfunc_crc(MsgNode.buffer, MsgNode.length)) {
                iKonkeAfSelfPrint("\r\n##UART Data CRC(0x%x%x) CHECK OK, macth[0](0x%x)[1](0x%x) buffer[%d](0x%x)[%d](0x%x) \r\n", \
                    MsgNode.buffer[MsgNode.length - 2], MsgNode.buffer[MsgNode.length - 1], pNode->matcher[0], pNode->matcher[1], pNode->matcher_offset, MsgNode.buffer[pNode->matcher_offset], pNode->matcher_offset+1, MsgNode.buffer[pNode->matcher_offset+1] );
                recving = false;
                // process;
                if((pNode) && (memcmp(&MsgNode.buffer[pNode->matcher_offset], pNode->matcher, pNode->matcher_n) == 0)) {
                  if( pNode->ack ) {
                    uiMs_Count = 0;
                    memcpy(pNode->ack, MsgNode.buffer, MsgNode.length);
                    pNode->ack_length = MsgNode.length;
                    iKonkeAfSelfPrint(">>>>>>>>>>Get Ack Data,length= %0x\r\n", MsgNode.length);
                    emberAfCorePrintBuffer(pNode->ack, pNode->ack_length, true);
                    iKonkeAfSelfPrint("-------------end\r\n");
                    break ;
                  }
                }else {
                  PushMsg(EQU_RECV, &MsgNode);
                }
              }
            }else {
              recving = false;
            }
          }
        }
      }
    }
  }
#endif
}

void ikkUartRecvLoopCheckEventHandler(void)
{
	emberEventControlSetInactive(ikkUartRecvLoopCheckEventControl);
	ikkUartRecvLoopCheck(NULL);

	bool _b_check_goingon = true;

	if( g_uManagerInsc.bSleepSupport ) {
		if( --g_uManagerInsc.u16SleepCounter == 0 ) {
			_b_check_goingon = false;
		}else if (g_uManagerInsc.u16SleepCounter > (UART_RECV_TIMEOUT / UART_RECVLOOP_INTV)){
			g_uManagerInsc.u16SleepCounter = UART_RECV_TIMEOUT / UART_RECVLOOP_INTV;
		}
	}

	if( _b_check_goingon ) {
		//iKonkeAfSelfPrint("uart recv loop: %d\r\n", UART_RECVLOOP_INTV);
		emberEventControlSetDelayMS(ikkUartRecvLoopCheckEventControl, UART_RECVLOOP_INTV);
	}else {
		iKonkeAfSelfPrint("==================== uart recv loop over, to sleep =======================\r\n");
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
		emberAfForceEndDeviceToStayAwake(false);
		emberAfSetDefaultSleepControlCallback(EMBER_AF_OK_TO_SLEEP);
		emberAfAddToCurrentAppTasks(EMBER_AF_FORCE_SHORT_POLL);
#endif
	}
}
/*	DESP: serial usart receive message dispatch handler.
 * 	Auth: dingmz_frc.20190626.
 * */
void ikkUartMsgDispatchEventHandler(void )
{
	emberEventControlSetInactive(ikkUartMsgDispatchEventControl);

	UMsgNodeSt *pMsgNode = PopMsg(EQU_RECV), *pMsgSending = PopMsg(EQU_SENT);
	if( pMsgNode ) {
		bool bDispatchEnable = true;
		// reset the recving timeout counter.
		g_uManagerInsc.u16SleepCounter = UART_RECV_TIMEOUT / UART_RECVLOOP_INTV;

		// Does the matching message send a response for the specified message?
		if( pMsgSending ) {
			iKonkeAfSelfPrint("match: org(%X:%X), des(%X:%X)\r\n"
				, pMsgNode->buffer[pMsgSending->matcher_offset], pMsgNode->buffer[pMsgSending->matcher_offset + 1]
				, pMsgSending->matcher[0], pMsgSending->matcher[1]);

			if( memcmp(pMsgNode->buffer + pMsgSending->matcher_offset, pMsgSending->matcher, pMsgSending->matcher_n) == 0 ) {
				pMsgSending->sent_try = 0;
				emberEventControlSetActive(ikkUartSentLoopCheckEventControl);
			}
		}

		if( g_uManagerInsc.pfunc_incoming ) {
			g_uManagerInsc.pfunc_incoming(pMsgNode);
		}

		// Remove the Msg form head;
		RemoveMsg(EQU_RECV, 0);

		emberEventControlSetDelayMS(ikkUartMsgDispatchEventControl, UART_RECVLOOP_INTV);
	}else if( emberEventControlGetActive(ikkUartRecvLoopCheckEventControl)) {
		emberEventControlSetDelayMS(ikkUartMsgDispatchEventControl, UART_RECVLOOP_INTV);
	}
}

/*  DESP: serial usart message data sent interface.
 *  Note: Actively sending data requiring detection and retransmitting needs to join the sending queue,
 *  	But the ACK packet is sent only once.
    Auth: dingmz_frc.20190627.
*/
kk_err_t kUartMsgSent(UMsgNodeSt *pMsgNode ,int waitto_ms)
{
	kk_err_t err = KET_ERR_UNKNOW;
	if (g_uManagerInsc.port == COM_UNKNOW) {
		iKonkeAfSelfPrint("Err:COM not init\r\n");
		return KET_FAILED;
	}

	if( pMsgNode->ack ) {
		pMsgNode->wait_timeout = waitto_ms;
		err = PushMsg(EQU_SENT, pMsgNode);
	}else {
		if( pMsgNode->sent_try > 0 ) {
			pMsgNode->sent_try += 1; //add by maozj 20200828 must add 1, avoid sent_try is only sent once;
			err = PushMsg(EQU_SENT, pMsgNode);
		}else {
			err = MsgSent(pMsgNode->buffer, pMsgNode->length);
		}
	}

	return err;
}

/*	DESP: In sleep mode, this interface is needed to activate the Uart module when the system is awakened.
 * 	Auth: dingmz_frc.20191113.
 * */
void kUartActiveTrigger(void )
{
	//modify by maozj 20200908
	if( g_uManagerInsc.bSleepSupport && g_uManagerInsc.port != COM_UNKNOW) {
		g_uManagerInsc.u16SleepCounter = UART_RECV_TIMEOUT / UART_RECVLOOP_INTV;

#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
		emberAfForceEndDeviceToStayAwake(true);
		emberAfSetDefaultSleepControlCallback(EMBER_AF_STAY_AWAKE);
#endif

		if( !emberEventControlGetActive(ikkUartRecvLoopCheckEventControl) ) {
			// setup recv loop check event.
			emberEventControlSetActive(ikkUartRecvLoopCheckEventControl);
		}

		if( !emberEventControlGetActive(ikkUartMsgDispatchEventControl) ) {
			// setup recv loop check event.
			emberEventControlSetActive(ikkUartMsgDispatchEventControl);
		}
	}
}


