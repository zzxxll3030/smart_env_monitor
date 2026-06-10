# Smart Environment Monitor
智能环境监测系统

基于物联网架构的**端—云—端**环境数据采集、远程控制与阈值报警系统。

## 系统架构

```
STM32F103C8T6 ──UART──► ESP32-S3 ──WiFi/MQTT──► OneNET 云平台 ──HTTPS API──► 微信小程序
 (FreeRTOS)     JSON    (ESP-IDF)                (MQTT Broker)                  (原生框架)
```

## 核心功能

- 🌡️ **环境采集** — 温度、湿度、光照、超声波测距 4 路传感器
- 🚨 **自动报警** — 6 项阈值可配置，蜂鸣器 + OLED 显示
- 🎮 **执行器控制** — LED、舵机窗帘、直流电机风扇，自动/手动模式
- ☁️ **云端同步** — ESP32 UART→MQTT 桥接，OneNET 云平台存储
- 📱 **小程序端** — 数据看板、设备控制、阈值设置、我的 4 个页面

## 项目结构

| 目录 | 说明 | 技术栈 |
|------|------|--------|
| [`stm32/`](stm32/) | STM32 固件（传感器采集 + 执行器控制） | FreeRTOS + HAL + Keil MDK |
| [`esp/`](esp/) | ESP32 桥接固件（串口→MQTT） | ESP-IDF v5.5.4 + ESP-MQTT |
| [`miniprogram/`](miniprogram/) | 微信小程序（数据展示 + 远程控制） | 微信原生框架 + OneNET API |

## 硬件清单

| 组件 | 型号 | 接口 |
|------|------|------|
| 主控 | STM32F103C8T6 | — |
| 通信 | ESP32-S3 | — |
| 温湿度 | SHT20 | I2C2 |
| 超声波 | HC-SR04 | TIM2 输入捕获 |
| 光照 | 光敏电阻 | ADC1 DMA |
| 显示 | OLED 0.96" 128×64 | I2C |
| 存储 | W25Q64 8MB Flash | SPI2 |
| 执行器 | SG90 舵机 + TB6612 电机 + LED | PWM / GPIO |

## 详细文档

完整技术文档请参阅 [项目介绍.md](项目介绍.md)，涵盖：
- FreeRTOS 8 任务架构设计
- ESP32 WiFi/MQTT 通信机制
- OneNET 物模型属性定义
- 微信小程序页面生命周期与 API
- 硬件引脚分配总览

## 版本

- 项目版本：1.2
- 云平台：OneNET（中国移动）
