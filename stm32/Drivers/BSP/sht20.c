#include "sht20.h"

static uint8_t SHT20_CRC8(uint8_t *buf, uint8_t len)
{
    uint8_t crc = 0x00;
    uint8_t i, j;

    for (i = 0; i < len; i++) {
        crc ^= buf[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

uint8_t SHT20_CRC8_Check(uint8_t *buf, uint8_t len)
{
    uint8_t crc_calc = SHT20_CRC8(buf, len - 1);
    if (crc_calc == buf[len - 1]) {
        return 1;
    }
    return 0;
}

uint8_t SHT20_Init(void)
{
    uint8_t cmd = SHT20_CMD_RESET;
    HAL_StatusTypeDef ret;

    ret = HAL_I2C_Master_Transmit(&hi2c2, SHT20_I2C_ADDR, &cmd, 1, 100);

    if (ret != HAL_OK) {
        return 0;
    }

    HAL_Delay(15);

    return 1;
}

uint8_t SHT20_ReadData(SensorData_t *data)
{
    uint8_t cmd;
    uint8_t buf[3];
    uint16_t raw;
    HAL_StatusTypeDef ret;

    /* === 读温度 === */
    cmd = SHT20_CMD_TEMP_NH;
    ret = HAL_I2C_Master_Transmit(&hi2c2, SHT20_I2C_ADDR, &cmd, 1, 100);
    if (ret != HAL_OK) return 0;

    HAL_Delay(SHT20_WAIT_TEMP_MS);  /* 等待测量，释放总线 */

    ret = HAL_I2C_Master_Receive(&hi2c2, SHT20_I2C_ADDR, buf, 3, 100);
    if (ret != HAL_OK) return 0;

    if (!SHT20_CRC8_Check(buf, 3)) return 0;

    raw = (buf[0] << 8) | buf[1];
    raw &= ~0x03;
    data->temperature = -46.85f + 175.72f * ((float)raw / 65536.0f);

    /* === 读湿度 === */
    cmd = SHT20_CMD_HUMI_NH;
    ret = HAL_I2C_Master_Transmit(&hi2c2, SHT20_I2C_ADDR, &cmd, 1, 100);
    if (ret != HAL_OK) return 0;

    HAL_Delay(SHT20_WAIT_HUMI_MS);

    ret = HAL_I2C_Master_Receive(&hi2c2, SHT20_I2C_ADDR, buf, 3, 100);
    if (ret != HAL_OK) return 0;

    if (!SHT20_CRC8_Check(buf, 3)) return 0;

    raw = (buf[0] << 8) | buf[1];
    raw &= ~0x03;
    data->humidity = -6.0f + 125.0f * ((float)raw / 65536.0f);
    if (data->humidity > 100.0f) data->humidity = 100.0f;
    if (data->humidity < 0.0f)  data->humidity = 0.0f;

    return 1;
}
