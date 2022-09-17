#ifndef __IKONKE_MODULE_COMMAND_H____________________________
#define __IKONKE_MODULE_COMMAND_H____________________________


#include "stdint.h"
#include "ctype.h"
#include "app/framework/include/af.h"
#include "app/framework/plugin/ota-storage-simple-ram/ota-static-file-data.h"
#include "ikk-module-def.h"
#include "../driver/ikk-uart.h"
// OTA Image?o???????��|???��?D??��o??����?��|?��??aOTA1y3?��??D?��oy?Y???��1o?��|??2???��1???��??��??��2????|��??��o??��o?
#define PAGE_CACHE_MAXN			4		// Maximum number of buffer pages supported!
#define PAGE_CACHE_SIZE			2048	// Single buffer page space size.

#define GLOBAL_BUFFER_SIZE 		256

typedef struct tag_image_chunk {
	unsigned char data[PAGE_CACHE_SIZE];
	int offset;
}ImageChunkSt;

typedef struct tag_image_descriptor {
	ImageChunkSt *pChunk;		// image chunk????

	// Add by dingmz_frc.20191203.last packet buffer
	unsigned char last_pt[128];
	int last_offset;

	unsigned int u32ImageSize;	// image???t?����?��1???��?D?
}ImageDesprSt;


#define PROTOCOL_VERSION_DEF	(2)

// the size of single queuen node name and payload!
#define COMMNAD_MSG_MAXLEN	(PAGE_CACHE_SIZE * 2 + 32)

// Command name max length
#define COMMAND_NAME_MAXLEN		(48)
// Command param list max length
#define COMMAND_PARAM_MAXLEN	(320)

// Command param list max number
#define COMMAND_PARAM_MAXN		(16)

// List of supported command types
typedef uint8_t CommandTypeEnum;

enum em_supported_command_type_list { ECT_UNKNOW = 0, ECT_LOCAL, ECT_ZDO, ECT_ZCL };

// List of supported commands
typedef int CommandEnum;

// command region: down-req[1,999], up-req[2000,2999], down-ack[6001, 6999], up-ack[8000,8999]; up-notify[9000, 9998];
enum em_supported_command_list {
	ECMD_UNKNOW 				= 0,

	// host down to soc-gateway commands list
	ECMD_LINK_SET_REQ 			= 1,		// <kksper-dLinkSetReq>
	ECMD_LINK_SET_ACK 			= 6001,		// <kksper-uLinkSetAck>

	ECMD_LOCAL_INFO_REQ 		= 2,		// <kksper-dLocalInfoReq>
	ECMD_LOCAL_INFO_ACK 		= 6002,		// <kksper-uLocalInfoAck>
	ECMD_NETWORK_UP_REQ 		= 3,		// <kksper-dNetworkUpReq>
	ECMD_NETWORK_UP_ACK			= 6003,		// <kksper-uNetworkUpAck>
	ECMD_NETWORK_DOWN_REQ		= 4,		// <kksper-dNetworkDownReq>
	ECMD_NETWORK_DOWN_ACK		= 6004,		// <kksper-uNetworkDownAck>
	ECMD_PJOIN_ON_REQ			= 5,		// <kksper-dPermitJoinOnReq>
	ECMD_PJOIN_ON_ACK			= 6005,		// <kksper-uPermitJoinOnAck>
	ECMD_PJOIN_OFF_REQ			= 6,		// <kksper-dPermitJoinOffReq>
	ECMD_PJOIN_OFF_ACK			= 6006,		// <kksper-uPermitJoinOffAck>
	ECMD_LEAVE_REQ				= 7,		// <kksper-dLeaveReq>
	ECMD_LEAVE_ACK				= 6007,		// <kksper-uLeaveAck>
	ECMD_BIND_REQ				= 8,		// <kksper-dBindReq>
	ECMD_BIND_ACK				= 6008,		// <kksper-uBindAck>
	ECMD_ACTIVE_EPS_REQ			= 9,		// <kksper-dActiveEpsReq>
	ECMD_ACTIVE_EPS_ACK			= 6009,		// <kksper-uActiveEpsAck>
	ECMD_SIMPLE_DESP_REQ		= 10,		// <kksper-dSimpleDespReq>
	ECMD_SIMPLE_DESP_ACK		= 6010,		// <kksper-uSimpleDespAck>
	ECMD_MATCH_DESP_REQ			= 11,		// <kksper-dMatchDespReq>
	ECMD_MATCH_DESP_ACK			= 6011,		// <kksper-uMatchDespAck>
	ECMD_NODE_DESP_REQ			= 12,		// <kksper-dNodeDespReq>
	ECMD_NODE_DESP_ACK			= 6012,		// <kksper-uNodeDespAck>
	ECMD_IEEE_ADDR_REQ			= 13,		// <kksper-dIeeeAddrReq>
	ECMD_IEEE_ADDR_ACK			= 6013,		// <kksper-uIeeeAddrAck>
	ECMD_IP_ADDR_REQ			= 14,		// <kksper-dIPAddrReq>
	ECMD_IP_ADDR_ACK			= 6014,		// <kksper-uIPAddrAck>
	ECMD_CHILD_RECORD_REQ		= 15,		// <kksper-dChildRecordReq>
	ECMD_CHILD_RECORD_ACK		= 6015,		// <kksper-uChildRecordAck>
	ECMD_CLUSTER_CMD_REQ		= 16,		// <kksper-dClusterCmdReq>
	ECMD_CLUSTER_CMD_ACK		= 6016,		// <kksper-uClusterCmdAck>
	ECMD_READ_ATTR_REQ			= 17,		// <kksper-dReadAttrReq>
	ECMD_READ_ATTR_ACK			= 6017,		// <kksper-uReadAttrAck>
	ECMD_WRITE_ATTR_REQ			= 18,		// <kksper-dWriteAttrReq>
	ECMD_WRITE_ATTR_ACK			= 6018,		// <kksper-uWriteAttrAck>
	ECMD_SET_IC_REQ				= 19,		// <kksper-dSetInstallCodeReq>
	ECMD_SET_IC_ACK				= 6019,		// <kksper-uSetInstallCodeAck>
	ECMD_GET_RSSI_REQ			= 20,		// <kksper-dRssiReq>
	ECMD_GET_RSSI_ACK			= 6020,		// <kksper-uRssiAck>
	ECMD_OOD_CMD_REQ			= 21,		// <kksper-dOodCmdReq>
	ECMD_OOD_CMD_ACK			= 6021,		// <kksper-uOodCmdAck>
	ECMD_OTA_NOTIFY_REQ			= 22,		// <kksper-dOtaNotifyReq>
	ECMD_OTA_NOTIFY_ACK			= 6022,		// <kksper-uOtaNotifyAck>
	ECMD_NWK_STEERING_REQ		= 23,		// <kksper-dNetworkSteeringReq> add by maozj 20191218
	ECMD_NWK_STEERING_ACK		= 6023,		// <kksper-uNetworkSteeringAck>

	ECMD_DLINK_ACK 				= 999,		// <kksper-dLinkAck>
	ECMD_ULINK_ACK 				= 6999,		// <kksper-uLinkAck>

	// soc-gateway up to host commands list
	ECMD_OTA_BLOCK_REQ			= 2000,		// <kksper-uOtaBlockReq>
	ECMD_OTA_BLOCK_ACK			= 8000,		// <kksper-dOtaBlockAck>

	// soc-gateway up to host notify commands list
	ECMD_JOINED_NOTIFY			= 9000,		// <kksper-uJoinedNotify>
	ECMD_LEAVED_NOTIFY			= 9001,		// <kksper-uLeavedNotify>
	ECMD_DEVANNOUNCE_NOTIFY		= 9002,		// <kksper-uDevAnnounceNotify>
	ECMD_CLUSTER_CMD_NOTIFY		= 9003,		// <kksper-uClusterCmdNotify>
	ECMD_OOD_CMD_NOTIFY			= 9004,		// <kksper-uOodCmdNotify>
	ECMD_ATTR_REPORT			= 9005,		// <kksper-uAttrReport>
	ECMD_OTA_START_NOTIFY		= 9006,		// <kksper-uOtaStartNotify>
	ECMD_OTA_COMPLETED_NOTIFY	= 9007,		// <kksper-uOtaCompletedNotify>
};

#define CMD_REQ_ACK_DIFF		6000

#define DCMD_REQ_SECTION_ID		0			// down command REQ  base ID, section id(thousands number)
#define UCMD_REQ_SECTION_ID		2			// up   command REQ  base ID, section id(thousands number)
#define DCMD_ACK_SECTION_ID		6			// down command ACK  base ID, section id(thousands number)
#define UCMD_ACK_SECTION_ID		8			// up   command ACK  base ID, section id(thousands number)
#define UCMD_NOTIFY_SECTION_ID	9			// up notify commnad base ID, section id(thousands number)

#define SECTION_ID(command)		((command) / 1000)
#define CMDACK(command)			((command) + CMD_REQ_ACK_DIFF)

// the supported commands list. the corresponding ack command name of the command that does not need to ack is itself.
// Data Type:
// int(i), uint8_t(u), uint16_t(v), uint32_t(w), uint64_t(l), uint8_t[](a), uint16_t[](b), uint32_t[](c), uint64_t[d]
#define STATIC_COMMAND_DEF	{	\
	{ECT_LOCAL, ECMD_LINK_SET_REQ,	  	  "<kksper-dLinkSetReq>",	 	  "<kksper-dLinkSetAck>",	  		"uu"	}, \
	{ECT_LOCAL, ECMD_LOCAL_INFO_REQ,	  "<kksper-dLocalInfoReq>",	 	  "<kksper-uLocalInfoAck>",	  		"" 		}, \
	{ECT_LOCAL, ECMD_NETWORK_UP_REQ,	  "<kksper-dNetworkUpReq>",	 	  "<kksper-uNetworkUpAck>",	  		"uvii"	}, \
	{ECT_LOCAL, ECMD_NETWORK_DOWN_REQ, 	  "<kksper-dNetworkDownReq>",	  "<kksper-uNetworkDownAck",	  	""		}, \
	{ECT_LOCAL, ECMD_PJOIN_ON_REQ, 	 	  "<kksper-dPermitJoinOnReq>", 	  "<kksper-uPermitJoinOnAck>", 		""		}, \
	{ECT_LOCAL, ECMD_PJOIN_OFF_REQ, 	  "<kksper-dPermitJoinOffReq>",	  "<kksper-uPermitJoinOffAck>",		""		}, \
	{ECT_ZDO,   ECMD_LEAVE_REQ, 		  "<kksper-dLeaveReq>",		 	  "<kksper-uLeaveAck>",				"vl"	}, \
	{ECT_ZDO,   ECMD_BIND_REQ, 		 	  "<kksper-dBindReq>",			  "<kksper-uBindAck>",				"vuullv"}, \
	{ECT_ZDO,   ECMD_ACTIVE_EPS_REQ, 	  "<kksper-dActiveEpsReq>",		  "<kksper-uActiveEpsAck>",			"v"		}, \
	{ECT_ZDO,   ECMD_SIMPLE_DESP_REQ,  	  "<kksper-dSimpleDespReq>",	  "<kksper-uSimpleDespAck>",		"vu"	}, \
	{ECT_ZDO,   ECMD_MATCH_DESP_REQ, 	  "<kksper-dMatchDespReq>",		  "<kksper-uMatchDespAck>",			"vvbb"	}, \
	{ECT_ZDO,   ECMD_NODE_DESP_REQ, 	  "<kksper-dNodeDespReq>",		  "<kksper-uNodeDespAck>",			"v"		}, \
	{ECT_ZDO,   ECMD_IEEE_ADDR_REQ, 	  "<kksper-dIeeeAddrReq>",		  "<kksper-uIeeeAddrAck>",			"v"		}, \
	{ECT_ZDO,   ECMD_IP_ADDR_REQ, 	 	  "<kksper-dIpAddrReq>",		  "<kksper-uIpAddrAck>",			"l"		}, \
	{ECT_LOCAL, ECMD_CHILD_RECORD_REQ, 	  "<kksper-dChildRecordReq>",	  "<kksper-uChildRecordAck>",		"uvl"	}, \
	{ECT_ZCL,   ECMD_CLUSTER_CMD_REQ,	  "<kksper-dClusterCmdReq>",	  "<kksper-uClusterCmdAck>",		"vuvvuuuau"}, \
	{ECT_ZCL,   ECMD_READ_ATTR_REQ,		  "<kksper-dReadAttrReq>",		  "<kksper-uReadAttrAck>",			"vuvuvb"}, \
	{ECT_ZCL,   ECMD_WRITE_ATTR_REQ,	  "<kksper-dWriteAttrReq>",		  "<kksper-uWriteAttrAck>",			"vuvuva"}, \
	{ECT_LOCAL, ECMD_SET_IC_REQ,		  "<kksper-dSetInstallCodeReq>",  "<kksper-uSetInstallCodeAck>",	"lua"	}, \
	{ECT_LOCAL, ECMD_GET_RSSI_REQ,		  "<kksper-dRssiReq>",			  "<kksper-uRssiAck>",				"v"		}, \
	{ECT_ZCL, 	ECMD_OOD_CMD_REQ,		  "<kksper-dOodCmdReq>",		  "<kksper-uOodCmdAck>",			"vwuua"	}, \
	{ECT_ZCL, 	ECMD_OTA_NOTIFY_REQ,	  "<kksper-dOtaNotifyReq>",		  "<kksper-uOtaNotifyAck>",			"vuuuvvw"},\
	{ECT_LOCAL, ECMD_NWK_STEERING_REQ, 	  "<kksper-dNetworkSteeringReq>",  "<kksper-uNetworkSteeringAck>",	"v"	}, \
	{ECT_LOCAL, ECMD_DLINK_ACK, 		  "<kksper-dLinkAck>",			  "<kksper-uLinkAck>",				"u"		}, \
	\
	{ECT_ZCL, 	ECMD_OTA_BLOCK_REQ,	  	  "<kksper-uOtaBlockReq>",		  "<kksper-dOtaBlockAck>",			"uiia"	}, \
	\
	{ECT_LOCAL, ECMD_JOINED_NOTIFY, 	  "<kksper-uJoinedNotify>",		  "<kksper-uJoinedNotify>",			""		}, \
	{ECT_LOCAL, ECMD_LEAVED_NOTIFY, 	  "<kksper-uLeavedNotify>",		  "<kksper-uLeavedNotify>",			""		}, \
	{ECT_LOCAL, ECMD_DEVANNOUNCE_NOTIFY,  "<kksper-uDevAnnounceNotify>",  "<kksper-uDevAnnounceNotify>",	""		}, \
	{ECT_ZCL, 	ECMD_CLUSTER_CMD_NOTIFY,  "<kksper-uClusterCmdNotify>",	  "<kksper-uClusterCmdNotify>",		""		}, \
	{ECT_ZCL, 	ECMD_OOD_CMD_NOTIFY,	  "<kksper-uOodCmdNotify>",		  "<kksper-uOodCmdNotify>",			""		}, \
	{ECT_ZCL, 	ECMD_ATTR_REPORT,	  	  "<kksper-uAttrReport>",		  "<kksper-uAttrReport>",			""		}, \
	{ECT_ZCL, 	ECMD_OTA_START_NOTIFY,	  "<kksper-uOtaStartNotify>",	  "<kksper-uOtaStartNotify>",		""		}, \
	{ECT_ZCL, 	ECMD_OTA_COMPLETED_NOTIFY,"<kksper-uOtaCompletedNotify>", "<kksper-uOtaCompletedNotify>",	""		}, \
}

typedef struct tag_command_incoming_manager {
	char cmdlins[COMMNAD_MSG_MAXLEN];
	int counter;

	CommandEnum command;
	char *param[COMMAND_PARAM_MAXN];

	uint8_t seq;
	uint16_t crc16;
	bool brecving;
}CommandIncomingMagSt;

typedef struct tag_command_outgoing_manager {
	char cmdname[COMMAND_NAME_MAXLEN];
	char cmdpmls[COMMAND_PARAM_MAXLEN];

	// sequence for current outgoing sending.
	uint8_t seq;
	// sequence for global sending.
	uint8_t seq_counter;
}CommandOutgoingMagSt;

// Link ack status
typedef enum {
	ELS_SUCCESS 				= 0x00,	// Link communication successful
	ELS_ERR_INVALID_CMMAND 		= 0x80,	// Command error
	ELS_ERR_CRC_WRONG 			= 0x81,	// Command CRC16 value wrong
	ELS_ERR_CMD_TOOLONG 		= 0x82,	// Command is too long
	ELS_ERR_CMD_FORMAT			= 0x83,	// Command format error
	LS_ERR_INVALID_PARAM		= 0x84,	// invalid parameter
	ELS_ERR_LINK_BUSY			= 0xfe,	// Link busy, please try again later
	ELS_ERR_UNKNOW				= 0xff,	// unknow error!
}LinkStatusEm;

#define NODE_ID_LOCAL			0X0000
#define ENDPOINT_UNKNOW			0X00
#define CLUSTER_UNKNOW			0XFFFF
#define CLUSTER_CMD_UNKNOW		0XFF
#define CLUSTER_ATTR_UNKNOW		0XFFFF
#define TRANSMIT_SEQ_UNKNOW		0XFF


#define	IMAGE_LOOP_NMS			100  // ms
// command images cache list for ack reply detection. a single record can only exist for COMMAND_IMAGE_DEADLINE second!
#define COMMAND_IMAGE_DEADLINE	10000// ms
// Max depth of the command image list!
#define CMDIMAGE_LIST_DEPTH 	6
// up-request command re-send count and period!
#define UCMD_RETRY_COUNT		3
#define UCMD_RETRY_PERIOD		1000 // ms

#define CMD_CONTROL_FIELD_HOST_TO_ZIGBEE_NEED_ACK		0x10
#define CMD_CONTROL_FIELD_HOST_TO_ZIGBEE_NO_ACK			0x00
#define CMD_CONTROL_FIELD_ZIGBEE_TO_HOST_NEED_ACK		0x30
#define CMD_CONTROL_FIELD_ZIGBEE_TO_HOST_NO_ACK			0x20

typedef enum {
	H_TO_Z_WITH_ACK=CMD_CONTROL_FIELD_HOST_TO_ZIGBEE_NEED_ACK,
	H_TO_Z_NO_ACK=CMD_CONTROL_FIELD_HOST_TO_ZIGBEE_NO_ACK,
	Z_TO_H_WITH_ACK=CMD_CONTROL_FIELD_ZIGBEE_TO_HOST_NEED_ACK,
	Z_TO_H_NO_ACK=CMD_CONTROL_FIELD_ZIGBEE_TO_HOST_NO_ACK,
}CMD_CONTROL_FIELD_E;

typedef struct tag_command_image_node_st {
	CommandEnum command;		// command type
	uint16_t node_id;			// device node id: local with 0x0000;
	uint8_t endpoint;			// server endpoint
	uint16_t cluster_id;		// cluster for operating
	uint8_t cls_cmd_id;			// cluster command id for operating
	uint8_t transmit_seq;		// APS payload(zdo or zcl format) transmit sequence number, Used to detect remote device response!
	uint8_t command_seq;		// command sequence record

	uint8_t valid_counter;		// image record valid counter.
	uint8_t retry_counter;		// retry period

	// the list of parameters after serialization.
	char cmdpmls[COMMAND_PARAM_MAXLEN];
}CommandImageNodeSt;

typedef struct {
	CommandTypeEnum type;
	CommandEnum command;
	char *cmdname_req;
	char *cmdname_ack;

	char *incoming_param_type;
}CommandDefSt;


typedef struct{
	uint16_t seq;
	uint8_t u8Datalen;
	uint8_t u8ChannelID;
	uint16_t u16Opcode;
	uint8_t u8ARG[128];
}DataField_st;

typedef struct{
	uint8_t u8SOFHeaderBuf[2];
	uint16_t u16DataLength;
	uint8_t u8ControlFied;
	//uint16_t u16Seq;
	DataField_st stDataField;
}MsgFrameworkFormat_st;

#define NETWORK_UP_PAN_DEFAULT  	0xFF17
#define NETWORK_UP_CHANNEL_DEFAULT	20
#define NETWORK_UP_POWER_DEFAULT	20

uint16_t kCmdGetMsgCrc16Value( uint8_t* msg, uint8_t len);

uint8_t kCmdGetMsgSum(uint8_t *pdat, int length );

bool kCmdCheckMsgCRC16(uint8_t *pdat, int length );
bool CheckSum(uint8_t *str, int str_length);
uint8_t kCmdMsgDataSent(uint8_t ucControl_Field ,DataField_st data, bool isGetResult);
uint8_t kCmdMsgDataSentByPort(uint8_t u8Control_Field, DataField_st data, bool isGetResult, ComPortEm port);

kk_err_t kCmdGeneralMsgPaser(UMsgNodeSt *pMsgNode, ComPortEm port, DataField_st *pDataOut);

void kCmdGeneralMsgHanderCallback(UMsgNodeSt *pMsgNode );

static uint16_t kCmdOpcodeRemap(uint16_t Opcode, uint8_t direction);
#endif
