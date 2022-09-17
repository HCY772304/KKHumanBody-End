#ifndef __IKONKE_COMMON_UTILS_H______________________________
#define __IKONKE_COMMON_UTILS_H______________________________


#include "stdint.h"
#include "stdbool.h"
#include "ctype.h"
#include "app/framework/include/af.h"
//#include "stack/include/ember-types.h"


#define U16MERG(h,l)	((((uint16_t)(h)) << 8) | (l))
#define U32MERG(b3,b2,b1,b0)	((((uint32_t)(b3)) << 24) | (((uint32_t)(b2)) << 16) | (((uint32_t)(b1)) << 8) | (b0))

#define UINT16_HL(a,b)        (((((uint16_t)(a)) << 8)&(0XFF00)) | ((0X00FF)&(b)))
#define UINT32_HL(a,b,c,d)    ( ((((uint32_t)(a)) << 24)&(0XFF000000)) | ((((uint32_t)(b)) << 16)&(0X00FF0000))| \
                                ((((uint32_t)(c)) << 8)&(0X0000FF00)) | ((0X000000FF)&(d)))

#define S_CMP(s1,s2)	(strcmp((s1),(s2)) == 0)
#define M_CMP(m1,m2,n)	(memcmp((m1),(m2),(n)) == 0)

// ���ַ�������
#define STRING_ITEM_NUM_MAX		8
#define STRING_ITEM_SIZE_MAX	48

typedef char (*StrSubItem)[STRING_ITEM_SIZE_MAX];

typedef struct tag_data_type_info {
	uint8_t datatype;
	uint8_t length;
}DataInfoSt;

#define STATIC_DATA_INFO_DEF	{ 			\
	{ZCL_DATA8_ATTRIBUTE_TYPE, 			1}, \
	{ZCL_DATA16_ATTRIBUTE_TYPE, 		2}, \
	{ZCL_DATA24_ATTRIBUTE_TYPE, 		3}, \
	{ZCL_DATA32_ATTRIBUTE_TYPE, 		4}, \
	{ZCL_DATA40_ATTRIBUTE_TYPE, 		5}, \
	{ZCL_DATA48_ATTRIBUTE_TYPE, 		6}, \
	{ZCL_DATA56_ATTRIBUTE_TYPE, 		7}, \
	{ZCL_DATA64_ATTRIBUTE_TYPE, 		8}, \
	{ZCL_BITMAP16_ATTRIBUTE_TYPE, 		2}, \
	{ZCL_BITMAP24_ATTRIBUTE_TYPE, 		3}, \
	{ZCL_BITMAP32_ATTRIBUTE_TYPE, 		4}, \
	{ZCL_BITMAP48_ATTRIBUTE_TYPE, 		6}, \
	{ZCL_BITMAP64_ATTRIBUTE_TYPE, 		8}, \
	{ZCL_BITMAP8_ATTRIBUTE_TYPE, 		1}, \
	{ZCL_BOOLEAN_ATTRIBUTE_TYPE, 		1}, \
	{ZCL_DATA8_ATTRIBUTE_TYPE, 			1}, \
	{ZCL_ENUM16_ATTRIBUTE_TYPE, 		2},	\
	{ZCL_ENUM8_ATTRIBUTE_TYPE, 			1}, \
	{ZCL_FLOAT_SINGLE_ATTRIBUTE_TYPE,	4}, \
	{ZCL_IEEE_ADDRESS_ATTRIBUTE_TYPE,	8},	\
	{ZCL_INT16S_ATTRIBUTE_TYPE, 		2}, \
	{ZCL_INT16U_ATTRIBUTE_TYPE, 		2}, \
	{ZCL_INT24S_ATTRIBUTE_TYPE, 		3}, \
	{ZCL_INT24U_ATTRIBUTE_TYPE, 		3}, \
	{ZCL_INT32S_ATTRIBUTE_TYPE, 		4}, \
	{ZCL_INT32U_ATTRIBUTE_TYPE, 		4}, \
	{ZCL_INT48U_ATTRIBUTE_TYPE, 		6}, \
	{ZCL_INT56U_ATTRIBUTE_TYPE, 		7}, \
	{ZCL_INT8S_ATTRIBUTE_TYPE, 			1},	\
	{ZCL_INT8U_ATTRIBUTE_TYPE, 			1}, \
	{ZCL_SECURITY_KEY_ATTRIBUTE_TYPE, 	16},\
	{ZCL_UTC_TIME_ATTRIBUTE_TYPE, 		4},	\
	{ZCL_TIME_OF_DAY_ATTRIBUTE_TYPE, 	4},	\
	{ZCL_DATE_ATTRIBUTE_TYPE, 			4},	\
	{ZCL_CLUSTER_ID_ATTRIBUTE_TYPE, 	2},	\
	{ZCL_ATTRIBUTE_ID_ATTRIBUTE_TYPE, 	2},	\
	{ZCL_BACNET_OID_ATTRIBUTE_TYPE, 	4},	\
}

#define GLOBAL_BUFFER_SIZE 	256

extern uint8_t g_u8GlobalBufferN1[];
extern uint8_t g_u8GlobalBufferN2[];
extern uint8_t g_u8GlobalLengthN1;
extern uint8_t g_u8GlobalLengthN2;

/*	DESP: 64bits��С��ת��.
	Auth: dingmz_frc.20180627.
*/
extern uint64_t __CPU_ENDIAN_HTONLL__(uint64_t u64_host );
// ת���ֽ���
#define NT16(nv)	((unsigned short)(((nv) << 8) | ((nv) >> 8)))
#define NT32(nv)	((unsigned int)((NT16((nv) & 0xffff) << 16) | (NT16((nv) >> 16))))
#define NT64(nv)	__CPU_ENDIAN_HTONLL__(nv)


void kSetMfgTokenUserData(uint16_t token, void *data, uint32_t len);
void kGetMfgTokenUserData(uint16_t token, void *data, uint32_t len);

void NWK_AddRssi(uint16_t node_id, int8_t rssi);
bool NWK_DeleteRssiByNodeid(uint16_t node_id);
bool getEndDeviceRssiByNodeid(uint16_t node_id, int8_t *rssi, uint8_t *index);

/*	DESP: Gets the byte length of the specified data type.
 * 	Auth: dingmz_frc.20191025.
 * */
uint8_t kUtilsGetDataLength(uint8_t type );
/*	DESP: check whether the data set content is specified byte data!
 * 	Auth: dingmz_frc.20190520.
 * */
bool kUtilsCheckDataSetContext(void *data_set, uint8_t spec_v, int length );
/*	DESP: get the crc16 value of specific InstallCode.
 * 	Auth: dingmz_frc.20191007.
 * */
uint16_t kUtilsInstallCodeX25CRC16(uint8_t* u8InstallCode, uint8_t length );
/*	DESP: CRC16 algorithm.
 * 	Auth: dingmz_frc.20191018.
 * */
uint16_t kUtilsCRC16Sum(const unsigned char *msg, unsigned int length );
/*	DESP: ��Ϣ��ʽ���� - �ַ����ֶν������Ӵ�.
	Auth: dingmz_frc.20180607.
*/
int	kUtilsStringSplit(char *string, char (*item)[STRING_ITEM_SIZE_MAX], char spec/*�ָ���*/);

/*	DESP: Check whether the given string is a number.
 * 	Auth: dingmz_frc.20191023.
 * */
bool kUtilsIsNumber(char *string );
/*	DESP: Check whether the given string is OCT string.
 * 	Auth: dingmz_frc.20191023.
 * */
bool kUtilsIsOct(char *string );
/*	DESP: Gets the value[in 64-bits] of the specified length from the specified OCT string.
 * 	Note: expected output value max 64bits, so max length input is 16!!!
	Auth: dingmz_frc.20191018.
*/
uint64_t kUtilsOct2Value(char *oct, int length/* �� 16 */ );
/*	DESP: Convert OCT string to hex data stream.
	Auth: dingmz_frc.20191018.
*/
int kUtilsStr2Hex(char *octstr, unsigned char *phexout, int max_length );
/*	DESP: High precision random interface��value in[minNum, maxNum].
	Auth: dingmz_frc.20190701.
*/
uint32_t kUtilsGetRandNum(uint32_t minNum, uint32_t maxNum );
/*	DESP: Reverse uint8 array contents.
 * 	PARAM[bModifyOriginalData]: Note that this interface modifies the original data content When bModifyOriginalData is true!!!
	Auth: dingmz_frc.20191019.
*/
uint8_t *kUtilsReverseArrayU8int(uint8_t *org_data, int length, bool bModifyOriginalData );
/*	DESP: Check if it is a valid IEEE address.
 * 	Auth: dingmz_frc.20191106.
 * */
bool kUtilsIsValidEui64Addr(EmberEUI64 ineui64addr );

/*	DESP: ��һ��buffer���ݽ�������ȥ�����ĺ���С���˲�����ȡƽ��ֵ
	filter_num �˲�������С�ĸ����� ��ȵ�
 * 	Auth: maozj.20191123.
 * */
uint32_t kUtilsGetAverage(uint32_t *buf, uint8_t len, uint8_t filter_num); //����ƽ��ֵ

/*	DESP: Get the first endpoint containing the specified cluster with mask.
	Auth: dingmz_frc.20191024.
*/
uint8_t kCmdGetEndpointByCluserID(uint16_t cluster_id, uint8_t mask );

/*	Partition the filename.
	Auth: dingmz_frc.20180620.
*/
char *__FNAME__(char *dname);
#endif
