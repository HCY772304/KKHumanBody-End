#ifndef __IKONKE_MODULE_ZCL_H________________________________
#define __IKONKE_MODULE_ZCL_H________________________________


#include "stdint.h"
#include "ctype.h"
#include "app/framework/include/af.h"
#include "ikk-module-def.h"

#include "ikk-network.h"

#include "KKTemplateCodeProject.h"

// sleepy device poll procedule control manager
typedef struct tag_poll_control_manager {
	uint8_t nodata_counter;
	uint8_t nodata_upperline;
	bool bAutoAdjustUpperLine;
	uint32_t poll_check_count;
}PollControlManagerSt;

typedef struct tag_zcl_report_table{
	uint8_t endpoint;
	uint16_t cluster;
	bool reportEnable;
	bool sceneRecallFlg;
	bool reportGatewayEnable;
	bool reportChildDeviceEnable;
}ZclReportTableSt;

typedef enum{ EOOC_OFF = 0, EOOC_ON, EOOC_TOGGLE, EOOC_UNKNOW }OnOffCtrlEnum;
typedef enum{ EOOS_OFF = 0, EOOS_ON, EOOS_UNKNOW }OnOffStatusEnum;



#ifdef ZCL_USING_IAS_ZONE_CLUSTER_SERVER
typedef enum{
	BIT_INIT=0,\
	ALARM1=0, ALARM2, TAMPER, BATTERY, SUPERVISION_REPORT, \
	RESTOR_REPORT, TROUBLE, AC_MAINS, TEST, BATTRY_DEFECT
}IasZoneStatusBitEnum;
#endif

typedef void (*pOnOffClusterOnOffStatusCallback)(uint8_t endpoint, OnOffStatusEnum estatus );
typedef void (*pSceneClusterRecallCallback)(uint8_t endpoint,  uint16_t cluster, uint8_t command_id
		, bool reportEnable, bool sceneRecallFlg, bool reportGatewayEnable, bool reportChildDeviceEnable);
typedef void (*pLevelControlClusterCurrentLevelStatusCallback)(uint8_t endpoint, uint8_t estatus);

/*	DESP: get the onoff status of the specified endpoint.
 * 	Auth: dingmz_frc.20191112.
 * */
OnOffStatusEnum kZclOnOffClusterServerOnOffGet(uint8_t endpoint );
/*	DESP: update(write) the onoff cluster onoff attribute value.
 * 	Auth: dingmz_frc.20191112.
 * */
kk_err_t kZclOnOffClusterServerOnOffControl(uint8_t endpoint, OnOffCtrlEnum ctrlopt );
/*	DESP: on-off cluster initialization interface.
 * 	Auth: dingmz_frc.20191112.
 * */
kk_err_t kZclOnOffClusterServerInit(pOnOffClusterOnOffStatusCallback pOnOffStatusCallback );

/*	DESP: OTA cluster client initialization interface.
 * 	Auth: dingmz_frc.20191112.
 * */
kk_err_t kZclOTAClusterClientInit(uint8_t led_id );

/*	DESP: IAS status report common interface.
 * 	Note: When bRetryAction == true, it means that the operation is a retransmit operation after detecting the network.
 * 	This time, the network state is no longer detected.
 * 	Auth: dingmz_frc.20190626.
 * */
kk_err_t kZclIASZoneStatusUpdate(uint8_t end_point, uint8_t status, bool bRetryAction );

/*	DESP: Reset the reporting procedure for the specified attribute, running from the start point.
 * 	PARAM[bImmediateEffect][bool]: Is it effective immediately?
 * 	Auth: dingmz_frc.20190513.
 * */
bool kkResetReportingProcedure2StartPoint(uint8_t endpoint, uint16_t cluster_id, uint16_t attribute_id
	, bool bImmediateEffect );
/*	DESP: reset all server attribute period report, force to start.
 * 	Auth: dingmz_frc.20190512.
 * */
void kMsgSetServerAttributesReportingPeriod(void );

/*	DESP: reset the reporting period of the specified attribute.
 * 	PARAM[min_interval][uint16_t]: min period to be reset.
 * 	PARAM[max_interval][uint16_t]: max period to be set.
 *	PARAM[reportable_change][uint32_t]: Set the number of analog changes that can be reported
 * 	PARAM[bReset][bool]: reset period timer to zero!
 * 	Auth: dingmz_frc.20190513.
 * */
bool kkResetReportingPeriod(uint8_t endpoint, uint16_t cluster_id, uint16_t attribute_id,
		uint16_t min_interval, uint16_t max_interval, uint32_t reportable_change, bool bReset );

bool kkClusterGetReportingPeriod(uint8_t endpoint, uint16_t cluster_id, uint16_t attribute_id,uint16_t *min_interval, uint16_t *max_interval );
/*	DESP: trigger poll procedule based on current sleepy device status detection.
 * 	PARAM[poll_attempts] number of polls without data!
 * 	Auth: dingmz_frc.20190711.
 * */
void kkPollProceduleTrigger(uint8_t poll_attempts, bool bAutoAdjust );
/*	DESP: poll procedule completed callback interface.
 * 	Auth: dingmz_frc.20190711.
 * */
void kkPollProceduleCompletedCallback(EmberStatus status );

/*	DESP: prevent sensor flg clear when short poll or long poll.
 * 	Auth: maozj.20191209.
 * */
void kkClearPreventSensorFlg(void);
/*	DESP: prevent sensor flg get when short poll or long poll.
 * 	Auth: maozj.20191209.
 * */
bool kkGetPreventSensorFlg(void);

/*	DESP: Get Remote Gataway Type.
 * 	Auth: maozj.20191209.
 * */
bool kIsKonkeRemoteGateway(void);

/*	DESP: heartbeat report
 * 	Auth: maozj.20191213.
 * */
void kZclClusterHeartbeatControl(uint8_t endpoint, uint16_t clusterId, uint16_t attributeId);
void kOptTunnelReportingPlagiarizeOriginal(uint8_t endpoint, uint16_t clusterId, uint16_t attributeId, uint8_t mask );
#ifdef ZCL_USING_SCENES_CLUSTER_SERVER
void kZclClusterSceneRecallCallbackInit(pSceneClusterRecallCallback callback);
#endif

//table init
void kZclClusterPermitReportTableInit(BindObjSt clusterBindList[], uint8_t size);

bool kZclClusterGetPermitReportInfo(uint8_t endpoint, EmberAfClusterId cluster, ZclReportTableSt *zclReportPermit);

bool kZclClusterSetPermitReportInfo(uint8_t endpoint, EmberAfClusterId cluster, bool reportEnable, bool sceneRecallFlg
					, bool reportGatewayEnable, bool reportChildDeviceEnable);
#if 0
/*	DESP: Send different data separately according to whether or not to bind multiple controllers
 * 	Auth: maozj.20200511.
 * */
void kZclClusterConditionallySendReport(uint8_t endpoint, EmberAfClusterId clusterId, bool sendToGateway, bool sendToChildDevice);
#endif
/*	DESP: Check whether the binding table is bound to a non-gateway Ieee address
 * 	Note: just for the standard specification clusters.
 * 	Auth: maozj.20200511.
 * */
bool kZclNodeIsBindNotGatewayIeeeAddress(uint8_t src_endpoint, uint16_t cluster_id);
#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED || Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
void kkEnterSleep(bool sleep, bool isNow);
#endif
#endif
