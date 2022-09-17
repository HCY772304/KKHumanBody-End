#ifndef __IKONKE_PRIVATE_CLUSTER_SUPPORT_H____________________________
#define __IKONKE_PRIVATE_CLUSTER_SUPPORT_H____________________________


#include "stdint.h"
#include "ikk-module-def.h"


// Private Cluster ID - FCC0
#define PRIV_CLUSTER_FCC0				0XFCC0

#define OPTTUNNEL_CHUNK_MAXLEN			32

#define OPTTUNNEL_OPTDATA_ID			ZCL_OPT_DATA_ATTRIBUTE_ID	// FCC0 Cluster Attribute 0x0000
#define OPTTUNNEL_OPTDATA_MAXLEN		128
#define OPTTUNNEL_TTS_ID				ZCL_TTS_ATTRIBUTE_ID		// FCC0 Cluster Attribute 0x0001
#define OPTTUNNEL_TTS_MAXLEN			128
#define OPTTUNNEL_MTORR_RSPRANDOM_ID	ZCL_MTO_RR_RspRandom_ATTRIBUTE_ID
#define OPTTUNNEL_ASSOCIATED_AOUNCE_ID	ZCL_AssociatedAnnounce_ATTRIBUTE_ID
#define OPTTUNNEL_ASSOCIATED_AOUNCE_MAXLEN	16
#define OPTTUNNEL_CMEI_ID				ZCL_CMEI_ATTRIBUTE_ID		// FCC0 Cluster Attribute 0x0010
#define OPTTUNNEL_CMEI_MAXLEN			OPTTUNNEL_CHUNK_MAXLEN
#define OPTTUNNEL_ISN_ID				ZCL_ISN_ATTRIBUTE_ID		// FCC0 Cluster Attribute 0x0011
#define OPTTUNNEL_ISN_MAXLEN			OPTTUNNEL_CHUNK_MAXLEN
#define OPTTUNNEL_INSTALL_CODE_ID		ZCL_InstallCode_ATTRIBUTE_ID// FCC0 Cluster Attribute 0x0012
#define OPTTUNNEL_INSTALL_CODE_MAXLEN	OPTTUNNEL_CHUNK_MAXLEN
#define OPTTUNNEL_CHUNK_N1_ID			ZCL_CHUNK_N1_ATTRIBUTE_ID	// FCC0 Cluster Attribute 0x0013
#define OPTTUNNEL_CHUNK_N1_MAXLEN		OPTTUNNEL_CHUNK_MAXLEN
#define OPTTUNNEL_UNKNOW				0xFFFF	// FCC0 Cluster Attribute UNKNOW

enum {
	ECA_OPTDATA  			= 0X0000,
	ECA_TTS		 			= 0X0001,
	ECA_MTORR_RR 			= 0X0002,
	ECA_ASSOCIATED_AOUNCE	= 0X0003,
	ECA_CMEI	 			= 0X0010,
	ECA_ISN		 			= 0X0011,
	ECA_INSTALL_CODE 		= 0X0012,
	ECA_CHUNK_N1 			= 0X0013,
	ECA_UNKNOW	 			= 0XFFFF,
};

enum OOD_GLOBAL_ERRNO {
	ERR_NO_NONE				= 0X00,
	ERR_NO_COMMAND_FORMAT 	= 0X80,
	ERR_NO_ENDPOINT 		= 0X81,
	ERR_NO_OPCODE 			= 0X82,
	ERR_NO_UNKNOW 			= 0XFF,
};

typedef enum { EOM_READ = 0, EOM_WRITE }OptMethodEm;

/*	DESP: OptData message incoming process callback..
 * 	PARAM[pframe][in/out] input command frame content, output command process result ack content.
 * 	Auth: dingmz_frc.20190701.
 * */
typedef kk_err_t (*pFUNC_OPTDATA_MESSAGE_CALLBACK)(uint8_t channel, uint8_t opcode, uint8_t args_in_out[], uint8_t *length_in_out );

/*	DESP: Private protocol common report interface.
 * 	Auth: dingmz_frc.20191113.
 * */
kk_err_t kOptTunnelCommonReport(uint16_t attribute_id );
/*	DESP: Private protocol message(command) reporting interface
 * 	Auth: dingmz_frc.20190701.
 * */
kk_err_t kOptTunnelOODReport(uint8_t channel, uint8_t opcode, uint8_t *arg, uint8_t length );
/*	DESP: private protocol command response process.
 * 	Auth: dingmz_frc.20191113.modify by maozj 20200820
 * */
void kOptTunnelMessageResponse(OptMethodEm method, uint16_t attribute_id, EmberAfStatus status, uint8_t *content, uint8_t length );
/*	DESP: private clsuter protocol module init interface.
 * 	PARAM[pOptdataIncomingCallback]: private protocol optdata message incoming process callback.
 * 	Auth: dingmz_frc.20190704.
 * */
kk_err_t kOptTunnelModuleInit(pFUNC_OPTDATA_MESSAGE_CALLBACK pOptdataIncomingCallback );

/*	DESP: check node id is or not changed
 * 	Auth: maozj.20191125.
 * */
void kOptTunneCheckNodeIdIsChangedReport(void);

/*	DESP: write token
 * 	Auth: maozj.20191211
 * */
EmberAfStatus kOptTunnelChunkWrite(uint8_t endpoint,
                                                    EmberAfClusterId clusterId,
													EmberAfAttributeId attributeId,
                                                    uint16_t manufacturerCode,
                                                    uint8_t *buffer,
													uint8_t length);
/*	DESP: read token
 * 	Auth: maozj.20191211
 * */
EmberAfStatus kOptTunnelChunkRead(uint8_t endpoint,
                                                   EmberAfClusterId clusterId,
												   EmberAfAttributeId attributeId,
                                                   uint16_t manufacturerCode,
                                                   uint8_t *buffer,
												   uint8_t *length_out);
bool kOptTunnelTrytoWriteInstallCode(void);

void kOptTunnelTrytoWriteModelIdenfierMfgToken(void);

/*	DESP: private protocol write attribute response interface.
 * 	Auth: maozj.20200822
 * */
void kOptTunnelWriteAttributeResponseToGateway(uint8_t recordedZclSeq, uint8_t attributeId, uint8_t status, uint8_t *content, uint8_t length);
/*	DESP: Private protocol message(command) write attribute interface
 * 	Auth: maozj.20200822.
 * */
kk_err_t kOptTunnelWriteAttributeToGateway(uint8_t channel, uint8_t opcode, uint8_t *arg, uint8_t length);

#endif
