#include <00ikonke-app/general/ikk-cluster.h>
#include <00ikonke-app/general/ikk-opt-tunnel.h>
#include "ikk-network.h"
#include "app/framework/plugin/address-table/address-table.h"
#include "app/framework/plugin/reporting/reporting.h"
#include "ikk-debug.h"
#include "ikk-module-def.h"
#include "../driver/ikk-led.h"
#include "../driver/ikk-adc.h"
#include "ikk-common-utils.h"
#include "app/framework/plugin/scenes/scenes.h"
#include "ikk-tick-handler.h"
#include "ikk-factory-test.h"
#include <00ikonke-app/ikk-user.h>

// Add by dingmz_frc.20191107.need to modify by script!!!


// the try count to poll data form parent node, failure to retrieve data overtime forces polling to stop!
#define POLL_TRY_MAXN				4
#define ENDPOINT_MAXN				4
#define CLUSTER_MAXN				11

#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
PollControlManagerSt g_stPollCtrlManager = { 0 };
#endif

void emAfPluginReportingCliClearLastReportTime(void);
void emberAfPluginReportingInitCallback(void);
extern kk_err_t kOptTunnelMessageIncoming(OptMethodEm method, uint16_t attribute_id, uint8_t *payload, uint16_t length );
static bool kZclNodeIsBindLinkageObject(EmberEUI64 eui64_id, uint16_t cluster_id, uint8_t src_endpoint );

static EmberStatus kZclClusterSendUnicastToBindingsWithCallback(EmberApsFrame *apsFrame,
                                                     uint16_t messageLength,
                                                     uint8_t* message,
                                                     EmberAfMessageSentFunction callback,
													 bool sendToGateway, bool sendToChildDevice);
static void kZclClusterRetrySendReport(EmberOutgoingMessageType type,
                            uint16_t indexOrDestination,
                            EmberApsFrame *apsFrame,
                            uint16_t msgLen,
                            uint8_t *message,
                            EmberStatus status);

extern void zclBufferSetup(uint8_t frameType, uint16_t clusterId, uint8_t commandId);
extern uint8_t * emberAfPutInt8uInResp(uint8_t value);
extern uint16_t * emberAfPutInt16uInResp(uint16_t value);

#ifdef ZCL_USING_IAS_ZONE_CLUSTER_SERVER
EmberStatus emberAfPluginIasZoneServerUpdateZoneStatus(
  uint8_t endpoint,
  uint16_t newStatus,
  uint16_t timeSinceStatusOccurredQs);
/*	DESP: report the current temperature value.
 * 	Auth: dingmz_frc.20190701.
 * */
void kMsgIASZoneStatusReport_Callback(void * );

#endif

typedef struct tag_cluster_attribute_rep {
_IO uint8_t  endpoint;
_IO uint16_t cluster_id;
_IO uint16_t attribute_id;
_IO uint8_t  mask;
}ZclAttrRepSt;

ZclAttrRepSt g_sZclAttrRept = { 0 };

typedef struct {
	ZclReportTableSt g_stZclReportPermitList[ENDPOINT_MAXN * CLUSTER_MAXN];
	uint8_t size;
}ZclClusterReportTableSt;

ZclClusterReportTableSt g_stZclClusterReportPermitTable;

// multi-control(binding linkage execute) enable flag!!!
bool g_bOnOffBindLinkageExecute = true;
bool g_bLevelControlBindLinkageExecute = true;
bool g_bWindowCoveringBindLinkageExecute = true;

//short poll or long poll flag,used to prevent a wrong trigger of human body sensor
static bool g_bPollCompleteFlg = false;


extern EmberApsFrame globalApsFrame;
extern uint8_t appZclBuffer[];
extern uint16_t appZclBufferLen;
extern EmberEUI64 g_Eui64GatewayAddr;

uint8_t g_u8Value = 0;
uint8_t g_u8Endpoint = 1;

#ifdef ZCL_USING_SCENES_CLUSTER_SERVER
static pSceneClusterRecallCallback g_pfSceneClusterRecallCallback = NULL;

void kZclClusterSceneRecallCallbackInit(pSceneClusterRecallCallback callback)
{
	g_pfSceneClusterRecallCallback = callback;
}
#endif

/*	DESP: Modified attribute reporting interface, reference from SDK callback[emberAfReportingAttributeChangeCallback].
 * 	Auth: dingmz_frc.20190701.
 * */
void kOptTunnelReportingPlagiarizeOriginal(uint8_t endpoint, uint16_t clusterId, uint16_t attributeId, uint8_t mask )
{
	extern EmAfPluginReportVolatileData emAfPluginReportVolatileData[];
	extern EmberEventControl emberAfPluginReportingTickEventControl;

	for(int index = 0; index < REPORT_TABLE_SIZE; ++index ) {
		EmberAfPluginReportingEntry entry;

		emAfPluginReportingGetEntry(index, &entry);

		if( entry.direction == EMBER_ZCL_REPORTING_DIRECTION_REPORTED
			&& entry.endpoint == endpoint
			&& entry.clusterId == clusterId
			&& entry.attributeId == attributeId
			&& entry.mask == mask
			&& entry.manufacturerCode == EMBER_AF_NULL_MANUFACTURER_CODE) {
			emAfPluginReportVolatileData[index].reportableChange = true;
			emberEventControlSetActive(emberAfPluginReportingTickEventControl);
		}
	}
}

/*	DESP: write attribute reporting interface
 * 	Auth: maozj.201200320.
 * */
void kClusterAttrWriteReport(uint8_t endpoint,
        EmberAfClusterId cluster,
        EmberAfAttributeId attributeID,
        uint8_t* dataPtr,
        EmberAfAttributeType dataType)
{
	kk_err_t err = kNwkClusterBindingObjIsOK(endpoint, cluster);
	if (err == KET_OK){
		emberAfWriteServerAttribute(endpoint, cluster, attributeID ,(uint8_t*) dataPtr,dataType);
	}else if( err == KET_ERR_NON_EXIST ) {
		kNwkClusterBindingTrigger(NULL);
	}
}
/** @brief Pre Command Received
 *
 * This callback is the second in the Application Framework's message processing
 * chain. At this point in the processing of incoming over-the-air messages, the
 * application has determined that the incoming message is a ZCL command. It
 * parses enough of the message to populate an EmberAfClusterCommand struct. The
 * Application Framework defines this struct value in a local scope to the
 * command processing but also makes it available through a global pointer
 * called emberAfCurrentCommand, in app/framework/util/util.c. When command
 * processing is complete, this pointer is cleared.
 *
 * @param cmd   Ver.: always
 */
bool emberAfPreCommandReceivedCallback(EmberAfClusterCommand* cmd )
{
#if 1
	/*begin add by maozj 20191127 add get gateway EUI64*/
	EmberNodeId sender = emberGetSender();
	// Gateway Eui64 address cache.
	EmberEUI64 eui64GatewayAddr = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	EmberEUI64 tmpEui64 = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	EmberStatus status = emberGetSenderEui64(eui64GatewayAddr);

//	iKonkeAfSelfPrint("Get EUI64 PreCommand: status(0x%x)%X%X%X%X%X%X%X%X\r\n",status, eui64GatewayAddr[7], eui64GatewayAddr[6]
//						, eui64GatewayAddr[5], eui64GatewayAddr[4], eui64GatewayAddr[3], eui64GatewayAddr[2]
//						, eui64GatewayAddr[1], eui64GatewayAddr[0]);

	//source is gateway
	if (sender == 0x0000){
		kNwkGetGateWayEui64(eui64GatewayAddr);
		//iKonkeAfSelfPrint("----xxx   getEui64FromFlash %x%x%x%x%x%x%x%x --",g_Eui64GatewayAddr[7], g_Eui64GatewayAddr[6], \
		//		g_Eui64GatewayAddr[5], g_Eui64GatewayAddr[4], g_Eui64GatewayAddr[3], g_Eui64GatewayAddr[2], g_Eui64GatewayAddr[1],g_Eui64GatewayAddr[0]);
		if(MEMCOMPARE(eui64GatewayAddr, tmpEui64, EUI64_SIZE)){
//			iKonkeAfSelfPrint("Get EUI64 PreCommand is existed\r\n");
		}else {
			EmberStatus status = emberGetSenderEui64(eui64GatewayAddr);
			if (status == EMBER_SUCCESS){
				if(kUtilsIsValidEui64Addr(eui64GatewayAddr)) {
					kNwkSetGateWayEui64(eui64GatewayAddr);
					iKonkeAfSelfPrint("Get EUI64 PreCommand: %X%X%X%X%X%X%X%X\r\n", eui64GatewayAddr[7], eui64GatewayAddr[6]
						, eui64GatewayAddr[5], eui64GatewayAddr[4], eui64GatewayAddr[3], eui64GatewayAddr[2]
						, eui64GatewayAddr[1], eui64GatewayAddr[0]);
				}
			}else {
				iKonkeAfSelfPrint("Get EUI64 failed\r\n");
			}
		}
	}
	/*end add by maozj 20191127*/
#endif

	/*begin add by maozj 20191218*/
	int8_t lastHopRssi;
	emberGetLastHopRssi(&lastHopRssi);
	NWK_DeleteRssiByNodeid(cmd->source);
	NWK_AddRssi(cmd->source, lastHopRssi);
	/*end add by maozj 20191218*/

	// @dingmz_frc.20191026. Special handling of private protocol data operations.

	switch(cmd->apsFrame->clusterId) {
		case (PRIV_CLUSTER_FCC0):
		{
			uint16_t attribute_id = U16MERG(cmd->buffer[cmd->payloadStartIndex + 1], cmd->buffer[cmd->payloadStartIndex + 0]);

			switch(cmd->commandId) {
				case (ZCL_READ_ATTRIBUTES_COMMAND_ID ):
				case (ZCL_WRITE_ATTRIBUTES_COMMAND_ID):
				{
					// Make the ZCL header for the response
					// note: cmd byte is set below
					uint8_t frameControl = (ZCL_GLOBAL_COMMAND | (cmd->direction == ZCL_DIRECTION_CLIENT_TO_SERVER
										 ? ZCL_FRAME_CONTROL_SERVER_TO_CLIENT | EMBER_AF_DEFAULT_RESPONSE_POLICY_RESPONSES
										 : ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | EMBER_AF_DEFAULT_RESPONSE_POLICY_RESPONSES));
					if( cmd->mfgSpecific ) {
						frameControl |= ZCL_MANUFACTURER_SPECIFIC_MASK;
					}

					uint8_t header_len = 3;

					emberAfPutInt8uInResp(frameControl);
					if( cmd->mfgSpecific ) {
						emberAfPutInt16uInResp(cmd->mfgCode);
						header_len += 2;
					}
					emberAfPutInt8uInResp(cmd->seqNum);

					emberAfGetCommandApsFrame()->destinationEndpoint = cmd->apsFrame->sourceEndpoint;
					emberAfGetCommandApsFrame()->sourceEndpoint = cmd->apsFrame->destinationEndpoint;

					if( cmd->commandId == ZCL_READ_ATTRIBUTES_COMMAND_ID ) {
						kOptTunnelMessageIncoming(EOM_READ , attribute_id, NULL, 0);
					}else {
						kOptTunnelMessageIncoming(EOM_WRITE, attribute_id, cmd->buffer + cmd->payloadStartIndex + 3 + 1
							, cmd->bufLen - cmd->payloadStartIndex - header_len - 1);
					}

					break;
				}
				default: break;
			}

			return true;
		}
		case (ZCL_SCENES_CLUSTER_ID):
		{

#ifdef ZCL_USING_SCENES_CLUSTER_SERVER
			if (cmd->commandId == ZCL_RECALL_SCENE_COMMAND_ID){
				uint16_t groupId = U16MERG(cmd->buffer[cmd->payloadStartIndex + 1], cmd->buffer[cmd->payloadStartIndex + 0]);
				uint8_t sceneId = cmd->buffer[cmd->payloadStartIndex + 2];
				iKonkeAfSelfPrint("####Scene Cluster groupid(0x%02x) sceneid(0x%01x)\r\n",groupId,  sceneId);
				for (uint8_t i = 0; i < EMBER_AF_PLUGIN_SCENES_TABLE_SIZE; i++) {
					EmberAfSceneTableEntry entry;
					emberAfPluginScenesServerRetrieveSceneEntry(entry, i);
					if (entry.groupId == groupId
					  && entry.sceneId == sceneId) {
						iKonkeAfSelfPrint("####Scene GroupId(0x%02x) SceneId(%01x) EP(%d)\r\n",groupId,  sceneId, entry.endpoint);
//						if (g_pfSceneClusterRecallCallback){
//							//first report to child device ,then random delay report to gateway
//							g_pfSceneClusterRecallCallback(entry.endpoint, cmd->apsFrame->clusterId, cmd->commandId, false, true, false, true);
//						}
					}
				}
			}
			else if (cmd->commandId == ZCL_ADD_SCENE_COMMAND_ID){
				uint16_t groupId = U16MERG(cmd->buffer[cmd->payloadStartIndex + 1], cmd->buffer[cmd->payloadStartIndex + 0]);
				uint8_t sceneId = cmd->buffer[cmd->payloadStartIndex + 2];
				iKonkeAfSelfPrint("####Add Scene Cluster groupid(0x%02x) sceneid(0x%01x)\r\n", groupId, sceneId);

				for (int i = 0; i < EMBER_AF_PLUGIN_SCENES_TABLE_SIZE; i++) {
					EmberAfSceneTableEntry entry;
					emberAfPluginScenesServerRetrieveSceneEntry(entry, i);
					if (cmd->apsFrame->destinationEndpoint == entry.endpoint){
						iKonkeAfSelfPrint("####Remove Scene groupid(0x%02x) sceneid(0x%01x)\r\n", entry.groupId, entry.sceneId);
						emberAfScenesClusterRemoveSceneCallback(entry.groupId, entry.sceneId);
					}
				}
			}
#endif
			break;
		}
		case ZCL_BASIC_CLUSTER_ID:
			uint16_t attributeId = U16MERG(cmd->buffer[cmd->payloadStartIndex + 1], cmd->buffer[cmd->payloadStartIndex + 0]);
			if (attributeId == ZCL_MANUFACTURER_NAME_ATTRIBUTE_ID && cmd->commandId == ZCL_WRITE_ATTRIBUTES_COMMAND_ID){
				kClusterExitFullDeviceTestCallback(cmd);

				return true;
			}else {
				return false;
			}
			break;
		default:
			break;
	}

	return false;
}

/*	DESP: get the onoff status of the specified endpoint.
 * 	Auth: dingmz_frc.20191113.
 * */
static void kZclClusterAttributeReport_Callback(void *param)
{
	kOptTunnelReportingPlagiarizeOriginal(g_sZclAttrRept.endpoint, g_sZclAttrRept.cluster_id \
		, g_sZclAttrRept.attribute_id, g_sZclAttrRept.mask);
}

/*	DESP: check whether this device is an associated multi control object of this device.
 * 	Auth: dingmz_frc.20191012.
 * */
#if 0
static bool kZclNodeIsBindLinkageObject(uint16_t node_id, uint16_t cluster_id, uint8_t src_endpoint )
{
	iKonkeAfSelfPrint("Cluster Bind Execute: node_id(%2x), cluster(%2x), src_ep(%d)\r\n", node_id, cluster_id, src_endpoint);

	// ע�⣺�豸����ָ��ǧ��Ҫת�������أ�����
	EmberEUI64 uGatewayEui64 = { 0 };

	kNwkGetGateWayEui64(uGatewayEui64);

	// if successed, send via bind record. find a binding to send on.
	for (int index = 0; index < EMBER_BINDING_TABLE_SIZE; ++index ) {
		EmberBindingTableEntry candidate = { 0 };

		EmberStatus status = emberGetBinding(index, &candidate);

		if( status == EMBER_SUCCESS ) {
			// if we can read the binding, it is unicast, the endpoint is the
			// one we want (or we have no preference) and the cluster matches
			// then use that binding to send the message
			if(	candidate.type == EMBER_UNICAST_BINDING && (src_endpoint == 0 || candidate.local == src_endpoint)
				&& candidate.clusterId == cluster_id
				&& memcmp(candidate.identifier, uGatewayEui64, EUI64_SIZE) != 0 ) {

				// ��ѯaddress table�Ƿ����Ŀ���豸��¼��������ֱ����DIRECT��ʽ���ͣ��������Ա���NWK request��
#if 0
				uint8_t index = emberAfPluginAddressTableLookupByEui64(candidate.identifier);
#endif
				uint8_t index = 0;

				if( index < EMBER_NULL_ADDRESS_TABLE_INDEX ) {
					EmberNodeId des_node_id = emberGetAddressTableRemoteNodeId(index);

					if( node_id == des_node_id ) {
						return true;
					}
				}
			}
		}
	}

	return false;
}
#else
static bool kZclNodeIsBindLinkageObject(EmberEUI64 eui64_id, uint16_t cluster_id, uint8_t src_endpoint )
{
	iKonkeAfSelfPrint("Cluster Bind check: eui64_id(%x%x%x%x%x%x%x%x), cluster(%2x), src_ep(%d)\r\n"
			, eui64_id[7], eui64_id[6], eui64_id[5], eui64_id[4], eui64_id[3], eui64_id[2], eui64_id[1], eui64_id[0]
			, cluster_id, src_endpoint);

	// if successed, send via bind record. find a binding to send on.
	for (int index = 0; index < EMBER_BINDING_TABLE_SIZE; ++index ) {
		EmberBindingTableEntry candidate = { 0 };

		EmberStatus status = emberGetBinding(index, &candidate);

		if( status == EMBER_SUCCESS ) {
			// if we can read the binding, it is unicast, the endpoint is the
			// one we want (or we have no preference) and the cluster matches
			// then use that binding to send the message
			if(	candidate.type == EMBER_UNICAST_BINDING && (src_endpoint == 0 || candidate.local == src_endpoint)
				&& candidate.clusterId == cluster_id
				&& memcmp(candidate.identifier, eui64_id, EUI64_SIZE) == 0 ) {
				return true;
			}
		}
	}

	return false;
}

/*	DESP: Check whether the binding table is bound to a non-gateway Ieee address
 * 	Note: just for the standard specification clusters.
 * 	Auth: maozj.20200511.
 * */
bool kZclNodeIsBindNotGatewayIeeeAddress(uint8_t src_endpoint, uint16_t cluster_id)
{
	iKonkeAfSelfPrint("Cluster Bind check, cluster(%2x), src_ep(%d)\r\n", cluster_id, src_endpoint);

	//add by maozj 20200511
	EmberEUI64 eui64GatewayAddr;
	extern void kNwkGetGateWayEui64(EmberEUI64 eui64);
	kNwkGetGateWayEui64(eui64GatewayAddr);

	if( kUtilsIsValidEui64Addr(eui64GatewayAddr) != true) {
		iKonkeAfSelfPrint("Gateway Ieee is invalid\r\n");
		return false;
	}
	// if successed, send via bind record. find a binding to send on.
	for (int index = 0; index < EMBER_BINDING_TABLE_SIZE; ++index ) {
		EmberBindingTableEntry candidate = { 0 };

		EmberStatus status = emberGetBinding(index, &candidate);

		if( status == EMBER_SUCCESS ) {
			// if we can read the binding, it is unicast, the endpoint is the
			// one we want (or we have no preference) and the cluster matches
			// then use that binding to send the message
			if(	candidate.type == EMBER_UNICAST_BINDING && (src_endpoint == 0 || candidate.local == src_endpoint)
				&& candidate.clusterId == cluster_id
				&& memcmp(candidate.identifier, eui64GatewayAddr, EUI64_SIZE) != 0 ) {
				return true;
			}
		}
	}

	return false;
}
#endif


uint8_t kAppZclBuffer[82];
uint16_t kAppZclBufferLen;
extern uint16_t mfgSpecificId;
extern uint8_t disableDefaultResponse;
void kZclClusterBufferSetup(uint8_t frameType, uint16_t clusterId, uint8_t commandId)
{
	  uint8_t index = 0;
	  emAfApsFrameClusterIdSetup(clusterId);
	  kAppZclBuffer[index++] = (frameType
	                           | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER
	                           | (mfgSpecificId != EMBER_AF_NULL_MANUFACTURER_CODE
	                              ? ZCL_MANUFACTURER_SPECIFIC_MASK
	                              : 0)
	                           | (disableDefaultResponse
	                              ? ZCL_DISABLE_DEFAULT_RESPONSE_MASK
	                              : 0));
	  if (mfgSpecificId != EMBER_AF_NULL_MANUFACTURER_CODE) {
		  kAppZclBuffer[index++] = (uint8_t)mfgSpecificId;
		  kAppZclBuffer[index++] = (uint8_t)(mfgSpecificId >> 8);
	  }
	  kAppZclBuffer[index++] = emberAfGetLastSequenceNumber();
	  kAppZclBuffer[index++] = commandId;
	  kAppZclBufferLen = index;
}

/*	DESP: execute the specified cluster command according to the binding table.
 * 	Note: just for the standard specification clusters.
 * 	Auth: dingmz_frc.20191012.
 * */
void kZclClusterBindLinkageExecute(uint16_t cluster_id, uint8_t command, uint8_t src_endpoint, uint8_t payload[], uint8_t length )
{
	iKonkeAfSelfPrint("Cluster Bind Execute: cluster(%2x), cmd(%x), src_ep(%d)\r\n", cluster_id, command, src_endpoint);

	// ����APS-ZCL���ģ�Ȼ�����bind�����ͣ�����
	kZclClusterBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER, cluster_id, command);
//	memcpy(appZclBuffer, payload, length);
	kAppZclBufferLen += length;
//
//	cliBufferPrint();


	// ע�⣺�豸����ָ��ǧ��Ҫת�������أ�����
	kNwkGetGateWayEui64(g_Eui64GatewayAddr);

	// if successed, send via bind record. find a binding to send on.
	for (int index = 0; index < EMBER_BINDING_TABLE_SIZE; ++index ) {
		EmberBindingTableEntry candidate = { 0 };

		EmberStatus status = emberGetBinding(index, &candidate);

		if( status == EMBER_SUCCESS ) {
#if 0
			iKonkeAfSelfPrint("bind[%x]: type(%d), cluster(%2x), des_ep(%d), src_ep(%d), desEUI64(%x%x%x%x%x%x%x%x)\r\n"
				, index, candidate.type, candidate.clusterId, candidate.remote, candidate.local
				, candidate.identifier[7], candidate.identifier[6], candidate.identifier[5], candidate.identifier[4]
				, candidate.identifier[3], candidate.identifier[2], candidate.identifier[1], candidate.identifier[0]
				);
#endif
			// if we can read the binding, it is unicast, the endpoint is the
			// one we want (or we have no preference) and the cluster matches
			// then use that binding to send the message
			if(	candidate.type == EMBER_UNICAST_BINDING && (src_endpoint == 0 || candidate.local == src_endpoint)
				/*&& candidate.clusterId == globalApsFrame.clusterId*/
				&& (candidate.clusterId  == CLUSTER_MULTI_CONTROL_ON_OFF_ID || candidate.clusterId == CLUSTER_MULTI_CONTROL_WINDOW_COVERING_OPTTUNNEL_ID)\
				&& memcmp(candidate.identifier, g_Eui64GatewayAddr, EUI64_SIZE) != 0 ) {

				iKonkeAfSelfPrint("sending to bind %d: cluster(%2x), des_ep(%d), src_ep(%d), cmd(%x), eui(%x%x%x%x%x%x%x%x)\r\n"
					, index, globalApsFrame.clusterId, candidate.remote, candidate.local, command
					, candidate.identifier[7], candidate.identifier[6], candidate.identifier[5], candidate.identifier[4]
					, candidate.identifier[3], candidate.identifier[2], candidate.identifier[1], candidate.identifier[0]
					);

				globalApsFrame.sourceEndpoint = candidate.local;
				globalApsFrame.destinationEndpoint = candidate.remote;

				// ��ѯaddress table�Ƿ����Ŀ���豸��¼��������ֱ����DIRECT��ʽ���ͣ��������Ա���NWK request��
#if 0
				uint8_t index = emberAfPluginAddressTableLookupByEui64(candidate.identifier);

				if( index < EMBER_NULL_ADDRESS_TABLE_INDEX ) {
					EmberNodeId des_node_id = emberGetAddressTableRemoteNodeId(index);
					iKonkeAfSelfPrint("999999999999999999-node_id: %2x, %x%x%x%x%x%x%x%x\r\n", des_node_id
						, candidate.identifier[7], candidate.identifier[6], candidate.identifier[5], candidate.identifier[4]
						, candidate.identifier[3], candidate.identifier[2], candidate.identifier[1], candidate.identifier[0]);
					// �̵�ַ�Ƿ���Ч��
					if((des_node_id != EMBER_UNKNOWN_NODE_ID) && (des_node_id != EMBER_DISCOVERY_ACTIVE_NODE_ID) && (des_node_id != EMBER_TABLE_ENTRY_UNUSED_NODE_ID)) {
						status = emberAfSendUnicast(EMBER_OUTGOING_DIRECT, des_node_id, &globalApsFrame, appZclBufferLen, appZclBuffer);
						iKonkeAfSelfPrint("++++++++++++++++++=send linkexec by address(%x%x%x%x%x%x%x%x:%2x) %d\r\n"
												, candidate.identifier[7], candidate.identifier[6], candidate.identifier[5], candidate.identifier[4]
												, candidate.identifier[3], candidate.identifier[2], candidate.identifier[1], candidate.identifier[0]
												, des_node_id
												, status);
					}else {
						status = EMBER_INVALID_CALL;
					}
				}else {
					status = EMBER_INVALID_CALL;
				}
#endif

#if 0
				                            EmberApsFrame *apsFrame,
				                            uint16_t messageLength,
				                            uint8_t *message);
				status = emberAfSendUnicastToEui64(candidate.identifier, &globalApsFrame, appZclBufferLen, appZclBuffer);
				iKonkeAfSelfPrint("++++++++++++++++++=send linkexec by address(%x%x%x%x%x%x%x%x:%2x) %d\r\n"
						, candidate.identifier[7], candidate.identifier[6], candidate.identifier[5], candidate.identifier[4]
						, candidate.identifier[3], candidate.identifier[2], candidate.identifier[1], candidate.identifier[0]
						,
						, status);
#endif
				//if( status != EMBER_SUCCESS ) {
					kZclClusterBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER, cluster_id, command);
					kAppZclBuffer[1] = emberAfNextSequence();
					//kAppZclBufferLen += length;
					emberAfSendUnicast(EMBER_OUTGOING_VIA_BINDING, index, &globalApsFrame, kAppZclBufferLen, kAppZclBuffer);
				//}
			}
		}
	}
	cliBufferPrint();
}

#ifdef ZCL_USING_ON_OFF_CLUSTER_SERVER

static pOnOffClusterOnOffStatusCallback g_pfOnOffStatusCallback = NULL;

/*	DESP: get the onoff status of the specified endpoint.
 * 	Auth: dingmz_frc.20191112.
 * */
OnOffStatusEnum kZclOnOffClusterServerOnOffGet(uint8_t endpoint )
{
	bool bCurrentValue = false;

	// read current on/off value
	EmberAfStatus status = emberAfReadAttribute(endpoint, ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
								(uint8_t *)&bCurrentValue, sizeof(bCurrentValue), NULL); // data type

	if( status == EMBER_ZCL_STATUS_SUCCESS ) {
		return (bCurrentValue)?EOOS_ON:EOOS_OFF;
	}

	return EOOS_UNKNOW;
}

/*	DESP: on-off cluster control operate.
 * 	Auth: dingmz_frc.20191112.
 * */
kk_err_t kZclOnOffClusterServerOnOffControl(uint8_t endpoint, OnOffCtrlEnum ctrlopt )
{
	iKonkeAfSelfPrint("OnOffControl: endpoint(%d), opt(%d)\r\n", endpoint, ctrlopt);
	g_bOnOffBindLinkageExecute = true;

	emberAfOnOffClusterSetValueCallback(endpoint, ctrlopt, false);

	g_sZclAttrRept.endpoint     = endpoint;
	g_sZclAttrRept.cluster_id   = CLUSTER_ONOFF_ID;
	g_sZclAttrRept.attribute_id = 0X0000;
	g_sZclAttrRept.mask 	    = CLUSTER_MASK_SERVER;

	if( kNwkGetCurrentStatus() == ENS_ONLINE ) {
		// check binding table
		kk_err_t err = kNwkClusterBindingObjIsOK(endpoint, CLUSTER_ONOFF_ID);
		if( err == KET_ERR_NON_EXIST ) {
			kNwkClusterBindingTrigger(kZclClusterAttributeReport_Callback);
		}
	}
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
	else if( kNwkGetCurrentStatus() == ENS_OFFLINE ) {
		kNwkRejoinMethodTrigger(kZclClusterAttributeReport_Callback, NULL);
	}
#endif

	return KET_OK;
}

/** @brief On/off Cluster Server Attribute Changed
 *
 * Server Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 */
void emberAfOnOffClusterServerAttributeChangedCallback(int8u endpoint, EmberAfAttributeId attributeId )
{
	EmberAfAttributeMetadata *metadata = NULL;
	EmberAfAttributeSearchRecord record;
	uint8_t value = 0;

	record.endpoint    = endpoint;
	record.clusterId   = ZCL_ON_OFF_CLUSTER_ID;
	record.clusterMask = CLUSTER_MASK_SERVER;
	record.attributeId = attributeId;
	record.manufacturerCode = EMBER_AF_NULL_MANUFACTURER_CODE;

	EmberAfStatus status = emAfReadOrWriteAttribute(&record, &metadata, &value, sizeof(value), false);

	iKonkeAfSelfPrint("Read-Attr[on-off]: attr_id(%2X), value(%0x)\r\n", attributeId, value);

	if( g_pfOnOffStatusCallback ) {
		g_pfOnOffStatusCallback(endpoint, (OnOffStatusEnum)value);
	}else {
		iKonkeAfSelfPrint("WARNING: OnOffSattusCallback is not registered\r\n");
	}

	// binding linkage execute trigger.

	if( g_bOnOffBindLinkageExecute){
		uint64_t u64TimeMS 	= halCommonGetInt64uMillisecondTick();
		if (u64TimeMS < 1000){
			// modify by maozj 20200229 �޸��ϵ��������������豸
			return;
		}
		//kZclClusterBindLinkageExecute(ZCL_ON_OFF_CLUSTER_ID, value?ZCL_ON_COMMAND_ID:ZCL_OFF_COMMAND_ID, endpoint, NULL, 0);
	}else {
		g_bOnOffBindLinkageExecute = true;
	}
}
//void kDelaySentMultiControlMessageEventHandler(void)
//{
//	emberEventControlSetInactive(kDelaySentMultiControlMessageEventControl);
//	kZclClusterBindLinkageExecute(ZCL_ON_OFF_CLUSTER_ID, g_u8Value?ZCL_ON_COMMAND_ID:ZCL_OFF_COMMAND_ID, g_u8Endpoint, NULL, 0);
//}
/*	DESP: on-off cluster initialization interface.
 * 	Auth: dingmz_frc.20191112.
 * */
kk_err_t kZclOnOffClusterServerInit(pOnOffClusterOnOffStatusCallback pOnOffStatusCallback )
{
	g_pfOnOffStatusCallback = pOnOffStatusCallback;

	return KET_OK;
}
#endif /* end of ZCL_USING_ON_OFF_CLUSTER_SERVER */

#ifdef ZCL_USING_OTA_BOOTLOAD_CLUSTER_CLIENT

static uint8_t g_u8UpgradeLedID = LED_UNKNOW_ID;

/*	DESP: OTA cluster client initialization interface.
 * 	Auth: dingmz_frc.20191112.
 * */
kk_err_t kZclOTAClusterClientInit(uint8_t led_id )
{
	g_u8UpgradeLedID = led_id;

	return KET_OK;
}
#endif /* end of ZCL_USING_OTA_BOOTLOAD_CLUSTER_CLIENT */

#ifdef ZCL_USING_IAS_ZONE_CLUSTER_SERVER
/*	DESP: report the current temperature value.
 * 	Auth: dingmz_frc.20190701.
 * */
static void kMsgIASZoneStatusUpdate_Callback(void *param )
{
	kZclIASZoneStatusUpdate(0, 0, true);
}

/*	DESP: IAS status report common interface.
 * 	Note: When bRetryAction == true, it means that the operation is a retransmit operation after detecting the network.
 * 	bRetryAction is not retry enable switch,bRetryAction must be false when use this function.
 * 	This time, the network state is no longer detected.
 * 	Auth: dingmz_frc.20190626.
 * */
kk_err_t kZclIASZoneStatusUpdate(uint8_t end_point, uint8_t status, bool bRetryAction )
{
	static uint8_t s_endpoint = 0, s_status = 0;
	kk_err_t err = KET_OK;

	if( bRetryAction ) {
		end_point = s_endpoint; status = s_status;
	}else {
		s_endpoint = end_point; s_status = status;


	}

	extern bool emberAfIasZoneClusterAmIEnrolled(uint8_t endpoint);
	//iKonkeAfSelfPrint("~~~~~~kZclIASZoneStatusUpdate 11: %d\r\n", emberAfIasZoneClusterAmIEnrolled(1));
	// To detect the current enroll status, we need to try enroll first without enroll!

#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
	if( (emberAfNetworkState() == EMBER_JOINED_NETWORK_NO_PARENT) \
			|| ( emberAfNetworkState() == EMBER_JOINED_NETWORK && !emberAfIasZoneClusterAmIEnrolled(1))) {
		kNwkRejoinMethodTrigger(kMsgIASZoneStatusUpdate_Callback, NULL);
		return KET_ERR_NO_PERMISSIONS;
	}
#endif

	if((emberAfNetworkState() == EMBER_JOINED_NETWORK || emberAfNetworkState() == EMBER_JOINED_NETWORK) && (!emberAfIasZoneClusterAmIEnrolled(1))) {
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
		kNwkEnrollMethodTrigger(Z3D_ENDDEVICE, kMsgIASZoneStatusUpdate_Callback);
#else
		kNwkEnrollMethodTrigger(Z3D_ROUTER, kMsgIASZoneStatusUpdate_Callback);
#endif
		return KET_ERR_NO_PERMISSIONS;
	}
	if (emberAfNetworkState() == EMBER_JOINED_NETWORK || emberAfNetworkState() == EMBER_JOINED_NETWORK){
		// check binding table
		err = kNwkClusterBindingObjIsOK(end_point, CLUSTER_IAS_ZONE_ID);
		if( err == KET_OK ) {


			if( emberAfPluginIasZoneServerUpdateZoneStatus(end_point, status, 0) != EMBER_SUCCESS ) {
				iKonkeAfSelfPrint("Err: IAS Zone status report failed!!\r\n");
				err = KET_ERR_UNKNOW;
			}
		}else if( err == KET_ERR_NON_EXIST ) {
			kNwkClusterBindingTrigger(kMsgIASZoneStatusUpdate_Callback);
		}
	}
	return err;
}
#endif

/*	DESP: Reset the reporting procedure for the specified attribute, running from the start point.
 * 	PARAM[bImmediateEffect][bool]: Is it effective immediately?
 * 	Auth: dingmz_frc.20190513.
 * */
bool kkResetReportingProcedure2StartPoint(uint8_t endpoint, uint16_t cluster_id, uint16_t attribute_id
		, bool bImmediateEffect )
{
	EmberAfPluginReportingEntry entry;
	uint8_t index = 0;

	for(index = 0; index < REPORT_TABLE_SIZE; ++index ) {
		emAfPluginReportingGetEntry(index, &entry);

		if( entry.direction == EMBER_ZCL_REPORTING_DIRECTION_REPORTED &&
			entry.endpoint == endpoint &&
			entry.clusterId == cluster_id &&
			entry.attributeId == attribute_id ) {
			break;
		}
	}

	if( index < REPORT_TABLE_SIZE ) {
		emAfPluginReportVolatileData[index].lastReportTimeMs = halCommonGetInt32uMillisecondTick();

		if( bImmediateEffect ) {
			// reset entry and immediate effect.
			if( emberAfPluginReportingConfigureReportedAttribute(&entry) != EMBER_ZCL_STATUS_SUCCESS ) {
				return false;
			}
		}
	}else return false;

	return true;
}

/*	DESP: reset all server attributes period report, force to start.
 * 	Auth: dingmz_frc.20190512.
 * */
static void kMsgSetServerAttributesReportingPeriod(void )
{
	iKonkeAfSelfPrint("Reset all Servers Reporting period.\r\n");

	// ���������ϱ���������
	emAfPluginReportingCliClearLastReportTime();
	// �������ں���Ҫˢ�¸������ϱ���ʱ��ע��˽ӿ��ڱ���ָ��reportableChangeΪfalse����Ϊtrue��ᵼ�����������ϱ���
	emberAfPluginReportingInitCallback();
}

/*	DESP: reset the reporting period of the specified attribute.
 * 	PARAM[min_interval][uint16_t]: min period to be reset.
 * 	PARAM[max_interval][uint16_t]: max period to be set.
 *	PARAM[reportable_change][uint32_t]: Set the number of analog changes that can be reported
 * 	PARAM[bReset][bool]: reset period timer to zero!
 * 	Auth: dingmz_frc.20190513.
 * */
bool kkResetReportingPeriod(uint8_t endpoint, uint16_t cluster_id, uint16_t attribute_id,
		uint16_t min_interval, uint16_t max_interval, uint32_t reportable_change, bool bReset )
{
	EmberAfPluginReportingEntry entry;
	uint8_t index = 0;

	for(index = 0; index < REPORT_TABLE_SIZE; ++index ) {
	    emAfPluginReportingGetEntry(index, &entry);

	    if( entry.direction == EMBER_ZCL_REPORTING_DIRECTION_REPORTED &&
	    	entry.endpoint == endpoint &&
	    	entry.clusterId == cluster_id &&
			entry.attributeId == attribute_id ) {
	    	break;
	    }
	}

	if( index < REPORT_TABLE_SIZE ) {
		bool _b_need_update = false;

		if( entry.data.reported.maxInterval != max_interval ) {
			// reset the repointing period.
			entry.data.reported.maxInterval  = max_interval;
			_b_need_update = true;
		}
		//add by maozj 20191213
		entry.data.reported.minInterval = min_interval;
		entry.data.reported.reportableChange = reportable_change;

		if( bReset ) { // clear the lastReportTimeMs value.
			if( _b_need_update ) {
				kkResetReportingProcedure2StartPoint(endpoint, cluster_id, attribute_id, false);
			}else {
				kkResetReportingProcedure2StartPoint(endpoint, cluster_id, attribute_id, true);
			}
		}

		if( _b_need_update ) {
			// reset entry and immediate effect.
			if( emberAfPluginReportingConfigureReportedAttribute(&entry) != EMBER_ZCL_STATUS_SUCCESS ) {
				iKonkeAfSelfPrint("Reset Reporting(%2x:%2x:%d) Period: FAIL!\r\n", cluster_id, attribute_id, max_interval);
				return false;
			}
		}

		iKonkeAfSelfPrint("Reset Reporting(%2x:%2x:%d) Period: SUCC.\r\n", cluster_id, attribute_id, max_interval);
		return true;
	}

	iKonkeAfSelfPrint("Reset Reporting(%2x:%2x) Period: no record!!\r\n", cluster_id, attribute_id);

	return false;
}


bool kkClusterGetReportingPeriod(uint8_t endpoint, uint16_t cluster_id, uint16_t attribute_id,uint16_t *min_interval, uint16_t *max_interval )
{
	EmberAfPluginReportingEntry entry;
	uint8_t index = 0;

	for(index = 0; index < REPORT_TABLE_SIZE; ++index ) {
		emAfPluginReportingGetEntry(index, &entry);

		if( entry.direction == EMBER_ZCL_REPORTING_DIRECTION_REPORTED &&
			entry.endpoint == endpoint &&
			entry.clusterId == cluster_id &&
			entry.attributeId == attribute_id ) {
			break;
		}
	}

	if( index < REPORT_TABLE_SIZE ) {
		//add by maozj 20191213
		*min_interval = entry.data.reported.minInterval;
		*max_interval = entry.data.reported.maxInterval;
		return true;
	}
	return false;
}
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
/*	DESP: trigger poll procedule based on current sleepy device status detection.
 * 	PARAM[poll_attempts] number of polls without data!
 * 	Auth: dingmz_frc.20190711. modify by maozj 20200708
 * */
void kkPollProceduleTrigger(uint8_t poll_attempts, bool bAutoAdjust )
{
	// When a sleeping device wakes up, it is necessary to force poll to actively receive messages from the parent node.
	//iKonkeAfSelfPrint("++++++++++ emberAfCurrentAppTasks() = %4x\r\n", emberAfCurrentAppTasks());
	iKonkeAfSelfPrint("$$$$$$$ kkPollProceduleTrigger times(%d), auto(%d) \r\n"
		, poll_attempts, bAutoAdjust);

	// check network status before polling.
	if( emberAfNetworkState() == EMBER_JOINED_NETWORK_NO_PARENT ) {
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
		emberAfPluginEndDeviceSupportStackStatusCallback(0);
#endif
	}

	emberAfAddToCurrentAppTasks(EMBER_AF_FORCE_SHORT_POLL);
	g_stPollCtrlManager.nodata_counter = 0;
	g_stPollCtrlManager.nodata_upperline = poll_attempts;
	g_stPollCtrlManager.bAutoAdjustUpperLine = bAutoAdjust;

	//add by maozj 20200708
	g_stPollCtrlManager.poll_check_count = MS2COUNT(poll_attempts * 1000);

	if (poll_attempts == 0xFF){
		g_stPollCtrlManager.poll_check_count = 0xFFFFFFF;
	}

	kTickRunnningTrigger();
}

/*	DESP: poll procedule completed callback interface.
 * 	Auth: dingmz_frc.20190711.
 * */
void kkPollProceduleCompletedCallback(EmberStatus status )
{
	//iKonkeAfSelfPrint("$$$$$$$$$$poll sta: status(0x%x), counter(%d), line(%d)\r\n"
	//		,status	, g_stPollCtrlManager.nodata_counter, g_stPollCtrlManager.nodata_upperline);

	if( EMBER_SUCCESS == status ) {
		g_stPollCtrlManager.nodata_counter = 0;
		if( g_stPollCtrlManager.bAutoAdjustUpperLine ) {
			g_stPollCtrlManager.nodata_upperline = POLL_TRY_MAXN;
		}

        if (emberAfGetDefaultSleepControlCallback() == EMBER_AF_OK_TO_SLEEP){
            emberAfAddToCurrentAppTasks(EMBER_AF_FORCE_SHORT_POLL);
        }
    }else {
        if( ++g_stPollCtrlManager.nodata_counter >= g_stPollCtrlManager.nodata_upperline ) {
            g_stPollCtrlManager.nodata_counter = 0;
            g_stPollCtrlManager.nodata_upperline = 0;
            g_stPollCtrlManager.bAutoAdjustUpperLine = true;
            kGetBatteryTriggerSet(0, DEFAULT_GETBATTERY_DELAY_S);
            emberAfPrint(0xffff, "INTO kGetBatteryTriggerSet 333\r\n");
            //add by maozj 20191125 enter long poll sleep mode
            emberAfAddToCurrentAppTasks(EMBER_AF_FORCE_SHORT_POLL);
            emberAfRemoveFromCurrentAppTasks(EMBER_AF_FORCE_SHORT_POLL);
        }
    }
}

bool kkPollProceduleIsGoing(void)
{
	if (g_stPollCtrlManager.poll_check_count > 0){
		return true;
	}
	return false;
}

uint32_t kkPollCheckCountdown(void)
{
	if (g_stPollCtrlManager.poll_check_count > 0){
		g_stPollCtrlManager.poll_check_count--;

	}
	return g_stPollCtrlManager.poll_check_count;
}
/*	DESP: Clear Prevent Sensor Flag interface.
 * 	Auth: maozj.20191209.
 * */
void kkClearPreventSensorFlg(void)
{
	g_bPollCompleteFlg = false;
}
/*	DESP: Get Prevent Sensor Flag interface.
 * 	Auth: maozj.20191209.
 * */
bool kkGetPreventSensorFlg(void)
{
	return g_bPollCompleteFlg;
}
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
/** @brief Poll Completed
 *
 * This function is called by the End Device Support plugin after a poll is
 * completed.
 *
 * @param status Return status of a completed poll operation Ver.: always
 */
void emberAfPluginEndDeviceSupportPollCompletedCallback(EmberStatus status)
{
	uint64_t currentTimeMS = halCommonGetInt64uMillisecondTick();
	extern SENSOR_CHECK_STATUS_E g_eSensorCheckStatus;
	extern EmberEventControl kDelaySetLedStatusEventControl;

	extern bool g_bFirstJionFlg;
	if(g_bFirstJionFlg == false){
		if (g_eSensorCheckStatus == SCS_INIT) {
			g_eSensorCheckStatus = SCS_LOCK;
			iKonkeAfSelfPrint("###Sensor Locked After Message Sent\r\n");
			iKonkeAfSelfPrint("!!!!!!!!!INTO g_eSensorCheckStatus time222(%d) !!!!!!!!!!!\r\n", halCommonGetInt64uMillisecondTick());
			emberEventControlSetDelayMS(kDelaySetLedStatusEventControl, 3.2 * 1000);
		}
	}

	kkPollProceduleCompletedCallback(status);

    kBatteryGetVoltageWithMsgSent();
    kGetBatteryTriggerSet(0, DEFAULT_GETBATTERY_DELAY_S);
    emberAfPrint(0xffff, "INTO kGetBatteryTriggerSet 444\r\n");
    iKonkeAfSelfPrint("\r\nPoll Complete time(%d) status(%d)\r\n", halCommonGetInt64uMillisecondTick(), status);
    g_bPollCompleteFlg = true;
}
#endif

#endif

/** @brief Pre ZDO Message Received
 *
 * This function passes the application an incoming ZDO message and gives the
 * appictation the opportunity to handle it. By default, this callback returns
 * false indicating that the incoming ZDO message has not been handled and
 * should be handled by the Application Framework.
 *
 * @param emberNodeId   Ver.: always
 * @param apsFrame   Ver.: always
 * @param message   Ver.: always
 * @param length   Ver.: always
 */
boolean emberAfPreZDOMessageReceivedCallback(EmberNodeId emberNodeId,
                                          EmberApsFrame* apsFrame,
                                          int8u* message,
                                          int16u length)

{

	iKonkeAfSelfPrint("ZDO Node(0x%2x) cluster(0x%2x) len(%d)",emberNodeId, apsFrame->clusterId, length);
	//Get Node Desc, identify Konke Gateway or other
	iKonkeAfSelfPrintBuffer(message, length, true);

	if (emberNodeId == 0x0000 && apsFrame->clusterId == NODE_DESCRIPTOR_RESPONSE){
		EmberStatus status = message[1];
		if (status == EMBER_SUCCESS){
			uint16_t mfgCode = message[7] | ((uint16_t)message[8] << 8);
			iKonkeAfSelfPrint("MFGCODE(%2x)\r\n",mfgCode);
			uint8_t isKonkeGateway = 1;
			if (mfgCode != EMBER_AF_MANUFACTURER_CODE){
				isKonkeGateway = 0;
			}
			halCommonSetToken(TOKEN_IS_KONKE_GATEWAY, &isKonkeGateway);
		}
	}
	return false;
}
/*	DESP: Used to Get the Konke gateway or other gateways
 * 	Auth: maozj.20191209.
 * */
bool kIsKonkeRemoteGateway(void)
{
	uint8_t isKonkeGateway = 0;
	halCommonGetToken(&isKonkeGateway, TOKEN_IS_KONKE_GATEWAY);
	return isKonkeGateway;
}

/*	DESP: heartbeat report
 * 	Auth: maozj.20191213.
 * */
void kZclClusterHeartbeatControl(uint8_t endpoint, uint16_t clusterId, uint16_t attributeId)
{
	iKonkeAfSelfPrint("kZclClusterHeartbeatControl ep(%d) cluster(%02x)\r\n", endpoint, clusterId);

	if( kNwkGetCurrentStatus() == ENS_ONLINE ) {
		// check binding table
		kk_err_t err = kNwkClusterBindingObjIsOK(endpoint, clusterId);
		if (err == KET_OK){
			//if (clusterId != ZCL_POWER_CONFIG_CLUSTER_ID){
				kOptTunnelReportingPlagiarizeOriginal(endpoint, clusterId, attributeId, CLUSTER_MASK_SERVER); //direct report; //direct report
			//}
		}else if( err == KET_ERR_NON_EXIST ) {
			kNwkClusterBindingTrigger(NULL);
		}
	}
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
	else if( kNwkGetCurrentStatus() == ENS_OFFLINE ) {
		kNwkRejoinMethodTrigger(kZclClusterAttributeReport_Callback, NULL);
	}
#endif
}

//table init
void kZclClusterPermitReportTableInit(BindObjSt clusterBindList[], uint8_t size)
{
	if (clusterBindList == NULL || size == 0 || size >= ENDPOINT_MAXN * CLUSTER_MAXN){
		iKonkeAfSelfPrint("###clusterBindList is invalid size(%d)\r\n", size);
		return;
	}

	for (uint8_t i = 0; i < size; i++){
		g_stZclClusterReportPermitTable.g_stZclReportPermitList[i].endpoint = clusterBindList[i].endpoint;
		g_stZclClusterReportPermitTable.g_stZclReportPermitList[i].cluster = clusterBindList[i].cluster;
		g_stZclClusterReportPermitTable.g_stZclReportPermitList[i].reportEnable = false;
		g_stZclClusterReportPermitTable.g_stZclReportPermitList[i].sceneRecallFlg = false;
		g_stZclClusterReportPermitTable.g_stZclReportPermitList[i].reportGatewayEnable = false;
		g_stZclClusterReportPermitTable.g_stZclReportPermitList[i].reportChildDeviceEnable = false;
	}
	g_stZclClusterReportPermitTable.size = size;
}

//index get
static uint8_t kZclClusterPermitTableIndexGetByClusterAndEp(uint8_t endpoint, EmberAfClusterId cluster)
{
	uint8_t index = 0xFF;
	for (uint8_t i = 0; i < g_stZclClusterReportPermitTable.size; i++){
		if (g_stZclClusterReportPermitTable.g_stZclReportPermitList[i].endpoint == endpoint \
				&& g_stZclClusterReportPermitTable.g_stZclReportPermitList[i].cluster == cluster){
			index = i;
			break;
		}
	}
	return index;
}
//permit info get
bool kZclClusterGetPermitReportInfo(uint8_t endpoint, EmberAfClusterId cluster, ZclReportTableSt *zclReportPermit)
{
	if (endpoint > ENDPOINT_MAXN){
		iKonkeAfSelfPrint("###11Endpoint %d is too large\r\n", endpoint);
		return false;
	}

	uint8_t index = kZclClusterPermitTableIndexGetByClusterAndEp(endpoint , cluster);
	if (index != 0xFF){
		zclReportPermit->endpoint = g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].endpoint;
		zclReportPermit->cluster = g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].cluster;
		zclReportPermit->reportEnable = g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].reportEnable;
		zclReportPermit->sceneRecallFlg = g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].sceneRecallFlg;
		zclReportPermit->reportGatewayEnable = g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].reportGatewayEnable;
		zclReportPermit->reportChildDeviceEnable = g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].reportChildDeviceEnable;
		return true;
	}

	return false;
}
//permit info set
bool kZclClusterSetPermitReportInfo(uint8_t endpoint, EmberAfClusterId cluster, bool reportEnable, bool sceneRecallFlg, bool reportGatewayEnable, bool reportChildDeviceEnable)
{
	bool ret = false;

	if (endpoint > ENDPOINT_MAXN && endpoint != 0xFF){
		iKonkeAfSelfPrint("###22Endpoint %d is too large\r\n", endpoint);
		return false;
	}

	uint8_t index = 0;
	if (endpoint != 0xFF){
		index = kZclClusterPermitTableIndexGetByClusterAndEp(endpoint , cluster);
		if (index != 0xFF){
			g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].reportEnable = reportEnable;
			g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].sceneRecallFlg = sceneRecallFlg;
			g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].reportGatewayEnable = reportGatewayEnable;
			g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].reportChildDeviceEnable = reportChildDeviceEnable;
			ret =  true;
		}
	}else { //�㲥
		for (uint8_t i = 0; i < g_stZclClusterReportPermitTable.size; i++){
			for (uint8_t j = 1; j < ENDPOINT_MAXN; j++){
				index = kZclClusterPermitTableIndexGetByClusterAndEp(j , cluster);
				if (index != 0xFF){
					g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].reportEnable = reportEnable;
					g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].sceneRecallFlg = sceneRecallFlg;
					g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].reportGatewayEnable = reportGatewayEnable;
					g_stZclClusterReportPermitTable.g_stZclReportPermitList[index].reportChildDeviceEnable = reportChildDeviceEnable;
					ret = true;
				}
			}
		}
	}

	iKonkeAfSelfPrint("######kZclClusterSetPermitReportInfo index(%d)ep(%d) cluster(%02x)enable(%d)scene(%d)gateway(%d)child(%d)\r\n",
			index, endpoint, cluster, reportEnable, sceneRecallFlg, reportGatewayEnable, reportChildDeviceEnable );
	return ret;
}
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
//isNow: is or not immediately enter sleep mode
void kkEnterSleep(bool sleep, bool isNow)
{
	//���ٽ���long poll���͹���
	if (sleep == true){
		emberAfSetDefaultSleepControlCallback(EMBER_AF_OK_TO_SLEEP);
		emberAfForceEndDeviceToStayAwake(false);
		emberAfAddToCurrentAppTasks(EMBER_AF_FORCE_SHORT_POLL);
		if (isNow == true){
			emberAfRemoveFromCurrentAppTasks(EMBER_AF_FORCE_SHORT_POLL);
		}
	}else {
		emberAfSetDefaultSleepControlCallback(EMBER_AF_STAY_AWAKE);
		emberAfForceEndDeviceToStayAwake(true); //����
	}
}
#endif
#if 0
/*	DESP: Send different data separately according to whether or not to bind multiple controllers
 * 	Auth: maozj.20200511.
 * */
void kZclClusterConditionallySendReport(uint8_t endpoint, EmberAfClusterId clusterId, bool sendToGateway, bool sendToChildDevice)
{
	// Pointer to where this API should put the length
	extern uint16_t *emAfResponseLengthPtr;

	// Pointer to where the API should put the cluster ID
	//extern EmberApsFrame *emAfCommandApsFrame ;

	EmberStatus status;
	if (emberAfIsDeviceEnabled(endpoint)
	  || clusterId == ZCL_IDENTIFY_CLUSTER_ID) {
	status = kZclClusterSendUnicastToBindingsWithCallback(emAfCommandApsFrame,
														  *emAfResponseLengthPtr,
														  emAfZclBuffer,
														  (EmberAfMessageSentFunction)(&kZclClusterRetrySendReport),
														  sendToGateway,  sendToChildDevice);

    // If the callback table is full, attempt to send the message with no
    // callback.  Note that this could lead to a message failing to transmit
    // with no notification to the user for any number of reasons (ex: hitting
    // the message queue limit), but is better than not sending the message at
    // all because the system hits its callback queue limit.
    if (status == EMBER_TABLE_FULL) {
     // emberAfSendCommandUnicastToBindings();
    	 kZclClusterSendUnicastToBindingsWithCallback(emAfCommandApsFrame,
                 	 	 	 	 	 	 	 	 	  *emAfResponseLengthPtr,
													  emAfZclBuffer, NULL,
													  sendToGateway,  sendToChildDevice);
    }

#ifdef EMBER_AF_PLUGIN_REPORTING_ENABLE_GROUP_BOUND_REPORTS
    emberAfSendCommandMulticastToBindings();
#endif // EMBER_AF_PLUGIN_REPORTING_ENABLE_GROUP_BOUND_REPORTS
  }
}

static EmberStatus kZclClusterSendUnicastToBindingsWithCallback(EmberApsFrame *apsFrame,
                                                     uint16_t messageLength,
                                                     uint8_t* message,
                                                     EmberAfMessageSentFunction callback,
													 bool sendToGateway, bool sendToChildDevice)
{
	EmberStatus status = EMBER_INVALID_BINDING_INDEX;
	uint8_t i;
	//add by maozj 20200511
	extern EmberStatus send(EmberOutgoingMessageType type,
	                        uint16_t indexOrDestination,
	                        EmberApsFrame *apsFrame,
	                        uint16_t messageLength,
	                        uint8_t *message,
	                        bool broadcast,
	                        EmberNodeId alias,
	                        uint8_t sequence,
	                        EmberAfMessageSentFunction callback);
//
//	EmberEUI64 eui64GatewayAddr;
//	extern void kNwkGetGateWayEui64(EmberEUI64 eui64);
//	kNwkGetGateWayEui64(eui64GatewayAddr);

	for (i = 0; i < EMBER_BINDING_TABLE_SIZE; i++) {
		EmberBindingTableEntry binding;
		status = emberGetBinding(i, &binding);
		if (status != EMBER_SUCCESS) {
		  return status;
		}
		if (binding.type == EMBER_UNICAST_BINDING
			&& binding.local == apsFrame->sourceEndpoint
			&& binding.clusterId == apsFrame->clusterId) {
			apsFrame->destinationEndpoint = binding.remote;
			//add by maozj 20200511, send to gateway or multi-control report to child device according to enable flag.
			if (!sendToGateway || !sendToChildDevice){
				if ((sendToGateway && !MEMCOMPARE(g_Eui64GatewayAddr, binding.identifier, EUI64_SIZE)) \
					||  (sendToChildDevice && MEMCOMPARE(g_Eui64GatewayAddr, binding.identifier, EUI64_SIZE))){
					iKonkeAfSelfPrint("\r\n###1111Send child(%d), gateway(%d)\r\n");
					//continue;
				}else {
					continue;
				}
				iKonkeAfSelfPrint("\r\n###2222Send child(%d), gateway(%d)\r\n");
			}

			status = send(EMBER_OUTGOING_VIA_BINDING,
						i,
						apsFrame,
						messageLength,
						message,
						false, // broadcast?
						0, //alias
						0, //sequence
						callback);
			if (status != EMBER_SUCCESS) {
				return status;
			}
		}
	}

  return status;
}

static void kZclClusterRetrySendReport(EmberOutgoingMessageType type,
                            uint16_t indexOrDestination,
                            EmberApsFrame *apsFrame,
                            uint16_t msgLen,
                            uint8_t *message,
                            EmberStatus status)
{
  // Retry once, and do so by unicasting without a pointer to this callback
  if (status != EMBER_SUCCESS) {
    emberAfSendUnicast(type, indexOrDestination, apsFrame, msgLen, message);
  }
}
#endif
