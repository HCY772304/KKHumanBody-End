#include "ustimer.h"
#include "gpiointerrupt.h"
#include "em_bus.h"
#include "app/framework/include/af.h"
#include "ikk-p824m.h"
#include "driver/ikk-button.h"
#include "general/ikk-debug.h"


#define DEV_KKIT_HUMAN_SER_PORT             gpioPortF
#define DEV_KKIT_HUMAN_SER_PIN              2
#define DEV_KKIT_HUMAN_SER_HIGH             GPIO_PinOutSet(DEV_KKIT_HUMAN_SER_PORT, DEV_KKIT_HUMAN_SER_PIN)
#define DEV_KKIT_HUMAN_SER_LOW              GPIO_PinOutClear(DEV_KKIT_HUMAN_SER_PORT, DEV_KKIT_HUMAN_SER_PIN)


// factory sensor config
#define SENS_C      							0x3C // 00111100   8
#define BLIND_C    								0x04 // 00110000   4
#define PULSE_C     							0x00 // 00000000   2
#define WINDOW_C    							0x00 // 00000000   2
#define MOTION_C    							0x01 // 10000000   1
#define INT_C       							0x00 // 00000000   1
#define VOLT_C      							0x00 // 00000000   2
#define SUPP_C      							0x00 // 00000000   1
#define RSV_C       							0x00 // 00000000   4

#define SENSOR_LOWEST_TEMPERATURE				(-14.0) //unit:degC
#define SENSOR_HIGHEST_TEMPERATURE				(40.0) //unit:degC
#define SENSOR_LOWEST_SENSITIVITY				(0x48)
#define SENSOR_HIGHEST_SENSITIVITY				(0x2B)
#define SENSOR_CHANGE_SENSITIVITY_MAX_TIMES		(1)


static bool g_bInit = false;
static float g_fLastSetTemp = 25.0;
static uint8_t kP824GetSensivityValueByTemp(float temperature);

void kP824SerInitHard(void)
{
	USTIMER_Init();
	USTIMER_Delay(50000);
	GPIO_DbgSWOEnable(false); //�ر�SWO�ܽŹ���
	//GPIO_PinModeSet(DEV_KKIT_HUMAN_SER_PORT, DEV_KKIT_HUMAN_SER_PIN,gpioModePushPull,0);
	kP824mReset();
	//DEV_KKIT_HUMAN_SER_HIGH;
	//USTIMER_Delay(50000);
	/*DEV_KKIT_HUMAN_SER_HIGH;
	USTIMER_Delay(10000);
	DEV_KKIT_HUMAN_SER_LOW;
	USTIMER_Delay(20000);
	DEV_KKIT_HUMAN_SER_HIGH;
	USTIMER_Delay(10000);
	DEV_KKIT_HUMAN_SER_LOW;
	USTIMER_Delay(20000);
	DEV_KKIT_HUMAN_SER_HIGH;
	USTIMER_Delay(10000);
	DEV_KKIT_HUMAN_SER_LOW;
	*/
}

void kP824mWrite(uint8_t data, uint8_t cnt)
{
    int i = 0;
    
    for (i = cnt; i > 0; i --)
    {
       // DEV_KKIT_HUMAN_SER_LOW;
        USTIMER_Delay(1);
        
       // DEV_KKIT_HUMAN_SER_HIGH;
        USTIMER_Delay(1);
        
        if (data & 0x80)
        {
            //DEV_KKIT_HUMAN_SER_HIGH;
            USTIMER_Delay(80);
        }
        else
        {
            //DEV_KKIT_HUMAN_SER_LOW;
            USTIMER_Delay(80);
        }
        
        data = data << 1;
    }

}

void kP824mInit(float temperature)
{
	kP824SensitivitySetByTemp(temperature);
   // USTIMER_Delay(800);
   // DEV_KKIT_HUMAN_SER_LOW;
   // USTIMER_Delay(800);
}

//��λ���͵紫����P824M��ÿ��������븴λ
void kP824mReset(void)
{
	 USTIMER_Delay(15000);
//	 kZclIASZoneStatusUpdate(1, 0, false);
//	 kZclClusterSetPermitReportInfo(1, ZCL_IAS_ZONE_CLUSTER_ID,  true, false, true, false);
	 //�иߵ�ƽ�ź�������zigbee���һ����
	 GPIO_PinModeSet(GPIO_SENSOR_PORT, GPIO_SENSOR_PIN, gpioModePushPull, 0);
	 GPIO_PinOutClear(GPIO_SENSOR_PORT, GPIO_SENSOR_PIN);
	 USTIMER_Delay(1000);
	 //�������ô�������Ӧ����ܽ�Ϊ�ⲿ�ж�����
	 GPIO_PinModeSet(GPIO_SENSOR_PORT, GPIO_SENSOR_PIN,gpioModeInput,0);

	 //halGpioSensorInitialize();
}

void kP824SensorGpioInit(float temperature)
{
	iKonkeAfSelfPrint("###kP824SensorGpioInit temperature(%d degC)\r\n", (int8_t)(temperature));
	//GPIO_PinModeSet(IR_SENSOR_PORT, IR_SENSOR_PIN, gpioModeInput,0);//add by maozj 20190418��PD15�����ó��ⲿ�жϣ�Ϊ�ɻɹܸ�������
	kP824SerInitHard();
	USTIMER_Delay(100000);

	//g_bInit = false;

	kP824mInit(temperature);
	//�������ʱ��5S
	//kkSetHumanSensorLockTimeMs(5);
}
void kP824SensitivitySetByTemp(float temperature)
{
#if 1

	static uint8_t recordCounter = 0;


	if (g_bInit == true) {
		//temperature difference is more than 2 degC, to change sensitivity immediately
		if (fabs(temperature - g_fLastSetTemp) >= 2 * SENSOR_CHANGE_SENSITIVITY_TEMP_DIFF) {
			g_fLastSetTemp = temperature;
			recordCounter = 0;
		} else {
			//temperature difference is more than 2 degC, and counter is more than 2 times ,to change sensitivity immediately
			if (fabs(temperature - g_fLastSetTemp) >= SENSOR_CHANGE_SENSITIVITY_TEMP_DIFF && recordCounter++ >= SENSOR_CHANGE_SENSITIVITY_MAX_TIMES ) {
				recordCounter = 0;
				g_fLastSetTemp = temperature;
			} else {
				return;
			}
		}
	} else {
		g_bInit = true;
		g_fLastSetTemp = temperature;
	}

	uint8_t sensitivity = kP824GetSensivityValueByTemp(temperature);
	iKonkeAfSelfPrint("sensitivity(0x%x)\r\n", sensitivity);
#else
	uint8_t sensitivity = 0x78;
#endif
	kP824mWrite(sensitivity, 8);
	kP824mWrite(BLIND_C << 4, 4);
	kP824mWrite(PULSE_C << 6, 2);
	kP824mWrite(WINDOW_C << 6, 2);
	kP824mWrite(MOTION_C << 7, 1);
	kP824mWrite(INT_C << 7, 1);
	kP824mWrite(VOLT_C << 6, 2);
	kP824mWrite(SUPP_C << 7, 1);
	kP824mWrite(RSV_C << 4, 4);
}

////mzj
//static uint8_t kP824GetSensivityValueByTemp(float temperature)
//{
//	float tempDiff = SENSOR_HIGHEST_TEMPERATURE - SENSOR_LOWEST_TEMPERATURE;
//	uint8_t sensDiff = SENSOR_LOWEST_SENSITIVITY - SENSOR_HIGHEST_SENSITIVITY;
//
//	if (temperature > SENSOR_HIGHEST_TEMPERATURE) {
//		return SENSOR_HIGHEST_SENSITIVITY - 2;
//	} else if (temperature < SENSOR_LOWEST_TEMPERATURE){
//		return SENSOR_LOWEST_SENSITIVITY + 4;
//	} else {
//		float tempPercent = fabs(temperature - SENSOR_LOWEST_TEMPERATURE) / tempDiff;
//		iKonkeAfSelfPrint("####P824 tempDiff(%d)sensDiff(%d)tempPercent*100(%d)\r\n", (int8_t)tempDiff, sensDiff, (int8_t)(tempPercent * 100));
//		return (uint8_t)(SENSOR_LOWEST_SENSITIVITY - tempPercent * sensDiff );
//	}
//	return SENSOR_LOWEST_SENSITIVITY;
//}

//zxh
//static uint8_t kP824GetSensivityValueByTemp(float temperature)
//
//{
//
//	float tempDiff = SENSOR_HIGHEST_TEMPERATURE - SENSOR_LOWEST_TEMPERATURE;
//
//	uint8_t sensDiff = SENSOR_LOWEST_SENSITIVITY - SENSOR_HIGHEST_SENSITIVITY;
//
//
//                 //40���϶�����
//	if (temperature > SENSOR_HIGHEST_TEMPERATURE) {
//
//		return SENSOR_HIGHEST_SENSITIVITY - 8;
//                      //����15���϶�����
//	} else if (temperature < SENSOR_LOWEST_TEMPERATURE){
//
//		return SENSOR_LOWEST_SENSITIVITY +7;
//
//	} else {
//		float tempPercent = (SENSOR_HIGHEST_SENSITIVITY-temperature)*(SENSOR_HIGHEST_SENSITIVITY-temperature)/100;
//		//ȷ�ϴ�ӡ���Ƿ�ֵ�������ط�
//	    //iKonkeAfSelfPrint("####P824 tempDiff(%d)sensDiff(%d)tempPercent*100(%d)\r\n", (int8_t)tempDiff, sensDiff, (int8_t)(tempPercent * 100));
//	  //����15���϶�-10���϶�
//	  if(temperature<10)
//	  {
//		  return (uint8_t)(tempPercent+37+fabs(temperature-15)/3);
//	  }
//	 //10-20���϶�
//	  if(temperature<21)
//	  {
////		  return (uint8_t)(tempPercent+39-temperature/4.5);
//		  return (uint8_t)(tempPercent+39-((temperature*2)/9));
//      }
//        //21-40���϶�
//		return (uint8_t)(tempPercent+35);
//	}
//	return SENSOR_LOWEST_SENSITIVITY;
//
//}

static uint8_t kP824GetSensivityValueByTemp(float temperature)

{

	float tempDiff = SENSOR_HIGHEST_TEMPERATURE - SENSOR_LOWEST_TEMPERATURE;

	uint8_t sensDiff = SENSOR_LOWEST_SENSITIVITY - SENSOR_HIGHEST_SENSITIVITY;


                 //40���϶�����
	if (temperature > SENSOR_HIGHEST_TEMPERATURE) {

		return SENSOR_HIGHEST_SENSITIVITY - 8;
                      //����15���϶�����
	} else if (temperature < SENSOR_LOWEST_TEMPERATURE){

		return SENSOR_LOWEST_SENSITIVITY + 28;

	} else {
		float tempPercent = (SENSOR_HIGHEST_SENSITIVITY-temperature)*(SENSOR_HIGHEST_SENSITIVITY-temperature)/100;
		//ȷ�ϴ�ӡ���Ƿ�ֵ�������ط�
		//iKonkeAfSelfPrint("####P824 tempDiff(%d)sensDiff(%d)tempPercent*100(%d)\r\n", (int8_t)tempDiff, sensDiff, (int8_t)(tempPercent * 100));
		//����15���϶�-���
	  if(temperature<0)
	  {
		  return (uint8_t)(tempPercent+37+fabs(temperature-15));
	  }
	  //0���϶�-10���϶�
	  if(temperature<10)
	  {
		  return (uint8_t)(tempPercent+37+fabs(temperature-15)/3);
	  }
	 //10-20���϶�
	  if(temperature<21)
	   {
		  return (uint8_t)(tempPercent+39-((temperature*2)/9));
       }
     //21-40���϶�
		return (uint8_t)(tempPercent+35);

	}

	return SENSOR_LOWEST_SENSITIVITY;

}

float kP824GetLastRecordTemperature(void)
{
	return g_fLastSetTemp;
}
