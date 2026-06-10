#ifndef __BSP_BUZZER_H
#define __BSP_BUZZER_H

#include "main.h"
#include "pin_config.h"

#define Buzzer_ON()     HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET)
#define Buzzer_OFF()    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET)

void Buzzer_Init(void);
void Buzzer_Beep(uint16_t on_ms, uint16_t off_ms, uint8_t count);  /* 响count次 */

#endif
