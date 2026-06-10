#include "bsp_light.h"
#include "adc.h"

/* DMA 循环缓冲区，由 ADC1_IN2 (PA2) 通过 DMA1_CH1 持续填充 */
static uint16_t adc_buf[LIGHT_ADC_BUF_SIZE];

/* DMA 状态标志 */
static volatile uint8_t dma_half_done = 0;
static volatile uint8_t dma_full_done = 0;

/**
 * @brief  启动 ADC-DMA 采集
 * @note   调用 HAL_ADC_Start_DMA 后，DMA 以 Circular 模式持续将 ADC 数据搬运到 adc_buf
 */
void LightSensor_Init(void)
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, LIGHT_ADC_BUF_SIZE);
}

/**
 * @brief  DMA 半满中断回调（前32个数据就绪）
 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1) {
        dma_half_done = 1;
    }
}

/**
 * @brief  DMA 全满中断回调（全部64个数据就绪）
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1) {
        dma_full_done = 1;
    }
}

/**
 * @brief  获取 ADC 原始值的滑动均值
 * @return 64次采样的平均值
 */
uint16_t LightSensor_GetRaw(void)
{
    uint32_t sum = 0;

    for (uint8_t i = 0; i < LIGHT_ADC_BUF_SIZE; i++) {
        sum += adc_buf[i];
    }

    return (uint16_t)(sum / LIGHT_ADC_BUF_SIZE);
}

/**
 * @brief  获取光照强度百分比
 * @return 0-100（全暗≈0%，全亮≈100%）
 * @note   光敏电阻下拉接法（光敏→GND）：光照越强，ADC值越小
 *         因此用 100 - raw/4095 反向映射
 */
uint8_t LightSensor_GetPercent(void)
{
    uint16_t raw = LightSensor_GetRaw();

    if (raw >= 4095) return 0;
    if (raw <= 5) return 100;

    return 100 - (uint8_t)(((uint32_t)raw * 100) / 4095);
}
