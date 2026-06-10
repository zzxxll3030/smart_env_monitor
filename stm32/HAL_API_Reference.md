# STM32 HAL 库常用 API 速查手册

> 按外设分类，同类函数放一起；按使用频率从高到低排列。
> 标注 `[生成]` = CubeMX 自动生成，`[手写]` = 需手动调用。

---

## 一、GPIO

| 频率 | 函数 | 来源 | 说明 |
|:--:|---|---|---|
| ★★★ | `HAL_GPIO_WritePin(GPIOx, Pin, State)` | 手写 | 写引脚（GPIO_PIN_SET / RESET） |
| ★★★ | `HAL_GPIO_TogglePin(GPIOx, Pin)` | 手写 | 翻转电平 |
| ★★★ | `HAL_GPIO_ReadPin(GPIOx, Pin)` | 手写 | 读引脚 |
| ★★ | `HAL_GPIO_Init(GPIOx, &InitStruct)` | 生成 | 初始化引脚模式（CubeMX 生成在 MX_GPIO_Init） |
| ★ | `HAL_GPIO_DeInit(GPIOx, Pin)` | 手写 | 复位引脚到默认 |
| ★ | `HAL_GPIO_EXTI_Callback(Pin)` | 手写 | 外部中断回调（弱函数） |

```c
/* --- 手写常用 --- */
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
uint8_t val = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

/* --- 生成示例 --- */
GPIO_InitTypeDef g = {0};
g.Pin   = GPIO_PIN_13;
g.Mode  = GPIO_MODE_OUTPUT_PP;
g.Pull  = GPIO_NOPULL;
g.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOC, &g);

/* --- 中断回调 --- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) { /* PA0 */ }
}
```

---

## 二、I2C

| 频率 | 函数 | 来源 | 说明 |
|:--:|---|---|---|
| ★★★ | `HAL_I2C_Master_Transmit(&hi2c, DevAddr, pData, Size, Timeout)` | 手写 | 发送（无寄存器地址） |
| ★★★ | `HAL_I2C_Master_Receive(&hi2c, DevAddr, pData, Size, Timeout)` | 手写 | 接收（无寄存器地址） |
| ★★★ | `HAL_I2C_Mem_Write(&hi2c, DevAddr, MemAddr, MemAddSize, pData, Size, T)` | 手写 | 写寄存器 |
| ★★★ | `HAL_I2C_Mem_Read(&hi2c, DevAddr, MemAddr, MemAddSize, pData, Size, T)` | 手写 | 读寄存器 |
| ★★ | `HAL_I2C_Init(&hi2c)` | 生成 | 初始化 I2C（MX_I2Cx_Init） |
| ★★ | `HAL_I2C_MspInit(&hi2c)` | 生成 | 引脚/时钟配置（HAL_I2C_Init 自动调用） |
| ★ | `HAL_I2C_IsDeviceReady(&hi2c, DevAddr, Trials, Timeout)` | 手写 | 检测设备在线 |
| ★ | `HAL_I2C_GetError(&hi2c)` | 手写 | 获取错误码 |
| ★ | `HAL_I2C_GetState(&hi2c)` | 手写 | 获取当前状态 |

```c
// SHT20 — 无寄存器
uint8_t cmd = 0xF3;
HAL_I2C_Master_Transmit(&hi2c1, 0x80, &cmd, 1, 100);
uint8_t buf[3];
HAL_I2C_Master_Receive(&hi2c1, 0x80, buf, 3, 100);

// SSD1306 — 有寄存器
HAL_I2C_Mem_Write(&hi2c2, 0x78, 0x00, I2C_MEMADD_SIZE_8BIT, &cmd, 1, 100);
```

### 后缀模式（I2C / UART / SPI 通用）

| 后缀 | 说明 |
|---|---|
| （无后缀） | 阻塞，等完成才返回 |
| `_IT` | 中断，不阻塞，完成进回调 |
| `_DMA` | DMA，最省 CPU，完成进回调 |

---

## 三、USART / UART

| 频率 | 函数 | 来源 | 说明 |
|:--:|---|---|---|
| ★★★ | `HAL_UART_Transmit(&huart, pData, Size, Timeout)` | 手写 | 阻塞发送（含 fputc 重定向） |
| ★★ | `HAL_UART_Receive(&huart, pData, Size, Timeout)` | 手写 | 阻塞接收 |
| ★★ | `HAL_UART_Init(&huart)` | 生成 | 初始化（MX_USARTx_UART_Init） |
| ★★ | `HAL_UART_MspInit(&huart)` | 生成 | 引脚/时钟配置 |
| ★ | `HAL_UART_Receive_IT(&huart, pData, Size)` | 手写 | 中断接收 |
| ★ | `HAL_UART_Transmit_DMA(&huart, pData, Size)` | 手写 | DMA 发送 |
| ★ | `HAL_UARTEx_RxEventCallback(&huart, Size)` | 手写 | 接收完成回调 |
| ★ | `HAL_UART_TxCpltCallback(&huart)` | 手写 | 发送完成回调 |

```c
HAL_UART_Transmit(&huart1, (uint8_t*)"Hello\r\n", 7, 100);

// printf 重定向（生成在 usart.c 的 USER CODE 0 区）
int fputc(int ch, FILE *f) {
    HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, 1000);
    return ch;
}
```

---

## 四、延时与系统时钟

| 频率 | 函数 | 来源 | 说明 |
|:--:|---|---|---|
| ★★★ | `HAL_Delay(ms)` | 手写 | 毫秒延时（TIM 时基，FreeRTOS 安全） |
| ★★ | `HAL_GetTick()` | 手写 | 获取当前毫秒数 |
| ★ | `HAL_Init()` | 生成 | 初始化 HAL 库、NVIC、SysTick |
| ★ | `SystemClock_Config()` | 生成 | 配置 HSE/PLL/总线时钟 |
| ★ | `HAL_RCC_OscConfig(&s)` | 生成 | 配置振荡器 |
| ★ | `HAL_RCC_ClockConfig(&s, Latency)` | 生成 | 配置总线时钟 |
| ★ | `HAL_SYSTICK_Config(Ticks)` | 生成 | 配置 SysTick（FreeRTOS 下用 TIM4 替代） |

```c
HAL_Delay(100);                    // 100ms
uint32_t now = HAL_GetTick();      // 系统运行毫秒数

// 时钟使能宏（CubeMX 生成的 Init 中已包含，通常不手写）
__HAL_RCC_GPIOA_CLK_ENABLE();
__HAL_RCC_I2C1_CLK_ENABLE();
__HAL_RCC_USART1_CLK_ENABLE();
__HAL_RCC_ADC1_CLK_ENABLE();
```

---

## 五、ADC

| 频率 | 函数 | 来源 | 说明 |
|:--:|---|---|---|
| ★★★ | `HAL_ADC_GetValue(&hadc)` | 手写 | 读转换值（12 位：0~4095） |
| ★★★ | `HAL_ADC_Start(&hadc)` | 手写 | 启动单次转换 |
| ★★ | `HAL_ADC_PollForConversion(&hadc, Timeout)` | 手写 | 等待转换完成 |
| ★★ | `HAL_ADC_Start_DMA(&hadc, pData, Length)` | 手写 | 启动 DMA 连续采集 |
| ★★ | `HAL_ADC_Init(&hadc)` | 生成 | 初始化（MX_ADCx_Init） |
| ★ | `HAL_ADC_Stop(&hadc)` | 手写 | 停止 |
| ★ | `HAL_ADC_Stop_DMA(&hadc)` | 手写 | 停止 DMA |
| ★ | `HAL_ADC_ConvCpltCallback(&hadc)` | 手写 | 转换完成回调 |
| ★ | `HAL_ADC_ConvHalfCpltCallback(&hadc)` | 手写 | DMA 半传输回调 |

```c
// 单次
HAL_ADC_Start(&hadc1);
HAL_ADC_PollForConversion(&hadc1, 10);
uint16_t val = HAL_ADC_GetValue(&hadc1);

// DMA — 64 点连续采集
uint16_t adc_buf[64];
HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, 64);
```

---

## 六、SPI

| 频率 | 函数 | 来源 | 说明 |
|:--:|---|---|---|
| ★★★ | `HAL_SPI_Transmit(&hspi, pData, Size, Timeout)` | 手写 | 阻塞发送 |
| ★★ | `HAL_SPI_TransmitReceive(&hspi, pTx, pRx, Size, Timeout)` | 手写 | 全双工收发 |
| ★★ | `HAL_SPI_Receive(&hspi, pData, Size, Timeout)` | 手写 | 阻塞接收 |
| ★★ | `HAL_SPI_Init(&hspi)` | 生成 | 初始化（MX_SPIx_Init） |
| ★ | `HAL_SPI_Transmit_DMA(&hspi, pData, Size)` | 手写 | DMA 发送 |
| ★ | `HAL_SPI_Receive_DMA(&hspi, pData, Size)` | 手写 | DMA 接收 |
| ★ | `HAL_SPI_TxCpltCallback(&hspi)` | 手写 | 发送完成回调 |
| ★ | `HAL_SPI_RxCpltCallback(&hspi)` | 手写 | 接收完成回调 |

```c
// W25Q64 读 ID — 全双工
uint8_t tx[] = {0x90, 0x00, 0x00, 0x00}, rx[4];
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // CS = 0
HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 1000);
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);    // CS = 1
```

---

## 七、TIM（定时器）

### 基础时基

| 频率 | 函数 | 来源 | 说明 |
|:--:|---|---|---|
| ★★ | `HAL_TIM_Base_Init(&htim)` | 生成 | 初始化时基（CubeMX 配置） |
| ★★ | `HAL_TIM_Base_Start_IT(&htim)` | 生成 | 启动定时器中断（HAL 时基用） |
| ★ | `HAL_TIM_Base_Start(&htim)` | 手写 | 启动（无中断） |
| ★ | `HAL_TIM_PeriodElapsedCallback(&htim)` | 手写 | 溢出回调 |

### PWM / 输出比较

| 频率 | 函数 | 来源 | 说明 |
|:--:|---|---|---|
| ★★★ | `__HAL_TIM_SET_COMPARE(&htim, CH, val)` | 手写 | 设置占空比（CCR 值） |
| ★★ | `HAL_TIM_PWM_Start(&htim, Channel)` | 手写 | 启动 PWM |
| ★★ | `HAL_TIM_PWM_Stop(&htim, Channel)` | 手写 | 停止 PWM |
| ★ | `HAL_TIM_PWM_Start_IT(&htim, Channel)` | 手写 | 启动 PWM 中断模式 |
| ★ | `HAL_TIM_OC_Start(&htim, Channel)` | 手写 | 启动输出比较 |

```c
HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 500);  // 占空比 = 500 / ARR
```

### 输入捕获

| 频率 | 函数 | 来源 | 说明 |
|:--:|---|---|---|
| ★★ | `HAL_TIM_IC_Start_DMA(&htim, Channel, pData, Length)` | 手写 | 启动输入捕获 DMA |
| ★ | `HAL_TIM_IC_Start(&htim, Channel)` | 手写 | 启动输入捕获（无 DMA） |
| ★ | `HAL_TIM_IC_CaptureCallback(&htim)` | 手写 | 捕获完成回调 |
| ★ | `HAL_TIM_ReadCapturedValue(&htim, Channel)` | 手写 | 读取捕获值 |

### 常用宏

| 宏 | 说明 |
|---|---|
| `__HAL_TIM_SET_COMPARE(&htim, CH, val)` | 设置 CCR（PWM 占空比） |
| `__HAL_TIM_GET_COMPARE(&htim, CH)` | 读取 CCR |
| `__HAL_TIM_SET_COUNTER(&htim, val)` | 设置计数器 |
| `__HAL_TIM_GET_COUNTER(&htim)` | 读取计数器 |
| `__HAL_TIM_SET_AUTORELOAD(&htim, val)` | 设置自动重装载 |

---

## 八、中断回调汇总

> 以下均为弱函数（`__weak`），需在用户代码中重写。
> 注意：同一个外设可触发多个不同回调，通过 `htim->Instance` 等区分。

### GPIO 外部中断

| 回调函数 | 触发条件 |
|---|---|
| `HAL_GPIO_EXTI_Callback(Pin)` | 任意 GPIO 外部中断 |

### 定时器

| 回调函数 | 触发条件 |
|---|---|
| `HAL_TIM_PeriodElapsedCallback(&htim)` | 定时器溢出 |
| `HAL_TIM_IC_CaptureCallback(&htim)` | 输入捕获完成 |
| `HAL_TIM_OC_DelayElapsedCallback(&htim)` | 输出比较匹配 |
| `HAL_TIM_PWM_PulseFinishedCallback(&htim)` | PWM 脉冲完成 |

### 串口

| 回调函数 | 触发条件 |
|---|---|
| `HAL_UARTEx_RxEventCallback(&huart, Size)` | 接收完成（推荐） |
| `HAL_UART_TxCpltCallback(&huart)` | 发送完成 |
| `HAL_UART_ErrorCallback(&huart)` | 通信错误 |

### ADC / DMA

| 回调函数 | 触发条件 |
|---|---|
| `HAL_ADC_ConvCpltCallback(&hadc)` | ADC 转换完成 |
| `HAL_ADC_ConvHalfCpltCallback(&hadc)` | DMA 半传输完成 |
| `HAL_ADC_LevelOutOfWindowCallback(&hadc)` | 模拟看门狗触发 |

### SPI / I2C

| 回调函数 | 触发条件 |
|---|---|
| `HAL_SPI_TxCpltCallback(&hspi)` | SPI 发送完成 |
| `HAL_SPI_RxCpltCallback(&hspi)` | SPI 接收完成 |
| `HAL_I2C_MasterTxCpltCallback(&hi2c)` | I2C 发送完成 |
| `HAL_I2C_MasterRxCpltCallback(&hi2c)` | I2C 接收完成 |

---

## 九、PWR（低功耗）

| 函数 | 说明 |
|---|---|
| `HAL_PWR_EnterSLEEPMode(Regulator, Entry)` | 睡眠模式（CPU 停，外设跑，任意中断唤醒） |
| `HAL_PWR_EnterSTOPMode(Regulator, Entry)` | 停机模式（唤醒后从该行继续执行） |
| `HAL_PWR_EnterSTANDBYMode()` | 待机模式（唤醒=复位，SRAM 全丢） |
| `__WFI()` | Wait For Interrupt |
| `__WFE()` | Wait For Event |
| `HAL_PWR_EnableWakeUpPin(Pin)` | 使能唤醒引脚（PA0 上升沿） |
| `HAL_PWR_DisableWakeUpPin(Pin)` | 禁用唤醒引脚 |

```c
// 进入停机，PA0 上升沿唤醒
HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);

// 待机 — 等同于复位
HAL_PWR_EnterSTANDBYMode();
```

| 模式 | 功耗 | SRAM | 唤醒方式 | 唤醒后 |
|---|---|---|---|---|
| SLEEP | 低 | 保持 | 任意中断 | 从下一行继续 |
| STOP | 更低 | 保持 | EXTI / RTC | 从下一行继续 |
| STANDBY | 极低 | 丢失 | PA0 / RTC | 从头复位 |

---

## 十、看门狗

### IWDG（独立看门狗，LSI ~40kHz）

| 函数 | 来源 | 说明 |
|---|---|---|
| `HAL_IWDG_Init(&hiwdg)` | 生成 | 初始化（分频 + 重装载） |
| `HAL_IWDG_Refresh(&hiwdg)` | 手写 | 喂狗 |

```c
hiwdg.Init.Prescaler = IWDG_PRESCALER_64;  // 40k/64 = 625Hz
hiwdg.Init.Reload    = 625;                 // 1 秒溢出
HAL_IWDG_Init(&hiwdg);
HAL_IWDG_Refresh(&hiwdg);  // 1 秒内喂一次
```

### WWDG（窗口看门狗，APB1 时钟）

| 函数 | 来源 | 说明 |
|---|---|---|
| `HAL_WWDG_Init(&hwwdg)` | 生成 | 初始化 |
| `HAL_WWDG_Refresh(&hwwdg)` | 手写 | 在窗口期喂狗 |
| `HAL_WWDG_EarlyWakeupCallback(&hwwdg)` | 手写 | 超时前最后一次中断 |

---

## 十一、DMA

> DMA 通常随外设初始化，一般只手动调起停。

| 函数 | 说明 |
|---|---|
| `HAL_DMA_Start(&hdma, Src, Dst, Length)` | 启动传输 |
| `HAL_DMA_Start_IT(&hdma, Src, Dst, Length)` | 启动（中断模式） |
| `HAL_DMA_Abort(&hdma)` | 中止 |
| `HAL_DMA_GetState(&hdma)` | 获取状态 |

---

## 十二、NVIC / 中断管理

| 函数 | 来源 | 说明 |
|---|---|---|
| `HAL_NVIC_SetPriority(IRQn, Preempt, Sub)` | 生成 | 设置中断优先级 |
| `HAL_NVIC_EnableIRQ(IRQn)` | 生成 | 使能中断 |
| `HAL_NVIC_DisableIRQ(IRQn)` | 手写 | 禁用中断 |
| `__enable_irq()` | 手写 | 开全局中断（CMSIS） |
| `__disable_irq()` | 手写 | 关全局中断（CMSIS） |

---

## 十三、常用调参与查询

> 运行时修改配置、查询频率/参数，不重新初始化。

### 时钟频率查询

| 函数 | 说明 |
|---|---|
| `HAL_RCC_GetSysClockFreq()` | 系统时钟（SYSCLK）频率 Hz |
| `HAL_RCC_GetHCLKFreq()` | AHB 总线（HCLK）频率 Hz |
| `HAL_RCC_GetPCLK1Freq()` | APB1 总线频率 Hz（TIM2-7 时钟源） |
| `HAL_RCC_GetPCLK2Freq()` | APB2 总线频率 Hz（TIM1 时钟源） |

### TIM 运行时改参数

| 函数 / 宏 | 说明 |
|---|---|
| `__HAL_TIM_SET_PRESCALER(&htim, val)` | 修改分频系数（PSC） |
| `__HAL_TIM_SET_AUTORELOAD(&htim, val)` | 修改自动重装载（ARR） |
| `__HAL_TIM_SET_COUNTER(&htim, val)` | 设置计数器当前值 |
| `HAL_TIM_GenerateEvent(&htim, TIM_EVENTSOURCE_UPDATE)` | 软件触发更新事件（让新 PSC/ARR 生效） |
| `HAL_TIM_Base_Stop(&htim)` | 暂停定时器 |
| `HAL_TIM_Base_Start(&htim)` | 恢复定时器 |

```c
// 动态改 PWM 频率：先停 → 改参数 → 生成更新 → 启动
HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
__HAL_TIM_SET_AUTORELOAD(&htim2, 999);     // ARR = 999
__HAL_TIM_SET_PRESCALER(&htim2, 71);       // PSC = 71
HAL_TIM_GenerateEvent(&htim2, TIM_EVENTSOURCE_UPDATE);
HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
// f_pwm = PCLK1 / (PSC+1) / (ARR+1) = 72M / 72 / 1000 = 1kHz
```

### ADC 校准与通道配置

| 函数 | 说明 |
|---|---|
| `HAL_ADCEx_Calibration_Start(&hadc)` | ADC 自校准（每次上电建议做一次） |
| `HAL_ADC_ConfigChannel(&hadc, &sConfig)` | 修改通道采样时间等参数 |
| `HAL_ADC_GetState(&hadc)` | 获取 ADC 状态 |

### USART 运行时改波特率

> `HAL_UART_Init()` 可重复调用来改波特率，但需先 `HAL_UART_DeInit()`。

```c
HAL_UART_DeInit(&huart1);
huart1.Init.BaudRate = 9600;
HAL_UART_Init(&huart1);  // 重新初始化
```

### FLASH 存储参数

| 函数 | 说明 |
|---|---|
| `HAL_FLASH_Unlock()` | 解锁 Flash（写入前必须） |
| `HAL_FLASH_Lock()` | 上锁 Flash |
| `HAL_FLASH_Program(Type, Address, Data)` | 写入半字/字 |
| `HAL_FLASHEx_Erase(&EraseInit, &PageError)` | 擦除页 |

```c
// STM32F103C8 Flash: 0x08000000, 最后一页 0x0800FC00 存参数
HAL_FLASH_Unlock();
FLASH_EraseInitTypeDef e = {FLASH_TYPEERASE_PAGES, 0x0800FC00, 1};
uint32_t err;
HAL_FLASHEx_Erase(&e, &err);
HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, 0x0800FC00, 1234);
HAL_FLASH_Lock();
```

---

## 十四、错误码与返回值

```c
HAL_OK       // 成功
HAL_ERROR    // 错误
HAL_BUSY     // 外设忙
HAL_TIMEOUT  // 超时

if (HAL_I2C_Master_Transmit(&hi2c1, 0x80, &cmd, 1, 100) != HAL_OK) {
    // 错误处理
}
```

---

## 十五、I2C 地址计算

```c
DevAddr = 7 位地址 << 1

SHT20   → 0x40 << 1 = 0x80
SSD1306 → 0x3C << 1 = 0x78
MPU6050 → 0x68 << 1 = 0xD0

MemAddSize:
  I2C_MEMADD_SIZE_8BIT   // 1 字节寄存器地址（常用）
  I2C_MEMADD_SIZE_16BIT  // 2 字节寄存器地址
```
