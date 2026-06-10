#ifndef __PIN_CONFIG_H
#define __PIN_CONFIG_H

#include "main.h"

/* ============================================================
 *  LED — 运行指示灯
 * ============================================================ */
#define LED_PORT        GPIOC
#define LED_PIN         GPIO_PIN_13

/* ============================================================
 *  蜂鸣器 — GPIO PA8, 低电平驱动
 * ============================================================ */
#define BUZZER_PORT     GPIOA
#define BUZZER_PIN      GPIO_PIN_8

/* ============================================================
 *  EC11 旋转编码器 + 按键
 * ============================================================ */
#define ENC_A_PORT      GPIOB
#define ENC_A_PIN       GPIO_PIN_0
#define ENC_B_PORT      GPIOB
#define ENC_B_PIN       GPIO_PIN_1
#define KEY1_PORT       GPIOB
#define KEY1_PIN        GPIO_PIN_12
#define KEY2_PORT       GPIOB
#define KEY2_PIN        GPIO_PIN_13
#define KEY3_PORT       GPIOB
#define KEY3_PIN        GPIO_PIN_6

/* ============================================================
 *  HC-SR04 超声波 — Trig 引脚, Echo 用 TIM2_CH2
 * ============================================================ */
#define HCSR04_TRIG_PORT    GPIOA
#define HCSR04_TRIG_PIN     GPIO_PIN_0

/* ============================================================
 *  OLED — 软件 I2C (PB3=SCL, PB4=SDA)
 * ============================================================ */
#define OLED_SCL_PORT       GPIOB
#define OLED_SCL_PIN        GPIO_PIN_3
#define OLED_SDA_PORT       GPIOB
#define OLED_SDA_PIN        GPIO_PIN_4

/* ============================================================
 *  SHT20 — 硬件 I2C2 (CubeMX 管理)
 * ============================================================ */
/* PB10 = I2C2_SCL, PB11 = I2C2_SDA */

/* ============================================================
 *  光敏传感器 — ADC1_IN2 (CubeMX 管理)
 * ============================================================ */
/* PA2 = ADC1_IN2 */

/* ============================================================
 *  串口 — USART1 (CubeMX 管理): PA9=TX, PA10=RX
 * ============================================================ */

#endif
