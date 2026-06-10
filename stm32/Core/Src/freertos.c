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
osMessageQueueId_t sht20Queue;
osMessageQueueId_t analysisQueue;
osMessageQueueId_t modeNotifyQueue;
osMutexId_t        statsMutex;
osEventFlagsId_t   alarmEventFlags;

osThreadId_t sensorTaskHandle;
const osThreadAttr_t sensorTask_attributes = {
  .name = "sensorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t displayTaskHandle;
const osThreadAttr_t displayTask_attributes = {
  .name = "displayTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t inputTaskHandle;
const osThreadAttr_t inputTask_attributes = {
  .name = "inputTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t analysisTaskHandle;
const osThreadAttr_t analysisTask_attributes = {
  .name = "analysisTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t monitorTaskHandle;
const osThreadAttr_t monitorTask_attributes = {
  .name = "monitorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

osThreadId_t alarmTaskHandle;
const osThreadAttr_t alarmTask_attributes = {
  .name = "alarmTask",
  .stack_size = 128 * 4,
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
void StartSensorTask(void *argument)
{
    SensorData_t local_data;

    /* 初始化外设 */
    if (!SHT20_Init()) {
         printf("SHT20 Init Failed!\r\n");
    } else {
         printf("SHT20 Init OK\r\n");
    }

    LightSensor_Init();
    HCSR04_Init();

    for (;;) {
        /* 读取温湿度 */
        if (!SHT20_ReadData(&local_data)) {
            printf("SHT20 Read Error\r\n");
            osDelay(1000);
            continue;
        }

        /* 读取光照（DMA 循环缓冲区均值滤波） */
        local_data.light_raw     = LightSensor_GetRaw();
        local_data.light_percent = LightSensor_GetPercent();

        /* 读取距离（超声波测距） */
        HCSR04_Trig();
        osDelay(60);  /* 等待 Echo 返回（最长 38ms + 余量） */
        local_data.distance = HCSR04_GetDistance();

        /* 发送到显示队列和数据分析队列 */
        osMessageQueuePut(sht20Queue, &local_data, 0, 0);
        osMessageQueuePut(analysisQueue, &local_data, 0, 0);

        printf("Temp=%.1f C  Humi=%.1f%%  Light=%d%%  Dist=%.1f cm\r\n",
                local_data.temperature, local_data.humidity,
                local_data.light_percent, local_data.distance);

        /* JSON 发送给 ESP32（USART1, PA9→GPIO17） */
        {
            char json[128];
            snprintf(json, sizeof(json),
                "{\"t\":%.1f,\"h\":%.1f,\"l\":%d,\"d\":%.1f}\r\n",
                local_data.temperature, local_data.humidity,
                local_data.light_percent, local_data.distance);
            HAL_UART_Transmit(&huart1, (uint8_t *)json, strlen(json), 100);
        }

        osDelay(440);
    }
}

void StartDisplayTask(void *argument)
{
    SensorData_t local_data = {0};
    uint8_t      mode_notify;

    OLED_Init();
    Menu_Init();
    OLED_ShowString(0, 0, "SHT20 Init...", OLED_8X16);
    OLED_Update();
    osDelay(1000);

    for (;;) {
        /* 非阻塞检测模式切换通知 → 全屏显示 1 秒 */
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
            osDelay(1200);
            /* 刷新菜单界面 */
            OLED_Clear();
        }

        if (osMessageQueueGet(sht20Queue, &local_data, NULL, 100) == osOK) {
            /* 拿到新数据 */
        }
        Menu_Render(&local_data);
        osDelay(100);
    }
}

void StartInputTask(void *argument)
{
    uint8_t  k1_last = 1, k2_last = 1, k3_last = 1;
    uint32_t k1_down = 0, k2_down = 0, k3_down = 0;

    for (;;) {
        uint8_t k1_now = (uint8_t)HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN);
        uint8_t k2_now = (uint8_t)HAL_GPIO_ReadPin(KEY2_PORT, KEY2_PIN);
        uint8_t k3_now = (uint8_t)HAL_GPIO_ReadPin(KEY3_PORT, KEY3_PIN);

        /* ---- KEY1 下降沿（按下）---- */
        if (k1_last == 1 && k1_now == 0) {
            k1_down = osKernelGetTickCount();
        }
        /* ---- KEY1 上升沿（松开）---- */
        if (k1_last == 0 && k1_now == 1) {
            uint32_t hold = (osKernelGetTickCount() - k1_down)
                            * portTICK_PERIOD_MS;
            if (hold >= BTN_LONG_PRESS_MS)
                Menu_ProcessKey1(BTN_LONG);
            else if (hold >= BTN_DEBOUNCE_MS)
                Menu_ProcessKey1(BTN_SHORT);
        }
        k1_last = k1_now;

        /* ---- KEY2 下降沿（按下）---- */
        if (k2_last == 1 && k2_now == 0) {
            k2_down = osKernelGetTickCount();
        }
        /* ---- KEY2 上升沿（松开）---- */
        if (k2_last == 0 && k2_now == 1) {
            uint32_t hold = (osKernelGetTickCount() - k2_down)
                            * portTICK_PERIOD_MS;
            if (hold >= BTN_LONG_PRESS_MS)
                Menu_ProcessKey2(BTN_LONG);
            else if (hold >= BTN_DEBOUNCE_MS)
                Menu_ProcessKey2(BTN_SHORT);
        }
        k2_last = k2_now;

        /* ---- KEY3: 模式切换（阈值自动 ↔ 手动开关）---- */
        if (k3_last == 1 && k3_now == 0) {
            k3_down = osKernelGetTickCount();
        }
        if (k3_last == 0 && k3_now == 1) {
            uint32_t hold = (osKernelGetTickCount() - k3_down)
                            * portTICK_PERIOD_MS;
            if (hold >= BTN_DEBOUNCE_MS) {
                auto_mode = !auto_mode;
                uint8_t notify = (uint8_t)auto_mode;
                osMessageQueuePut(modeNotifyQueue, &notify, 0, 0);
                printf("[KEY3] Mode: %s\r\n", auto_mode ? "AUTO" : "MANUAL");
            }
        }
        k3_last = k3_now;

        /* ---- 编码器 ---- */
        int32_t steps = Encoder_GetSteps();
        if (steps) Menu_ProcessEncoder(steps);

        osDelay(10);
    }
}

void StartAnalysisTask(void *argument)
{
    SensorData_t data;

    printf("[Analysis] started\r\n");

    for (;;) {
        /* 等待传感器数据 */
        if (osMessageQueueGet(analysisQueue, &data, NULL, 500) == osOK) {

            /* 阈值检查 */
            uint32_t flags = 0;
            const RuntimeThreshold_t *thr = Menu_GetThreshold();

            if (data.temperature  > thr->temp_high)  flags |= ALARM_FLAG_TEMP_HIGH;
            if (data.temperature  < thr->temp_low)   flags |= ALARM_FLAG_TEMP_LOW;
            if (data.humidity     > thr->humi_high)  flags |= ALARM_FLAG_HUMI_HIGH;
            if (data.humidity     < thr->humi_low)   flags |= ALARM_FLAG_HUMI_LOW;
            if (data.light_percent < thr->light_low) flags |= ALARM_FLAG_LIGHT_LOW;
            if (data.distance     < thr->dist_near)  flags |= ALARM_FLAG_DIST_NEAR;

            /* ====== 自动执行器控制（手动模式下跳过） ====== */
            if (auto_mode) {
                if (data.temperature > thr->temp_high)
                    Motor_Forward(80);
                else if (data.temperature <= thr->temp_high - 1.0f)
                    Motor_Brake();

                if (data.light_percent > 70)
                    Servo_Close();
                else if (data.light_percent < 40)
                    Servo_Open();

                if (data.light_percent < thr->light_low)
                    LED_Ctrl_ON();
                else if (data.light_percent > thr->light_low + 10)
                    LED_Ctrl_OFF();
            }
            /* ============================= */

            /* 有告警则设置事件标志 */
            if (flags) {
                osEventFlagsSet(alarmEventFlags, flags);
                printf("[Alarm] flags=0x%lX\r\n", flags);
            }
        }
    }
}

void StartAlarmTask(void *argument)
{
    Buzzer_Init();
    printf("[Alarm] started\r\n");

    for (;;) {
        /* 阻塞等待距离告警 */
        osEventFlagsWait(alarmEventFlags,
                          ALARM_FLAG_DIST_NEAR,
                          osFlagsWaitAny,
                          osWaitForever);

        printf("[Alarm] DIST NEAR\r\n");
        Buzzer_Beep(BEEP_FAST_ON_MS, BEEP_FAST_OFF_MS, BEEP_FAST_COUNT);

        osDelay(3000);
    }
}

void StartMonitorTask(void *argument)
{
    char buf[256];

    printf("[Monitor] started\r\n");

    char rx_buf[256];
    uint16_t rx_idx = 0;

    for (;;) {
        /* 每 1ms 检查一次环形缓冲区，满 10 秒打印一次系统信息 */
        for (int t = 0; t < 10000; t++) {
            /* 从 ISR 驱动的环形缓冲区读取所有可用字节 */
            while (UART_RxAvailable()) {
                uint8_t ch = UART_RxGetChar();
                if (ch == '\n' || ch == '\r') {
                    if (rx_idx > 0) {
                        rx_buf[rx_idx] = '\0';
                        rx_idx = 0;

                        char *start = strchr(rx_buf, '{');
                        char *end   = strchr(rx_buf, '}');
                        if (start && end && end > start) {
                            end[1] = '\0';

                            /* 处理 init 命令（ESP32 启动恢复阈值） */
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
                                        float v = atof(vs);
                                        int ti = Menu_PropNameToIndex(keys[k]);
                                        if (ti >= 0) Menu_SetThresholdByIndex(ti, v);
                                    }
                                }
                                Menu_SaveThresholds();
                                printf("[UART] INIT thresholds applied\r\n");
                            }
                            /* 处理 set 命令 */
                            else {
                                char prop[32] = {0};
                                char *pp = strstr(start, "\"prop\":\"");
                                if (pp) {
                                    pp += 8;
                                    int i = 0;
                                    while (*pp && *pp != '\"' && i < 31) prop[i++] = *pp++;
                                    prop[i] = '\0';
                                }

                                float val = 0;
                                char *vp = strstr(start, "\"value\":");
                                if (vp) {
                                    vp += 8;
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
                                    Menu_SaveThresholds();
                                    printf("[UART] SET %s = %.1f\r\n", prop, val);
                                }
                            }

                            /* 处理 actuator 命令（仅手动模式下执行）:
                             * {"cmd":"actuator","act":"led","val":1} */
                            if (strstr(start, "\"cmd\":\"actuator\"")) {
                                char act[16] = {0};
                                float aval = 0;

                                char *ap = strstr(start, "\"act\":\"");
                                if (ap) {
                                    ap += 7;
                                    int i = 0;
                                    while (*ap && *ap != '\"' && i < 15) act[i++] = *ap++;
                                    act[i] = '\0';
                                }

                                char *vp2 = strstr(start, "\"val\":");
                                if (vp2) {
                                    vp2 += 6;
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
                    rx_buf[rx_idx++] = ch;
                }
            }
            osDelay(1);
        }

        vTaskList(buf);
        printf("--- Task List ---\r\n%s\r\n", buf);
        printf("FreeHeap: %lu\r\n", xPortGetFreeHeapSize());
    }
}

/* USER CODE END Application */

