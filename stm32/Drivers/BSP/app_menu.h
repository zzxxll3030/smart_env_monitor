#ifndef __APP_MENU_H
#define __APP_MENU_H

#include "main.h"
#include "sensor_types.h"

/* ============================================================
 *  页面枚举
 * ============================================================ */
typedef enum {
    PAGE_HOME = 0,          /* 首页：选择进入哪个页面 */
    PAGE_MAIN,              /* 主界面：传感器实时数据   */
    PAGE_DETAIL,            /* 详情：统计数据           */
    PAGE_THRESHOLD,         /* 阈值：设置告警阈值       */
    PAGE_INFO,              /* 系统：版本/运行时间      */
    PAGE_COUNT
} MenuPage_t;

/* ============================================================
 *  按钮事件
 * ============================================================ */
typedef enum {
    BTN_NONE = 0,
    BTN_SHORT,              /* 短按（<1s） */
    BTN_LONG                /* 长按（≥1s） */
} ButtonEvent_t;

/* ============================================================
 *  阈值编辑状态
 * ============================================================ */
typedef enum {
    EDIT_NONE = 0,          /* 不在编辑模式 */
    EDIT_SELECT,            /* 选择要修改哪个阈值 */
    EDIT_VALUE              /* 正在修改阈值数值 */
} EditState_t;

/* ============================================================
 *  阈值项目索引
 * ============================================================ */
typedef enum {
    THR_TEMP_HIGH_IDX = 0,
    THR_TEMP_LOW_IDX,
    THR_HUMI_HIGH_IDX,
    THR_HUMI_LOW_IDX,
    THR_LIGHT_LOW_IDX,
    THR_DIST_NEAR_IDX,
    THR_COUNT
} ThrIdx_t;

/* ============================================================
 *  阈值参数（运行时可改）
 * ============================================================ */
typedef struct {
    float    temp_high;
    float    temp_low;
    float    humi_high;
    float    humi_low;
    uint8_t  light_low;
    float    dist_near;
} RuntimeThreshold_t;

/* ============================================================
 *  API
 * ============================================================ */
void Menu_Init(void);
void Menu_SetPage(MenuPage_t page);
MenuPage_t Menu_GetPage(void);

void Menu_ProcessKey1(ButtonEvent_t evt);
void Menu_ProcessKey2(ButtonEvent_t evt);
void Menu_ProcessEncoder(int32_t steps);

void Menu_Render(const SensorData_t *data);

/* 获取当前运行阈值（供分析任务使用） */
const RuntimeThreshold_t* Menu_GetThreshold(void);

/* 外部设置阈值（供 UART/云端控制） */
void Menu_SetThresholdByIndex(uint8_t idx, float val);
void Menu_SaveThresholds(void);

/* 属性名 → 索引映射 */
int  Menu_PropNameToIndex(const char *prop);

#endif
