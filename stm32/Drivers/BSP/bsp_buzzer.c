/**
 * @file    bsp_buzzer.c
 * @brief   蜂鸣器驱动 (GPIO PA8, 低电平驱动)
 */
#include "bsp_buzzer.h"
#include "cmsis_os.h"

void Buzzer_Init(void)
{
    Buzzer_OFF();   /* 初始关 */
}

void Buzzer_Beep(uint16_t on_ms, uint16_t off_ms, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++) {
        Buzzer_ON();
        osDelay(on_ms);
        Buzzer_OFF();
        if (i < count - 1) {
            osDelay(off_ms);
        }
    }
}
