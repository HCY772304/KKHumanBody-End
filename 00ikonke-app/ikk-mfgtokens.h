
// MANUFACTURING CREATORS
// The creator codes are here in one list instead of next to their token
// definitions so comparision of the codes is easier.  The only requirement
// on these creator definitions is that they all must be unique.  A favorite
// method for picking creator codes is to use two ASCII characters in order
// to make the codes more memorable.  Also, the msb of Stack and Manufacturing
// token creator codes is always 1, while the msb of Application token creator
// codes is always 0.  This distinction allows Application tokens
// to freely use the lower 15 bits for creator codes without the risk of
// duplicating a Stack or Manufacturing token creator code.

//here is mfg token ,msb is 1
#define CREATOR_MFG_CMEI 						0xEF43   //MSB+'m'+'C', (80+7f) << 8 | 43
#define CREATOR_MFG_ISN 						0xEF49  //MSB+'m'+'I', (80+7f) << 8 | 49
#define CREATOR_MFG_CHUNK_N1 					0xEF4E   //MSB+'m'+'N', (80+7f) << 8 | 4E
#define CREATOR_MFG_CUSTOM_INSTALL_CODE			0xEF69    //MSB+'m' + 'i' (80+7f) << 8 | 69

#define CREATOR_MFG_CUSTOM_MODEL_IDENTIFIER		0xEF4D    //MSB+'m' + 'M' (80+7f) << 8 | 69

#ifdef DEFINETYPES
typedef uint8_t tokTypeChunk[33];
typedef uint8_t tokTypeModelId[34];
typedef struct {
  // The bottom flag bit is 1 for uninitialized, 0 for initialized.
  // Bits 1 and 2 give the size of the value string:
  // 0 = 6 bytes, 1 = 8 bytes, 2 = 12 bytes, 3 = 16 bytes.
  // The other flag bits should be set to 0 at initialization.
  // Special flags support.  Due to a bug in the way some customers
  // had programmed the flags field, we will also examine the upper
  // bits 9 and 10 for the size field.  Those bits are also reserved.
  uint16_t flags;
  uint8_t value[16];
  uint16_t crc;
} tokTypeMfgInstallCode;


#endif


#ifdef DEFINETOKENS
//according to token-manufacturing-series-1.h or token-manufacturing-series-2.h
#define USERDATA_TOKENS_OPTTUNNEL     	  		0x1000
#define MFG_CMEI_LOCATION 				 		(USERDATA_TOKENS_OPTTUNNEL | 0x106)
#define MFG_ISN_LOCATION				 		(USERDATA_TOKENS_OPTTUNNEL | 0x128)
#define MFG_CHUNK_N1_LOCATION			 		(USERDATA_TOKENS_OPTTUNNEL | 0x150)
#define MFG_CUSTOM_INSTALL_CODE_LOCATION		(USERDATA_TOKENS_OPTTUNNEL | 0x172)

#define CUSTOM_LOCKBITSDATA_TOKENS           	0x3000
#define MFG_CUSTOM_MODEL_IDENTIFIER_LOCATION	(CUSTOM_LOCKBITSDATA_TOKENS | 0x460)

DEFINE_MFG_TOKEN(MFG_CMEI,
				tokTypeChunk,
				MFG_CMEI_LOCATION,
			   {0xFF,})
DEFINE_MFG_TOKEN(MFG_ISN,
				tokTypeChunk,
				MFG_ISN_LOCATION,
			   {0xFF,})
DEFINE_MFG_TOKEN(MFG_CHUNK_N1,
			tokTypeChunk,
			MFG_CHUNK_N1_LOCATION,
		   {0xFF,})
//copy to mfg installation code token place after power on
DEFINE_MFG_TOKEN(MFG_CUSTOM_INSTALL_CODE,
		 tokTypeMfgInstallCode,
		 MFG_CUSTOM_INSTALL_CODE_LOCATION,
		 {0xFF, })
DEFINE_MFG_TOKEN(MFG_CUSTOM_MODEL_IDENTIFIER,
		tokTypeModelId,
		MFG_CUSTOM_MODEL_IDENTIFIER_LOCATION,
	   {0xFF,})
#endif
