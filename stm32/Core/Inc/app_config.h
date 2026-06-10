#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include "main.h"

/* ============================================================
 *  消息队列容量
 * ============================================================ */
#define QUEUE_SENSOR_DISPLAY    1       /* 传感器 → 显示          */
#define QUEUE_SENSOR_ANALYSIS   10      /* 传感器 → 分析          */
#define QUEUE_MODE_NOTIFY       2       /* 模式切换 → 显示        */

/* ============================================================
 *  传感器任务延迟
 * ============================================================ */
#define HCSR04_ECHO_WAIT_MS     60      /* 超声波 Echo 等待        */

/* ============================================================
 *  编码器 + 按键参数
 * ============================================================ */
#define ENC_DEBOUNCE_MS     3
#define BTN_DEBOUNCE_MS     30
#define BTN_LONG_PRESS_MS   600

/* ============================================================
 *  蜂鸣器参数
 * ============================================================ */
#define BEEP_FAST_ON_MS     100
#define BEEP_FAST_OFF_MS    100
#define BEEP_FAST_COUNT     3
#define BEEP_SLOW_ON_MS     500
#define BEEP_SLOW_OFF_MS    500
#define BEEP_SLOW_COUNT     2

#endif
