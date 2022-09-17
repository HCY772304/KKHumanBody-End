#ifndef __IKONKE_MODULE_SEQ_H_______________________
#define __IKONKE_MODULE_SEQ_H_______________________

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"



#define UNKNOW_NODEID						0xFFFF
#define UNKNOW_SEQUENCE						0xFF

#define DELAY_CLEAR_SEQUENCE_TIME_MS		(10 * 1000)

uint8_t kSequenceFindByNodeidAndEp(EmberNodeId nodeId, uint8_t endpoint, uint8_t *index);
void kSequenceAdd(EmberNodeId nodeId, uint8_t endpoint, uint8_t sequence);
#endif

