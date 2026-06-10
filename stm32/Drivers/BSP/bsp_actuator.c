#include "bsp_actuator.h"
#include "tim.h"

/* ============================ LED 灯 (PB5) ============================ */

void LED_Ctrl_Init(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
}

void LED_Ctrl_ON(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
}

void LED_Ctrl_OFF(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
}

void LED_Ctrl_Toggle(void)
{
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
}

/* ============================ 舵机 (PA3, TIM2_CH4) ============================ */

void Servo_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    Servo_SetAngle(SERVO_OPEN_ANGLE);   /* 默认开帘 */
}

void Servo_SetAngle(uint8_t angle)
{
    if (angle > 180) angle = 180;
    uint16_t ccr = 500 + (angle * 2000U / 180);  /* 500~2500 → 0.5~2.5ms */
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, ccr);
}

void Servo_Open(void)
{
    Servo_SetAngle(SERVO_OPEN_ANGLE);
}

void Servo_Close(void)
{
    Servo_SetAngle(SERVO_CLOSE_ANGLE);
}

/* ============================ 直流电机 TB6612 (PA11= PWMA, PB7=AIN1, PB8=AIN2) ============================ */

void Motor_Init(void)
{
    /* 启动 TIM1_CH4 PWM + 使能高级定时器主输出 */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
    htim1.Instance->BDTR |= TIM_BDTR_MOE;

    /* 刹车态：IN1=IN2=0 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
}

void Motor_SetSpeed(uint8_t duty)
{
    if (duty > 100) duty = 100;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, duty);  /* period=100, ccr=duty */
}

void Motor_Forward(uint8_t duty)
{
    Motor_SetSpeed(duty);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);   /* IN1=1 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET); /* IN2=0 */
}

void Motor_Reverse(uint8_t duty)
{
    Motor_SetSpeed(duty);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET); /* IN1=0 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);   /* IN2=1 */
}

void Motor_Stop(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);   /* IN1=1 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);   /* IN2=1（滑行） */
}

void Motor_Brake(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET); /* IN1=0 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET); /* IN2=0（短路制动） */
}
