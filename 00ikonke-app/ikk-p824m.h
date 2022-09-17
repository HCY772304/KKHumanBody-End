#ifndef __IKK__P824M_H__
#define __IKK__P824M_H__
#include "math.h"
#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_LOCK_TIME_MS		10000

#define GPIO_SENSOR_PORT gpioPortD
#define GPIO_SENSOR_PIN 	15

#define SENSOR_CHANGE_SENSITIVITY_TEMP_DIFF		(2.0) //unit:deg C

void kP824mInit(float temperature);
void kP824SerInitHard(void);
//��λ���͵紫����P824M��ÿ��������븴λ
void kP824mReset(void);
void kP824SensorGpioInit(float temperature);
void kP824SensitivitySetByTemp(float temperature);
float kP824GetLastRecordTemperature(void);

#ifdef __cplusplus
}
#endif

#endif /* __IKK__P824M_H__ */
