#include "ikk-network.h"
#include "ikk-debug.h"
#include "ikk-tick-handler.h"

#include "ikk-module-def.h"


EmberEventControl kTickEventControl;


extern void kBtnModuleActionDetectCallback(void );
extern bool kBtnModuleActionIsGoing(void );
extern void kLedModuleActionDetectCallback(void );
extern bool kLedModuleActionIsGoing(void );
extern void kNwkModuleActionDetectCallback(void );
extern bool kNwkModuleActionIsGoing(void );
extern void kRelayModuleActionDetectCallback(uint8_t poll_time);
//add by maozj 20200708
extern bool kkPollProceduleIsGoing(void);
/*	DESP: Callback processing function after successfully obtaining the gateway EUI64 address.
 * 	Auth: dingmz_frc.20191106.
 * */
void kTickEventHandler(void )
{
	emberEventControlSetInactive(kTickEventControl);

	kBtnModuleActionDetectCallback();
	kLedModuleActionDetectCallback();
	kNwkModuleActionDetectCallback();
	kRelayModuleActionDetectCallback(TICK_LOOP_NMS);

#if (Z30_DEVICE_DTYPE == Z30_DEVICE_ZED_SLEEPY)
	// As long as there is another module action task to be executed, this tick task needs to be running all the time!!
	if( kBtnModuleActionIsGoing() || kLedModuleActionIsGoing() || kNwkModuleActionIsGoing() || kkPollProceduleIsGoing()) {
		emberEventControlSetDelayMS(kTickEventControl, TICK_LOOP_NMS);
	}
#else
	emberEventControlSetDelayMS(kTickEventControl, TICK_LOOP_NMS);
#endif
}

/* DESP: tick handler continue running trigger interface.
 * Auth: dingmz_frc.20191107.
 * */
void kTickRunnningTrigger(void )
{
	if(!emberEventControlGetActive(kTickEventControl)) {
		//modify by maozj 20200303
		//emberEventControlSetDelayMS(kTickEventControl, TICK_LOOP_NMS);
		emberEventControlSetActive(kTickEventControl);
	}
}
