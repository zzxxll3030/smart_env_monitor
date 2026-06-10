#ifndef __BSP_ACTUATOR_H
#define __BSP_ACTUATOR_H

#include "main.h"

// ====== LED 灯 ======
void LED_Ctrl_Init(void);
void LED_Ctrl_ON(void);
void LED_Ctrl_OFF(void);
void LED_Ctrl_Toggle(void);

// ====== 舵机（模拟窗帘） ======
#define SERVO_CLOSE_ANGLE   0     // 关帘角度
#define SERVO_OPEN_ANGLE   180    // 开帘角度
void Servo_Init(void);
void Servo_SetAngle(uint8_t angle);   // 0~180
void Servo_Open(void);                // 开帘（180°）
void Servo_Close(void);               // 关帘（0°）

// ====== 直流电机（TB6612 驱动，模拟风扇） ======
// 控制真值表：IN1/IN2 = 00=刹车 10=正转 01=反转 11=停止
void Motor_Init(void);
void Motor_SetSpeed(uint8_t duty);      // 0~100 PWM 占空比
void Motor_Forward(uint8_t duty);       // 正转（风扇吹风）
void Motor_Reverse(uint8_t duty);       // 反转（可选）
void Motor_Stop(void);                  // 滑行停止（IN1=IN2=1）
void Motor_Brake(void);                 // 短路刹车（IN1=IN2=0）

#endif
