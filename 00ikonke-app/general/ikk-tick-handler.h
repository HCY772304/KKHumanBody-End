/*
 * ikk-tick-handler.h
 *
 *  Created on: 2019Äê11ÔÂ7ÈÕ
 *      Author: dingmz
 */

#ifndef __IKONKE_MODULE_TICK_HANDLER_H____________________________
#define __IKONKE_MODULE_TICK_HANDLER_H____________________________


#include "stdint.h"
#include "ctype.h"
#include "app/framework/include/af.h"
#include "ikk-module-def.h"


#define TICK_LOOP_NMS		20	// MS

#define MS2COUNT(nms)		((nms) / TICK_LOOP_NMS)

/* DESP: tick handler continue running trigger interface.
 * Auth: dingmz_frc.20191107.
 * */
void kTickRunnningTrigger(void );


#endif
