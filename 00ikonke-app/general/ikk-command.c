#include "app/framework/include/af.h"
#include "app/util/zigbee-framework/zigbee-device-common.h"
#include "stdio.h"
#include "stdbool.h"

#include "ikk-debug.h"
#include "ikk-module-def.h"
#include "ikk-common-utils.h"
#include "ikk-opt-tunnel.h"
#include "ikk-command.h"
#include "ikk-network.h"
#include "../tools/ikk-rtt.h"


// command incoming global manager.
CommandIncomingMagSt g_stCmdIncomingMager = { 0 };
// command outgoing global manager.
CommandOutgoingMagSt g_stCmdOutgoingMager = { 0 };

// supported commands list
CommandDefSt g_stCommandsDef[] = STATIC_COMMAND_DEF;

// command images cache list for ack reply detection
CommandImageNodeSt g_stCmdImageList[CMDIMAGE_LIST_DEPTH] = { 0 };

EmberEventControl kCmdCommandImageHandleEventControl;

// link layer ack enable switch. when disable, the downlink command will no longer check errors and confirm
// , and the uplink command will no longer be re-send!!!
static bool g_bLinkAckEnableFlag = true;
static bool g_bLinkCRCVerifyEnableFlag = true;

// EXTERN FUNCTIONS DECLARE
void emberAfGetEui64(EmberEUI64 returnEui64);
EmberNodeId emberAfGetNodeId(void);
EmberStatus emberAfGetNetworkParameters(EmberNodeType* nodeType, EmberNetworkParameters* parameters);
EmberStatus emberAfPluginNetworkCreatorNetworkForm(bool centralizedNetwork, EmberPanId panId, int8_t radioTxPower, uint8_t channel);
void removeServer(uint8_t* ieeeAddress);
uint8_t emberAfGetChildTableSize(void);
void networkLeaveCommand(void);
void NWK_ClearRssi(void);
EmberStatus emberLeaveRequest(EmberNodeId target, EmberEUI64 deviceAddress, uint8_t leaveRequestFlags, EmberApsOption options);
//void emAfClearServers(void);
EmberStatus emberAfGetChildData(uint8_t index, EmberChildData *childData);
//EmberStatus emberAfPluginNetworkCreatorSecurityOpenNetwork(void);
//EmberStatus emberAfPluginNetworkCreatorSecurityCloseNetwork(void);
//EmberStatus emberAfPluginNetworkCreatorStart(bool centralizedNetwork);
void emAfApsFrameClusterIdSetup(uint16_t clusterId);
EmberStatus emberAfSendUnicast(EmberOutgoingMessageType type, uint16_t indexOrDestination, EmberApsFrame *apsFrame,
                               uint16_t messageLength, uint8_t *message);
bool emberAfOtaServerSendImageNotifyCallback(EmberNodeId dest, uint8_t endpoint, uint8_t payloadType, uint8_t queryJitter, const EmberAfOtaImageId* id);
//bool kkOtaImageBlockWrite(int offset_in, char *pdata );


// Obtain the next sequence number to be used in ZDP message, application layer!!!
#define ZDPAPP_NEXT_SEQ() ((emberGetLastAppZigDevRequestSequence() + 1) & 0X7F/*SAME TO APPLICATION_ZDO_SEQUENCE_MASK*/)
// Obtain the next sequence number to be used in ZDP message, stack layer!!!
#define ZDPSTK_NEXT_SEQ() ((emberGetLastStackZigDevRequestSequence() == 0XFF)?(0X80):(emberGetLastStackZigDevRequestSequence() + 1))
// Obtain the next sequence number to be used in ZCL message, application layer!!!
#define ZCLAPP_NEXT_SEQ() ((emberAfGetLastSequenceNumber() + 1) & 0X7F/*SAME TO EMBER_AF_ZCL_SEQUENCE_MASK*/)

uint16_t kCmdGetMsgCrc16Value( uint8_t* msg, uint8_t len)
{
   uint16_t crc = 0xFFFF;
   uint16_t i, j = 0;

    for (i = 0; i < len; i++)
    {
        for (j = 0; j < 8; j++)
        {
            //ǿ��ת�� ADD BY YWQ
            uint8_t c15 = (uint8_t)((crc >> 15 & 1) == 1);
            uint8_t bit = (uint8_t)((msg[i] >> (7 - j) & 1) == 1);

            crc <<= 1;

            if (c15 ^ bit)
            {
                crc ^= 0x1021;
            }
        }
    }

//    iKonkeAfSelfPrint("---create_crc16_data(0x%2x)----\r\n",crc);
//	iKonkeAfSelfPrint("--------------------\r\n");
    return crc;
}

// Reverse the bits in a byte
uint8_t reverse(uint8_t b)
{
#if defined(EZSP_HOST) || defined(BOARD_SIMULATION)
  return ((b * 0x0802UL & 0x22110UL) | (b * 0x8020UL & 0x88440UL)) * 0x10101UL >> 16;
#else
  return (__RBIT((uint32_t)b) >> 24); // Cortex function __RBIT uses uint32_t
#endif // EZSP_HOST
}

uint8_t kCmdMsgDataSentByPort(uint8_t u8Control_Field, DataField_st data, bool isGetResult, ComPortEm port)
{
  UMsgNodeSt uMsgNode;
  uint8_t length = 0;

  data.u8Datalen = data.u8Datalen +3;

  uMsgNode.buffer[length++] = 0xAA; //msg header
  uMsgNode.buffer[length++] = 0x55; //msg header

  uMsgNode.buffer[length++] = 0x00; //msg length
  uMsgNode.buffer[length++] = 0x00; //msg length
  uMsgNode.buffer[length++] = u8Control_Field;
//  seq++;
  uMsgNode.buffer[length++] = (uint8_t) (data.seq >> 8);
  uMsgNode.buffer[length++] = (uint8_t) data.seq;
  uMsgNode.buffer[length++] = data.u8Datalen;
  uMsgNode.buffer[length++] = data.u8ChannelID;
  uMsgNode.buffer[length++] = (uint8_t) (data.u16Opcode >> 8);
  uMsgNode.buffer[length++] = (uint8_t) data.u16Opcode;
  memcpy(&uMsgNode.buffer[length], data.u8ARG, data.u8Datalen - 3);
  length = length + data.u8Datalen - 3;
  uMsgNode.buffer[2] = (uint8_t) ((length - 4) >> 8);
  uMsgNode.buffer[3] = (uint8_t) (length - 4);
  uint16_t crc = kCmdGetMsgCrc16Value(&uMsgNode.buffer[4], length - 4);

  uMsgNode.buffer[length++] = (uint8_t) (crc >> 8);
  uMsgNode.buffer[length++] = (uint8_t) (crc);

  uMsgNode.length = length;

  // setting for matcher content.

  uMsgNode.matcher[0] = uMsgNode.buffer[2]; //seq
  uMsgNode.matcher[1] = uMsgNode.buffer[3];
  uMsgNode.matcher_offset = 2;
  uMsgNode.matcher_n = 2;
  //zigbee send to host ack
  if (u8Control_Field == Z_TO_H_NO_ACK) {
    uMsgNode.sent_try = 0;
  } else if (u8Control_Field == Z_TO_H_WITH_ACK) {
    //zigbee send to host notify or control
    uMsgNode.sent_try = 1;
  }

  if (isGetResult) {
    uMsgNode.ack = uMsgNode.buffer;
  } else {
    uMsgNode.ack = NULL;
    uMsgNode.ack_length = 0;
  }

  if (port == 0) {
    //return kUartMsgSent(&uMsgNode, 300);
  } else if (port == 1) {
    return kRttMsgSent(&uMsgNode, 300);
  } else if (port == 3) {
    //return kInterPanSend(&uMsgNode, 300);
  }

  return 0;
}



uint8_t kCmdGetMsgSum(uint8_t *pdat, int length )
{
	if( NULL == pdat ) return false;

	uint8_t crc_value = 0;

	for(int index = 0; index < length; ++index ) {
		crc_value += pdat[index];
	}

	return (crc_value);
}

bool kCmdCheckMsgCRC16(uint8_t *pdat, int length )
{
	if((NULL == pdat) || (length < 8)) {
		return false;
	}

//	iKonkeAfSelfPrint("-- CRC[%d]:(0x%0x)\r\n", length,pdat[length - 1]);

	return (kCmdGetMsgCrc16Value(pdat + 4, length - 6) == (uint16_t)(((uint16_t)pdat[length - 2] << 8) | (uint16_t)pdat[length - 1]));
}

/* 发送方测试代码 */
bool CheckSum(uint8_t *str, int str_length)
{
    uint8_t CheckSum_Value = 0;
    int i = 0;
    for(i = 0; i < str_length; i++)
    {
       CheckSum_Value += str[i];
    }
    CheckSum_Value = CheckSum_Value & 0xff;
    //return CheckSum_Value;
    if(str[str_length] == CheckSum_Value){
       return true;
    }else{
       return false;
    }
}


kk_err_t kCommandMsgSent(DataField_st *buffer)
{
	UMsgNodeSt uMsgNode;

//	static uint16_t seq = 0x00;
//	iKonkeAfSelfPrint("-------------- 11: %d -------\r\n", halCommonGetInt32uMillisecondTick());
	uint8_t length = 0;

	uMsgNode.buffer[length++]  = 0XAA;
	uMsgNode.buffer[length++]  = 0X55;

	uMsgNode.buffer[length++]  = 0x00;//���ݳ���
	uMsgNode.buffer[length++]  = 0x00;	//���ݳ���
	uMsgNode.buffer[length++]  = 0x20;
//	seq++;
	uMsgNode.buffer[length++]  = (uint8_t)(buffer->seq >> 8);
	uMsgNode.buffer[length++]  = (uint8_t)buffer->seq;
	uMsgNode.buffer[length++]  = buffer->u8Datalen;
	uMsgNode.buffer[length++]  = buffer->u8ChannelID;
	uMsgNode.buffer[length++]  = (uint8_t)(buffer->u16Opcode >> 8);
	uMsgNode.buffer[length++]  = (uint8_t)buffer->u16Opcode;
	memcpy(&uMsgNode.buffer[length],buffer->u8ARG,buffer->u8Datalen - 3);
	length = length + buffer->u8Datalen - 3;
	uMsgNode.buffer[2] = (uint8_t)((length - 4) >> 8);
	uMsgNode.buffer[3] = (uint8_t)(length - 4);
	uint16_t crc = kCmdGetMsgCrc16Value(&uMsgNode.buffer[4],length - 4);

	uMsgNode.buffer[length++] = (uint8_t)(crc >> 8);
	uMsgNode.buffer[length++] = (uint8_t)(crc);

	uMsgNode.length = length;

	// setting for matcher content.
//	uMsgNode.matcher[0] = cmd_type;
//	uMsgNode.matcher_offset = CMD_OFFSET;
//	uMsgNode.matcher_n = 1;
//	uMsgNode.sent_try = 0;
//	uMsgNode.bRspNeedtoDispatch = false;

	kk_err_t err = kUartMsgSent(&uMsgNode, 200);
	return err;
}
#if 1
uint8_t kCmdMsgDataSent(uint8_t ucControl_Field ,DataField_st data, bool isGetResult)
{
	UMsgNodeSt uMsgNode;

//	static uint16_t seq = 0x00;
//	iKonkeAfSelfPrint("-------------- 11: %d -------\r\n", halCommonGetInt32uMillisecondTick());
	uint8_t length = 0;

	uMsgNode.buffer[length++]  = 0XAA; //msg header
	uMsgNode.buffer[length++]  = 0X55; //msg header

	uMsgNode.buffer[length++]  = 0x00;//���ݳ���
	uMsgNode.buffer[length++]  = 0x00;	//���ݳ���
	uMsgNode.buffer[length++]  = ucControl_Field;
//	seq++;
	uMsgNode.buffer[length++]  = (uint8_t)(data.seq >> 8);
	uMsgNode.buffer[length++]  = (uint8_t)data.seq;
	uMsgNode.buffer[length++]  = data.u8Datalen;
	uMsgNode.buffer[length++]  = data.u8ChannelID;
	uMsgNode.buffer[length++]  = (uint8_t)(data.u16Opcode >> 8);
	uMsgNode.buffer[length++]  = (uint8_t)data.u16Opcode;
	memcpy(&uMsgNode.buffer[length], data.u8ARG, data.u8Datalen - 3);
	length = length + data.u8Datalen - 3;
	uMsgNode.buffer[2] = (uint8_t)((length - 4) >> 8);
	uMsgNode.buffer[3] = (uint8_t)(length - 4);
	uint16_t crc = kCmdGetMsgCrc16Value(&uMsgNode.buffer[4], length - 4);

	uMsgNode.buffer[length++] = (uint8_t)(crc >> 8);
	uMsgNode.buffer[length++] = (uint8_t)(crc);

	uMsgNode.length = length;

	// setting for matcher content.

	uMsgNode.matcher[0] = uMsgNode.buffer[2];						//seq
	uMsgNode.matcher[1] = uMsgNode.buffer[3];
	uMsgNode.matcher_offset = 2;
	uMsgNode.matcher_n = 2;
	//zigbee send to host ack
	if (ucControl_Field == Z_TO_H_NO_ACK) {
		uMsgNode.sent_try = 0;
	}else if (ucControl_Field == Z_TO_H_WITH_ACK) {
		//zigbee send to host notify or control
		uMsgNode.sent_try = 1;
	}

	if(isGetResult)
		uMsgNode.ack = uMsgNode.buffer;
	else
		uMsgNode.ack = NULL;
	uMsgNode.ack_length = 0;
	return kUartMsgSent(&uMsgNode, 300);
}

#endif
/*	DESP: Obtain the information of child nodes according to the short address.
 * 	Auth: dingmz_frc.20191019.
 * */
void kCmdGetChildDataByNodeID(uint16_t node_id, EmberChildData *childData )
{
	uint8_t index = 0, size = emberAfGetChildTableSize();

	childData->type = EMBER_UNKNOWN_DEVICE;

	for(index = 0; index < size; ++ index ) {
		EmberStatus status = emberAfGetChildData(index, childData);

		if( status == EMBER_SUCCESS ) {
			if( childData->id == node_id ) {
				break;
			}
		}
	}

	if( childData->type == EMBER_UNKNOWN_DEVICE ) {
		memset(childData->eui64, 0, EUI64_SIZE);
		childData->id = 0;
	}
}

/*	DESP: Obtain the information of child nodes according to the IEEE address.
 * 	Auth: dingmz_frc.20191019.
 * */
void kCmdGetChildDataByIEEE(EmberEUI64 ieee, EmberChildData *childData )
{
	uint8_t index = 0, size = emberAfGetChildTableSize();

	childData->type = EMBER_UNKNOWN_DEVICE;

	for(index = 0; index < size; ++ index ) {
		EmberStatus status = emberAfGetChildData(index, childData);

		if( status == EMBER_SUCCESS ) {
			if( memcmp(childData->eui64, ieee, EUI64_SIZE) == 0 ) {
				break;
			}
		}
	}

	if( childData->type == EMBER_UNKNOWN_DEVICE ) {
		memset(childData->eui64, 0, EUI64_SIZE);
		childData->id = 0;
	}
}

void kGetMfgTokenUserData(uint16_t token, void *data, uint32_t len)
{
  uint32_t realAddress = 0;
  realAddress = ((USERDATA_BASE + (token & 0x0FFF)));
  iKonkeAfSelfPrint("realAddress(%d)\r\n", realAddress);
  memcpy((uint8_t *)data,(uint8_t *)(realAddress), len);
}


void kSetMfgTokenUserData(uint16_t token, void *data, uint32_t len)
{
  EmberStatus flashStatus = EMBER_ERR_FATAL;
  uint32_t realAddress = 0;
  iKonkeAfSelfPrint("\r\n---[token = %d]----\r\n",token);
  //Initializing to a high memory address adds protection by causing a

  uint32_t i;
  //The flash library requires the address and length to both
  //be multiples of 16bits.  Since this API is only valid for writing to
  //the UserPage or LockBits page, verify that the token+len falls within
  //acceptable addresses..
  assert((token & 1) != 1);
  assert((len & 1) != 1);

  if ((token & 0xF000) == (USERDATA_TOKENS & 0xF000)) {
    //realAddress = ((USERDATA_BASE + (token & 0x0FFF)));
    realAddress = (((token & 0x0FFF)));
    iKonkeAfSelfPrint("\r\n---[realAddress = %d]----\r\n",realAddress);
  }

  // 1. read user data
  uint8_t raw_buffer[USERDATA_SIZE] = 0;
  memcpy(raw_buffer, (uint8_t *)USERDATA_BASE, USERDATA_SIZE);

  // 2.rewrite user data buffer
#if defined(_SILICON_LABS_32B_SERIES_2)
  uint32_t startWordToWrite, endWordToWrite, startWordAddress, endWordAddress;
  bool isStartWord = false;
  bool isEndWord = false;

// if address is 2 byte aligned instead of 4, write two buffer bytes of 0xFFFF with
// the first 2 bytes of data separately from the rest of data 2 bytes before
// realAddress. There is buffer space built into the token map to account for these
// extra two bytes at the beginning
  if ((realAddress & 3U) != 0U) {
    // address for start word at aligned address 2 bytes before realAddress,
    // realAddress shifts to aligned address 2 bytes later
    startWordAddress = realAddress - 2;
    realAddress += 2;
    assert((realAddress & 3U) == 0U);

    // create word to write buffer bytes and first 2 bytes of token
    startWordToWrite = 0x0000FFFF | (*(uint16_t *) data << 16);

    // adjust data pointer and byte count
    data = (uint16_t *) data + 1;
    len -= 2;

    isStartWord = true;
  }

// If data size is 2 byte aligned instead of 4, write the last 2 bytes of data and
// two buffer bytes of 0xFFFF at the end of the token separately from the rest of
// data. Buffer space is built into the mfg token map to account for the extra bytes
  if ((len & 3U) != 0) {
    // adjust to new length
    len -= 2;

    // address for end word at aligned address where the last two bytes of data would
    // be written
    endWordAddress = realAddress + len;

    // create word to write last 2 bytes of token and buffer bytes
    endWordToWrite = 0xFFFF0000 | *((uint16_t *) data + len / 2);

    isEndWord = true;
  }
  iKonkeAfSelfPrint("isStartWord(%d)isEndWord(%d)\r\n",isStartWord,isEndWord);
// if writing to USERDATA use secure element
  if ((token & 0xF000) == (USERDATA_TOKENS & 0xF000)) {
    //SE_Response_t seStatus = SE_RESPONSE_ABORT;
    uint32_t offset;

    // write first word if necessary
    if (isStartWord) {
      offset = startWordAddress & 0x0FFF;
      iKonkeAfSelfPrint("offset1(%d)\r\n", offset);
      memcpy(&raw_buffer[offset], (void *) &startWordToWrite, 4);
      //seStatus = SE_writeUserData(offset, (void *) &startWordToWrite, 4);
      //assert(seStatus == SE_RESPONSE_OK);
    }

    // write main area of token if necessary
    if (len > 0) {
      offset = realAddress & 0x0FFF;
      iKonkeAfSelfPrint("offset2(%d)\r\n", offset);
      memcpy(&raw_buffer[offset], data, len);
      //seStatus = SE_writeUserData(offset, data, len);
      //assert(seStatus == SE_RESPONSE_OK);
    }

    // write last word if necessary
    if (isEndWord) {
      offset = endWordAddress & 0x0FFF;
      //seStatus = SE_writeUserData(offset, (void *) &endWordToWrite, 4);
      iKonkeAfSelfPrint("offset3(%d)\r\n", offset);
      memcpy(&raw_buffer[offset], (void *) &endWordToWrite, 4);
      //assert(seStatus == SE_RESPONSE_OK);
    }
  }

  SE_Response_t rsp = SE_RESPONSE_OK;
  // 3.write user to flash
  rsp = SE_eraseUserData();

  iKonkeAfSelfPrint("USERDATA_BASE(%d)(%08x)\r\n", USERDATA_BASE, rsp);
  rsp = SE_writeUserData(0, raw_buffer, USERDATA_SIZE);
  iKonkeAfSelfPrint("USERDATA_BASE(%d)(%08x)\r\n", USERDATA_BASE, rsp);
#endif

#if defined(_SILICON_LABS_32B_SERIES_2)

#elif defined(_SILICON_LABS_32B_SERIES_1)
  //Remember, the flash library's token support operates in 16bit quantities,
  //but the token system operates in 8bit quantities.  Hence the divide by 2.
  //NOTE: The actual flash for the EFM32 device prefer to work with 32b writes,
  //but 16b is possible and supported by the flash driver - primarily for the
  //purposes of the SimEE/Token system that likes to use 8b and 16b.
  // THIS LENGTH IS A 32 BIT LENGTH
  return;

#else
#error Unknown device series
#endif
}

#if 1

static uint16_t opcodeReampTable[][2] =
{
  //FCC0, AA55
  {0xE0, 0x0001}, //write install code,
  {0xE1, 0x0003}, //write cmei
  {0xE2, 0x0005}, //write isn
  {0xE3, 0x0007}, //write mac
  {0xE6, 0x0002},
  {0xE7, 0x0004},
  {0xE8, 0x0006},
  {0xE9, 0x0008},
  {0xEA, 0x0009},
  {0xEB, 0x000A},
  //{0xF0, 0xED14},//�򿪲���
  //{0xF1, 0xED04},//�˳�����
  //{0xF2, 0xED05},//��ѯ������Ϣ����bitλ��
    //{0xF8, 0xED16},//��ѯ������Ϣ�����ֽڣ�
  {0xF3, 0x000C}, //�����ϻ�ʱ��
  {0xF4, 0x000D}, //��ȡ�ϻ�ʱ��
  {0xF5, 0x000E}, //����interPan����
  {0xF6, 0x000F}, //��ѯinterPan����
  {0xF7, 0x0010}, //��ѯ�豸��Ϣ
};


/*  DESP: remap opcode, arg direction 0: FCC0->AA55  1:AA55->FCC0
 *  Auth:
 * */
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

/*	DESP: Custom private command processing interface.
 * 	Auth: maozj.20200723.
 * */


kk_err_t kCmdGeneralMsgPaser(UMsgNodeSt *pMsgNode, ComPortEm port, DataField_st *pDataOut)
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
  iKonkeAfSelfPrintBuffer(pMsgNode->buffer, pMsgNode->length, true);
  iKonkeAfSelfPrint("\r\n--------------------\r\n");
#endif

  // check for parsable packets
  if( pMsgNode->length < 5 /* Minimum Packet Length */ ) {
    iKonkeAfSelfPrint("Err: Unparsable Packets!!\r\n");
    return KET_ERR_INVALID_PARAM;
  }
  //length(1) pid(4) ch(1) opcode(1) arg
  if (port == ECOM_PORT_FCC0) {
    length = (uint16_t)pMsgNode->length;
    //control_field = pMsgNode->buffer[4];
    data.seq = 0x00;
    data.u8Datalen = pMsgNode->buffer[0];
    data.u8Datalen = data.u8Datalen - 6; // fcc0 - id (4) - ch(1) - opcode(1)
    data.u8ChannelID = pMsgNode->buffer[5];
    data.u16Opcode =   kCmdOpcodeRemap((uint16_t)(pMsgNode->buffer[6]), 0);

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
  send_buf.u8Datalen = 0;
  switch(data.u16Opcode) {

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

    case (UART_MSG_WRITE_INSTALL_CODE_OPCODE): // write install code
    {
      tokTypeMfgInstallationCode custom_install_code;
      uint16_t crc = 0xFFFF;
      reply_control_field = Z_TO_H_NO_ACK;
     // halCommonGetToken(&custom_install_code, TOKEN_MFG_INSTALLATION_CODE);

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
      for(uint8_t i =0; i<8; i++){
          iKonkeAfSelfPrint("[ins[%d]= %x:]",i,custom_install_code.value[i]);
      }
      for (uint8_t index = 0; index < data.u8ARG[0]; index++) {
        crc = halCommonCrc16(reverse(custom_install_code.value[index]), crc);
      }

      crc = ~HIGH_LOW_TO_INT(reverse(LOW_BYTE(crc)), reverse(HIGH_BYTE(crc)));
      custom_install_code.crc = crc;

      //halCommonSetToken(TOKEN_MFG_CUSTOM_INSTALL_CODE, &custom_install_code);
      //halCommonSetToken(TOKEN_MFG_INSTALLATION_CODE,&custom_install_code);
      kSetMfgTokenUserData(TOKEN_MFG_INSTALLATION_CODE, &custom_install_code, sizeof(tokTypeMfgInstallationCode));
      //write installationcode
      //kOptTunnelTrytoWriteInstallCode();

      send_buf.u8ARG[0] = ERR_OK;
      send_buf.u8ARG[1] = data.u8ARG[0];
      memcpy(&send_buf.u8ARG[2], &data.u8ARG[1], data.u8ARG[0]);
      send_buf.u8Datalen = data.u8ARG[0] + 1 + 1 -3;
      //send_buf.u8Datalen = data.u8ARG[0] + 5;

      break;
    }

    case (UART_MSG_READ_INSTALL_CODE_OPCODE): //read install code
    {
      uint8_t install_code_len = 0;
      tokTypeMfgInstallationCode install_code;

      kSetMfgTokenUserData(4466, &install_code, sizeof(tokTypeMfgInstallationCode));
      //halCommonGetToken(&install_code, TOKEN_MFG_INSTALLATION_CODE);
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

    case (UART_MSG_WRITE_CMEI_CODE_OPCODE): // write cmei code
    {

      break;
    }

    case (UART_MSG_READ_CMEI_CODE_OPCODE): //read cmei code
    {

      break;
    }

    case (UART_MSG_WRITE_ISN_CODE_OPCODE):
    {

      break;
    }

    case (UART_MSG_READ_ISN_CODE_OPCODE):
    {

      break;
    }
    case (UART_MSG_LEAVE_NWK_REQUEST_OPCODE):
    {

      break;
    }
    case (UART_MSG_QUERY_NWK_STATUS_REQUEST_OPCODE):

      break;

    //add bby maozj 20191220
    case (UART_MSG_JOIN_NWK_REQUEST_OPCODE):
    {

      break;
    }

    case (UART_MSG_WRITE_MAC_CODE_OPCODE):
    {

      break;
    }

    case (UART_MSG_READ_MAC_CODE_OPCODE):
    {

      break;
    }

    case (UART_MSG_WRITE_MOUDLE_ID_OPCODE):
    {

      break;
    }

    case (UART_MSG_READ_DEV_RSSI_OPCODE):
    {

      break;
    }
    case (UART_MSG_WRITE_AGING_TIME_OPCODE):
    {

      break;
    }

    case (UART_MSG_READ_AGING_TIME_OPCODE):
    {

      break;
    }

    case (UART_MSG_WRITE_INTERPAN_PARA_OPCODE):
    {

      break;
    }

    case (UART_MSG_READ_INTERPAN_PARA_OPCODE):
    {

      break;
    }

    case (UART_MSG_READ_DEVICE_SNAP_OPCODE):
    {

      break;
    }
    default:
    {
      break;
    }
  }

  memcpy(pDataOut, (uint8_t *)&send_buf, sizeof(DataField_st));

  if (send_buf.u8Datalen == 0) { //if cmd exist, lengtg must > 0
    return KET_ERR_CMD_INVALID;
  } else {
    if (port == ECOM_PORT_FCC0) {
      pDataOut->u8Datalen = pDataOut->u8Datalen;
    } else {
      pDataOut->u8Datalen = pDataOut->u8Datalen + 3; //AA 55 (opcode(2), ch(1))
    }
    return KET_OK;
  }
  //kCmdMsgDataSentByPort(reply_control_field, send_buf, false, port);
}




#endif


