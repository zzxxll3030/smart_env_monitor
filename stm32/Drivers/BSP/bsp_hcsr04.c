#include "bsp_hcsr04.h"
#include "tim.h"

/* 输入捕获状态机 */
typedef enum {
    HCSR04_IDLE,
    HCSR04_WAIT_FALLING
} HCSR04_State_t;

static volatile HCSR04_State_t state = HCSR04_IDLE;
static volatile uint32_t    rising_cnt  = 0;
static volatile uint32_t    pulse_width = 0;   /* 高电平宽度，单位 µs */
static volatile uint8_t     data_valid  = 0;

/**
 * @brief  启动 TIM2 输入捕获（CH2, PA1）
 * @note   PSC=72 → CK_CNT≈1MHz → 1 计数≈1µs
 */
void HCSR04_Init(void)
{
    state       = HCSR04_IDLE;
    pulse_width = 0;
    data_valid  = 0;

    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
}

/**
 * @brief  发送 Trig 脉冲（≥10µs 高电平）
 * @note   HAL_Delay 在传感器任务中调用，不在 ISR 中使用
 */
void HCSR04_Trig(void)
{
    HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_SET);
    /* 约 15µs 延时（72MHz 下约 200 次循环） */
    for (volatile uint32_t i = 0; i < 200; i++) {
        __NOP();
    }
    HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);
}

/**
 * @brief  获取距离值
 * @return 距离（cm），数据无效时返回 0
 */
float HCSR04_GetDistance(void)
{
    if (!data_valid) {
        return 0.0f;
    }

    /* 距离(cm) = 时间(µs) / 58 */
    return (float)pulse_width / 58.0f;
}

/**
 * @brief  判断是否有有效数据
 */
uint8_t HCSR04_IsValid(void)
{
    return data_valid;
}

/**
 * @brief  TIM2 输入捕获中断回调
 * @note   CK_CNT ≈ 1MHz，每次计数 ≈ 1µs
 *         上升沿 → 记录 CNT，切换为下降沿捕获
 *         下降沿 → 计算脉宽，切换回上升沿，标记有效
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2 || htim->Channel != HAL_TIM_ACTIVE_CHANNEL_2) {
        return;
    }

    uint32_t cnt = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

    if (state == HCSR04_IDLE) {
        /* 上升沿：记录起点，准备捕获下降沿 */
        rising_cnt = cnt;
        state = HCSR04_WAIT_FALLING;

        /* 切换为下降沿捕获 */
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_FALLING);

    } else if (state == HCSR04_WAIT_FALLING) {
        /* 下降沿：计算脉宽 */
        if (cnt > rising_cnt) {
            pulse_width = cnt - rising_cnt;
        } else {
            /* 计数器溢出回绕 */
            pulse_width = (0xFFFF - rising_cnt) + cnt + 1;
        }

        data_valid = 1;
        state = HCSR04_IDLE;

        /* 切换回上升沿捕获，准备下一次测量 */
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
    }
}
