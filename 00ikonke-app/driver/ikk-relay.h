#ifndef __IKONKE_MODULE_ZERO_ACCESS_H_______________________________
#define __IKONKE_MODULE_ZERO_ACCESS_H_______________________________


#include "app/framework/include/af.h"
#include "../general/ikk-module-def.h"

#define RELAY_OBJS_SUPPORT_MAXN		8

// the number of physical gpios that can be mapped for a single Zero-Access channel instance!
#define RELAY_GPIO_MAPPING_MAXN		4

#define RELAY_CHNN_UNKNOW_ID		0	// Valid[1,254], 255 for Operate all at the same time!
#define RELAY_CHNN_ALLOPT_ID		255	// Valid[1,254], 255 for Operate all at the same time!


typedef enum { EZAO_OFF = 0, EZAO_ON = 1 , EZAO_DEFAULT = 0xFF}RelayOptEnum;
// Zero-Access channel status
typedef enum { EZAP_LOW = 0/* low to on */, EZAP_HIGH/* high to on */ }RelayPolarityEnum;

typedef struct {
	GPIO_Port_TypeDef port;
	unsigned int pin;
}RelayGpioListSt;

// ZA channel configuration struct
typedef struct tag_relay_config_st {
	unsigned char u8RelayChannelId; 	// 0 ~ 254

	unsigned char u8GpioMappNum;   //relay numbers
	RelayGpioListSt gpios[RELAY_OBJS_SUPPORT_MAXN]; //relay gpio buffer

	RelayPolarityEnum eActionPolarity;
}RelayChannelConfSt;

typedef struct {
	unsigned char u8RelayChannelId; 	// 0 ~ 254
	RelayOptEnum opt;
}RelayStatusSt;

//user led control callback when relay on off
typedef void (*pRelayOptLedCallback)(RelayStatusSt stRelayStatus);

/*	DESP: Zero-Access channel operate interface.
	Auth: dingmz_frc.20191115.
*/
void kRelayChannelOpt(uint8_t channel_id, RelayOptEnum opt );
/*	DESP: Relay control module Initialize configuration interface.alse zero gpio is not necessary
 * 	PARAM[port]: the GPIO port for Zero-Access detection.
 * 	PARAM[pin]: the GPIO pin for Zero-Access detection.
 *  PARAM[zero_on_time]: relay delay on time when get zero interrupt
 *  PARAM[zero_off_time]: relay delay off time when get zero interrupt
 *  PARAM[ePolarity]: the Zero GPIO pin interrupt edge
 * 	PARAM[list]: the config list of all relay dection
 * 	PARAM[channel_num]: total group number of relay.
 * 	PARAM[callback]: callback function after relay operate done
 * 	Auth: maozj.20191115.
 * */
kk_err_t kRelayModuleInit(GPIO_Port_TypeDef port, uint8_t pin, uint32_t zero_on_time, uint32_t zero_off_time \
		, RelayPolarityEnum ePolarity,  RelayChannelConfSt *list, uint8_t channel_num,  pRelayOptLedCallback callback);

#endif

