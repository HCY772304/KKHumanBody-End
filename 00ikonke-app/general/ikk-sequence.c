//#include "app/framework/plugin/network-steering/network-steering.h"
//#include "app/framework/plugin/network-steering/network-steering-internal.h"
#include "ikk-debug.h"
#include "ikk-sequence.h"

#define SEQUENCE_MAX_LEN		20


typedef struct {
	uint8_t endpoint;
	uint8_t sequence;
	EmberNodeId nodeId;
}SequenceSt;

//½â¾ö¶à¿Ø

SequenceSt g_stSequnceBuffer[SEQUENCE_MAX_LEN];
uint8_t g_u8SequenceLen = 0;


EmberEventControl kDelayClearSequenceEventControl;



uint8_t kSequenceFindByNodeidAndEp(EmberNodeId nodeId, uint8_t endpoint,  uint8_t *index)
{
	for (uint8_t i = 0; i < SEQUENCE_MAX_LEN; i++){
		if (g_stSequnceBuffer[i].nodeId == nodeId && g_stSequnceBuffer[i].endpoint == endpoint){
			*index = i;
//			iKonkeAfSelfPrint("\r\n#####kSequenceFindByNodeid nodeId(%02x) seq(%x) index(%d)\r\n", nodeId,  g_stSequnceBuffer[i].sequence, *index);
			return g_stSequnceBuffer[i].sequence;
		}
	}
//	iKonkeAfSelfPrint("#####2222kSequenceFindByNodeid nodeId(%02x) index(%d)\r\n", nodeId, *index);
	return UNKNOW_SEQUENCE;
}

uint8_t kSequenceFindEarliestIndex(void)
{
	uint8_t index = 0;
	uint8_t sequence = 0;
	for (uint8_t i= 0; i < SEQUENCE_MAX_LEN; i++){
		if (g_stSequnceBuffer[i].sequence != UNKNOW_SEQUENCE){
			if (sequence > g_stSequnceBuffer[i].sequence){
				sequence = g_stSequnceBuffer[i].sequence;
				index = i;
			}
		}
	}
	return index;
}

void kDelayClearSequenceEventHandler(void)
{
	emberEventControlSetInactive(kDelayClearSequenceEventControl);
	memset(g_stSequnceBuffer, 0xFF, sizeof(g_stSequnceBuffer));
	g_u8SequenceLen = 0;
}

void kSequenceAdd(EmberNodeId nodeId, uint8_t endpoint, uint8_t sequence)
{
	//gateway sender is not added
	if (nodeId == 0x0000){
		return;
	}

	if (nodeId != UNKNOW_NODEID && sequence != UNKNOW_SEQUENCE){
		uint8_t index = 0;
		if (kSequenceFindByNodeidAndEp(nodeId, endpoint, &index) != UNKNOW_SEQUENCE ){
			//replace sequence by node id, make sure one sequence one node id.
			iKonkeAfSelfPrint("\r\n#####Existed ep(%d) node id = 0x%02x sequence = 0x%x index(%d)\r\n", endpoint, nodeId, sequence, index );
			//if (sequence > 	g_stSequnceBuffer[index].sequence){
				g_stSequnceBuffer[index].nodeId = nodeId;
				g_stSequnceBuffer[index].sequence = sequence;
				g_stSequnceBuffer[index].endpoint = endpoint;
			//}
		}else {
			if (g_u8SequenceLen >= SEQUENCE_MAX_LEN){
				index = kSequenceFindEarliestIndex();
				g_stSequnceBuffer[index].nodeId = nodeId;
				g_stSequnceBuffer[index].sequence = sequence;
				g_stSequnceBuffer[index].endpoint = endpoint;
				iKonkeAfSelfPrint("\r\n nodeid is full , delete earliest node id by sequence\r\n");
			}else {
				iKonkeAfSelfPrint("#####kSequenceAdd ep(%d) nodeId(0x%02x), seq(0x%x) len(%d) \r\n", endpoint, nodeId, sequence, g_u8SequenceLen);
				g_stSequnceBuffer[g_u8SequenceLen].nodeId = nodeId;
				g_stSequnceBuffer[g_u8SequenceLen].sequence = sequence;
				g_stSequnceBuffer[g_u8SequenceLen].endpoint = endpoint;
				g_u8SequenceLen++;
			}

//			iKonkeAfSelfPrint("2222#####node id = 0x%02x sequence = 0x%x index(%d)\r\n", nodeId, sequence, index );
		}
	}

	//if (emberEventControlGetActive(kDelayClearSequenceEventControl)  != true){
		emberEventControlSetDelayMS(kDelayClearSequenceEventControl, DELAY_CLEAR_SEQUENCE_TIME_MS);
	//}
}


