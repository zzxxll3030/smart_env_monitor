#ifndef __SENSOR_TYPES_H
#define __SENSOR_TYPES_H

#include "main.h"

/* ============================================================
 *  传感器数据结构体
 * ============================================================ */
typedef struct {
    float    temperature;     /* 温度 °C           */
    float    humidity;        /* 湿度 %RH          */
    uint16_t light_raw;       /* 光照 ADC 原始值    */
    uint8_t  light_percent;   /* 光照百分比 0-100   */
    float    distance;        /* 超声波距离 cm      */
} SensorData_t;

/* ============================================================
 *  告警事件标志位
 * ============================================================ */
#define ALARM_FLAG_TEMP_HIGH    (1 << 0)   /* 温度过高     */
#define ALARM_FLAG_TEMP_LOW     (1 << 1)   /* 温度过低     */
#define ALARM_FLAG_HUMI_HIGH    (1 << 2)   /* 湿度过高     */
#define ALARM_FLAG_HUMI_LOW     (1 << 3)   /* 湿度过低     */
#define ALARM_FLAG_LIGHT_LOW    (1 << 4)   /* 光照过低     */
#define ALARM_FLAG_DIST_NEAR    (1 << 5)   /* 距离过近     */

/* ============================================================
 *  默认阈值
 * ============================================================ */
#define DEFAULT_THR_TEMP_HIGH   35.0f
#define DEFAULT_THR_TEMP_LOW     0.0f
#define DEFAULT_THR_HUMI_HIGH   80.0f
#define DEFAULT_THR_HUMI_LOW    20.0f
#define DEFAULT_THR_LIGHT_LOW   20
#define DEFAULT_THR_DIST_NEAR   10.0f

#endif
