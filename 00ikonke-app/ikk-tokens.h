
// Identifier tags for tokens
// Creator for attribute: CMEI, endpoint: 1
#if 0
#define CREATOR_REJOIN_MODE 			0x0001
#define CREATOR_FACTORY_TEST_FLG 		0x0002
#define CREATOR_SINGLE_BOARD_TEST_FLG 	0x0003
#define CREATOR_FULL_DEVICE_TEST_FLG 	0x0004
#else
#define CREATOR_GATEWAY_EUI64_VALUE 		0x0000
#define NVM3KEY_GATEWAY_EUI64_VALUE 		( NVM3KEY_DOMAIN_USER | 0x0000 )
#define CREATOR_REJOIN_MODE 				0x0001
#define NVM3KEY_REJOIN_MODE 				( NVM3KEY_DOMAIN_USER | 0x0001 )
#define CREATOR_SINGLE_BOARD_TEST_FLG 		0x0002
#define NVM3KEY_SINGLE_BOARD_TEST_FLG 		( NVM3KEY_DOMAIN_USER | 0x0002 )
#define CREATOR_FULL_DEVICE_TEST_FLG 		0x0003
#define NVM3KEY_FULL_DEVICE_TEST_FLG 		( NVM3KEY_DOMAIN_USER | 0x0003 )
#define CREATOR_AGING_TEST_FLG 				0x0004
#define NVM3KEY_AGING_TEST_FLG 				( NVM3KEY_DOMAIN_USER | 0x0004 )

#define CREATOR_INDICATOR_NOT_DISTURB_MODE_FLG 	0x0005
#define NVM3KEY_INDICATOR_NOT_DISTURB_MODE_FLG 	( NVM3KEY_DOMAIN_USER | 0x0005 )

#define CREATOR_NODE_ID						(0x0006)
#define NVM3KEY_NODE_ID	 					( NVM3KEY_DOMAIN_USER | 0x0006 )

#define CREATOR_IS_KONKE_GATEWAY			(0x0007)
#define NVM3KEY_IS_KONKE_GATEWAY 			( NVM3KEY_DOMAIN_USER | 0x0007 )

#define CREATOR_BATTERY_VOLTAGE             (0x0008)
#define NVM3KEY_BATTERY_VOLTAGE             ( NVM3KEY_DOMAIN_USER | CREATOR_BATTERY_VOLTAGE )


//#define CREATOR_OPTTUNNEL_CMEI_FLG 		0x0007
//#define NVM3KEY_OPTTUNNEL_CMEI_FLG 		( NVM3KEY_DOMAIN_USER | 0x0007 )
//#define CREATOR_OPTTUNNEL_ISN_FLG 		0x0008
//#define NVM3KEY_OPTTUNNEL_ISN_FLG 		( NVM3KEY_DOMAIN_USER | 0x0008 )
//#define CREATOR_OPTTUNNEL_CHUNK_N1_FLG 	0x0009
//#define NVM3KEY_OPTTUNNEL_CHUNK_N1_FLG 	( NVM3KEY_DOMAIN_USER | 0x0009 )
//#define CREATOR_OPTTUNNEL_CHUNKN2_FLG 	0x0013
//#define NVM3KEY_OPTTUNNEL_CHUNKN2_FLG 	( NVM3KEY_DOMAIN_USER | 0x0013 )

#endif
#ifdef DEFINETYPES
//typedef uint8_t tokTypeChunk[33];
#endif //DEFINETYPES

// Actual token definitions
#ifdef DEFINETOKENS
DEFINE_BASIC_TOKEN(GATEWAY_EUI64_VALUE, EmberEUI64, {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF})
DEFINE_BASIC_TOKEN(REJOIN_MODE, uint8_t, 0)
DEFINE_BASIC_TOKEN(SINGLE_BOARD_TEST_FLG, uint8_t, 0)
DEFINE_BASIC_TOKEN(FULL_DEVICE_TEST_FLG, uint8_t, 0)
DEFINE_BASIC_TOKEN(AGING_TEST_FLG , uint8_t, 0)
DEFINE_BASIC_TOKEN(INDICATOR_NOT_DISTURB_MODE_FLG, uint8_t, 0)

DEFINE_BASIC_TOKEN(NODE_ID, uint16_t, 0xFFFF)

DEFINE_BASIC_TOKEN(IS_KONKE_GATEWAY, uint8_t, 0)
DEFINE_BASIC_TOKEN(BATTERY_VOLTAGE, uint16_t, 0)

//DEFINE_BASIC_TOKEN(OPTTUNNEL_CMEI_FLG, tokTypeChunk, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00})
//DEFINE_BASIC_TOKEN(OPTTUNNEL_CMEI_FLG, tokTypeChunk, { 0, })
//DEFINE_BASIC_TOKEN(OPTTUNNEL_ISN_FLG, tokTypeChunk, { 0, })
//DEFINE_BASIC_TOKEN(OPTTUNNEL_CHUNK_N1_FLG, tokTypeChunk, { 0, })
//DEFINE_BASIC_TOKEN(OPTTUNNEL_CHUNKN2_FLG, tokTypeChunk, { 0, })
//#undef DEFINETOKENS
#endif



