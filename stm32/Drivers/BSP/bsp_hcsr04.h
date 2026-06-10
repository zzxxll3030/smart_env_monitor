#ifndef __BSP_HCSR04_H
#define __BSP_HCSR04_H

#include "main.h"
#include "pin_config.h"

/* Trig 引脚已在 pin_config.h 定义 */

void HCSR04_Init(void);
void HCSR04_Trig(void);
float HCSR04_GetDistance(void);
uint8_t HCSR04_IsValid(void);

/* TIM2 输入捕获中断回调 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);

#endif
