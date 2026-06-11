/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sht20.h"
#include "led.h"
#include "usart.h"
#include "OLED.h"
#include "bsp_light.h"
#include "bsp_hcsr04.h"
#include "bsp_encoder.h"
#include "app_menu.h"
#include "bsp_buzzer.h"
#include "bsp_actuator.h"
#include "sensor_types.h"
#include "pin_config.h"
#include "app_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* (告警标志/阈值已统一在 sensor_types.h 中定义) */

/* 全局模式: 1=阈值自动控制, 0=手动开关控制 (KEY3 切换) */
static volatile int auto_mode = 1;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/*
 * ============================================================
 *  IPC 通信对象（任务间数据传递）
 * ============================================================
 *
 *  数据流示意:
 *  SensorTask ──sht20Queue────► DisplayTask    传感器数据 → OLED 刷新
 *  SensorTask ──analysisQueue─► AnalysisTask   传感器数据 → 告警判断 + 自动控制
 *  InputTask  ──modeNotifyQueue─► DisplayTask  模式切换通知 → OLED 全屏提示
 *  AnalysisTask ──alarmEventFlags──► AlarmTask 告警标志位 → 蜂鸣器触发
 */

osMessageQueueId_t sht20Queue;       /* 传感器 → 显示: 容量1, 只保留最新数据 */
osMessageQueueId_t analysisQueue;    /* 传感器 → 分析: 容量10, 缓冲防止丢数据 */
osMessageQueueId_t modeNotifyQueue;  /* 模式切换 → 显示: 容量2 */
osMutexId_t        statsMutex;       /* 统计互斥锁（预留） */
osEventFlagsId_t   alarmEventFlags;  /* 告警事件组: 6 种告警标志位 */

/*
 * ============================================================
 *  任务属性定义
 * ============================================================
 *
 *  stack_size 单位: 字 (32-bit) → 实际字节数 = stack_size × 4
 *  STM32F103C8T6 总 RAM: 20KB，当前任务总栈约 4.8KB
 *
 *  优先级说明:
 *    所有任务同优先级 (Normal) — 依赖 FreeRTOS 时间片轮转调度
 *    MonitorTask 使用 Low 优先级 — 系统监控不抢实时任务
 */

/* ---- SensorTask: 传感器采集 ---- */
osThreadId_t sensorTaskHandle;
const osThreadAttr_t sensorTask_attributes = {
  .name = "sensorTask",
  .stack_size = 256 * 4,    /* 1KB — printf 消耗栈较多 */
  .priority = (osPriority_t) osPriorityNormal,
};

/* ---- DisplayTask: OLED 显示刷新 ---- */
osThreadId_t displayTaskHandle;
const osThreadAttr_t displayTask_attributes = {
  .name = "displayTask",
  .stack_size = 256 * 4,    /* 1KB — Menu_Render 递归调用 */
  .priority = (osPriority_t) osPriorityNormal,
};

/* ---- InputTask: 按键 + 编码器轮询 ---- */
osThreadId_t inputTaskHandle;
const osThreadAttr_t inputTask_attributes = {
  .name = "inputTask",
  .stack_size = 128 * 4,    /* 512B — 纯逻辑无浮点，栈需求小 */
  .priority = (osPriority_t) osPriorityNormal,
};

/* ---- AnalysisTask: 数据分析 + 阈值比对 + 自动控制 ---- */
osThreadId_t analysisTaskHandle;
const osThreadAttr_t analysisTask_attributes = {
  .name = "analysisTask",
  .stack_size = 256 * 4,    /* 1KB — 浮点比较 + 执行器调用 */
  .priority = (osPriority_t) osPriorityNormal,
};

/* ---- MonitorTask: UART 命令解析 + 系统监控 ---- */
osThreadId_t monitorTaskHandle;
const osThreadAttr_t monitorTask_attributes = {
  .name = "monitorTask",
  .stack_size = 256 * 4,    /* 1KB — JSON 字符串解析消耗栈 */
  .priority = (osPriority_t) osPriorityLow,   /* 低优先级，不干扰实时采集 */
};

/* ---- AlarmTask: 告警响应（蜂鸣器） ---- */
osThreadId_t alarmTaskHandle;
const osThreadAttr_t alarmTask_attributes = {
  .name = "alarmTask",
  .stack_size = 128 * 4,    /* 512B — 仅等待事件 + 控制蜂鸣器 */
  .priority = (osPriority_t) osPriorityNormal,
};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/*
 * 6 个自定义 FreeRTOS 任务（+1 个 CubeMX 默认任务）
 *
 * ┌─────────────────┬──────────────────────────────────────────┬──────────┐
 * │ 任务             │ 职责                                     │ 周期      │
 * ├─────────────────┼──────────────────────────────────────────┼──────────┤
 * │ StartSensorTask  │ 采集 4 路传感器, 通过队列分发, JSON 上报  │ ~500ms   │
 * │ StartDisplayTask │ OLED 菜单渲染, 响应模式切换通知           │ ~100ms   │
 * │ StartInputTask   │ 按键长/短按检测 + 编码器步数消费          │ 10ms     │
 * │ StartAnalysisTask│ 阈值对比 → 告警标志, 自动模式下控制执行器 │ 事件驱动  │
 * │ StartAlarmTask   │ 阻塞等待告警事件 → 触发蜂鸣器             │ 事件驱动  │
 * │ StartMonitorTask │ UART 环形缓冲区消费 + 10s 系统状态打印    │ 1ms 轮询 │
 * └─────────────────┴──────────────────────────────────────────┴──────────┘
 */
void StartSensorTask(void *argument);
void StartDisplayTask(void *argument);
void StartInputTask(void *argument);
void StartAnalysisTask(void *argument);
void StartMonitorTask(void *argument);
void StartAlarmTask(void *argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  statsMutex = osMutexNew(NULL);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  sht20Queue      = osMessageQueueNew(QUEUE_SENSOR_DISPLAY, sizeof(SensorData_t), NULL);
  analysisQueue   = osMessageQueueNew(QUEUE_SENSOR_ANALYSIS, sizeof(SensorData_t), NULL);
  modeNotifyQueue = osMessageQueueNew(QUEUE_MODE_NOTIFY, sizeof(uint8_t), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  sensorTaskHandle   = osThreadNew(StartSensorTask, NULL, &sensorTask_attributes);
  displayTaskHandle  = osThreadNew(StartDisplayTask, NULL, &displayTask_attributes);
  inputTaskHandle    = osThreadNew(StartInputTask, NULL, &inputTask_attributes);
  analysisTaskHandle = osThreadNew(StartAnalysisTask, NULL, &analysisTask_attributes);
  alarmTaskHandle    = osThreadNew(StartAlarmTask, NULL, &alarmTask_attributes);
  monitorTaskHandle  = osThreadNew(StartMonitorTask, NULL, &monitorTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  alarmEventFlags = osEventFlagsNew(NULL);
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  LED_Init();
  for (;;)
  {
    osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
 * @brief  传感器采集任务 — 系统数据入口
 * @note   周期约 500ms: SHT20(85ms阻塞) + osDelay(60) + osDelay(440)
 *         采集 4 路传感器 → 打包 SensorData_t → 分发到 2 个队列 + USART1 JSON 上报
 * @warning SHT20_ReadData 内部 HAL_Delay(85) 阻塞，会阻塞本任务但不影响其他 RTOS 任务
 */
void StartSensorTask(void *argument)
{
    SensorData_t local_data;

    /* ---- 一次性初始化 ---- */
    if (!SHT20_Init()) {
         printf("SHT20 Init Failed!\r\n");
    } else {
         printf("SHT20 Init OK\r\n");
    }

    LightSensor_Init();   /* ADC+DMA 启动，此后 DMA 在后台持续搬运 */
    HCSR04_Init();        /* TIM2 输入捕获启动，此后 ISR 自动处理 Echo */

    for (;;) {
        /* ① 读温湿度 — I2C 主机模式，内部阻塞 85ms */
        if (!SHT20_ReadData(&local_data)) {
            printf("SHT20 Read Error\r\n");
            osDelay(1000);   /* 失败等 1s 再重试，避免死循环刷屏 */
            continue;
        }

        /* ② 读光照 — DMA 循环缓冲区均值滤波，零等待 */
        local_data.light_raw     = LightSensor_GetRaw();
        local_data.light_percent = LightSensor_GetPercent();

        /* ③ 读距离 — 发 Trig 脉冲 → 等 Echo → 读 ISR 捕获结果 */
        HCSR04_Trig();
        osDelay(60);  /* 等待 Echo 返回（最长 38ms + 余量），此期间 ISR 已完成捕获 */
        local_data.distance = HCSR04_GetDistance();

        /* ④ 分发数据到下游任务 */
        osMessageQueuePut(sht20Queue, &local_data, 0, 0);     /* → DisplayTask */
        osMessageQueuePut(analysisQueue, &local_data, 0, 0);  /* → AnalysisTask */

        printf("Temp=%.1f C  Humi=%.1f%%  Light=%d%%  Dist=%.1f cm\r\n",
                local_data.temperature, local_data.humidity,
                local_data.light_percent, local_data.distance);

        /* ⑤ 上报 JSON 给 ESP32（USART1 TX → ESP32 GPIO17 RX） */
        {
            char json[128];
            snprintf(json, sizeof(json),
                "{\"t\":%.1f,\"h\":%.1f,\"l\":%d,\"d\":%.1f}\r\n",
                local_data.temperature, local_data.humidity,
                local_data.light_percent, local_data.distance);
            HAL_UART_Transmit(&huart1, (uint8_t *)json, strlen(json), 100);
        }

        osDelay(440);   /* 补齐到 ~500ms 周期 */
    }
}

/**
 * @brief  OLED 显示任务 — 菜单渲染 + 模式切换动画
 * @note   100ms 刷新周期
 *         从 sht20Queue 取最新传感器数据 → 调用 Menu_Render 绘制当前页面
 *         非阻塞监听 modeNotifyQueue → 收到通知后全屏显示 AUTO/MANUAL 1.2 秒
 */
void StartDisplayTask(void *argument)
{
    SensorData_t local_data = {0};
    uint8_t      mode_notify;

    OLED_Init();
    Menu_Init();
    OLED_ShowString(0, 0, "SHT20 Init...", OLED_8X16);
    OLED_Update();
    osDelay(1000);   /* 给传感器 1s 初始化时间 */

    for (;;) {
        /* 非阻塞检测模式切换通知 — 0ms timeout，有则处理无则跳过 */
        if (osMessageQueueGet(modeNotifyQueue, &mode_notify, NULL, 0) == osOK) {
            OLED_Clear();
            if (mode_notify) {
                OLED_ShowString(0, 0, "  AUTO MODE", OLED_8X16);
                OLED_ShowString(0, 2, "Threshold Ctrl", OLED_8X16);
            } else {
                OLED_ShowString(0, 0, " MANUAL MODE", OLED_8X16);
                OLED_ShowString(0, 2, "Switch Control", OLED_8X16);
            }
            OLED_Update();
            osDelay(1200);   /* 动画保持 1.2s */
            OLED_Clear();    /* 清屏，恢复菜单渲染 */
        }

        /* 尝试取最新传感器数据（100ms 超时，没拿到就用旧数据渲染） */
        if (osMessageQueueGet(sht20Queue, &local_data, NULL, 100) == osOK) {
            /* 拿到新数据 */
        }
        Menu_Render(&local_data);   /* 绘制当前页面 */
        osDelay(100);
    }
}

/**
 * @brief  输入处理任务 — 按键长短按检测 + 编码器步数消费
 * @note   10ms 轮询周期
 *         按键消抖策略: GPIO 下降沿记时刻 → GPIO 上升沿算按键时长
 *         编码器: ISR 已做消抖并累积到 enc_steps，这里只需消费
 *         KEY1: 页面切换
 *         KEY2: 确认/返回（根据当前页面 + 编辑状态分发）
 *         KEY3: 自动/手动模式切换
 */
void StartInputTask(void *argument)
{
    uint8_t  k1_last = 1, k2_last = 1, k3_last = 1;   /* 上一次电平，初始 HIGH（上拉） */
    uint32_t k1_down = 0, k2_down = 0, k3_down = 0;    /* 按下时刻（tick） */

    for (;;) {
        uint8_t k1_now = (uint8_t)HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN);
        uint8_t k2_now = (uint8_t)HAL_GPIO_ReadPin(KEY2_PORT, KEY2_PIN);
        uint8_t k3_now = (uint8_t)HAL_GPIO_ReadPin(KEY3_PORT, KEY3_PIN);

        /* ---- KEY1: 下降沿记时刻，上升沿判长短按 ---- */
        if (k1_last == 1 && k1_now == 0) {
            k1_down = osKernelGetTickCount();           /* 按下瞬间 */
        }
        if (k1_last == 0 && k1_now == 1) {
            uint32_t hold = (osKernelGetTickCount() - k1_down)
                            * portTICK_PERIOD_MS;        /* 按键持续时长(ms) */
            if (hold >= BTN_LONG_PRESS_MS)               /* ≥600ms → 长按 */
                Menu_ProcessKey1(BTN_LONG);
            else if (hold >= BTN_DEBOUNCE_MS)            /* ≥30ms → 短按（滤除毛刺） */
                Menu_ProcessKey1(BTN_SHORT);
        }
        k1_last = k1_now;   /* 更新状态，为下次边沿检测做准备 */

        /* ---- KEY2: 同上逻辑 ---- */
        if (k2_last == 1 && k2_now == 0) {
            k2_down = osKernelGetTickCount();
        }
        if (k2_last == 0 && k2_now == 1) {
            uint32_t hold = (osKernelGetTickCount() - k2_down)
                            * portTICK_PERIOD_MS;
            if (hold >= BTN_LONG_PRESS_MS)
                Menu_ProcessKey2(BTN_LONG);
            else if (hold >= BTN_DEBOUNCE_MS)
                Menu_ProcessKey2(BTN_SHORT);
        }
        k2_last = k2_now;

        /* ---- KEY3: 模式切换（仅判断短按） ---- */
        if (k3_last == 1 && k3_now == 0) {
            k3_down = osKernelGetTickCount();
        }
        if (k3_last == 0 && k3_now == 1) {
            uint32_t hold = (osKernelGetTickCount() - k3_down)
                            * portTICK_PERIOD_MS;
            if (hold >= BTN_DEBOUNCE_MS) {               /* 消抖即可，不区分长短按 */
                auto_mode = !auto_mode;                   /* 翻转自动/手动模式 */
                uint8_t notify = (uint8_t)auto_mode;
                osMessageQueuePut(modeNotifyQueue, &notify, 0, 0);  /* 通知 DisplayTask */
                printf("[KEY3] Mode: %s\r\n", auto_mode ? "AUTO" : "MANUAL");
            }
        }
        k3_last = k3_now;

        /* ---- 编码器步数消费: 读后即清零 ---- */
        int32_t steps = Encoder_GetSteps();
        if (steps) Menu_ProcessEncoder(steps);  /* 一次性传入本周期所有步数 */

        osDelay(10);   /* 10ms 轮询，100Hz 采样率对按键绰绰有余 */
    }
}

/**
 * @brief  数据分析任务 — 阈值比对 + 告警判断 + 自动执行器控制
 * @note   事件驱动: 等待 analysisQueue 有数据才执行
 *         6 项阈值逐一比对 → 满足条件则置位对应告警标志
 *         自动模式下直接驱动 3 个执行器（风扇/窗帘/LED），含回滞区间防抖动
 *         告警标志通过 alarmEventFlags 事件组通知 AlarmTask
 */
void StartAnalysisTask(void *argument)
{
    SensorData_t data;

    printf("[Analysis] started\r\n");

    for (;;) {
        /* 阻塞等待传感器数据（500ms 超时，防止死等） */
        if (osMessageQueueGet(analysisQueue, &data, NULL, 500) == osOK) {

            /* ---- 阈值检查：6 项逐一比对，生成告警位掩码 ---- */
            uint32_t flags = 0;
            const RuntimeThreshold_t *thr = Menu_GetThreshold();

            if (data.temperature  > thr->temp_high)  flags |= ALARM_FLAG_TEMP_HIGH;
            if (data.temperature  < thr->temp_low)   flags |= ALARM_FLAG_TEMP_LOW;
            if (data.humidity     > thr->humi_high)  flags |= ALARM_FLAG_HUMI_HIGH;
            if (data.humidity     < thr->humi_low)   flags |= ALARM_FLAG_HUMI_LOW;
            if (data.light_percent < thr->light_low) flags |= ALARM_FLAG_LIGHT_LOW;
            if (data.distance     < thr->dist_near)  flags |= ALARM_FLAG_DIST_NEAR;

            /* ---- 自动执行器控制（手动模式下跳过，由小程序/UART 控制） ---- */
            if (auto_mode) {
                /* 风扇: 温度 > 上限 → 开(80%占空比)；温度回落 > 1°C → 关闭
                 *      1°C 回滞区间防止临界值反复开关 */
                if (data.temperature > thr->temp_high)
                    Motor_Forward(80);
                else if (data.temperature <= thr->temp_high - 1.0f)
                    Motor_Brake();

                /* 窗帘: 光照 > 70% → 关闭（遮阳）；光照 < 40% → 打开（采光）
                 *       30% 宽回滞区间，避免云层飘过引起频繁开关 */
                if (data.light_percent > 70)
                    Servo_Close();
                else if (data.light_percent < 40)
                    Servo_Open();

                /* LED: 光照 < 阈值下限 → 开灯补光；光照回升 > 阈值+10% → 关灯
                 *      10% 回滞区间 */
                if (data.light_percent < thr->light_low)
                    LED_Ctrl_ON();
                else if (data.light_percent > thr->light_low + 10)
                    LED_Ctrl_OFF();
            }

            /* 有告警则设置事件标志，通知 AlarmTask */
            if (flags) {
                osEventFlagsSet(alarmEventFlags, flags);
                printf("[Alarm] flags=0x%lX\r\n", flags);
            }
        }
    }
}

/**
 * @brief  告警响应任务 — 阻塞等待告警事件 → 触发蜂鸣器
 * @note   永久阻塞等待 alarmEventFlags 中的 DIST_NEAR 标志
 *         蜂鸣器快速响 3 声后冷却 3 秒，防止连续报警扰民
 * @todo   当前仅响应距离告警(DIST_NEAR)，应扩展为响应全部 6 种告警
 *         并根据告警类型区分蜂鸣节奏（如温度用快节奏、湿度用慢节奏）
 */
void StartAlarmTask(void *argument)
{
    Buzzer_Init();
    printf("[Alarm] started\r\n");

    for (;;) {
        /* 阻塞等待距离告警事件（CPU 完全释放，不占时间片） */
        osEventFlagsWait(alarmEventFlags,
                          ALARM_FLAG_DIST_NEAR,   /* 只等这一个标志 */
                          osFlagsWaitAny,          /* 任一满足即唤醒 */
                          osWaitForever);           /* 无超时，永久等 */

        printf("[Alarm] DIST NEAR\r\n");
        Buzzer_Beep(BEEP_FAST_ON_MS, BEEP_FAST_OFF_MS, BEEP_FAST_COUNT);
        /* BEEP_FAST: 100ms响/100ms停 × 3次 = 总计 600ms */

        osDelay(3000);   /* 报警冷却: 3 秒内不重复触发 */
    }
}

/**
 * @brief  系统监控任务 — UART 命令解析 + 定期状态打印
 * @note   内层 1ms 轮询环形缓冲区，外层每 10 秒打印系统信息
 *         解析 3 种 JSON 命令:
 *           1. {"cmd":"init",...}          ESP32 启动后恢复阈值
 *           2. {"prop":"...","value":...}  云端/小程序远程设置阈值
 *           3. {"cmd":"actuator","act":"...","val":...}  远程控制执行器(仅手动模式)
 *         环形缓冲区在 USART1 RXNE ISR 中填充，本任务消费
 * @warning JSON 解析使用 strstr/atof 手工解析，非标准 JSON 库，容错性有限
 */
void StartMonitorTask(void *argument)
{
    char buf[256];   /* vTaskList 缓冲区 */

    printf("[Monitor] started\r\n");

    char rx_buf[256];          /* 命令接收缓冲区 */
    uint16_t rx_idx = 0;       /* 当前写入位置 */

    for (;;) {
        /* ---- 内层: 1ms 轮询 UART 环形缓冲区（10000 次 = 10 秒） ---- */
        for (int t = 0; t < 10000; t++) {
            /* 从 ISR 驱动的环形缓冲区读取所有可用字节 */
            while (UART_RxAvailable()) {
                uint8_t ch = UART_RxGetChar();
                if (ch == '\n' || ch == '\r') {
                    /* 遇到换行/回车 → 一行命令完整，开始解析 */
                    if (rx_idx > 0) {
                        rx_buf[rx_idx] = '\0';     /* 收尾 */
                        rx_idx = 0;                 /* 重置指针 */

                        /* 提取最外层 JSON 花括号 */
                        char *start = strchr(rx_buf, '{');
                        char *end   = strchr(rx_buf, '}');
                        if (start && end && end > start) {
                            end[1] = '\0';           /* 截断尾部多余字符 */

                            /* ---- 命令类型 A: ESP32 启动恢复阈值 ---- */
                            /* 格式: {"cmd":"init","maxtemp_set":35,"minitemp_set":0,...} */
                            if (strstr(start, "\"cmd\":\"init\"")) {
                                static const char *keys[] = {
                                    "maxtemp_set", "minitemp_set", "maxhum_set",
                                    "minihum_set", "minlight_set", "neardist_set"
                                };
                                for (int k = 0; k < 6; k++) {
                                    char search[64];
                                    snprintf(search, sizeof(search), "\"%s\":", keys[k]);
                                    char *pp = strstr(start, search);
                                    if (pp) {
                                        pp += strlen(search);
                                        char vs[32] = {0};
                                        int vi = 0;
                                        while (*pp && *pp != ',' && *pp != '}' && vi < 31)
                                            vs[vi++] = *pp++;
                                        vs[vi] = '\0';
                                        float v = atof(vs);           /* 字符串 → 浮点 */
                                        int ti = Menu_PropNameToIndex(keys[k]);
                                        if (ti >= 0) Menu_SetThresholdByIndex(ti, v);
                                    }
                                }
                                Menu_SaveThresholds();                /* 写 Flash */
                                printf("[UART] INIT thresholds applied\r\n");
                            }
                            /* ---- 命令类型 B: 云端/小程序设置单个阈值 ---- */
                            /* 格式: {"prop":"maxtemp_set","value":30} */
                            else {
                                char prop[32] = {0};
                                char *pp = strstr(start, "\"prop\":\"");
                                if (pp) {
                                    pp += 8;                         /* 跳过 "prop":" */
                                    int i = 0;
                                    while (*pp && *pp != '\"' && i < 31) prop[i++] = *pp++;
                                    prop[i] = '\0';
                                }

                                float val = 0;
                                char *vp = strstr(start, "\"value\":");
                                if (vp) {
                                    vp += 8;                         /* 跳过 "value": */
                                    char val_str[32] = {0};
                                    int i = 0;
                                    while (*vp && *vp != '}' && *vp != ',' && i < 31)
                                        val_str[i++] = *vp++;
                                    val_str[i] = '\0';
                                    val = atof(val_str);
                                }

                                int thr_idx = Menu_PropNameToIndex(prop);
                                if (thr_idx >= 0) {
                                    Menu_SetThresholdByIndex(thr_idx, val);
                                    Menu_SaveThresholds();           /* 写 Flash */
                                    printf("[UART] SET %s = %.1f\r\n", prop, val);
                                }
                            }

                            /* ---- 命令类型 C: 远程执行器控制（仅手动模式生效） ---- */
                            /* 格式: {"cmd":"actuator","act":"led","val":1} 或 {"act":"servo","val":0} */
                            if (strstr(start, "\"cmd\":\"actuator\"")) {
                                char act[16] = {0};
                                float aval = 0;

                                char *ap = strstr(start, "\"act\":\"");
                                if (ap) {
                                    ap += 7;                         /* 跳过 "act":" */
                                    int i = 0;
                                    while (*ap && *ap != '\"' && i < 15) act[i++] = *ap++;
                                    act[i] = '\0';
                                }

                                char *vp2 = strstr(start, "\"val\":");
                                if (vp2) {
                                    vp2 += 6;                        /* 跳过 "val": */
                                    char vs[16] = {0};
                                    int i = 0;
                                    while (*vp2 && *vp2 != '}' && *vp2 != ',' && i < 15)
                                        vs[i++] = *vp2++;
                                    vs[i] = '\0';
                                    aval = atof(vs);
                                }

                                if (!auto_mode) {
                                    if (strcmp(act, "led") == 0) {
                                        if (aval > 0) LED_Ctrl_ON();
                                        else          LED_Ctrl_OFF();
                                    } else if (strcmp(act, "servo") == 0) {
                                        if (aval > 0) Servo_Open();
                                        else          Servo_Close();
                                    } else if (strcmp(act, "motor") == 0) {
                                        if (aval > 0) Motor_Forward(80);
                                        else          Motor_Brake();
                                    }
                                    printf("[UART] ACTUATOR %s = %d\r\n", act, (int)aval);
                                } else {
                                    printf("[UART] ACTUATOR ignored (auto mode)\r\n");
                                }
                            }
                        }
                    }
                } else if (rx_idx < sizeof(rx_buf) - 1) {
                    rx_buf[rx_idx++] = ch;   /* 普通字节 → 追加到缓冲区 */
                }
                /* 缓冲区满时丢弃该字节（静默丢帧，不阻塞 ISR） */
            }
            osDelay(1);   /* 1ms 节拍 */
        }

        /* ---- 外层: 每 10 秒打印一次系统运行状态 ---- */
        vTaskList(buf);                                    /* FreeRTOS 内置: 任务名+状态+栈剩余 */
        printf("--- Task List ---\r\n%s\r\n", buf);
        printf("FreeHeap: %lu\r\n", xPortGetFreeHeapSize());   /* 剩余堆内存（字节） */
    }
}

/* USER CODE END Application */

