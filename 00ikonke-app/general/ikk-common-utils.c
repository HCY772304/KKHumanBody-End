#include <string.h>

#include "ikk-common-utils.h"
#include "ikk-debug.h"
#include "ikk-module-def.h"

#define NODE_MAX_NUM  		64
#define NODE_ID_DEFAULT		0xFFFF
#define RSSI_DEFAULT		(-127)

typedef struct {
	uint16_t node_id;
	int8_t rssi;
}NODE_RSSI;
typedef struct NetworkTableForRssi{
	NODE_RSSI nodeRssi[NODE_MAX_NUM];
	uint8_t len;
}NWK_RSSI;

static NWK_RSSI nwkRssi;

static void NWK_PopRssi(NODE_RSSI *node_rssi, uint8_t position);

static const DataInfoSt s_datatype_def[] = STATIC_DATA_INFO_DEF;

// global common buffer.
uint8_t g_u8GlobalBufferN1[GLOBAL_BUFFER_SIZE] = { 0 }, g_u8GlobalBufferN2[GLOBAL_BUFFER_SIZE] = { 0 };
uint8_t g_u8GlobalLengthN1 = 0, g_u8GlobalLengthN2 = 0;

// Reverse the bits in a byte
static uint8_t reverse(uint8_t b)
{
#if defined(EZSP_HOST) || defined(BOARD_SIMULATION)
  return ((b * 0x0802UL & 0x22110UL) | (b * 0x8020UL & 0x88440UL)) * 0x10101UL >> 16;
#else
  return (__RBIT((uint32_t)b) >> 24); // Cortex function __RBIT uses uint32_t
#endif // EZSP_HOST
}

/*	DESP: Gets the byte length of the specified data type.
 * 	Auth: dingmz_frc.20191025.
 * */
uint8_t kUtilsGetDataLength(uint8_t type )
{
	for(int index = 0; index < (sizeof(s_datatype_def) / sizeof(DataInfoSt)); ++index ) {
		if( s_datatype_def[index].datatype == type ) {
			return s_datatype_def[index].length;
		}
	}

	return 0;
}

bool getEndDeviceRssiByNodeid(uint16_t node_id, int8_t *rssi, uint8_t *index)
{
	int8_t i = 0;
	if (node_id != 0){
		//emberAfCorePrintln("start search node_id");
		for (i = NODE_MAX_NUM - 1; i >= 0; i--){
			//emberAfCorePrintln("%d ", nwkRssi.nodeRssi[i].node_id);
			if (nwkRssi.nodeRssi[i].node_id == node_id){ //从最后开始找最新的记录
				*rssi = nwkRssi.nodeRssi[i].rssi;
				*index = i;
				return true;
			}
		}
	}
	return false;
}

void NWK_AddRssi(uint16_t node_id, int8_t rssi)
{
	NODE_RSSI *node_rssi_tmp = NULL;
	//uint8_t index = 0;

	if(nwkRssi.len >=  NODE_MAX_NUM){   //满了之后， 删除第一个
		NWK_PopRssi(node_rssi_tmp, 0);
	}

	if(nwkRssi.len < NODE_MAX_NUM)
	{
		nwkRssi.nodeRssi[nwkRssi.len].node_id = node_id;
		nwkRssi.nodeRssi[nwkRssi.len].rssi = rssi;
#if 0
		emberAfCorePrintln("nwkRssi.len %d  node_id %02x rssi = %d",
				nwkRssi.len, nwkRssi.nodeRssi[nwkRssi.len].node_id, nwkRssi.nodeRssi[nwkRssi.len].rssi);
#endif
		nwkRssi.len++;
	}
}

void NWK_PopRssi(NODE_RSSI *node_rssi, uint8_t position)
{
	uint8_t i;
	if(nwkRssi.len)
	{
		node_rssi->node_id = nwkRssi.nodeRssi[position].node_id;
		node_rssi->rssi = nwkRssi.nodeRssi[position].rssi;
		nwkRssi.len--;
		for (i = position; i < nwkRssi.len; i++)
		{
			nwkRssi.nodeRssi[i].node_id = nwkRssi.nodeRssi[i+1].node_id;
			nwkRssi.nodeRssi[i].rssi = nwkRssi.nodeRssi[i+1].rssi;
		}
	}
}

void NWK_ClearRssi(void)
{
	nwkRssi.len = 0;
	for (uint8_t i = 0; i < NODE_MAX_NUM; i++)
	{
		nwkRssi.nodeRssi[i].node_id = NODE_ID_DEFAULT;
		nwkRssi.nodeRssi[i].rssi = RSSI_DEFAULT;
	}
	//emberAfCorePrintln(".........Clear RF_CMD..............\n");
}

bool NWK_DeleteRssiByNodeid(uint16_t node_id)
{
	int8_t rssi = RSSI_DEFAULT;
	//uint8_t i = 0;
	uint8_t index = 0;
	NODE_RSSI *node_rssi_tmp = NULL;
	if (getEndDeviceRssiByNodeid(node_id, &rssi, &index) == true){
		NWK_PopRssi(node_rssi_tmp, index);
		return true;
	}
	return false;
}

/*	DESP: Check whether the given string is a number.
 * 	Auth: dingmz_frc.20191023.
 * */
bool kUtilsIsNumber(char *string )
{
	if( NULL == string ) return false;

	int length = strlen(string);

	if( length <= 0 ) return false;

	for(int index = 0; index < length; ++index ) {
		if( !isdigit(string[index])) {
			if((length > 1) && (index == 0) && (string[index] == '-' || string[index] == '+')) {
				// do nothing.
			}else {
				return false;
			}
		}
	}

	return true;
}

/*	DESP: Check whether the given string is OCT string.
 * 	Auth: dingmz_frc.20191023.
 * */
bool kUtilsIsOct(char *string )
{
	if( NULL == string ) return false;

	int length = strlen(string);

	if((length > 0) || ((length % 2) == 0)) {
		for(int index = 0; index < length; ++index ) {
			if(	(string[index] >= '0' && string[index] <= '9') || (string[index] >= 'a' && string[index] <= 'f') ||
				(string[index] >= 'A' && string[index] <= 'F')) {
				// do nothing.
			}else {
				return false;
			}
		}
	}else {
		return false;
	}

	return true;
}

/*	DESP: Gets the value[in 64-bits] of the specified length from the specified OCT string.
 * 	Note: expected output value max 64bits, so max length input is 16!!!
	Auth: dingmz_frc.20191018.
*/
uint64_t kUtilsOct2Value(char *oct, int length/* ≤ 16 */ )
{
	char temp_string[(sizeof(uint64_t) << 1) + 1] = { 0 };
	uint64_t u64ResultValue = 0;
	int orgslen = strlen(oct);

	// Check whether the string length is less than the specified length. If it is less than the specified length, high-order complement is required.
	memset(temp_string, '0', sizeof(temp_string));

	if( orgslen < length ) {
		memcpy(temp_string + (length - orgslen), oct, orgslen);
	}else {
		memcpy(temp_string, oct, length);
	}

	//iKonkeAfSelfPrint("kUtilsOct2Value:::::[%s][%d]\r\n", temp_string, length);
    for(int i = 0; i < length; ++i ) {
        if( tolower(oct[i]) > '9') {
        	u64ResultValue = 16 * u64ResultValue + (10 + tolower(temp_string[i]) - 'a');
        } else{
        	u64ResultValue = 16 * u64ResultValue + (tolower(temp_string[i]) - '0');
        }
    }

    return u64ResultValue;
}



/*	DESP: 高精度随机值接口，取值[minNum, maxNum].
	Auth: dingmz_frc.20190701.
*/
uint32_t kUtilsGetRandNum(uint32_t minNum, uint32_t maxNum )
{
	if( minNum >= maxNum ) {
		return 0;
	}

	static uint32_t s_counter = 0;
	//halCommonGetInt32uMillisecondTick();

	uint8_t u8LocalEui64[8] = { 0 };

	emberAfGetEui64(u8LocalEui64);
	++s_counter;

	srand(s_counter * s_counter + u8LocalEui64[0] + (emberAfGetNodeId() & 0x0567)); //提供随机种子
	uint32_t randNum  =  minNum + rand() % (maxNum - minNum + 1);

	iKonkeAfSelfPrint(" xxx rand num = %d\r\n", randNum);

	return randNum;
}

/*	DESP: check whether the data set content is specified byte data!
 * 	Auth: dingmz_frc.20190520.
 * */
bool kUtilsCheckDataSetContext(void *data_set, uint8_t spec_v, int length )
{
	if( NULL == data_set ) {
		return false;
	}

	uint8_t *data_v = (uint8_t *)data_set;

	for(int index = 0; index < length; ++index ) {
		if( data_v[index] != spec_v ) {
			return false;
		}
	}

	return true;
}

/*	DESP: Convert OCT string to hex data stream.
	Auth: dingmz_frc.20191018.
*/
int kUtilsStr2Hex(char *octstr, unsigned char *phexout, int max_length )
{
	if( NULL == octstr || NULL == phexout ) {
		return -1;
	}

	int str_len = strlen(octstr), iox = 0;

	for(int index = 0; index < str_len; index += 2 ) {
		phexout[iox++] = (uint8_t)kUtilsOct2Value(octstr + index, 2);

		if( iox >= max_length ) {
			break;
		}
	}

	return iox;
}

/*	DESP: 64bits大小端转换.
	Auth: dingmz_frc.20180627.
*/
uint64_t __CPU_ENDIAN_HTONLL__(uint64_t u64_host )
{
	unsigned int u32_host_l = (u64_host >>  0) & 0xffffffff;
	unsigned int u32_host_h = (u64_host >> 32) & 0xffffffff;

	return ((((uint64_t)NT32(u32_host_l)) << 32 ) |
		NT32(u32_host_h));
}

/*	DESP: Check if it is a valid long address.
 * 	Auth: dingmz_frc.20191106.
 * */
bool kUtilsIsValidEui64Addr(EmberEUI64 ineui64addr )
{
	return !kUtilsCheckDataSetContext(ineui64addr, 0xFF, EUI64_SIZE);
}

/*	DESP: get the crc16 value of specific InstallCode.
 * 	Auth: dingmz_frc.20191007.
 * */
uint16_t kUtilsInstallCodeX25CRC16(uint8_t* u8InstallCode, uint8_t length )
{
  uint16_t crc = 0xFFFF;
  uint8_t index;

  if( NULL == u8InstallCode ) {
	  return EMBER_BAD_ARGUMENT;
  }

  // Compute the CRC and verify that it matches.  The bit reversals, byte swap,
  // and ones' complement are due to differences between halCommonCrc16 and the
  // Smart Energy version.
  for(index = 0; index < length; ++index ) {
	  crc = halCommonCrc16(reverse(u8InstallCode[index]), crc);
  }

  crc = ~HIGH_LOW_TO_INT(reverse(LOW_BYTE(crc)), reverse(HIGH_BYTE(crc)));

  return crc;
}

/*	DESP: CRC16 algorithm.
 * 	Auth: dingmz_frc.20191018.
 * */
uint16_t kUtilsCRC16Sum(const unsigned char *msg, unsigned int length )
{
	unsigned short CRC = 0xFFFF;
	int i;

	while (length--) {
		CRC ^= *msg++;
		for (i = 0; i < 8; ++i )  {
			CRC=(CRC & 0x0001)?((CRC>>1) ^0xA001):(CRC>>1);
		}
	}

	return CRC;
}

/*	DESP: message format parse, String Segmentation parse into Substrings.
	Auth: dingmz_frc.20180607.
*/
int	kUtilsStringSplit(char *string, char (*item)[STRING_ITEM_SIZE_MAX], char spec/*分隔符*/)
{
	if( NULL == string ) return 0;

	uint8_t idex, item_idex = 0, cp_idex = 0;
	char tmp_char;

	// Parse...
	for(idex = 0; idex < strlen(string); ++idex) {
		tmp_char = string[idex];
		if( tmp_char == spec ) {
			item[item_idex][cp_idex] = '\0';
			if( ++item_idex >= STRING_ITEM_NUM_MAX)
				return -1;
			cp_idex = 0;
		}else {
			item[item_idex][cp_idex] = tmp_char;
			if( ++cp_idex >= STRING_ITEM_SIZE_MAX)
				return -2;
		}
	}

	item[item_idex][cp_idex] = '\0';

	return item_idex + 1;
}


/*	DESP: Reverse uint8 array contents.
 * 	PARAM[bModifyOriginalData]: Note that this interface modifies the original data content When bModifyOriginalData is true!!!
	Auth: dingmz_frc.20191019.
*/
uint8_t *kUtilsReverseArrayU8int(uint8_t *org_data, int length, bool bModifyOriginalData )
{
	static uint8_t s_image_data[16] = { 0 };

	if((NULL == org_data || length <= 0) || length > sizeof(s_image_data)) {
		return NULL;
	}

	for(int index = 0; index < length; ++index ) {
		s_image_data[index] = org_data[length - 1 - index];
	}

	if( bModifyOriginalData ) {
		memcpy(org_data, s_image_data, length);
	}

	return s_image_data;
}


static void kArraySort(uint32_t *buf, uint8_t size)
{
	uint8_t i, j;
	uint32_t t;
	uint32_t *tmpBuf = buf;
//	if (size > 20) {
//		size = 20;
//	}
	//memcpy(tmpBuf, buf, size * sizeof(uint32_t));

	for (j = 0; j< size;j++) {
		for(i = 0;i < size-1-j; i++){
			if(tmpBuf[i] > tmpBuf[i+1]) /* 由小到大,由大到小时改为< */{
				t = tmpBuf[i];
				tmpBuf[i] = tmpBuf[i+1];
				tmpBuf[i+1] = t;
			}
		}
	}
	//memcpy(buf, tmpBuf, size * sizeof(uint32_t));
}
/*
对一个buffer数据进行排序去除最大的和最小的滤波个数取平均值
filter_num 滤波最大和最小的个数， 相等的
*/
uint32_t kUtilsGetAverage(uint32_t *buf, uint8_t len, uint8_t filter_num) //计算平均值
{
	uint64_t temp_val = 0;
	uint8_t i;
	if (len < 2){
		return 0;
	}
//	iKonkeAfSelfPrint("##kUtilsGetAverage11 len(%d) filterNum(%d)\r\n", len, filter_num);
	if (len <= filter_num * 2){
		filter_num = 0;
	}
//	iKonkeAfSelfPrint("##kUtilsGetAverage22 len(%d) filterNum(%d)\r\n", len, filter_num);
//	iKonkeAfSelfPrint("\r\nbefore average = ");
	for (int8_t i= 0; i < len; i++){
//		iKonkeAfSelfPrint("%d ", buf[i]);
	}
	kArraySort(buf, len); //排序
//	iKonkeAfSelfPrint("\r\nafter sort = ");
	for (i = 0; i < len ; i++){
//			iKonkeAfSelfPrint("%d ", buf[i]);
	}
//	iKonkeAfSelfPrint("\r\n");

//	iKonkeAfSelfPrint("\r\nfilter_num(%d)buffer data = ", filter_num);
	for (uint8_t j = filter_num; j < len - filter_num ; j++){
//		iKonkeAfSelfPrint("%d ", buf[j]);
		temp_val += buf[j];
	}
//	iKonkeAfSelfPrint("sum = %d filterNum(%d)\r\n", temp_val, filter_num);
	return temp_val / (len - 2 *filter_num);
}

/*	DESP: Get the first endpoint containing the specified cluster with mask.
	Auth: dingmz_frc.20191024.
*/
uint8_t kCmdGetEndpointByCluserID(uint16_t cluster_id, uint8_t mask )
{
	extern EmberAfDefinedEndpoint emAfEndpoints[];

	for(int index = 0; index < emberAfEndpointCount(); ++index ) {
		for(int ix = 0; ix < emAfEndpoints[index].endpointType->clusterCount; ++ix ) {
			if(	(emAfEndpoints[index].endpointType->cluster[ix].clusterId == cluster_id) &&
				(emAfEndpoints[index].endpointType->cluster[ix].mask & mask)) {
				return emAfEndpoints[index].endpoint;
			}
		}
	}

	return 0;
}

/*	Partition the filename.
	Auth: dingmz_frc.20180620.
*/
char *__FNAME__(char *dname) {
	char *pchar = dname + strlen(dname) - 1;

	while((pchar != dname) && (*pchar != '/'))
		pchar--;

	return ((pchar != dname)?(pchar + 1):dname);
}
