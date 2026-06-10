#ifndef __BSP_LIGHT_H
#define __BSP_LIGHT_H

#include "main.h"

/* ADC DMA 缓冲区大小 */
#define LIGHT_ADC_BUF_SIZE  64

/* 参考电压 3.3V，12位分辨率 = 4096 */
#define ADC_VREF            3.3f
#define ADC_RESOLUTION      4096

void LightSensor_Init(void);
uint16_t LightSensor_GetRaw(void);
uint8_t LightSensor_GetPercent(void);

/* DMA 半满/全满中断回调（HAL 弱函数重写） */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc);
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);

#endif
