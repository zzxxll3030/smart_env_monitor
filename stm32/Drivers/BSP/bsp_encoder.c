#include "bsp_encoder.h"
#include "pin_config.h"
#include "app_config.h"
#include "FreeRTOS.h"
#include "task.h"

static volatile int32_t  enc_steps      = 0;
static volatile uint32_t enc_last_tick  = 0;
static volatile uint32_t key1_last_tick = 0;
static volatile uint32_t key2_last_tick = 0;
static volatile uint8_t  key1_pressed   = 0;
static volatile uint8_t  key2_pressed   = 0;

static void Encoder_Process(void)
{
    uint32_t tick = xTaskGetTickCountFromISR();
    if ((tick - enc_last_tick) < pdMS_TO_TICKS(ENC_DEBOUNCE_MS)) return;
    enc_last_tick = tick;

    if (HAL_GPIO_ReadPin(ENC_A_PORT, ENC_A_PIN) == GPIO_PIN_SET) {
        if (HAL_GPIO_ReadPin(ENC_B_PORT, ENC_B_PIN) == GPIO_PIN_SET)
            enc_steps++;   /* CW */
        else
            enc_steps--;   /* CCW */
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t tick = xTaskGetTickCountFromISR();

    switch (GPIO_Pin) {
    case GPIO_PIN_0:
        Encoder_Process();
        break;
    case GPIO_PIN_1:
        break;
    case GPIO_PIN_12:
        if ((tick - key1_last_tick) >= pdMS_TO_TICKS(BTN_DEBOUNCE_MS)) {
            key1_pressed = 1;
            key1_last_tick = tick;
        }
        break;
    case GPIO_PIN_13:
        if ((tick - key2_last_tick) >= pdMS_TO_TICKS(BTN_DEBOUNCE_MS)) {
            key2_pressed = 1;
            key2_last_tick = tick;
        }
        break;
    default: break;
    }
}

int32_t Encoder_GetSteps(void)
{
    int32_t val = enc_steps;
    enc_steps = 0;
    return val;
}

uint8_t Key1_ReadState(void)
{
    uint8_t val = key1_pressed;
    key1_pressed = 0;
    return val;
}

uint8_t Key2_ReadState(void)
{
    uint8_t val = key2_pressed;
    key2_pressed = 0;
    return val;
}
