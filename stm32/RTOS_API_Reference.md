# CMSIS-RTOS v2 / FreeRTOS API 对照表

> STM32CubeMX 生成的 API 是 CMSIS-RTOS v2 封装（`osXxx`），底层调用原生 FreeRTOS（`xXxx`）。
> 参数不同但功能一一对应。

---

## 一、内核控制

| CMSIS-RTOS v2 | 原生 FreeRTOS | 说明 |
|---|---|---|
| `osKernelInitialize()` | 初始化内核数据结构 | 准备调度器，创建对象用 |
| `osKernelStart()` | `vTaskStartScheduler()` | 启动调度器，不再返回 |
| `osKernelGetTickCount()` | `xTaskGetTickCount()` | 获取系统 tick 计数 |
| `osKernelGetTickFreq()` | `configTICK_RATE_HZ` | 获取 tick 频率（Hz） |

---

## 二、任务管理

| CMSIS-RTOS v2 | 原生 FreeRTOS | 说明 |
|---|---|---|
| `osThreadNew(func, arg, attr)` | `xTaskCreate(func, name, stack, arg, prio, handle)` | 创建任务 |
| `osThreadTerminate(thread_id)` | `vTaskDelete(handle)` | 删除任务 |
| `osThreadYield()` | `taskYIELD()` | 让出 CPU |
| `osThreadGetId()` | `xTaskGetCurrentTaskHandle()` | 获取当前任务句柄 |
| `osThreadSuspend(thread_id)` | `vTaskSuspend(handle)` | 挂起任务 |
| `osThreadResume(thread_id)` | `vTaskResume(handle)` | 恢复任务 |
| `osThreadGetPriority(thread_id)` | `uxTaskPriorityGet(handle)` | 获取任务优先级 |
| `osThreadSetPriority(thread_id, prio)` | `vTaskPrioritySet(handle, prio)` | 设置任务优先级 |

### 任务属性结构体

```c
const osThreadAttr_t task_attr = {
    .name      = "myTask",         // 任务名（调试用）
    .stack_size = 256 * 4,         // 栈大小（字节），128 * 4 = 512 字节
    .priority  = osPriorityNormal, // 优先级，默认 osPriorityNormal = 24
};
```

> CMSIS v2 用 `osPriorityXxx` 枚举，底层映射到 FreeRTOS 优先级数字（0 = idle, 越大越高）。
> 常见值：`osPriorityLow(8)` < `osPriorityBelowNormal(16)` < `osPriorityNormal(24)` < `osPriorityAboveNormal(32)` < `osPriorityHigh(40)` < `osPriorityRealtime(56)`

---

## 三、延时

| CMSIS-RTOS v2 | 原生 FreeRTOS | 说明 |
|---|---|---|
| `osDelay(ticks)` | `vTaskDelay(ticks)` | 相对延时，参数单位是 **tick** |
| `osDelayUntil(prev_wake_time, ticks)` | `vTaskDelayUntil(&last, ticks)` | 绝对延时，精确周期 |

> `osDelay(500)` 在 tick=1000Hz 时 = 500ms。
> CubeMX 默认 tick=1000Hz，即 `osDelay(1)` = 1ms。

---

## 四、软件定时器

| CMSIS-RTOS v2 | 原生 FreeRTOS | 说明 |
|---|---|---|
| `osTimerNew(callback, type, arg, attr)` | `xTimerCreate(name, period, autoReload, id, callback)` | 创建定时器 |
| `osTimerStart(timer_id, ticks)` | `xTimerStart(handle, 0)` | 启动定时器 |
| `osTimerStop(timer_id)` | `xTimerStop(handle, 0)` | 停止定时器 |
| `osTimerDelete(timer_id)` | `xTimerDelete(handle, 0)` | 删除定时器 |

```c
// type 参数：
osTimerOnce    // 单次触发，对应 pdFALSE
osTimerPeriodic // 周期触发，对应 pdTRUE
```

---

## 五、互斥锁（Mutex）

| CMSIS-RTOS v2 | 原生 FreeRTOS | 说明 |
|---|---|---|
| `osMutexNew(attr)` | `xSemaphoreCreateMutex()` | 创建互斥锁 |
| `osMutexAcquire(mutex_id, timeout)` | `xSemaphoreTake(handle, timeout)` | 获取锁 |
| `osMutexRelease(mutex_id)` | `xSemaphoreGive(handle)` | 释放锁 |
| `osMutexDelete(mutex_id)` | `vSemaphoreDelete(handle)` | 删除锁 |

> `osWaitForever` = `portMAX_DELAY` = 一直等到拿到锁为止。
> `0` = 尝试一下，拿不到立即返回。

---

## 六、信号量（Semaphore）

| CMSIS-RTOS v2 | 原生 FreeRTOS | 说明 |
|---|---|---|
| `osSemaphoreNew(max_count, init_count, attr)` | `xSemaphoreCreateCounting(max, init)` | 创建计数信号量 |
| `osSemaphoreAcquire(sem_id, timeout)` | `xSemaphoreTake(handle, timeout)` | 获取（P 操作） |
| `osSemaphoreRelease(sem_id)` | `xSemaphoreGive(handle)` | 释放（V 操作） |
| `osSemaphoreDelete(sem_id)` | `vSemaphoreDelete(handle)` | 删除信号量 |

---

## 七、消息队列

| CMSIS-RTOS v2 | 原生 FreeRTOS | 说明 |
|---|---|---|
| `osMessageQueueNew(msg_count, msg_size, attr)` | `xQueueCreate(count, size)` | 创建队列 |
| `osMessageQueuePut(queue_id, msg_ptr, prio, timeout)` | `xQueueSend(handle, item, timeout)` | 入队 |
| `osMessageQueueGet(queue_id, msg_ptr, prio, timeout)` | `xQueueReceive(handle, item, timeout)` | 出队 |
| `osMessageQueueGetCapacity(queue_id)` | `uxQueueMessagesWaiting(handle)` | 当前消息数 |
| `osMessageQueueDelete(queue_id)` | `vQueueDelete(handle)` | 删除队列 |

```c
// 定义队列
osMessageQueueId_t queueHandle;
// 创建：最多 5 条消息，每条 4 字节
queueHandle = osMessageQueueNew(5, 4, NULL);
```

---

## 八、事件标志

| CMSIS-RTOS v2 | 原生 FreeRTOS | 说明 |
|---|---|---|
| `osEventFlagsNew(attr)` | `xEventGroupCreate()` | 创建事件组 |
| `osEventFlagsSet(ef_id, flags)` | `xEventGroupSetBits(handle, bits)` | 置位 |
| `osEventFlagsWait(ef_id, flags, options, timeout)` | `xEventGroupWaitBits(...)` | 等待 |
| `osEventFlagsDelete(ef_id)` | `vEventGroupDelete(handle)` | 删除 |

---

## 九、临界区

| CMSIS | 原生 FreeRTOS | 说明 |
|---|---|---|
| —（无直接封装） | `taskENTER_CRITICAL()` | 进入临界区，关中断 |
| — | `taskEXIT_CRITICAL()` | 退出临界区，恢复中断 |

> 互斥锁优于临界区：临界区关全局中断影响实时性，互斥锁只阻塞竞争者。

---

## 十、通知（Task Notification）

| CMSIS-RTOS v2 | 原生 FreeRTOS | 说明 |
|---|---|---|
| `osThreadFlagsSet(thread_id, flags)` | `xTaskNotifyGive(handle)` | 通知任务 |
| `osThreadFlagsWait(flags, options, timeout)` | `ulTaskNotifyTake(pdTRUE, timeout)` | 等待通知 |

> 通知比信号量快 N 倍，适合 1 对 1 的唤醒场景。

---

## 参数速查

| 参数 | 值 | 含义 |
|---|---|---|
| `osWaitForever` | `0xFFFFFFFF` | 无限等待 |
| `osPriorityNormal` | 24 | 普通优先级 |
| `osPriorityAboveNormal` | 32 | 高于普通 |
| `osTimerOnce` | 0 | 单次 |
| `osTimerPeriodic` | 1 | 周期 |
| `osFlagsWaitAny` | 0 | 任意一个标志位 |
| `osFlagsWaitAll` | 1 | 所有标志位 |
| `osFlagsNoClear` | 2 | 等待后不清除 |

---

## 快速对照

| 你要做什么 | 一句话 |
|---|---|
| 创建任务 | `osThreadNew(TaskFunc, NULL, &attr)` |
| 延时 | `osDelay(ms)` |
| 互斥锁 | `osMutexNew/Acquire/Release` |
| 数据传递 | `osMessageQueueNew/Put/Get` |
| 事件通知 | `osThreadFlagsSet/Wait` |
| 定时器 | `osTimerNew/Start` |
| 挂起/恢复 | `osThreadSuspend/Resume` |
