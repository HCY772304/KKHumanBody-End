#ifndef __IKONKE_MODULE_DEBUG_H______________________________
#define __IKONKE_MODULE_DEBUG_H______________________________

#include "ikk-common-utils.h"

#define IKONKE_DEBUG_LOG_ENABLE		(1)	// ENABLE

#define PROMPT_DSR 	""

#if (IKONKE_DEBUG_LOG_ENABLE)
#define iKonkeAfPrint(...) 	 do { \
	emberAfPrint(0xffff, PROMPT_DSR); \
	emberAfPrint(0xffff, __VA_ARGS__); \
}while(0)

#define iKonkeAfSelfPrint(...) do { \
  emberAfPrint(0xffff, PROMPT_DSR); \
	emberAfPrint(0xffff, __VA_ARGS__);\
}while(0)
#define iKonkeAfSelfDebugExec(x) if ( emberAfPrintEnabled(0xffff) ) { x; }
#define iKonkeAfSelfPrintBuffer(buffer, len, withSpace) emberAfPrintBuffer(0xffff, (buffer), (len), (withSpace))
#define iKonkeAfSelfPrintString(buffer) emberAfPrintString(0xffff, (buffer))
#else
#define iKonkeAfSelfPrint(...)
#define iKonkeAfSelfDebugExec(x)
#define iKonkeAfSelfPrintBuffer(buffer, len, withSpace)
#define iKonkeAfSelfPrintString(buffer)
#endif


#endif
