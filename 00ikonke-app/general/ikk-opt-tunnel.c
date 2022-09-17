#include <00ikonke-app/general/ikk-cluster.h>
#include <00ikonke-app/general/ikk-opt-tunnel.h>
#include "ikk-network.h"
#include "app/framework/plugin/reporting/reporting.h"
#include "app/framework/include/af-types.h"
#include "ikk-debug.h"
#include "ikk-common-utils.h"
#include "app/framework/security/af-security.h"
#include "00ikonke-app/ikk-user.h"

// FCC0-0000
#pragma pack(1)
typedef struct tag_private_clsFCC0_attr0000_frame_st {
	unsigned char length;	// frame length - not include itself!!!
	unsigned int  device_id;// device id
	unsigned char channel;	// channel
	unsigned char opcode;	// pocode
	unsigned char arg[OPTTUNNEL_OPTDATA_MAXLEN - 7]; // opcode arg
}
__attribute__((packed)) OptTunnelOptDataFrameSt;


// private protocol optdata message incoming process callback
pFUNC_OPTDATA_MESSAGE_CALLBACK g_pOptDataIncomingMessageCallback = NULL;
// global temp buffer
uint8_t g_tmp_buffer[OPTTUNNEL_CHUNK_MAXLEN+1] = { 0 }, g_tmp_length = 0;

EmberEventControl kOptTunnelMtoRRDelayRspEventControl;

EmberAfStatus emAfReadOrWriteAttribute(EmberAfAttributeSearchRecord *attRecord,
                                       EmberAfAttributeMetadata **metadata,
                                       uint8_t *buffer,
                                       uint16_t readLength,
                                       bool write);
void emAfSaveAttributeToToken(uint8_t *data,
                              uint8_t endpoint,
                              EmberAfClusterId clusterId,
                              EmberAfAttributeMetadata *metadata);
EmberStatus emberAfSendUnicast(EmberOutgoingMessageType type,
                               uint16_t indexOrDestination,
                               EmberApsFrame *apsFrame,
                               uint16_t messageLength,
                               uint8_t *message);

uint8_t * emberAfPutInt8uInResp(uint8_t value);
uint16_t * emberAfPutInt16uInResp(uint16_t value);


extern void kNwksaveNodeIdToFlash(EmberNodeId node_id);
extern EmberNodeId kNwkgetNodeIdFromFlash(void);

extern uint8_t generatedDefaults[];
extern uint8_t appZclBuffer[];
extern uint16_t appZclBufferLen;
extern EmberApsFrame globalApsFrame;




/*	DESP: check node id is or not changed
 * 	Auth: maozj.20191125.
 * */
void kOptTunneCheckNodeIdIsChangedReport(void)
{
	//static uint32_t count = 0;
	EmberNodeId currentNodeId= 0x1234;
	EmberNodeId savedNodeId;

	//if (++count >= 0){
	//	count = 0;
		//iKonkeAfSelfPrint("@@@checkNodeIdIsChanged1\r\n");
		EmberNetworkStatus status = emberAfNetworkState();
		if (status == EMBER_JOINED_NETWORK){
			currentNodeId = emberAfGetNodeId();
			savedNodeId = kNwkgetNodeIdFromFlash();
			//iKonkeAfSelfPrint("@@@checkNodeIdIsChanged %02x %02x\r\n", savedNodeId, currentNodeId);
			if (savedNodeId != currentNodeId){
				//report AssociateAnnounce attr of private cluster
				//kkOptTunnelAssociateAnnounceReport();
				iKonkeAfSelfPrint("@@@checkNodeIdIsChanged %02x %02x\r\n", savedNodeId, currentNodeId);
				if (savedNodeId != 0xFFFF){
					kOptTunnelCommonReport(ECA_ASSOCIATED_AOUNCE);
				}
				//save changed nodeId to flash
				kNwksaveNodeIdToFlash(currentNodeId);
			}
		}
	//}
}
/*	DESP: Private clsuter attribute read interface, for intsallcode
 * ��MFG TOKEN�ж�ȡintallcode��˽��cluster�У������ض�ȡ
 * 	Auth: maozj.20191125.
 * */
void kOptTunnelReadInstallCode(uint8_t *buffer, uint8_t *length_out)
{
	uint8_t temp_buffer[32];
	uint8_t buffer_len = 0;
	uint8_t install_code_len = 0;
	extern uint8_t generatedDefaults[];

	if (buffer == NULL){
		return;
	}

	// device id
	temp_buffer[1] = kUtilsOct2Value((char *)&generatedDefaults[33+ 9], 2);
	temp_buffer[2] = kUtilsOct2Value((char *)&generatedDefaults[33+11], 2);
	temp_buffer[3] = kUtilsOct2Value((char *)&generatedDefaults[33+13], 2);
	temp_buffer[4] = kUtilsOct2Value((char *)&generatedDefaults[33+15], 2);
	//halCommonGetToken(buffer, TOKEN_CHUNK_N2_1);
	tokTypeMfgInstallationCode install_code ;
	iKonkeAfSelfPrint("\r\nGet install code:\r\n");
	//halCommonGetToken(&install_code, TOKEN_MFG_INSTALLATION_CODE);
	kGetMfgTokenUserData(4466, &install_code, sizeof(tokTypeMfgInstallationCode));
	iKonkeAfSelfPrintBuffer((uint8_t *)&install_code, sizeof(tokTypeMfgInstallationCode), true);
	iKonkeAfSelfPrint("flg = 0x%x\r\n", install_code.flags);
	//installcode���ȱ���Ϊ6,8, 12��16,��0λ��ʾ�Ƿ��ʼ���ˣ�0��ʾ�ѳ�ʼ����
	if (install_code.flags & 01){
		install_code_len = 0;
	}else {
		uint8_t data = install_code.flags >> 1;
		switch (data)
		{
		case 0:
			install_code_len = 6;
			break;
		case 1:
			install_code_len = 8;
			break;
		case 2:
			install_code_len = 12;
			break;
		case 3:
			install_code_len = 16;
			break;
		default:
			break;
		}
	}
	//memcpy(buffer, install_code.value, 16);
	//install code���16�ֽ�
	//attributeMetadata->size = 16;
	buffer_len = 4 + install_code_len;
	temp_buffer[0] = buffer_len; //˽��cluster�еĳ���
	memcpy(temp_buffer + 5, install_code.value, install_code_len);
	memcpy(buffer, temp_buffer, buffer_len + 1);

	if (length_out != NULL){
		*length_out = buffer_len;
	}
	iKonkeAfSelfPrintBuffer(buffer, buffer_len, true);
}

/*	DESP: gets the length of the specified attribute.
 * 	Auth: dingmz_frc.20191113.
 * */
#if 0
uint8_t kOptTunnelGetAttrLength(uint16_t attribute_id )
{
	if( attribute_id == ECA_UNKNOW ) {
		return 0;
	}

	switch(attribute_id) {
		case (ECA_OPTDATA):
		case (ECA_TTS):
		{
			return OPTTUNNEL_OPTDATA_MAXLEN;
		}
		case (ECA_MTORR_RR):
		{
			return sizeof(uint16_t);
		}
		case (ECA_ASSOCIATED_AOUNCE):
		{
			return OPTTUNNEL_ASSOCIATED_AOUNCE_MAXLEN;
		}
		default:
		{
			return OPTTUNNEL_CHUNK_MAXLEN;
		}
	}
}
#endif

/*	DESP: Private clsuter attribute read interface, for chunk data.
 * 	Auth: dingmz_frc.20190703.
 * */
kk_err_t kOptTunnelAttrChunkLocalRead(uint16_t attribute_id, unsigned char *chunk_buffer, unsigned char *length_in_out )
{
	if( NULL == chunk_buffer || NULL == length_in_out ) {
		return KET_ERR_INVALID_POINTER;
	}

	EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;
	kk_err_t err = KET_OK;
#if 0
	EmberAfAttributeMetadata *metadata = NULL;
	EmberAfAttributeSearchRecord record;

	record.endpoint 	= 1;
	record.clusterId 	= PRIV_CLUSTER_FCC0;
	record.clusterMask 	= CLUSTER_MASK_SERVER;
	record.attributeId 	= attribute_id;
	record.manufacturerCode = EMBER_AF_NULL_MANUFACTURER_CODE;

	status = emAfReadOrWriteAttribute(&record, &metadata,
									chunk_buffer,
									*length_in_out,
									false);
#else
	status = kOptTunnelChunkRead(1, PRIV_CLUSTER_FCC0, attribute_id, 0x1268,  chunk_buffer, length_in_out);
#endif
	if( status == EMBER_ZCL_STATUS_SUCCESS ) {
		*length_in_out = chunk_buffer[0] + 1;
	}else {
		iKonkeAfSelfPrint("Err: Chunk read err(%d)!!\r\n", status);
		*length_in_out = 0;
		err = KET_ERR_UNKNOW;
	}

	return err;
}

/*	DESP: Private clsuter attribute write interface, for chunk data.
 * 	Auth: dingmz_frc.20190701.
 * */
kk_err_t kOptTunnelAttrChunkLocalWrite(uint16_t attribute_id, unsigned char *chunk_buffer, unsigned char length )
{
	if( NULL == chunk_buffer ) {
		return KET_ERR_INVALID_POINTER;
	}

	EmberAfStatus status = EMBER_ZCL_STATUS_FAILURE;
	kk_err_t err = KET_OK;
#if 0
	EmberAfAttributeMetadata *metadata = NULL;
	EmberAfAttributeSearchRecord record;

	record.endpoint 	= 1;
	record.clusterId 	= PRIV_CLUSTER_FCC0;
	record.clusterMask 	= CLUSTER_MASK_SERVER;
	record.attributeId 	= attribute_id;
	record.manufacturerCode = EMBER_AF_NULL_MANUFACTURER_CODE;

	// write the attribute
	status = emAfReadOrWriteAttribute(&record, &metadata, chunk_buffer, 0, true);
	if( status == EMBER_ZCL_STATUS_SUCCESS ) {
		emAfSaveAttributeToToken(chunk_buffer, 1, PRIV_CLUSTER_FCC0, metadata);
	}
#else
	//kOptTunnelChunkWrite(1, PRIV_CLUSTER_FCC0, attribute_id, 0x1268,  chunk_buffer, length);
#endif
	if( status != EMBER_ZCL_STATUS_SUCCESS ) {
		iKonkeAfSelfPrint("Err: Chunk write err(%d)!!\r\n", status);
		err = KET_ERR_UNKNOW;
	}

	return err;
}

static kk_err_t kOptTunnelCommonReportSend(uint16_t attribute_id, uint8_t content[], uint8_t length )
{

	appZclBufferLen = 0;
	zclBufferSetup(ZCL_GLOBAL_COMMAND | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT, PRIV_CLUSTER_FCC0, ZCL_REPORT_ATTRIBUTES_COMMAND_ID);

	zclBufferAddWord(attribute_id);
	zclBufferAddByte(ZCL_CHAR_STRING_ATTRIBUTE_TYPE);
	zclBufferAddByte(length);
	zclBufferAddBuffer(content, length);

	emAfApsFrameEndpointSetup(1, 1);
	EmberAfStatus status = emberAfSendUnicast(EMBER_OUTGOING_DIRECT, 0, &globalApsFrame, appZclBufferLen, appZclBuffer);
	iKonkeAfSelfPrint("######OPT SEND Attr(0x%2x) Time(%d)\r\n",attribute_id, halCommonGetInt64uMillisecondTick());
	cliBufferPrint();
	return ((status == EMBER_ZCL_STATUS_SUCCESS)?KET_OK:KET_ERR_UNKNOW);
}

/*	DESP: Private protocol common report interface.
 * 	Auth: dingmz_frc.20191113.
 * */
kk_err_t kOptTunnelCommonReport(uint16_t attribute_id )
{
  extern HUMANBODY_DATA humanbody;

	if( attribute_id == OPTTUNNEL_UNKNOW ) {
		return KET_ERR_NON_EXIST;
	}

	kk_err_t err = KET_OK;

	//emberAfGetCommandApsFrame()->sourceEndpoint = 1;
	//emberAfGetCommandApsFrame()->destinationEndpoint = 1;
	iKonkeAfSelfPrint("#######kOptTunnelCommonReport attr(0x%2x)\r\n", attribute_id);
	switch(attribute_id) {
		case (ECA_OPTDATA):    //fcc0快照
		{

			iKonkeAfSelfPrint("#######kOptTunnelCommonReport OPTDATA\r\n");
			g_tmp_buffer[1] = kUtilsOct2Value((char *)&generatedDefaults[33 +  9], 2);
			g_tmp_buffer[2] = kUtilsOct2Value((char *)&generatedDefaults[33 + 11], 2);
			g_tmp_buffer[3] = kUtilsOct2Value((char *)&generatedDefaults[33 + 13], 2);
			g_tmp_buffer[4] = kUtilsOct2Value((char *)&generatedDefaults[33 + 15], 2);
			g_tmp_buffer[5] = 0x00;//channel
			g_tmp_buffer[6] = 0x00;//opcode
			if( g_pOptDataIncomingMessageCallback ) {
				g_tmp_length = 0;
				g_tmp_buffer[7] = 0x00;//add by maozj 20200511
				g_pOptDataIncomingMessageCallback(0, 0, g_tmp_buffer + 7, &g_tmp_length);
				g_tmp_length += 7;
				g_tmp_buffer[0] = g_tmp_length - 1;
			}else {
				err = KET_ERR_INVALID_POINTER;
			}
			break;
		}
		default:
		{
			err = KET_ERR_OPRATE_ILLEGAL;
			break;
		}
	}

	if( err == KET_OK  && g_tmp_length > 1) {
		err = kOptTunnelCommonReportSend(attribute_id, g_tmp_buffer, g_tmp_length);
	}

	return err;
}

/*	DESP: Random delay trigger snapshot Report.
 * 	Auth: dingmz_frc.20191113.
 * */
void kOptTunnelMtoRRDelayRspEventHandler(void )
{
	emberEventControlSetInactive(kOptTunnelMtoRRDelayRspEventControl);
	iKonkeAfSelfPrint("####### MtoO report time(%d)\r\n", halCommonGetInt64uMillisecondTick());
	//Prevent interference when reading CMEI Or ISN Or installation code, resulting in error report
	kOptTunnelCommonReport(ECA_OPTDATA);
}

/*	DESP: private protocol command response process.
 * 	Auth: dingmz_frc.20191113.modify by maozj 20200820
 * */
void kOptTunnelMessageResponse(OptMethodEm method, uint16_t attribute_id, EmberAfStatus status, uint8_t *content, uint8_t length )
{
	extern uint16_t  appResponseLength;
	//appResponseLength = 0;
	// command ack.
	iKonkeAfSelfPrint("kOptTunnelMessageResponse ->>1 method(%d), attr(%2x), status(%d) length(%d) rspLenth(%d)\r\n",\
			method, attribute_id, status, length, appResponseLength);
	if( method == EOM_READ ) {
		emberAfPutInt8uInResp(ZCL_READ_ATTRIBUTES_RESPONSE_COMMAND_ID );
	}else {
		emberAfPutInt8uInResp(ZCL_WRITE_ATTRIBUTES_RESPONSE_COMMAND_ID);
	}

	//if( status == EMBER_ZCL_STATUS_SUCCESS ) {
		emberAfPutInt16uInResp(attribute_id);
		emberAfPutInt8uInResp(status);
		emberAfPutInt8uInResp(ZCL_CHAR_STRING_ATTRIBUTE_TYPE);

		if( content && length > 0 ) {
			emberAfPutInt8uInResp(length); 	// follow-up data stream length!

			for(int index = 0; index < (length); ++index ) {
				emberAfPutInt8uInResp(content[index]);
			}
		}else {
			emberAfPutInt8uInResp(0); //add by maozj 20191210
		}
	//}else {
		//emberAfPutInt8uInResp(status);
	//}

		iKonkeAfSelfPrint("kOptTunnelMessageResponse ->>2 method(%d), attr(%2x), status(%d) length(%d) rspLenth(%d)\r\n",\
				method, attribute_id, status, length, appResponseLength);
		emberAfSendResponse(); // sending...
		iKonkeAfSelfPrint("kOptTunnelMessageResponse ->>3 method(%d), attr(%2x), status(%d) length(%d) rspLenth(%d)\r\n",\
				method, attribute_id, status, length, appResponseLength);
}

/*	DESP: private protocol command process.
 * 	Auth: dingmz_frc.20190704.
 * */
kk_err_t kOptTunnelMessageIncoming(OptMethodEm method, uint16_t attribute_id, uint8_t *payload, uint16_t length )
{
	if( length >= OPTTUNNEL_OPTDATA_MAXLEN ) {
		return KET_ERR_INVALID_PARAM;
	}

	iKonkeAfSelfPrint("-- OT-INCOMING<method(%d):attr(%2X)>(len)%02d ------\r\n", method, attribute_id, length);
	if( payload ) {
		iKonkeAfSelfPrintBuffer(payload, length, true);
	}
	iKonkeAfSelfPrint("\r\n-------------------------\r\n");

	if( payload ) {
		memcpy(g_tmp_buffer, payload, length);
		g_tmp_length = length;
	}

	// device id
	uint8_t tmp_buffer[6];
	tmp_buffer[0] = 5;
	tmp_buffer[1] = kUtilsOct2Value((char *)&generatedDefaults[33+ 9], 2);
	tmp_buffer[2] = kUtilsOct2Value((char *)&generatedDefaults[33+11], 2);
	tmp_buffer[3] = kUtilsOct2Value((char *)&generatedDefaults[33+13], 2);
	tmp_buffer[4] = kUtilsOct2Value((char *)&generatedDefaults[33+15], 2);
	tmp_buffer[5] = EMBER_ZCL_STATUS_SUCCESS;

	switch(method) {
		case (EOM_WRITE):
		{
			switch(attribute_id) {
				case (ECA_OPTDATA):
				{
					if( length >= 7 ) {
						iKonkeAfSelfPrint("\r\n########################OPTDATA WRITE\r\n");
						if( g_pOptDataIncomingMessageCallback ) {
							g_tmp_length  = g_tmp_buffer[0] - 6;
							if(KET_NO_RESPONSE == g_pOptDataIncomingMessageCallback(g_tmp_buffer[5], g_tmp_buffer[6], g_tmp_buffer + 7, &g_tmp_length)){
								return KET_NO_RESPONSE;
							}
							g_tmp_length += 7;
							g_tmp_buffer[0] = g_tmp_length - 1;
							kOptTunnelMessageResponse(method, attribute_id, EMBER_ZCL_STATUS_SUCCESS, g_tmp_buffer, g_tmp_length);
						}
					}else {
						return KET_ERR_INVALID_PARAM;
					}

					break;
				}
				case (ECA_TTS):
				{
					kOptTunnelMessageResponse(method, attribute_id, EMBER_ZCL_STATUS_SUCCESS, tmp_buffer, 6);
					break;
				}
				case (ECA_MTORR_RR):
				{
					uint32_t rand_value = kUtilsGetRandNum(payload[0] * 1000, payload[0] * 1000 + payload[1]*1000 );

					//kOptTunnelAttrChunkLocalWrite(attribute_id, (uint8_t *)&rand_value, sizeof(uint16_t));
					emberEventControlSetDelayMS(kOptTunnelMtoRRDelayRspEventControl, rand_value);
					iKonkeAfSelfPrint("$$$$$$$$$$$$$ RSP-RANDOM: pay(%d, %d), %d\r\n", payload[0], payload[1], rand_value);
					break;
				}

				case (ECA_CMEI):
				case (ECA_ISN):
				case (ECA_CHUNK_N1):
				{
//					kOptTunnelAttrChunkLocalWrite(attribute_id, g_tmp_buffer, g_tmp_length);
//					kOptTunnelMessageResponse(method, attribute_id, EMBER_ZCL_STATUS_SUCCESS, tmp_buffer, 6);
//					iKonkeAfSelfPrint("Report OptTunnel Response attr(%2x)\r\n", attribute_id);
//					break;
				}
				case (ECA_ASSOCIATED_AOUNCE):
				case (ECA_INSTALL_CODE):
				{
					tmp_buffer[4] = EMBER_ZCL_STATUS_READ_ONLY;
					kOptTunnelMessageResponse(method, attribute_id, EMBER_ZCL_STATUS_READ_ONLY, tmp_buffer, 6);
					break;
				}
			}
			break;
		}
		case (EOM_READ ):
		{
			switch(attribute_id) {
				// OptData read command is same to write opcode 0x00, read the device snap.
				case (ECA_OPTDATA):
				{
					iKonkeAfSelfPrint("\r\n########################OPTDATA READ\r\n");
					if( g_pOptDataIncomingMessageCallback ) {
						g_tmp_buffer[1] = kUtilsOct2Value((char *)&generatedDefaults[33 +  9], 2);
						g_tmp_buffer[2] = kUtilsOct2Value((char *)&generatedDefaults[33 + 11], 2);
						g_tmp_buffer[3] = kUtilsOct2Value((char *)&generatedDefaults[33 + 13], 2);
						g_tmp_buffer[4] = kUtilsOct2Value((char *)&generatedDefaults[33 + 15], 2);
						//add by zhoubw @20200605
						g_tmp_buffer[5] = 0x00;
						g_tmp_buffer[6] = 0x00;
						g_tmp_buffer[7] = 0x00;
						//end add
						g_tmp_length = 0;
						g_pOptDataIncomingMessageCallback(0, 0, g_tmp_buffer + 7, &g_tmp_length);
						g_tmp_length += 7;
						g_tmp_buffer[0] = g_tmp_length - 1;

						kOptTunnelMessageResponse(method, attribute_id, EMBER_ZCL_STATUS_SUCCESS, g_tmp_buffer, g_tmp_length);
					}
					break;
				}
				case (ECA_TTS):
				case (ECA_MTORR_RR):
				{
					kOptTunnelMessageResponse(method, attribute_id, EMBER_ZCL_STATUS_SUCCESS, NULL, 0);
					break;
				}
				case (ECA_ASSOCIATED_AOUNCE):
				{
					EmberEUI64 u64LocalEui64 = { 0 };
					EmberNodeId NodeID = emberAfGetNodeId();

					emberAfGetEui64(u64LocalEui64);

					g_tmp_buffer[0] = (NodeID >> 8) & 0xff;
					g_tmp_buffer[1] = (NodeID >> 0) & 0xff;

					memcpy(g_tmp_buffer + 2, kUtilsReverseArrayU8int(u64LocalEui64, EUI64_SIZE, false), EUI64_SIZE);
					g_tmp_length = 10;
					kOptTunnelMessageResponse(method, attribute_id, EMBER_ZCL_STATUS_SUCCESS, g_tmp_buffer, g_tmp_length);
					break;
				}
				case (ECA_CMEI):
				case (ECA_ISN ):
				case (ECA_CHUNK_N1):
				case (ECA_INSTALL_CODE):
				{
					g_tmp_length = OPTTUNNEL_CHUNK_MAXLEN;
					iKonkeAfSelfPrint("####Report CHUNK1 Read Attr(0x%2x) length(0x%x) \r\n",attribute_id, g_tmp_length);
					kk_err_t status = kOptTunnelAttrChunkLocalRead(attribute_id, g_tmp_buffer, &g_tmp_length);
					iKonkeAfSelfPrint("####Report CHUNK2 Read Attr(0x%2x) length(0x%x) \r\n",attribute_id, g_tmp_length);
					if (status == KET_OK){
						iKonkeAfSelfPrint("#########Token Get Buffer(length(%d)): ", g_tmp_length);
						iKonkeAfSelfPrintBuffer(g_tmp_buffer, g_tmp_length, true);
						iKonkeAfSelfPrint("\r\n");
						kOptTunnelMessageResponse(method, attribute_id, EMBER_ZCL_STATUS_SUCCESS, g_tmp_buffer, g_tmp_length);
					}else {
						tmp_buffer[5] = EMBER_ZCL_STATUS_FAILURE;
						kOptTunnelMessageResponse(method, attribute_id, EMBER_ZCL_STATUS_FAILURE, tmp_buffer, 6);
					}
					break;
				}
			}
			break;
		}
		default: break;
	}

	return KET_OK;
}

/*	DESP: Private protocol message(command) reporting interface
 * 	Auth: dingmz_frc.20191113.
 * */
kk_err_t kOptTunnelOODReport(uint8_t channel, uint8_t opcode, uint8_t *arg, uint8_t length )
{
	uint8_t bufCount = 0;

	//emberAfGetCommandApsFrame()->sourceEndpoint      = 1;
	//emberAfGetCommandApsFrame()->destinationEndpoint = 1;

	g_tmp_buffer[bufCount++] = 0; // totle length
	g_tmp_buffer[bufCount++] = kUtilsOct2Value((char *)&generatedDefaults[33 +  9], 2); //basic�е�model id����ĸ��ֽ�
	g_tmp_buffer[bufCount++] = kUtilsOct2Value((char *)&generatedDefaults[33 + 11], 2);
	g_tmp_buffer[bufCount++] = kUtilsOct2Value((char *)&generatedDefaults[33 + 13], 2);
	g_tmp_buffer[bufCount++] = kUtilsOct2Value((char *)&generatedDefaults[33 + 15], 2);
	g_tmp_buffer[bufCount++] = channel;
	g_tmp_buffer[bufCount++] = opcode;

	if( arg && length > 0 ) {
		memcpy(g_tmp_buffer + bufCount, arg, length);
		bufCount += length;
	}

	g_tmp_buffer[0] = bufCount - 1;

	return kOptTunnelCommonReportSend(ECA_OPTDATA, g_tmp_buffer, bufCount);
}

/*	DESP: private clsuter protocol module init interface.
 * 	PARAM[pOptdataIncomingCallback]: private protocol optdata message incoming process callback.
 * 	Auth: dingmz_frc.20190704.
 * */
kk_err_t kOptTunnelModuleInit(pFUNC_OPTDATA_MESSAGE_CALLBACK pOptdataIncomingCallback )
{
	g_pOptDataIncomingMessageCallback = pOptdataIncomingCallback;

	return KET_OK;
}

/*	DESP: read token
 * 	Auth: maozj.20191211
 * */
EmberAfStatus kOptTunnelChunkRead(uint8_t endpoint,
                                                   EmberAfClusterId clusterId,
												   EmberAfAttributeId attributeId,
                                                   uint16_t manufacturerCode,
                                                   uint8_t *buffer,
												   uint8_t *length_out)
{
	iKonkeAfSelfPrint("&&&&&&&&&&&&&Token Read cluster(%2X), attr(%2X)\r\n", clusterId, attributeId);
//	buffer[0] = 1;
//	*length_out =  1;
//	return EMBER_ZCL_STATUS_SUCCESS;
	if (buffer == NULL){
		iKonkeAfSelfPrint("Err: Buffer is null(%d)!!\r\n");
		return EMBER_ZCL_STATUS_FAILURE;
	}

	if( clusterId == PRIV_CLUSTER_FCC0 ) {
		switch(attributeId) {
			case (OPTTUNNEL_CMEI_ID):
			{
				halCommonGetMfgToken(buffer, TOKEN_MFG_CMEI);
				//iKonkeAfSelfPrint("buffer[0] = %x\r\n", buffer[0]);
				//iKonkeAfSelfPrintBuffer(buffer, 32, true);
				break;
			}
			case (OPTTUNNEL_ISN_ID):
			{
				halCommonGetMfgToken(buffer, TOKEN_MFG_ISN);
				break;
			}
			case (OPTTUNNEL_CHUNK_N1_ID):
			{
				halCommonGetMfgToken(buffer, TOKEN_MFG_CHUNK_N1);
				break;
			}
			case (OPTTUNNEL_INSTALL_CODE_ID):
			{
				//token read
				kOptTunnelReadInstallCode(buffer, NULL);
				break;
			}
			default: return EMBER_ZCL_STATUS_FAILURE;
		}
	}
	//iKonkeAfSelfPrint("\r\nBuffer = ");
	if (buffer[0] <= OPTTUNNEL_CHUNK_MAXLEN){
		//iKonkeAfSelfPrintBuffer(buffer, buffer[0], true);
		*length_out = buffer[0];
	}else {
		*length_out = 0;
	}
	//iKonkeAfSelfPrint("\r\n");
	return EMBER_ZCL_STATUS_SUCCESS;
}

/*	DESP: write token
 * 	Auth: maozj.20191211
 * */
EmberAfStatus kOptTunnelChunkWrite(uint8_t endpoint,
                                                    EmberAfClusterId clusterId,
													EmberAfAttributeId attributeId,
                                                    uint16_t manufacturerCode,
                                                    uint8_t *buffer,
													uint8_t length)
{
	iKonkeAfSelfPrint("&&&&&&&&&&&&&Token Write(%2X, %2X)\r\n", clusterId, attributeId);

	if (length > OPTTUNNEL_CHUNK_MAXLEN){
		return EMBER_ZCL_STATUS_FAILURE;
	}

	if( clusterId == PRIV_CLUSTER_FCC0 ) {
		switch(attributeId) {
			case (OPTTUNNEL_CMEI_ID):
			{
				//halCommonSetToken(TOKEN_MFG_CMEI, buffer);
				break;
			}
			case (OPTTUNNEL_ISN_ID):
			{
				//halCommonSetToken(TOKEN_MFG_ISN, buffer);
				break;
			}
			case (OPTTUNNEL_CHUNK_N1_ID):
			{
				//halCommonSetToken(TOKEN_MFG_CHUNK_N1, buffer);
				break;
			}
			case (OPTTUNNEL_INSTALL_CODE_ID):
			default: return EMBER_ZCL_STATUS_FAILURE;
		}
	}

	return EMBER_ZCL_STATUS_SUCCESS;
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

bool kOptTunnelTrytoWriteInstallCode(void)
{
	uint16_t crc = 0xFFFF;
	uint8_t length = 6;
	//install code test
	tokTypeMfgInstallationCode install_code ;
	install_code.flags = 0x33;
	iKonkeAfSelfPrint("\r\nGet install code1111:\r\n");
	//halCommonGetToken(&install_code, TOKEN_MFG_INSTALLATION_CODE);
	kGetMfgTokenUserData(4466, &install_code, sizeof(tokTypeMfgInstallationCode));
	iKonkeAfSelfPrintBuffer((uint8_t *)&install_code, sizeof(tokTypeMfgInstallationCode), true);
	iKonkeAfSelfPrint("flg1111 = 0x%2x\r\n", install_code.flags);
	//install code is not writen, try to copy install code by custom mfg token.
	// 0 bit indicates whether it is initialized
	if (install_code.flags & 0x0001){
		tokTypeMfgInstallationCode custom_install_code ;
		halCommonGetToken(&custom_install_code, TOKEN_MFG_CUSTOM_INSTALL_CODE);
		iKonkeAfSelfPrintBuffer((uint8_t *)&custom_install_code, sizeof(tokTypeMfgInstallationCode), true);
		iKonkeAfSelfPrint("flg222 = 0x%2x\r\n", custom_install_code.flags);
#if 0
		install_code.flags = 0x0002;
		uint8_t buffer[8] = {0x12, 0x34,0x56, 0x78, 0x12, 0x34, 0x56, 0x88};
		memcpy(install_code.value, buffer, 8);
	//	install_code.crc = 0xED6F;
	//	halCommonSetToken(TOKEN_MFG_INSTALLATION_CODE, &install_code);
#else
		  // initialized
		if (!(custom_install_code.flags & 0x0001)){
			uint8_t data = custom_install_code.flags >> 1;
			if (data == 0 || data == 1 || data == 2 || data == 3){

				switch (data){
				case 0:
					length = 6;
					break;
				case 1:
					length = 8;
					break;
				case 2:
					length = 12;
					break;
				case 3:
					length = 16;
					break;
				}

				for (uint8_t index = 0; index < length; index++) {
					crc = halCommonCrc16(reverse(custom_install_code.value[index]), crc);
				}
				iKonkeAfSelfPrint("CRC length = %d crc = %2x\r\n", length, crc);
				crc = ~HIGH_LOW_TO_INT(reverse(LOW_BYTE(crc)), reverse(HIGH_BYTE(crc)));
				custom_install_code.crc = crc;
				halCommonSetToken(TOKEN_MFG_INSTALLATION_CODE, &custom_install_code);
				iKonkeAfSelfPrint("Custom mfg token flg111 = %2x crc = %02x %2x\r\n", custom_install_code.flags, crc, custom_install_code.crc);
			}else {
				iKonkeAfSelfPrint("Custom mfg token flg222 = %2x crc = %02x %2x\r\n", custom_install_code.flags, crc, custom_install_code.crc);
				return false;
			}
		}
		else {
			iKonkeAfSelfPrint("Custom mfg token flg222 = %02x\r\n", custom_install_code.flags);
			return false;
		}
#endif
		install_code.flags = 0x44;
		memset(install_code.value, 0xFF, 8);
		iKonkeAfSelfPrint("\r\nGet install code2222:\r\n");
		//halCommonGetToken(&install_code, TOKEN_MFG_INSTALLATION_CODE);
		kGetMfgTokenUserData(4466, &install_code, sizeof(tokTypeMfgInstallationCode));
		iKonkeAfSelfPrintBuffer((uint8_t *)&install_code, sizeof(tokTypeMfgInstallationCode), true);
		iKonkeAfSelfPrint("flg333 = 0x%x\r\n", install_code.flags);

	}else {
#if 0
		uint8_t code[18];
		memcpy(code, install_code.value, 8);
		code[8] = LOW_BYTE(install_code.crc);
		code[9] = HIGH_BYTE(install_code.crc);
		EmberKeyData key;
		EmberStatus status = emAfInstallCodeToKey(code, 10, &key);
		iKonkeAfSelfPrint("\r\nGet Linkkey From Install code status(%x):\r\n", status);
		iKonkeAfSelfPrintBuffer(key.contents, 16, true);
		iKonkeAfSelfPrint("\r\n");
#endif
		return false;
	}


	return true;
}

void kOptTunnelTrytoWriteModelIdenfierMfgToken(void)
{
	uint8_t buf[34];
//	uint8_t buffer[32] = {0x08, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	bool hasExistedFlg = false;
	extern uint8_t generatedDefaults[];
	halCommonGetToken(buf, TOKEN_MFG_CUSTOM_MODEL_IDENTIFIER);
	iKonkeAfSelfPrint("\r\nGet mfg model id token:");
	iKonkeAfSelfPrintBuffer((uint8_t *)buf, sizeof(buf), true);
	iKonkeAfSelfPrint("buffer[0] = 0x%x length = 0x%x\r\n", buf[0], strlen(buf));

	for (uint8_t i = 0; i < 33; i++){
		if (buf[i] != 0xFF){
			hasExistedFlg = true;
			break;
		}
	}

	if (hasExistedFlg != true){
		memcpy(buf, &generatedDefaults[33], 33);
		if (buf[0] != 0 && buf[0] != 0xFF){
			buf[33] = 0x00;
			halCommonSetToken(TOKEN_MFG_CUSTOM_MODEL_IDENTIFIER, buf);
		}
	}
}

static kk_err_t kOptTunnelCommonWriteAttributeSend(uint16_t attribute_id, uint8_t content[], uint8_t length )
{

	appZclBufferLen = 0;
	zclBufferSetup(ZCL_GLOBAL_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, PRIV_CLUSTER_FCC0, ZCL_WRITE_ATTRIBUTES_COMMAND_ID);

	zclBufferAddWord(attribute_id);
	zclBufferAddByte(ZCL_CHAR_STRING_ATTRIBUTE_TYPE);
	zclBufferAddByte(length);
	zclBufferAddBuffer(content, length);

	emAfApsFrameEndpointSetup(1, 1);
	EmberAfStatus status = emberAfSendUnicast(EMBER_OUTGOING_DIRECT, 0, &globalApsFrame, appZclBufferLen, appZclBuffer);
	iKonkeAfSelfPrint("######OPT SEND Attr(0x%2x) Time(%d)\r\n",attribute_id, halCommonGetInt64uMillisecondTick());
	cliBufferPrint();
	return ((status == EMBER_ZCL_STATUS_SUCCESS)?KET_OK:KET_ERR_UNKNOW);
}

/*	DESP: private protocol write attribute response interface.
 * 	Auth: maozj.20200822
 * */
void kOptTunnelWriteAttributeResponseToGateway(uint8_t recordedZclSeq, uint8_t attributeId, uint8_t status, uint8_t *content, uint8_t length)
{
	extern EmberApsFrame      emberAfResponseApsFrame;
	extern EmberNodeId        emberAfResponseDestination;
	extern uint16_t  appResponseLength;

	//appResponseLength = 0;
	// command ack.
	iKonkeAfSelfPrint("kOptTunnelWriteAttributeResponseToGateway zclSeq(0x%x)length(%d)\r\n", recordedZclSeq, length);
	//emberAfResponseApsFrame.profileId           = ;
	emberAfResponseApsFrame.clusterId           = PRIV_CLUSTER_FCC0;
	emberAfResponseApsFrame.sourceEndpoint      = 1;
	emberAfResponseApsFrame.destinationEndpoint = 1;
	emberAfResponseDestination = 0x0000;
	// Use the default APS options for the response, but also use encryption and
	// retries if the incoming message used them.  The rationale is that the
	// sender of the request cares about some aspects of the delivery, so we as
	// the receiver should make equal effort for the response.
	emberAfResponseApsFrame.options = EMBER_AF_DEFAULT_APS_OPTIONS;
	//emberAfResponseApsFrame.options |= EMBER_APS_OPTION_ENCRYPTION;
	//emberAfResponseApsFrame.options |= EMBER_APS_OPTION_RETRY;

	// Clear out the response buffer by setting its length to zero
	appResponseLength = 0;

	// Make the ZCL header for the response
	// note: cmd byte is set below
	uint8_t frameControl = ZCL_GLOBAL_COMMAND | ZCL_DIRECTION_CLIENT_TO_SERVER \
			| EMBER_AF_DEFAULT_RESPONSE_POLICY_RESPONSES | ZCL_DISABLE_DEFAULT_RESPONSE_MASK;

	emberAfPutInt8uInResp(frameControl);
	emberAfPutInt8uInResp(recordedZclSeq);
	emberAfPutInt8uInResp(ZCL_WRITE_ATTRIBUTES_RESPONSE_COMMAND_ID);
	//if( status == EMBER_ZCL_STATUS_SUCCESS ) {
	emberAfPutInt8uInResp(status);
	emberAfPutInt16uInResp(attributeId);
	emberAfPutInt8uInResp(ZCL_CHAR_STRING_ATTRIBUTE_TYPE);
//	emberAfPutInt8uInResp(length + 1); 	// follow-up data stream length!
//	emberAfPutInt8uInResp(length + 6 + 1); 	// follow-up data stream length!
//	emberAfPutInt8uInResp(length + 6); 	// follow-up data stream length - 1!
//	emberAfPutInt8uInResp(kUtilsOct2Value((char *)&generatedDefaults[33 +  9], 2)); //basic�е�model id����ĸ��ֽ�
//	emberAfPutInt8uInResp(kUtilsOct2Value((char *)&generatedDefaults[33 + 11], 2));
//	emberAfPutInt8uInResp(kUtilsOct2Value((char *)&generatedDefaults[33 + 13], 2));
//	emberAfPutInt8uInResp(kUtilsOct2Value((char *)&generatedDefaults[33 + 15], 2));
//	emberAfPutInt8uInResp(channel);
//	emberAfPutInt8uInResp(opcode);
	if( content && length > 0 ) {
		emberAfPutInt8uInResp(length); 	// follow-up data stream length!
		for(int index = 0; index < (length); ++index ) {
			emberAfPutInt8uInResp(content[index]);
		}
	}else {
		emberAfPutInt8uInResp(0); //add by maozj 20191210
	}
	//}else {
	//emberAfPutInt8uInResp(status);
	//}

	emberAfSendResponse(); // sending...
}

/*	DESP: Private protocol message(command) write attribute interface
 * 	Auth: maozj.20200822.
 * */
kk_err_t kOptTunnelWriteAttributeToGateway(uint8_t channel, uint8_t opcode, uint8_t *arg, uint8_t length )
{
	uint8_t bufCount = 0;

	//emberAfGetCommandApsFrame()->sourceEndpoint      = 1;
	//emberAfGetCommandApsFrame()->destinationEndpoint = 1;

	g_tmp_buffer[bufCount++] = 0; // totle length
	g_tmp_buffer[bufCount++] = kUtilsOct2Value((char *)&generatedDefaults[33 +  9], 2); //basic�е�model id����ĸ��ֽ�
	g_tmp_buffer[bufCount++] = kUtilsOct2Value((char *)&generatedDefaults[33 + 11], 2);
	g_tmp_buffer[bufCount++] = kUtilsOct2Value((char *)&generatedDefaults[33 + 13], 2);
	g_tmp_buffer[bufCount++] = kUtilsOct2Value((char *)&generatedDefaults[33 + 15], 2);
	g_tmp_buffer[bufCount++] = channel;
	g_tmp_buffer[bufCount++] = opcode;

	if( arg && length > 0 ) {
		memcpy(g_tmp_buffer + bufCount, arg, length);
		bufCount += length;
	}

	g_tmp_buffer[0] = bufCount - 1;

	return kOptTunnelCommonWriteAttributeSend(ECA_OPTDATA, g_tmp_buffer, bufCount);
}
