#ifndef __IKONKE_MODULE_UART_H_______________________________
#define __IKONKE_MODULE_UART_H_______________________________


#include "stdint.h"
#include "ctype.h"
#include "app/framework/include/af.h"
#include "../general/ikk-module-def.h"
#include "serial/com.h"

//add by maozj 20200908
#define COM_UNKNOW 						0xFF
// MSG BUFFER OF NODE MAXLENGTH
#define UMSG_NODE_SIZE					255
// Maximum number of message nodes in the queue
#define UMSG_NODE_MAX					10
// Maximum Cache Length of Message Receiver
#define UART_RECEIVER_CACHE_SIZE		128

#define UMSG_RECV_HEADER_MAX_SIZE		4

// ������������
typedef enum { EQU_RECV = 0, EQU_SENT }QueueEm;

// uart recv/send buffer node
typedef struct tag_uart_message_node {
	unsigned char buffer[UMSG_NODE_SIZE];
	unsigned char length;
	// just for sending, Used to match instruction responses. For example, according to the instruction opcode!
	unsigned char matcher[4];
	unsigned char matcher_offset;
	unsigned char matcher_n;
	// Attempt to send number of times, decreases to 0 when invalid!!!
	unsigned char sent_try;
	unsigned char *ack;
	unsigned char ack_length;
	int wait_timeout;  //ms
}UMsgNodeSt;

typedef struct tag_uart_message_queue {
	UMsgNodeSt node[UMSG_NODE_MAX];
	unsigned char counter;
}UMsgQueueSt;

typedef bool (*PFUNC_CRC)(uint8_t *pdat, int length );
typedef void (*PFUNC_INCOMING_CALLBACK)(UMsgNodeSt *pMsgNode );

typedef struct tag_uart_manager_st {
	COM_Port_t port;

	uint8_t recv_header[UMSG_RECV_HEADER_MAX_SIZE];
	uint8_t recv_header_n;
	// head detection assistant chunk!
	uint8_t header_chunk[4];
	uint8_t header_chunk_index;

	PFUNC_CRC pfunc_crc;
	PFUNC_INCOMING_CALLBACK pfunc_incoming;
	// support for sleepy device. when waking up, the serial port receives automatically,
	// and after receiving, it sleeps actively.
	bool bSleepSupport;
	// support for sleepy device. the countdown ends and goes to sleep!
	uint16_t u16SleepCounter;
}UartManagerSt;

#if 0
typedef enum {
	UCOM_USART0 = 1,
	UCOM_USART1 = 2,
	UCOM_USART2 = 3,
}UCOMPortEm;

typedef enum { // parity
  USART_PARITY_NONE = 0,
  USART_PARITY_EVEN,
  USART_PARITY_ODD,
}UParityTypeEm;
#endif


extern EmberEventControl ikkUartRecvLoopCheckEvent;
extern EmberEventControl ikkUartMsgDispatchEvent;


/*  DESP: serial usart message data sent interface.
    Auth: dingmz_frc.20190627.
*/
kk_err_t kUartMsgSent(UMsgNodeSt *pMsgNode ,int waitto_ms);
/*	DESP: In sleep mode, this interface is needed to activate the Uart module when the system is awakened.
 * 	Auth: dingmz_frc.20191113.
 * */
void kUartActiveTrigger(void );
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
		, bool bSleepSupport);

#endif

