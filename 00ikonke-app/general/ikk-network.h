#ifndef __IKONKE_MODULE_NETWORK_H____________________________
#define __IKONKE_MODULE_NETWORK_H____________________________

#include "stdint.h"
#include "ctype.h"
#include "app/framework/include/af.h"
#include "ikk-module-def.h"

#define REJOIN_TIME_DEBUG 			false
// auto binging objectors
#define BINDING_CLUSTER_MAXN		12
#define BINDING_EP_DEF				1	// default endpoint for binding

#define SCHEDULE_UNKNOW_ID			0	// Valid[1,254], 255 for Operate all at the same time!
#define SCHEDULE_ALLOPT_ID			255	// Valid[1,254], 255 for Operate all at the same time!

#define NWK_MIN_DELAY_TIME_MS_AFTER_POWER_ON	(500)

#define RAND_MIN_NUM							(10 * 1000) //��?D????����y,�̣�??MS
#define RAND_MAX_NUM 							(40 * 1000) //��?�䨮???����y

typedef struct tag_bind_objector_st {
	uint16_t cluster;
	uint16_t attribute; //as heartbeat report when gataway isnot kongke
	uint8_t endpoint;
}BindObjSt;

// Network status.
typedef enum {
	ENS_LEAVED = 0,				// Leaved network.
	ENS_JOINING, 			// Finding and Joining the network.
	ENS_ONLINE, 			// Joined, and connected!
	ENS_OFFLINE, 		// Joined, but disconnect to its parent!
	ENS_UNKNOW,				// UNKNOW.
}NwkStatusEnum;

typedef enum {
	EJC_JOIN_FAILED = 0,	// Join network failed!!
	EJC_JOIN_SUCCEED,		// Join network succeed.
	EJC_CONNLINK_OK,		// Join network succeed and communication link OK.
}NwkJoinCompltEnum;


extern EmberEventControl kNwkScheduleTaskEventControl[];
extern void kNwkScheduleTaskEventHandler(EmberEventControl *control);

#define IKK_NWK_SCHEDULE_EVENT_STRINGS \
  "IKK Schedule 0 NWK 0",    \
  "IKK Schedule 1 NWK 0",    \
  "IKK Schedule 2 NWK 0",    \
  "IKK Schedule 3 NWK 0",
#define IKK_NWK_SCHEDULE_EVENTS \
  { &kNwkScheduleTaskEventControl[0], (void(*)(void))kNwkScheduleTaskEventHandler }, \
  { &kNwkScheduleTaskEventControl[1], (void(*)(void))kNwkScheduleTaskEventHandler }, \
  { &kNwkScheduleTaskEventControl[2], (void(*)(void))kNwkScheduleTaskEventHandler }, \
  { &kNwkScheduleTaskEventControl[3], (void(*)(void))kNwkScheduleTaskEventHandler }, \


// Network status notify callback
typedef void (*pNetworkStatusNotifyCallback)(NwkStatusEnum );
// Joining procedure complete result callback
typedef void (*pJoinCompleteCallback)(NwkJoinCompltEnum );
//
typedef void (*pScheduleTaskHandlerCallback)(uint8_t schedule_id );
//add by maozj 20200511
typedef void (*pNetworkExitCompleteCallback)(void);
typedef void (*pNetworkStopTaskCallback)(uint8_t schedule_id );
/*	DESP: The interface of resource release and reset after the device is restored to the factory, which is executed when the network is removed.
 * 	Auth: dingmz_frc.20191106.
 * */
void kNwkFactoryReset(bool is_nwk_indicator);

/*	DESP: Check whether the specified cluster is binded.
 * 	RETURN[KET_OK]: is binded!
 * 	RETURN[KET_ERR_NON_EXIST]: is unbinded!
 * 	RETURN[KET_ERR_NO_PERMISSIONS]: this binding item is not configured!
 * 	Auth: dingmz_frc.20191112.
 * */
kk_err_t kNwkClusterBindingObjIsOK(uint8_t endpoint, uint16_t cluster );
/*	DESP: Traverse print the cluster binding table content.
 * 	Auth: dingmz_frc.20191106.
 * */
void kNwkClusterBindingTableTraverse(void );
/*	DESP: steering action trigger interface: eg. start or stop.
 * 	Note: The joining procedure will be triggered only when the network is not connected.
 * 	Auth: dingmz_frc.20190627.
 * */
void kNwkJoiningStart(uint32_t u32JoiningDuration/* MS */, pJoinCompleteCallback callback );

/*	DESP: start the scheduled task with specified ID.
 * 	Auth: dingmz_frc.20191111.
 * */
void kNwkScheduleTaskStart(uint8_t schedule_id );
/*	DESP: stop the scheduled task with specified ID.
 * 	Auth: dingmz_frc.20191111.
 * */
void kNwkScheduleTaskStop(uint8_t schedule_id );
/*	DESP: device online planning task processing interface.
 * 	Note: automatically start running after registering the scheduled task by default!
 * 	Auth: dingmz_frc.20191111.
 * */
kk_err_t kNwkScheduleTaskRegister(uint8_t schid, uint32_t u32PeroidIntervalMS, pScheduleTaskHandlerCallback pfScheduleCallback );
/*	DESP: network module initialize interface.
 * 	PARAM[item_list] the cluster objects requiring auto binding.
 * 	PARAM[nwk_led_id] Associate a light to indicate network status:
 * 		S1[offline] led off;
 * 		S2[joining]	led blink;
 * 		S3[online] led on for route device ,and led off for sleepy device.
 * 		Note: If this parameter is LED_UNKNOW_ID, it means that the network indicator is not associated!!
 * 	PARAM[nwk_button_id] Associate a button for network operation.
 * 	Auth: dingmz_frc.20191106. modify by maozj 20200325
 * */
void kNwkModuleInit(BindObjSt cluster_obj_list[], uint8_t cluster_obj_num, pNetworkStatusNotifyCallback pNSCallback
		, pNetworkExitCompleteCallback pNECallback, pNetworkStopTaskCallback pTaskStopCallback
		, uint32_t join_nwk_time, uint8_t nwk_led_id, uint8_t nwk_button_id );

/*	DESP: get the current network status.
 * 	Auth: dingmz_frc.20191109.
 * */
NwkStatusEnum kNwkGetCurrentStatus(void );
/*	DESP: Get the gateway`s address from flash token.
 * 	Auth: dingmz_frc.20191111.
 * */
void kNwkGetGateWayEui64(EmberEUI64 eui64);

kk_err_t kNwkEnrollMethodTrigger(Z3DevTypeEm dev_type, pFUNC_VOID pEnrolledCallback );

/*	DESP: Get whether  network is exiting
 * 	Auth: maozj.20200407* */
bool kNwkIsExiting(void);

/*	DESP: Rejoin procedure method trigger interface for ZED device.
 * 	Auth: dingmz_frc.20190701.modify by maozj 20200706
 * */
kk_err_t kNwkRejoinMethodTrigger(pFUNC_VOID pRejoinedCallback, void *param);
#endif
